// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include <bzlib.h>

#include "bzs_ext/string.h"

#include "bzs_ext/buffer.h"
#include "bzs_ext/common.h"
#include "bzs_ext/error.h"
#include "bzs_ext/gvl.h"
#include "bzs_ext/macro.h"
#include "bzs_ext/option.h"
#include "bzs_ext/utils.h"

// -- buffer --

static inline bzs_ext_result_t increase_destination_buffer(
  VALUE   destination_value,
  size_t  destination_length,
  size_t* remaining_destination_buffer_length_ptr,
  size_t  destination_buffer_length)
{
  if (*remaining_destination_buffer_length_ptr == destination_buffer_length) {
    // We want to write more data at once, than buffer has.
    return BZS_EXT_ERROR_NOT_ENOUGH_DESTINATION_BUFFER;
  }

  int exception;

  BZS_EXT_RESIZE_STRING_BUFFER(destination_value, destination_length + destination_buffer_length, exception);
  if (exception != 0) {
    return BZS_EXT_ERROR_ALLOCATE_FAILED;
  }

  *remaining_destination_buffer_length_ptr = destination_buffer_length;

  return 0;
}

// -- compress --

typedef struct
{
  bz_stream*       stream_ptr;
  int              stream_action;
  bzs_ext_byte_t** remaining_source_ptr;
  size_t*          remaining_source_length_ptr;
  bzs_ext_byte_t*  remaining_destination_buffer;
  size_t*          remaining_destination_buffer_length_ptr;
  bzs_result_t     result;
} compress_args_t;

static inline void* compress_wrapper(void* data)
{
  compress_args_t* args = data;

  args->stream_ptr->next_in   = (char*) *args->remaining_source_ptr;
  args->stream_ptr->avail_in  = bzs_consume_size(*args->remaining_source_length_ptr);
  args->stream_ptr->next_out  = (char*) args->remaining_destination_buffer;
  args->stream_ptr->avail_out = bzs_consume_size(*args->remaining_destination_buffer_length_ptr);

  args->result = BZ2_bzCompress(args->stream_ptr, args->stream_action);

  *args->remaining_source_ptr                    = (bzs_ext_byte_t*) args->stream_ptr->next_in;
  *args->remaining_source_length_ptr             = args->stream_ptr->avail_in;
  *args->remaining_destination_buffer_length_ptr = args->stream_ptr->avail_out;

  return NULL;
}

#define BUFFERED_COMPRESS(gvl, args)                                                                             \
  while (true) {                                                                                                 \
    bzs_ext_byte_t* remaining_destination_buffer =                                                               \
      (bzs_ext_byte_t*) RSTRING_PTR(destination_value) + destination_length;                                     \
    size_t prev_remaining_destination_buffer_length = remaining_destination_buffer_length;                       \
                                                                                                                 \
    args.remaining_destination_buffer            = remaining_destination_buffer;                                 \
    args.remaining_destination_buffer_length_ptr = &remaining_destination_buffer_length;                         \
                                                                                                                 \
    BZS_EXT_GVL_WRAP(gvl, compress_wrapper, &args);                                                              \
    if (                                                                                                         \
      args.result != BZ_OK && args.result != BZ_RUN_OK && args.result != BZ_FINISH_OK &&                         \
      args.result != BZ_STREAM_END && args.result != BZ_PARAM_ERROR) {                                           \
      return bzs_ext_get_error(args.result);                                                                     \
    }                                                                                                            \
                                                                                                                 \
    destination_length += prev_remaining_destination_buffer_length - remaining_destination_buffer_length;        \
                                                                                                                 \
    if (args.result == BZ_PARAM_ERROR && source_length != 0) {                                                   \
      ext_result = increase_destination_buffer(                                                                  \
        destination_value, destination_length, &remaining_destination_buffer_length, destination_buffer_length); \
                                                                                                                 \
      if (ext_result != 0) {                                                                                     \
        return ext_result;                                                                                       \
      }                                                                                                          \
                                                                                                                 \
      continue;                                                                                                  \
    }                                                                                                            \
                                                                                                                 \
    break;                                                                                                       \
  }

