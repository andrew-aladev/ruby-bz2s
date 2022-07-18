// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include "bzs_ext/io.h"

#include <bzlib.h>
#include <stdio.h>
#include <string.h>

#include "bzs_ext/buffer.h"
#include "bzs_ext/error.h"
#include "bzs_ext/gvl.h"
#include "bzs_ext/macro.h"
#include "bzs_ext/option.h"
#include "bzs_ext/utils.h"
#include "ruby/io.h"

// Additional possible results:
enum
{
  BZS_EXT_FILE_READ_FINISHED = 128
};

// -- file --

static inline bzs_ext_result_t
  read_file(FILE* source_file, bzs_ext_byte_t* source_buffer, size_t* source_length_ptr, size_t source_buffer_length)
{
  size_t read_length = fread(source_buffer, 1, source_buffer_length, source_file);
  if (read_length == 0 && feof(source_file)) {
    return BZS_EXT_FILE_READ_FINISHED;
  }

  if (read_length != source_buffer_length && ferror(source_file)) {
    return BZS_EXT_ERROR_READ_IO;
  }

  *source_length_ptr = read_length;

  return 0;
}

static inline bzs_ext_result_t
  write_file(FILE* destination_file, bzs_ext_byte_t* destination_buffer, size_t destination_length)
{
  size_t written_length = fwrite(destination_buffer, 1, destination_length, destination_file);
  if (written_length != destination_length) {
    return BZS_EXT_ERROR_WRITE_IO;
  }

  return 0;
}

// -- buffer --

static inline bzs_ext_result_t create_buffers(
  bzs_ext_byte_t** source_buffer_ptr,
  size_t           source_buffer_length,
  bzs_ext_byte_t** destination_buffer_ptr,
  size_t           destination_buffer_length)
{
  bzs_ext_byte_t* source_buffer = malloc(source_buffer_length);
  if (source_buffer == NULL) {
    return BZS_EXT_ERROR_ALLOCATE_FAILED;
  }

  bzs_ext_byte_t* destination_buffer = malloc(destination_buffer_length);
  if (destination_buffer == NULL) {
    free(source_buffer);
    return BZS_EXT_ERROR_ALLOCATE_FAILED;
  }

  *source_buffer_ptr      = source_buffer;
  *destination_buffer_ptr = destination_buffer;

  return 0;
}

// We have read some source from file into source buffer.
// Than algorithm has read part of this source.
// We need to move remaining source to the top of source buffer.
// Than we can read more source from file.
// Algorithm can use same buffer again.

static inline bzs_ext_result_t read_more_source(
  FILE*                  source_file,
  const bzs_ext_byte_t** source_ptr,
  size_t*                source_length_ptr,
  bzs_ext_byte_t*        source_buffer,
  size_t                 source_buffer_length)
{
  const bzs_ext_byte_t* source        = *source_ptr;
  size_t                source_length = *source_length_ptr;

  if (source != source_buffer) {
    if (source_length != 0) {
      memmove(source_buffer, source, source_length);
    }

    // Source can be accessed even if next code will fail.
    *source_ptr = source_buffer;
  }

  size_t remaining_source_buffer_length = source_buffer_length - source_length;
  if (remaining_source_buffer_length == 0) {
    // We want to read more data at once, than buffer has.
    return BZS_EXT_ERROR_NOT_ENOUGH_SOURCE_BUFFER;
  }

  bzs_ext_byte_t* remaining_source_buffer = source_buffer + source_length;
  size_t          new_source_length;

  bzs_ext_result_t ext_result =
    read_file(source_file, remaining_source_buffer, &new_source_length, remaining_source_buffer_length);

  if (ext_result != 0) {
    return ext_result;
  }

  *source_length_ptr = source_length + new_source_length;

  return 0;
}

