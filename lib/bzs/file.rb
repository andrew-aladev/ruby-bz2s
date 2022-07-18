# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/file"
require "bzs_ext"

require_relative "option"

module BZS
  # BZS::File class.
  class File < ADSP::File
    # Current option class.
    Option = BZS::Option

    # Bypass native compress.
    def self.native_compress_io(*args)
      BZS._native_compress_io(*args)
    end

    # Bypass native decompress.
    def self.native_decompress_io(*args)
      BZS._native_decompress_io(*args)
    end
  end
end
