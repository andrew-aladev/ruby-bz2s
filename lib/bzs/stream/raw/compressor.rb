# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/stream/raw/compressor"
require "bzs_ext"

require_relative "../../option"

module BZS
  module Stream
    module Raw
      # BZS::Stream::Raw::Compressor class.
      class Compressor < ADSP::Stream::Raw::Compressor
        # Current native compressor class.
        NativeCompressor = Stream::NativeCompressor

        # Current option class.
        Option = BZS::Option
      end
    end
  end
end