#define BUFFERED_READ_SOURCE(function, ...)                                                                     \
  do {                                                                                                          \
    bool is_function_called = false;                                                                            \
                                                                                                                \
    while (true) {                                                                                              \
      ext_result = read_more_source(source_file, &source, &source_length, source_buffer, source_buffer_length); \
      if (ext_result == BZS_EXT_FILE_READ_FINISHED) {                                                           \
        break;                                                                                                  \
      } else if (ext_result != 0) {                                                                             \
        return ext_result;                                                                                      \
      }                                                                                                         \
                                                                                                                \
      ext_result = function(__VA_ARGS__);                                                                       \
      if (ext_result != 0) {                                                                                    \
        return ext_result;                                                                                      \
      }                                                                                                         \
                                                                                                                \
      is_function_called = true;                                                                                \
    }                                                                                                           \
                                                                                                                \
    if (!is_function_called) {                                                                                  \
      /* Function should be called at least once. */                                                            \
      ext_result = function(__VA_ARGS__);                                                                       \
      if (ext_result != 0) {                                                                                    \
        return ext_result;                                                                                      \
      }                                                                                                         \
    }                                                                                                           \
  } while (false);

// Algorithm has written data into destination buffer.
// We need to write this data into file.
// Than algorithm can use same buffer again.

static inline bzs_ext_result_t flush_destination_buffer(
  FILE*           destination_file,
  bzs_ext_byte_t* destination_buffer,
  size_t*         destination_length_ptr,
  size_t          destination_buffer_length)
{
  if (*destination_length_ptr == 0) {
    // We want to write more data at once, than buffer has.
    return BZS_EXT_ERROR_NOT_ENOUGH_DESTINATION_BUFFER;
  }

  bzs_ext_result_t ext_result = write_file(destination_file, destination_buffer, *destination_length_ptr);
  if (ext_result != 0) {
    return ext_result;
  }

  *destination_length_ptr = 0;

  return 0;
}

static inline bzs_ext_result_t
  write_remaining_destination(FILE* destination_file, bzs_ext_byte_t* destination_buffer, size_t destination_length)
{
  if (destination_length == 0) {
    return 0;
  }

  return write_file(destination_file, destination_buffer, destination_length);
}

// -- utils --

