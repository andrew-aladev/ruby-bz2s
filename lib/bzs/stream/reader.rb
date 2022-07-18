# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/stream/reader"

require_relative "raw/decompressor"

module BZS
  module Stream
    # BZS::Stream::Reader class.
    class Reader < ADSP::Stream::Reader
      # Current raw stream class.
      RawDecompressor = Raw::Decompressor
    end
  end
end
