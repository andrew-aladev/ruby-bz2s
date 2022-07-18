# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/stream/raw/decompressor"
require "bzs_ext"

require_relative "../../option"

module BZS
  module Stream
    module Raw
      # BZS::Stream::Raw::Decompressor class.
      class Decompressor < ADSP::Stream::Raw::Decompressor
        # Current native decompressor class.
        NativeDecompressor = Stream::NativeDecompressor

        # Current option class.
        Option = BZS::Option
      end
    end
  end
end
