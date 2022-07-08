// Ruby bindings for bzip2 library.
// Copyright (c) 2019 AUTHORS, MIT License.

#include "bzs_ext/option.h"

#include "bzs_ext/error.h"

// -- values --

static inline VALUE get_raw_value(VALUE options, const char* name)
{
  return rb_funcall(options, rb_intern("[]"), 1, ID2SYM(rb_intern(name)));
}

static inline bool get_bool_value(VALUE raw_value)
{
  int raw_type = TYPE(raw_value);
  if (raw_type != T_TRUE && raw_type != T_FALSE) {
    bzs_ext_raise_error(BZS_EXT_ERROR_VALIDATE_FAILED);
  }

  return raw_type == T_TRUE;
}

static inline unsigned int get_uint_value(VALUE raw_value)
{
  Check_Type(raw_value, T_FIXNUM);

  return NUM2UINT(raw_value);
}

bool bzs_ext_get_bool_option_value(VALUE options, const char* name)
{
  VALUE raw_value = get_raw_value(options, name);

  return get_bool_value(raw_value);
}

uint bzs_ext_get_uint_option_value(VALUE options, const char* name)
{
  VALUE raw_value = get_raw_value(options, name);

  return get_uint_value(raw_value);
}
