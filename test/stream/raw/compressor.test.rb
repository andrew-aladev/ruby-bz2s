# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/raw/compressor"
require "bzs/stream/raw/compressor"
require "bzs/string"

require_relative "../../minitest"
require_relative "../../option"

module BZS
  module Test
    module Stream
      module Raw
        class Compressor < ADSP::Test::Stream::Raw::Compressor
          Target = BZS::Stream::Raw::Compressor
          Option = Test::Option
          String = BZS::String
        end

        Minitest << Compressor
      end
    end
  end
end
