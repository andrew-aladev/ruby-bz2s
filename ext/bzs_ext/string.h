// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#if !defined(BZS_EXT_STRING_H)
#define BZS_EXT_STRING_H

#include "ruby.h"

VALUE bzs_ext_compress_string(VALUE self, VALUE source, VALUE options);
VALUE bzs_ext_decompress_string(VALUE self, VALUE source, VALUE options);

void bzs_ext_string_exports(VALUE root_module);

#endif // BZS_EXT_STRING_H
