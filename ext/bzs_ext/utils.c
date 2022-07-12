// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include "utils.h"

#include <limits.h>

unsigned int bzs_consume_size(size_t size)
{
  if (size > UINT_MAX) {
    return UINT_MAX;
  } else {
    return (unsigned int) size;
  }
}
