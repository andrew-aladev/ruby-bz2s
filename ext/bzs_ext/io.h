// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#if !defined(BZS_EXT_IO_H)
#define BZS_EXT_IO_H

#include "ruby.h"

VALUE bzs_ext_compress_io(VALUE self, VALUE source, VALUE destination, VALUE options);
VALUE bzs_ext_decompress_io(VALUE self, VALUE source, VALUE destination, VALUE options);

void bzs_ext_io_exports(VALUE root_module);

#endif // BZS_EXT_IO_H
