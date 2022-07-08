// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#if !defined(BZS_EXT_OPTIONS_H)
#define BZS_EXT_OPTIONS_H

#include <stdbool.h>

#include "ruby.h"

bool         bzs_ext_get_bool_option_value(VALUE options, const char* name);
unsigned int bzs_ext_get_uint_option_value(VALUE options, const char* name);

#define BZS_EXT_GET_BOOL_OPTION(options, name) bool name = bzs_ext_get_bool_option_value(options, #name);
#define BZS_EXT_GET_UINT_OPTION(options, name) unsigned int name = bzs_ext_get_uint_option_value(options, #name);

#define BZS_EXT_GET_COMPRESSOR_OPTIONS(options) \
  BZS_EXT_GET_UINT_OPTION(options, blockSize);  \
  BZS_EXT_GET_UINT_OPTION(options, verbosity);  \
  BZS_EXT_GET_UINT_OPTION(options, workFactor);

#define BZS_EXT_GET_DECOMPRESSOR_OPTIONS(options) \
  BZS_EXT_GET_BOOL_OPTION(options, small);        \
  BZS_EXT_GET_UINT_OPTION(options, verbosity);

#endif // BZS_EXT_OPTIONS_H
