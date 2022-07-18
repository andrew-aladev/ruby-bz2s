// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include "bzs_ext/stream/compressor.h"

#include "bzs_ext/error.h"
#include "bzs_ext/gvl.h"
#include "bzs_ext/option.h"

// -- initialization --

static void free_compressor(bzs_ext_compressor_t* compressor_ptr)
{
  bz_stream* stream_ptr = compressor_ptr->stream_ptr;
  if (stream_ptr != NULL) {
    BZ2_bzCompressEnd(stream_ptr);
  }

  bzs_ext_byte_t* destination_buffer = compressor_ptr->destination_buffer;
  if (destination_buffer != NULL) {
    free(destination_buffer);
  }

  free(compressor_ptr);
}

VALUE bzs_ext_allocate_compressor(VALUE klass)
{
  bzs_ext_compressor_t* compressor_ptr;
  VALUE                 self = Data_Make_Struct(klass, bzs_ext_compressor_t, NULL, free_compressor, compressor_ptr);

  compressor_ptr->stream_ptr                          = NULL;
  compressor_ptr->destination_buffer                  = NULL;
  compressor_ptr->destination_buffer_length           = 0;
  compressor_ptr->remaining_destination_buffer        = NULL;
  compressor_ptr->remaining_destination_buffer_length = 0;
  compressor_ptr->gvl                                 = false;

  return self;
}

#define GET_COMPRESSOR(self)            \
  bzs_ext_compressor_t* compressor_ptr; \
  Data_Get_Struct(self, bzs_ext_compressor_t, compressor_ptr);

VALUE bzs_ext_initialize_compressor(VALUE self, VALUE options)
{
  GET_COMPRESSOR(self);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, destination_buffer_length);
  BZS_EXT_GET_BOOL_OPTION(options, gvl);
  BZS_EXT_RESOLVE_COMPRESSOR_OPTIONS(options);

  bz_stream* stream_ptr = malloc(sizeof(bz_stream));
  if (stream_ptr == NULL) {
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  bzs_result_t result = BZ2_bzCompressInit(stream_ptr, block_size, verbosity, work_factor);
  if (result != BZ_OK) {
    free(stream_ptr);
    bzs_ext_raise_error(bzs_ext_get_error(result));
  }

  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_COMPRESSOR;
  }

  bzs_ext_byte_t* destination_buffer = malloc(destination_buffer_length);
  if (destination_buffer == NULL) {
    free(stream_ptr);
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  compressor_ptr->stream_ptr                          = stream_ptr;
  compressor_ptr->destination_buffer                  = destination_buffer;
  compressor_ptr->destination_buffer_length           = destination_buffer_length;
  compressor_ptr->remaining_destination_buffer        = destination_buffer;
  compressor_ptr->remaining_destination_buffer_length = destination_buffer_length;
  compressor_ptr->gvl                                 = gvl;

  return Qnil;
}

// -- compress --

#define DO_NOT_USE_AFTER_CLOSE(compressor_ptr)                                            \
  if (compressor_ptr->stream_ptr == NULL || compressor_ptr->destination_buffer == NULL) { \
    bzs_ext_raise_error(BZS_EXT_ERROR_USED_AFTER_CLOSE);                                  \
  }

// typedef struct
// {
//   zstds_ext_compressor_t* compressor_ptr;
//   ZSTD_inBuffer*          in_buffer_ptr;
//   ZSTD_outBuffer*         out_buffer_ptr;
//   bzs_result_t          result;
// } compress_args_t;

// static inline void* compress_wrapper(void* data)
// {
//   compress_args_t* args = data;
//
//   args->result =
//     ZSTD_compressStream2(args->compressor_ptr->ctx, args->out_buffer_ptr, args->in_buffer_ptr, ZSTD_e_continue);
//
//   return NULL;
// }

// VALUE zstds_ext_compress(VALUE self, VALUE source_value)
// {
//   GET_COMPRESSOR(self);
//   DO_NOT_USE_AFTER_CLOSE(compressor_ptr);
//   Check_Type(source_value, T_STRING);
//
//   const char* source        = RSTRING_PTR(source_value);
//   size_t      source_length = RSTRING_LEN(source_value);
//
//   ZSTD_inBuffer  in_buffer  = {.src = source, .size = source_length, .pos = 0};
//   ZSTD_outBuffer out_buffer = {
//     .dst  = compressor_ptr->remaining_destination_buffer,
//     .size = compressor_ptr->remaining_destination_buffer_length,
//     .pos  = 0};
//
//   compress_args_t args = {.compressor_ptr = compressor_ptr, .in_buffer_ptr = &in_buffer, .out_buffer_ptr =
//   &out_buffer};
//
//   ZSTDS_EXT_GVL_WRAP(compressor_ptr->gvl, compress_wrapper, &args);
//   if (ZSTD_isError(args.result)) {
//     zstds_ext_raise_error(zstds_ext_get_error(ZSTD_getErrorCode(args.result)));
//   }
//
//   compressor_ptr->remaining_destination_buffer += out_buffer.pos;
//   compressor_ptr->remaining_destination_buffer_length -= out_buffer.pos;
//
//   VALUE bytes_written          = SIZET2NUM(in_buffer.pos);
//   VALUE needs_more_destination = compressor_ptr->remaining_destination_buffer_length == 0 ? Qtrue : Qfalse;
//
//   return rb_ary_new_from_args(2, bytes_written, needs_more_destination);
// }

// -- compressor flush --

// typedef struct
// {
//   zstds_ext_compressor_t* compressor_ptr;
//   ZSTD_outBuffer*         out_buffer_ptr;
//   zstds_result_t          result;
// } compress_flush_args_t;