#define GET_FILE(target)                               \
  Check_Type(target, T_FILE);                          \
                                                       \
  rb_io_t* target##_io;                                \
  GetOpenFile(target, target##_io);                    \
                                                       \
  FILE* target##_file = rb_io_stdio_file(target##_io); \
  if (target##_file == NULL) {                         \
    bzs_ext_raise_error(BZS_EXT_ERROR_ACCESS_IO);      \
  }

// -- buffered compress --

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

#define BUFFERED_COMPRESS(gvl, args, RUN_OK)                                                                        \
  bzs_ext_result_t ext_result;                                                                                      \
                                                                                                                    \
  while (true) {                                                                                                    \
    bzs_ext_byte_t* remaining_destination_buffer             = destination_buffer + *destination_length_ptr;        \
    size_t          remaining_destination_buffer_length      = destination_buffer_length - *destination_length_ptr; \
    size_t          prev_remaining_destination_buffer_length = remaining_destination_buffer_length;                 \
                                                                                                                    \
    args.remaining_destination_buffer            = remaining_destination_buffer;                                    \
    args.remaining_destination_buffer_length_ptr = &remaining_destination_buffer_length;                            \
                                                                                                                    \
    BZS_EXT_GVL_WRAP(gvl, compress_wrapper, &args);                                                                 \
    if (args.result != RUN_OK && args.result != BZ_PARAM_ERROR && args.result != BZ_STREAM_END) {                   \
      return bzs_ext_get_error(args.result);                                                                        \
    }                                                                                                               \
                                                                                                                    \
    *destination_length_ptr += prev_remaining_destination_buffer_length - remaining_destination_buffer_length;      \
                                                                                                                    \
    if (args.result == BZ_STREAM_END) {                                                                             \
      break;                                                                                                        \
    }                                                                                                               \
                                                                                                                    \
    if (*args.remaining_source_length_ptr != 0 || remaining_destination_buffer_length == 0) {                       \
      ext_result = flush_destination_buffer(                                                                        \
        destination_file, destination_buffer, destination_length_ptr, destination_buffer_length);                   \
                                                                                                                    \
      if (ext_result != 0) {                                                                                        \
        return ext_result;                                                                                          \
      }                                                                                                             \
                                                                                                                    \
      continue;                                                                                                     \
    }                                                                                                               \
                                                                                                                    \
    break;                                                                                                          \
  }                                                                                                                 \
                                                                                                                    \
  return 0;

static inline bzs_ext_result_t buffered_compress(
  bz_stream*             stream_ptr,
  const bzs_ext_byte_t** source_ptr,
  size_t*                source_length_ptr,
  FILE*                  destination_file,
  bzs_ext_byte_t*        destination_buffer,
  size_t*                destination_length_ptr,
  size_t                 destination_buffer_length,
  bool                   gvl)
{
  compress_args_t run_args = {
    .stream_ptr                  = stream_ptr,
    .stream_action               = BZ_RUN,
    .remaining_source_ptr        = (bzs_ext_byte_t**) source_ptr,
    .remaining_source_length_ptr = source_length_ptr};
  BUFFERED_COMPRESS(gvl, run_args, BZ_RUN_OK);
}

// -- buffered compressor finish --

static inline bzs_ext_result_t buffered_compressor_finish(
  bz_stream*      stream_ptr,
  FILE*           destination_file,
  bzs_ext_byte_t* destination_buffer,
  size_t*         destination_length_ptr,
  size_t          destination_buffer_length,
  bool            gvl)
{
  bzs_ext_byte_t* remaining_source        = NULL;
  size_t          remaining_source_length = 0;

  compress_args_t finish_args = {
    .stream_ptr                  = stream_ptr,
    .stream_action               = BZ_FINISH,
    .remaining_source_ptr        = &remaining_source,
    .remaining_source_length_ptr = &remaining_source_length};
  BUFFERED_COMPRESS(gvl, finish_args, BZ_FINISH_OK);
}

// -- compress --

static inline bzs_ext_result_t compress(
  bz_stream*      stream_ptr,
  FILE*           source_file,
  bzs_ext_byte_t* source_buffer,
  size_t          source_buffer_length,
  FILE*           destination_file,
  bzs_ext_byte_t* destination_buffer,
  size_t          destination_buffer_length,
  bool            gvl)
{
  bzs_ext_result_t      ext_result;
  const bzs_ext_byte_t* source             = source_buffer;
  size_t                source_length      = 0;
  size_t                destination_length = 0;

  BUFFERED_READ_SOURCE(
    buffered_compress,
    stream_ptr,
    &source,
    &source_length,
    destination_file,
    destination_buffer,
    &destination_length,
    destination_buffer_length,
    gvl);

  ext_result = buffered_compressor_finish(
    stream_ptr, destination_file, destination_buffer, &destination_length, destination_buffer_length, gvl);

  if (ext_result != 0) {
    return ext_result;
  }

  return write_remaining_destination(destination_file, destination_buffer, destination_length);
}

VALUE bzs_ext_compress_io(VALUE BZS_EXT_UNUSED(self), VALUE source, VALUE destination, VALUE options)
{
  GET_FILE(source);
  GET_FILE(destination);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, source_buffer_length);
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

  if (source_buffer_length == 0) {
    source_buffer_length = BZS_DEFAULT_SOURCE_BUFFER_LENGTH_FOR_COMPRESSOR;
  }
  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_COMPRESSOR;
  }

  bzs_ext_byte_t* source_buffer;
  bzs_ext_byte_t* destination_buffer;

  bzs_ext_result_t ext_result =
    create_buffers(&source_buffer, source_buffer_length, &destination_buffer, destination_buffer_length);
  if (ext_result != 0) {
    BZ2_bzCompressEnd(&stream);
    bzs_ext_raise_error(ext_result);
  }

  ext_result = compress(
    &stream,
    source_file,
    source_buffer,
    source_buffer_length,
    destination_file,
    destination_buffer,
    destination_buffer_length,
    gvl);

  free(source_buffer);
  free(destination_buffer);
  BZ2_bzCompressEnd(&stream);

  if (ext_result != 0) {
    bzs_ext_raise_error(ext_result);
  }

  // Ruby itself won't flush stdio file before closing fd, flush is required.
  fflush(destination_file);

  return Qnil;
}

// -- buffered decompress --

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

