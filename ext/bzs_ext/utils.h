// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#if !defined(BZS_EXT_UTILS_H)
#define BZS_EXT_UTILS_H

#include <stdlib.h>

// Bzip2 size type may be limited to unsigned int.
// We need to prevent overflow by consuming max available unsigned int value.
unsigned int bzs_consume_size(size_t size);

#endif // BZS_EXT_UTILS_H
