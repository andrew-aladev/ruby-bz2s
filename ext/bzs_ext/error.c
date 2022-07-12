// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include <bzlib.h>

#include "bzs_ext/error.h"

bzs_ext_result_t bzs_ext_get_error(bzs_result_t error_code)
{
  switch (error_code) {
    case BZ_MEM_ERROR:
      return BZS_EXT_ERROR_ALLOCATE_FAILED;
    case BZ_PARAM_ERROR:
      return BZS_EXT_ERROR_VALIDATE_FAILED;
    case BZ_DATA_ERROR:
    case BZ_DATA_ERROR_MAGIC:
    case BZ_UNEXPECTED_EOF:
      return BZS_EXT_ERROR_DECOMPRESSOR_CORRUPTED_SOURCE;
    default:
      return BZS_EXT_ERROR_UNEXPECTED;
  }
}

static inline NORETURN(void raise_error(const char* name, const char* description))
{
  VALUE module = rb_define_module(BZS_EXT_MODULE_NAME);
  VALUE error  = rb_const_get(module, rb_intern(name));
  rb_raise(error, "%s", description);
}

void bzs_ext_raise_error(bzs_ext_result_t ext_result)
{
  switch (ext_result) {
    case BZS_EXT_ERROR_ALLOCATE_FAILED:
      raise_error("AllocateError", "allocate error");
    case BZS_EXT_ERROR_VALIDATE_FAILED:
      raise_error("ValidateError", "validate error");

    case BZS_EXT_ERROR_USED_AFTER_CLOSE:
      raise_error("UsedAfterCloseError", "used after closed");
    case BZS_EXT_ERROR_NOT_ENOUGH_SOURCE_BUFFER:
      raise_error("NotEnoughSourceBufferError", "not enough source buffer");
    case BZS_EXT_ERROR_NOT_ENOUGH_DESTINATION_BUFFER:
      raise_error("NotEnoughDestinationBufferError", "not enough destination buffer");
    case BZS_EXT_ERROR_DECOMPRESSOR_CORRUPTED_SOURCE:
      raise_error("DecompressorCorruptedSourceError", "decompressor received corrupted source");

    case BZS_EXT_ERROR_ACCESS_IO:
      raise_error("AccessIOError", "failed to access IO");
    case BZS_EXT_ERROR_READ_IO:
      raise_error("ReadIOError", "failed to read IO");
    case BZS_EXT_ERROR_WRITE_IO:
      raise_error("WriteIOError", "failed to write IO");

    case BZS_EXT_ERROR_NOT_IMPLEMENTED:
      raise_error("NotImplementedError", "not implemented error");

    default:
      // BZS_EXT_ERROR_UNEXPECTED
      raise_error("UnexpectedError", "unexpected error");
  }
}
