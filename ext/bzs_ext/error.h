// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#if !defined(BZS_EXT_ERROR_H)
#define BZS_EXT_ERROR_H

#include "bzs_ext/common.h"
#include "ruby.h"

// Results for errors listed in "lib/bzs/error" used in c extension.

enum
{
  BZS_EXT_ERROR_ALLOCATE_FAILED = 1,
  BZS_EXT_ERROR_VALIDATE_FAILED,

  BZS_EXT_ERROR_USED_AFTER_CLOSE,
  BZS_EXT_ERROR_NOT_ENOUGH_SOURCE_BUFFER,
  BZS_EXT_ERROR_NOT_ENOUGH_DESTINATION_BUFFER,
  BZS_EXT_ERROR_DECOMPRESSOR_CORRUPTED_SOURCE,

  BZS_EXT_ERROR_ACCESS_IO,
  BZS_EXT_ERROR_READ_IO,
  BZS_EXT_ERROR_WRITE_IO,

  BZS_EXT_ERROR_NOT_IMPLEMENTED,
  BZS_EXT_ERROR_UNEXPECTED
};

bzs_ext_result_t bzs_ext_get_error(bzs_result_t error_code);

NORETURN(void bzs_ext_raise_error(bzs_ext_result_t ext_result));

#endif // BZS_EXT_ERROR_H
