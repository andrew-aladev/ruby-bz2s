// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#if !defined(BZS_EXT_GVL_H)
#define BZS_EXT_GVL_H

#if defined(HAVE_RB_THREAD_CALL_WITHOUT_GVL)

#include "ruby/thread.h"

#define BZS_EXT_GVL_WRAP(gvl, function, data)                              \
  if (gvl) {                                                               \
    function((void*) data);                                                \
  } else {                                                                 \
    rb_thread_call_without_gvl(function, (void*) data, RUBY_UBF_IO, NULL); \
  }

#else

#define BZS_EXT_GVL_WRAP(_gvl, function, data) function((void*) data);

#endif // HAVE_RB_THREAD_CALL_WITHOUT_GVL

#endif // BZS_EXT_GVL_H