static inline bzs_ext_result_t compress(
  bz_stream*  stream_ptr,
  const char* source,
  size_t      source_length,
  VALUE       destination_value,
  size_t      destination_buffer_length,
  bool        gvl)
{
  bzs_ext_result_t ext_result;
  bzs_ext_byte_t*  remaining_source                    = (bzs_ext_byte_t*) source;
  size_t           remaining_source_length             = source_length;
  size_t           destination_length                  = 0;
  size_t           remaining_destination_buffer_length = destination_buffer_length;

  compress_args_t args = {
    .stream_ptr                  = stream_ptr,
    .stream_action               = BZ_RUN,
    .remaining_source_ptr        = &remaining_source,
    .remaining_source_length_ptr = &remaining_source_length};
  BUFFERED_COMPRESS(gvl, args);

  compress_args_t finish_args = {
    .stream_ptr                  = stream_ptr,
    .stream_action               = BZ_FINISH,
    .remaining_source_ptr        = &remaining_source,
    .remaining_source_length_ptr = &remaining_source_length};
  BUFFERED_COMPRESS(gvl, finish_args);

  int exception;

  BZS_EXT_RESIZE_STRING_BUFFER(destination_value, destination_length, exception);
  if (exception != 0) {
    return BZS_EXT_ERROR_ALLOCATE_FAILED;
  }

  return 0;
}

VALUE bzs_ext_compress_string(VALUE BZS_EXT_UNUSED(self), VALUE source_value, VALUE options)
{
  Check_Type(source_value, T_STRING);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, destination_buffer_length);
  BZS_EXT_GET_BOOL_OPTION(options, gvl);
  BZS_EXT_RESOLVE_COMPRESSOR_OPTIONS(options);

  bz_stream stream = {
    .bzalloc = NULL,
    .bzfree  = NULL,
    .opaque  = NULL,
  };

  bzs_result_t result = BZ2_bzCompressInit(&stream, block_size, verbosity, work_factor);
  if (result != BZ_OK) {
    bzs_ext_raise_error(bzs_ext_get_error(result));
  }

  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_COMPRESSOR;
  }

  int exception;

  BZS_EXT_CREATE_STRING_BUFFER(destination_value, destination_buffer_length, exception);
  if (exception != 0) {
    BZ2_bzCompressEnd(&stream);
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  const char* source        = RSTRING_PTR(source_value);
  size_t      source_length = RSTRING_LEN(source_value);

  bzs_ext_result_t ext_result =
    compress(&stream, source, source_length, destination_value, destination_buffer_length, gvl);

  result = BZ2_bzCompressEnd(&stream);
  if (result != BZ_OK) {
    ext_result = bzs_ext_get_error(result);
  }

  if (ext_result != 0) {
    bzs_ext_raise_error(ext_result);
  }

  return destination_value;
}

// -- decompress --

typedef struct
{
  bz_stream*       stream_ptr;
  bzs_ext_byte_t** remaining_source_ptr;
  size_t*          remaining_source_length_ptr;
  bzs_ext_byte_t*  remaining_destination_buffer;
  size_t*          remaining_destination_buffer_length_ptr;
  bzs_result_t     result;
} decompress_args_t;

static inline void* decompress_wrapper(void* data)
{
  decompress_args_t* args = data;

  args->stream_ptr->next_in   = (char*) *args->remaining_source_ptr;
  args->stream_ptr->avail_in  = bzs_consume_size(*args->remaining_source_length_ptr);
  args->stream_ptr->next_out  = (char*) args->remaining_destination_buffer;
  args->stream_ptr->avail_out = bzs_consume_size(*args->remaining_destination_buffer_length_ptr);

  args->result = BZ2_bzDecompress(args->stream_ptr);

  *args->remaining_source_ptr                    = (bzs_ext_byte_t*) args->stream_ptr->next_in;
  *args->remaining_source_length_ptr             = args->stream_ptr->avail_in;
  *args->remaining_destination_buffer_length_ptr = args->stream_ptr->avail_out;

  return NULL;
}

