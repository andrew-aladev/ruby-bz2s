# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/string"
require "bzs_ext"

require_relative "option"

module BZS
  # BZS::String class.
  class String < ADSP::String
    # Current option class.
    Option = BZS::Option

    # Bypasses native compress.
    def self.native_compress_string(*args)
      BZS._native_compress_string(*args)
    end

    # Bypasses native decompress.
    def self.native_decompress_string(*args)
      BZS._native_decompress_string(*args)
    end
  end
end
