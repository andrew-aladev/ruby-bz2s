// Ruby bindings for bzip2 library.
// Copyright (c) 2022 AUTHORS, MIT License.

#include <bzlib.h>

#include "bzs_ext/buffer.h"
#include "bzs_ext/common.h"
#include "bzs_ext/io.h"
#include "bzs_ext/option.h"
#include "bzs_ext/string.h"

void Init_bzs_ext()
{
  VALUE root_module = rb_define_module(BZS_EXT_MODULE_NAME);

  bzs_ext_buffer_exports(root_module);
  bzs_ext_io_exports(root_module);
  bzs_ext_option_exports(root_module);
  bzs_ext_string_exports(root_module);

  VALUE version = rb_str_new2(BZ2_bzlibVersion());
  rb_define_const(root_module, "LIBRARY_VERSION", version);
}
