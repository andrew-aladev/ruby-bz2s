// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#if !defined(BZS_EXT_STREAM_COMPRESSOR_H)
#define BZS_EXT_STREAM_COMPRESSOR_H

#include <bzlib.h>
#include <stdbool.h>

#include "bzs_ext/common.h"
#include "ruby.h"

typedef struct
{
  bz_stream*      stream_ptr;
  bzs_ext_byte_t* destination_buffer;
  size_t          destination_buffer_length;
  bzs_ext_byte_t* remaining_destination_buffer;
  size_t          remaining_destination_buffer_length;
  bool            gvl;
} bzs_ext_compressor_t;

VALUE bzs_ext_allocate_compressor(VALUE klass);
VALUE bzs_ext_initialize_compressor(VALUE self, VALUE options);
VALUE bzs_ext_compress(VALUE self, VALUE source);
VALUE bzs_ext_flush_compressor(VALUE self);
VALUE bzs_ext_finish_compressor(VALUE self);
VALUE bzs_ext_compressor_read_result(VALUE self);
VALUE bzs_ext_compressor_close(VALUE self);

void bzs_ext_compressor_exports(VALUE root_module);

#endif // BZS_EXT_STREAM_COMPRESSOR_H
