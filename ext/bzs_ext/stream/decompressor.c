// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include "bzs_ext/stream/decompressor.h"

#include <stdlib.h>

#include "bzs_ext/buffer.h"
#include "bzs_ext/error.h"
#include "bzs_ext/gvl.h"
#include "bzs_ext/option.h"
#include "bzs_ext/utils.h"

// -- initialization --

static void free_decompressor(bzs_ext_decompressor_t* decompressor_ptr)
{
  bz_stream* stream_ptr = decompressor_ptr->stream_ptr;
  if (stream_ptr != NULL) {
    BZ2_bzDecompressEnd(stream_ptr);
  }

  bzs_ext_byte_t* destination_buffer = decompressor_ptr->destination_buffer;
  if (destination_buffer != NULL) {
    free(destination_buffer);
  }

  free(decompressor_ptr);
}

VALUE bzs_ext_allocate_decompressor(VALUE klass)
{
  bzs_ext_decompressor_t* decompressor_ptr;
  VALUE self = Data_Make_Struct(klass, bzs_ext_decompressor_t, NULL, free_decompressor, decompressor_ptr);

  decompressor_ptr->stream_ptr                          = NULL;
  decompressor_ptr->destination_buffer                  = NULL;
  decompressor_ptr->destination_buffer_length           = 0;
  decompressor_ptr->remaining_destination_buffer        = NULL;
  decompressor_ptr->remaining_destination_buffer_length = 0;

  return self;
}

#define GET_DECOMPRESSOR(self)              \
  bzs_ext_decompressor_t* decompressor_ptr; \
  Data_Get_Struct(self, bzs_ext_decompressor_t, decompressor_ptr);

VALUE bzs_ext_initialize_decompressor(VALUE self, VALUE options)
{
  GET_DECOMPRESSOR(self);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, destination_buffer_length);
  BZS_EXT_GET_BOOL_OPTION(options, gvl);
  BZS_EXT_RESOLVE_DECOMPRESSOR_OPTIONS(options);

  bz_stream* stream_ptr = malloc(sizeof(bz_stream));
  if (stream_ptr == NULL) {
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  stream_ptr->bzalloc = NULL;
  stream_ptr->bzfree  = NULL;
  stream_ptr->opaque  = NULL;

  bzs_result_t result = BZ2_bzDecompressInit(stream_ptr, verbosity, small);
  if (result != BZ_OK) {
    free(stream_ptr);
    bzs_ext_raise_error(bzs_ext_get_error(result));
  }

  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_DECOMPRESSOR;
  }

  bzs_ext_byte_t* destination_buffer = malloc(destination_buffer_length);
  if (destination_buffer == NULL) {
    free(stream_ptr);
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  decompressor_ptr->stream_ptr                          = stream_ptr;
  decompressor_ptr->destination_buffer                  = destination_buffer;
  decompressor_ptr->destination_buffer_length           = destination_buffer_length;
  decompressor_ptr->remaining_destination_buffer        = destination_buffer;
  decompressor_ptr->remaining_destination_buffer_length = destination_buffer_length;
  decompressor_ptr->gvl                                 = gvl;

  return Qnil;
}

// -- decompress --

#define DO_NOT_USE_AFTER_CLOSE(decompressor_ptr)                                              \
  if (decompressor_ptr->stream_ptr == NULL || decompressor_ptr->destination_buffer == NULL) { \
    bzs_ext_raise_error(BZS_EXT_ERROR_USED_AFTER_CLOSE);                                      \
  }

typedef struct
{
  bz_stream*       stream_ptr;
  bzs_ext_byte_t** remaining_source_ptr;
  size_t*          remaining_source_length_ptr;
  bzs_ext_byte_t** remaining_destination_buffer_ptr;
  size_t*          remaining_destination_buffer_length_ptr;
  bzs_result_t     result;
} decompress_args_t;

static inline void* decompress_wrapper(void* data)
{
  decompress_args_t* args = data;

  args->stream_ptr->next_in   = (char*) *args->remaining_source_ptr;
  args->stream_ptr->avail_in  = bzs_consume_size(*args->remaining_source_length_ptr);
  args->stream_ptr->next_out  = (char*) *args->remaining_destination_buffer_ptr;
  args->stream_ptr->avail_out = bzs_consume_size(*args->remaining_destination_buffer_length_ptr);

  args->result = BZ2_bzDecompress(args->stream_ptr);

  *args->remaining_source_ptr                    = (bzs_ext_byte_t*) args->stream_ptr->next_in;
  *args->remaining_source_length_ptr             = args->stream_ptr->avail_in;
  *args->remaining_destination_buffer_ptr        = (bzs_ext_byte_t*) args->stream_ptr->next_out;
  *args->remaining_destination_buffer_length_ptr = args->stream_ptr->avail_out;

  return NULL;
}

