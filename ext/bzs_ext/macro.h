// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#if !defined(BZS_EXT_MACRO_H)
#define BZS_EXT_MACRO_H

#if defined(__GNUC__)
#define BZS_EXT_UNUSED(x) x __attribute__((__unused__))
#else
#define BZS_EXT_UNUSED(x) x
#endif // __GNUC__

#endif // BZS_EXT_MACRO_H
