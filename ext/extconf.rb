# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "mkmf"

have_func "rb_thread_call_without_gvl", "ruby/thread.h"

def require_header(name, constants: [], types: [])
  abort "Can't find #{name} header" unless find_header name

  constants.each do |constant|
    abort "Can't find #{constant} constant in #{name} header" unless have_const constant, name
  end

  types.each do |type|
    abort "Can't find #{type} type in #{name} header" unless find_type type, nil, name
  end
end

require_header(
  "bzlib.h",
  :constants => %w[
    BZ_RUN
    BZ_RUN_OK
    BZ_FINISH
    BZ_FINISH_OK
    BZ_FLUSH
    BZ_FLUSH_OK
    BZ_OK
    BZ_PARAM_ERROR
    BZ_STREAM_END
    BZ_MEM_ERROR
    BZ_DATA_ERROR
    BZ_DATA_ERROR_MAGIC
    BZ_UNEXPECTED_EOF
  ],
  :types     => %w[
    bz_stream
  ]
)

def require_library(name, functions)
  functions.each do |function|
    abort "Can't find #{function} function in #{name} library" unless find_library name, function
  end
end

require_library(
  "bz2",
  %w[
    BZ2_bzCompressInit
    BZ2_bzCompress
    BZ2_bzCompressEnd
    BZ2_bzDecompressInit
    BZ2_bzDecompress
    BZ2_bzDecompressEnd
    BZ2_bzlibVersion
  ]
)

extension_name = "bzs_ext".freeze
dir_config extension_name

# rubocop:disable Style/GlobalVars
$srcs = %w[
  buffer
  error
  main
  option
  string
  utils
]
.map { |name| "src/#{extension_name}/#{name}.c" }
.freeze

# Removing library duplicates.
$libs = $libs.split(%r{\s})
  .reject(&:empty?)
  .sort
  .uniq
  .join " "

if ENV["CI"]
  $CFLAGS << " --coverage"
  $LDFLAGS << " --coverage"
end

$VPATH << "$(srcdir)/#{extension_name}:$(srcdir)/#{extension_name}/stream"
# rubocop:enable Style/GlobalVars

create_makefile extension_name