VALUE bzs_ext_decompress(VALUE self, VALUE source_value)
{
  GET_DECOMPRESSOR(self);
  DO_NOT_USE_AFTER_CLOSE(decompressor_ptr);
  Check_Type(source_value, T_STRING);

  const char*     source                  = RSTRING_PTR(source_value);
  size_t          source_length           = RSTRING_LEN(source_value);
  bzs_ext_byte_t* remaining_source        = (bzs_ext_byte_t*) source;
  size_t          remaining_source_length = source_length;

  decompress_args_t args = {
    .stream_ptr                              = decompressor_ptr->stream_ptr,
    .remaining_source_ptr                    = &remaining_source,
    .remaining_source_length_ptr             = &remaining_source_length,
    .remaining_destination_buffer_ptr        = &decompressor_ptr->remaining_destination_buffer,
    .remaining_destination_buffer_length_ptr = &decompressor_ptr->remaining_destination_buffer_length};

  BZS_EXT_GVL_WRAP(decompressor_ptr->gvl, decompress_wrapper, &args);
  if (args.result != BZ_OK && args.result != BZ_PARAM_ERROR && args.result != BZ_STREAM_END) {
    bzs_ext_raise_error(bzs_ext_get_error(args.result));
  }

  VALUE bytes_read             = SIZET2NUM(source_length - remaining_source_length);
  VALUE needs_more_destination = args.result == BZ_OK && (remaining_source_length != 0 ||
                                                          decompressor_ptr->remaining_destination_buffer_length == 0) ?
                                   Qtrue :
                                   Qfalse;

  return rb_ary_new_from_args(2, bytes_read, needs_more_destination);
}

// -- other --

VALUE bzs_ext_decompressor_read_result(VALUE self)
{
  GET_DECOMPRESSOR(self);
  DO_NOT_USE_AFTER_CLOSE(decompressor_ptr);

  bzs_ext_byte_t* destination_buffer                  = decompressor_ptr->destination_buffer;
  size_t          destination_buffer_length           = decompressor_ptr->destination_buffer_length;
  size_t          remaining_destination_buffer_length = decompressor_ptr->remaining_destination_buffer_length;

  const char* result        = (const char*) destination_buffer;
  size_t      result_length = destination_buffer_length - remaining_destination_buffer_length;
  VALUE       result_value  = rb_str_new(result, result_length);

  decompressor_ptr->remaining_destination_buffer        = destination_buffer;
  decompressor_ptr->remaining_destination_buffer_length = destination_buffer_length;

  return result_value;
}

// -- cleanup --

VALUE bzs_ext_decompressor_close(VALUE self)
{
  GET_DECOMPRESSOR(self);
  DO_NOT_USE_AFTER_CLOSE(decompressor_ptr);

  bz_stream* stream_ptr = decompressor_ptr->stream_ptr;
  if (stream_ptr != NULL) {
    BZ2_bzDecompressEnd(stream_ptr);

    decompressor_ptr->stream_ptr = NULL;
  }

  bzs_ext_byte_t* destination_buffer = decompressor_ptr->destination_buffer;
  if (destination_buffer != NULL) {
    free(destination_buffer);

    decompressor_ptr->destination_buffer = NULL;
  }

  // It is possible to keep "destination_buffer_length", "remaining_destination_buffer"
  //   and "remaining_destination_buffer_length" as is.

  return Qnil;
}

// -- exports --

void bzs_ext_decompressor_exports(VALUE root_module)
{
  VALUE module = rb_define_module_under(root_module, "Stream");

  VALUE decompressor = rb_define_class_under(module, "NativeDecompressor", rb_cObject);

  rb_define_alloc_func(decompressor, bzs_ext_allocate_decompressor);
  rb_define_method(decompressor, "initialize", bzs_ext_initialize_decompressor, 1);
  rb_define_method(decompressor, "read", bzs_ext_decompress, 1);
  rb_define_method(decompressor, "read_result", bzs_ext_decompressor_read_result, 0);
  rb_define_method(decompressor, "close", bzs_ext_decompressor_close, 0);
}
