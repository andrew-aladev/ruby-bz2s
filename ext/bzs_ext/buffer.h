// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#if !defined(BZS_EXT_BUFFER_H)
#define BZS_EXT_BUFFER_H

#include "ruby.h"

#define BZS_DEFAULT_SOURCE_BUFFER_LENGTH_FOR_COMPRESSOR      (1 << 18) // 256 KB
#define BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_COMPRESSOR (1 << 16) // 64 KB

#define BZS_DEFAULT_SOURCE_BUFFER_LENGTH_FOR_DECOMPRESSOR      (1 << 16) // 64 KB
#define BZS_DEFAULT_DESTINATION_BUFFER_LENGTH_FOR_DECOMPRESSOR (1 << 18) // 256 KB

VALUE bzs_ext_create_string_buffer(VALUE length);

#define BZS_EXT_CREATE_STRING_BUFFER(buffer, length, exception) \
  VALUE buffer = rb_protect(bzs_ext_create_string_buffer, SIZET2NUM(length), &exception);

VALUE bzs_ext_resize_string_buffer(VALUE buffer_args);

#define BZS_EXT_RESIZE_STRING_BUFFER(buffer, length, exception)                          \
  VALUE buffer_args = rb_ary_new_from_args(2, buffer, SIZET2NUM(length));                \
  buffer            = rb_protect(bzs_ext_resize_string_buffer, buffer_args, &exception); \
  RB_GC_GUARD(buffer_args);

void bzs_ext_buffer_exports(VALUE root_module);

#endif // BZS_EXT_BUFFER_H