static inline bzs_ext_result_t buffered_decompress(
  bz_stream*             stream_ptr,
  const bzs_ext_byte_t** source_ptr,
  size_t*                source_length_ptr,
  FILE*                  destination_file,
  bzs_ext_byte_t*        destination_buffer,
  size_t*                destination_length_ptr,
  size_t                 destination_buffer_length,
  bool                   gvl)
{
  bzs_ext_result_t ext_result;

  decompress_args_t args = {
    .stream_ptr                  = stream_ptr,
    .remaining_source_ptr        = (bzs_ext_byte_t**) source_ptr,
    .remaining_source_length_ptr = source_length_ptr};

  while (true) {
    bzs_ext_byte_t* remaining_destination_buffer             = destination_buffer + *destination_length_ptr;
    size_t          remaining_destination_buffer_length      = destination_buffer_length - *destination_length_ptr;
    size_t          prev_remaining_destination_buffer_length = remaining_destination_buffer_length;

    args.remaining_destination_buffer            = remaining_destination_buffer;
    args.remaining_destination_buffer_length_ptr = &remaining_destination_buffer_length;

    BZS_EXT_GVL_WRAP(gvl, decompress_wrapper, &args);
    if (args.result != BZ_OK && args.result != BZ_PARAM_ERROR && args.result != BZ_STREAM_END) {
      return bzs_ext_get_error(args.result);
    }

    *destination_length_ptr += prev_remaining_destination_buffer_length - remaining_destination_buffer_length;

    if (args.result == BZ_STREAM_END) {
      break;
    }

    if (*args.remaining_source_length_ptr != 0 || remaining_destination_buffer_length == 0) {
      ext_result = flush_destination_buffer(
        destination_file, destination_buffer, destination_length_ptr, destination_buffer_length);

      if (ext_result != 0) {
        return ext_result;
      }

      continue;
    }

    break;
  }

  return 0;
}

// -- decompress --

static inline bzs_ext_result_t decompress(
  bz_stream*      stream_ptr,
  FILE*           source_file,
  bzs_ext_byte_t* source_buffer,
  size_t          source_buffer_length,
  FILE*           destination_file,
  bzs_ext_byte_t* destination_buffer,
  size_t          destination_buffer_length,
  bool            gvl)
{
  bzs_ext_result_t      ext_result;
  const bzs_ext_byte_t* source             = source_buffer;
  size_t                source_length      = 0;
  size_t                destination_length = 0;

  BUFFERED_READ_SOURCE(
    buffered_decompress,
    stream_ptr,
    &source,
    &source_length,
    destination_file,
    destination_buffer,
    &destination_length,
    destination_buffer_length,
    gvl);

  return write_remaining_destination(destination_file, destination_buffer, destination_length);
}

VALUE bzs_ext_decompress_io(VALUE BZS_EXT_UNUSED(self), VALUE source, VALUE destination, VALUE options)
{
  GET_FILE(source);
  GET_FILE(destination);
  Check_Type(options, T_HASH);
  BZS_EXT_GET_SIZE_OPTION(options, source_buffer_length);
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

  if (source_buffer_length == 0) {
    source_buffer_length = BZS_DEFAULT_SOURCE_BUFFER_LENGTH_FOR_DECOMPRESSOR;
  }
  if (destination_buffer_length == 0) {
    destination_buffer_length = BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_DECOMPRESSOR;
  }

  bzs_ext_byte_t* source_buffer;
  bzs_ext_byte_t* destination_buffer;

  bzs_ext_result_t ext_result =
    create_buffers(&source_buffer, source_buffer_length, &destination_buffer, destination_buffer_length);
  if (ext_result != 0) {
    BZ2_bzDecompressEnd(&stream);
    bzs_ext_raise_error(ext_result);
  }

  ext_result = decompress(
    &stream,
    source_file,
    source_buffer,
    source_buffer_length,
    destination_file,
    destination_buffer,
    destination_buffer_length,
    gvl);

  free(source_buffer);
  free(destination_buffer);
  BZ2_bzDecompressEnd(&stream);

  if (ext_result != 0) {
    bzs_ext_raise_error(ext_result);
  }

  // Ruby itself won't flush stdio file before closing fd, flush is required.
  fflush(destination_file);

  return Qnil;
}

// -- exports --

void bzs_ext_io_exports(VALUE root_module)
{
  rb_define_module_function(root_module, "_native_compress_io", RUBY_METHOD_FUNC(bzs_ext_compress_io), 3);
  rb_define_module_function(root_module, "_native_decompress_io", RUBY_METHOD_FUNC(bzs_ext_decompress_io), 3);
}