// static inline void* compress_flush_wrapper(void* data)
// {
//   compress_flush_args_t* args      = data;
//   ZSTD_inBuffer          in_buffer = {.src = NULL, .size = 0, .pos = 0};
//
//   args->result = ZSTD_compressStream2(args->compressor_ptr->ctx, args->out_buffer_ptr, &in_buffer, ZSTD_e_flush);
//
//   return NULL;
// }

// VALUE zstds_ext_flush_compressor(VALUE self)
// {
//   GET_COMPRESSOR(self);
//   DO_NOT_USE_AFTER_CLOSE(compressor_ptr);
//
//   ZSTD_outBuffer out_buffer = {
//     .dst  = compressor_ptr->remaining_destination_buffer,
//     .size = compressor_ptr->remaining_destination_buffer_length,
//     .pos  = 0};
//
//   compress_flush_args_t args = {.compressor_ptr = compressor_ptr, .out_buffer_ptr = &out_buffer};
//
//   ZSTDS_EXT_GVL_WRAP(compressor_ptr->gvl, compress_flush_wrapper, &args);
//   if (ZSTD_isError(args.result)) {
//     zstds_ext_raise_error(zstds_ext_get_error(ZSTD_getErrorCode(args.result)));
//   }
//
//   compressor_ptr->remaining_destination_buffer += out_buffer.pos;
//   compressor_ptr->remaining_destination_buffer_length -= out_buffer.pos;
//
//   return args.result != 0 ? Qtrue : Qfalse;
// }

// -- compressor finish --

// typedef struct
// {
//   zstds_ext_compressor_t* compressor_ptr;
//   ZSTD_outBuffer*         out_buffer_ptr;
//   zstds_result_t          result;
// } compress_finish_args_t;

// static inline void* compress_finish_wrapper(void* data)
// {
//   compress_finish_args_t* args      = data;
//   ZSTD_inBuffer           in_buffer = {.src = NULL, .size = 0, .pos = 0};
//
//   args->result = ZSTD_compressStream2(args->compressor_ptr->ctx, args->out_buffer_ptr, &in_buffer, ZSTD_e_end);
//
//   return NULL;
// }

// VALUE zstds_ext_finish_compressor(VALUE self)
// {
//   GET_COMPRESSOR(self);
//   DO_NOT_USE_AFTER_CLOSE(compressor_ptr);
//
//   ZSTD_outBuffer out_buffer = {
//     .dst  = compressor_ptr->remaining_destination_buffer,
//     .size = compressor_ptr->remaining_destination_buffer_length,
//     .pos  = 0};
//
//   compress_finish_args_t args = {.compressor_ptr = compressor_ptr, .out_buffer_ptr = &out_buffer};
//
//   ZSTDS_EXT_GVL_WRAP(compressor_ptr->gvl, compress_finish_wrapper, &args);
//   if (ZSTD_isError(args.result)) {
//     zstds_ext_raise_error(zstds_ext_get_error(ZSTD_getErrorCode(args.result)));
//   }
//
//   compressor_ptr->remaining_destination_buffer += out_buffer.pos;
//   compressor_ptr->remaining_destination_buffer_length -= out_buffer.pos;
//
//   return args.result != 0 ? Qtrue : Qfalse;
// }

// -- other --

VALUE bzs_ext_compressor_read_result(VALUE self)
{
  GET_COMPRESSOR(self);
  DO_NOT_USE_AFTER_CLOSE(compressor_ptr);

  bzs_ext_byte_t* destination_buffer                  = compressor_ptr->destination_buffer;
  size_t          destination_buffer_length           = compressor_ptr->destination_buffer_length;
  size_t          remaining_destination_buffer_length = compressor_ptr->remaining_destination_buffer_length;

  const char* result        = (const char*) destination_buffer;
  size_t      result_length = destination_buffer_length - remaining_destination_buffer_length;
  VALUE       result_value  = rb_str_new(result, result_length);

  compressor_ptr->remaining_destination_buffer        = destination_buffer;
  compressor_ptr->remaining_destination_buffer_length = destination_buffer_length;

  return result_value;
}

// -- cleanup --

VALUE bzs_ext_compressor_close(VALUE self)
{
  GET_COMPRESSOR(self);
  DO_NOT_USE_AFTER_CLOSE(compressor_ptr);

  bz_stream* stream_ptr = compressor_ptr->stream_ptr;
  if (stream_ptr != NULL) {
    BZ2_bzCompressEnd(stream_ptr);

    compressor_ptr->stream_ptr = NULL;
  }

  bzs_ext_byte_t* destination_buffer = compressor_ptr->destination_buffer;
  if (destination_buffer != NULL) {
    free(destination_buffer);

    compressor_ptr->destination_buffer = NULL;
  }

  // It is possible to keep "destination_buffer_length", "remaining_destination_buffer"
  //   and "remaining_destination_buffer_length" as is.

  return Qnil;
}

// -- exports --

void bzs_ext_compressor_exports(VALUE root_module)
{
  VALUE module = rb_define_module_under(root_module, "Stream");

  VALUE compressor = rb_define_class_under(module, "NativeCompressor", rb_cObject);

  rb_define_alloc_func(compressor, bzs_ext_allocate_compressor);
  rb_define_method(compressor, "initialize", bzs_ext_initialize_compressor, 1);
  rb_define_method(compressor, "write", bzs_ext_compress, 1);
  rb_define_method(compressor, "flush", bzs_ext_flush_compressor, 0);
  rb_define_method(compressor, "finish", bzs_ext_finish_compressor, 0);
  rb_define_method(compressor, "read_result", bzs_ext_compressor_read_result, 0);
  rb_define_method(compressor, "close", bzs_ext_compressor_close, 0);
}