static inline bzs_ext_result_t decompress(
  bz_stream*  stream_ptr,
  const char* source,
  size_t      source_length,
  VALUE       destination_value,
  size_t      destination_buffer_length,
  bool        gvl)
{
  bzs_ext_result_t ext_result;
  bzs_ext_byte_t*  remaining_source                    = (bzs_ext_byte_t*) source;
  size_t           remaining_source_length             = source_length;
  size_t           destination_length                  = 0;
  size_t           remaining_destination_buffer_length = destination_buffer_length;

  decompress_args_t args = {
    .stream_ptr                  = stream_ptr,
    .remaining_source_ptr        = &remaining_source,
    .remaining_source_length_ptr = &remaining_source_length};

  while (true) {
    bzs_ext_byte_t* remaining_destination_buffer =
      (bzs_ext_byte_t*) RSTRING_PTR(destination_value) + destination_length;
    size_t prev_remaining_destination_buffer_length = remaining_destination_buffer_length;

    args.remaining_destination_buffer            = remaining_destination_buffer;
    args.remaining_destination_buffer_length_ptr = &remaining_destination_buffer_length;

    BZS_EXT_GVL_WRAP(gvl, decompress_wrapper, &args);
    if (args.result != BZ_OK && args.result != BZ_STREAM_END && args.result != BZ_PARAM_ERROR) {
      return bzs_ext_get_error(args.result);
    }

    destination_length += prev_remaining_destination_buffer_length - remaining_destination_buffer_length;

    if (args.result == BZ_PARAM_ERROR && source_length != 0) {
      ext_result = increase_destination_buffer(
        destination_value, destination_length, &remaining_destination_buffer_length, destination_buffer_length);

      if (ext_result != 0) {
        return ext_result;
      }

      continue;
    }

    break;
  }

  int exception;

  BZS_EXT_RESIZE_STRING_BUFFER(destination_value, destination_length, exception);
  if (exception != 0) {
    return BZS_EXT_ERROR_ALLOCATE_FAILED;
  }

  return 0;
}

VALUE bzs_ext_decompress_string(VALUE BZS_EXT_UNUSED(self), VALUE source_value, VALUE options)
{
  Check_Type(source_value, T_STRING);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, destination_buffer_length);
  BZS_EXT_GET_BOOL_OPTION(options, gvl);
  BZS_EXT_RESOLVE_DECOMPRESSOR_OPTIONS(options);

  bz_stream stream = {
    .bzalloc = NULL,
    .bzfree  = NULL,
    .opaque  = NULL,
  };

  bzs_result_t result = BZ2_bzDecompressInit(&stream, verbosity, small);
  if (result != BZ_OK) {
    bzs_ext_raise_error(bzs_ext_get_error(result));
  }

  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_DECOMPRESSOR;
  }

  int exception;

  BZS_EXT_CREATE_STRING_BUFFER(destination_value, destination_buffer_length, exception);
  if (exception != 0) {
    BZ2_bzDecompressEnd(&stream);
    bzs_ext_raise_error(BZS_EXT_ERROR_ALLOCATE_FAILED);
  }

  const char* source        = RSTRING_PTR(source_value);
  size_t      source_length = RSTRING_LEN(source_value);

  bzs_ext_result_t ext_result =
    decompress(&stream, source, source_length, destination_value, destination_buffer_length, gvl);

  result = BZ2_bzDecompressEnd(&stream);
  if (result != BZ_OK) {
    ext_result = bzs_ext_get_error(result);
  }

  if (ext_result != 0) {
    bzs_ext_raise_error(ext_result);
  }

  return destination_value;
}

// -- exports --

void bzs_ext_string_exports(VALUE root_module)
{
  rb_define_module_function(root_module, "_native_compress_string", RUBY_METHOD_FUNC(bzs_ext_compress_string), 2);
  rb_define_module_function(root_module, "_native_decompress_string", RUBY_METHOD_FUNC(bzs_ext_decompress_string), 2);
}
