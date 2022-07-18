# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/stream/writer"

require_relative "raw/compressor"

module BZS
  module Stream
    # BZS::Stream::Writer class.
    class Writer < ADSP::Stream::Writer
      # Current raw stream class.
      RawCompressor = Raw::Compressor
    end
  end
end
