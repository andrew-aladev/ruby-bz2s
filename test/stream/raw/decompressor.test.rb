# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/raw/decompressor"
require "bzs/stream/raw/decompressor"
require "bzs/string"

require_relative "../../minitest"
require_relative "../../option"

module BZS
  module Test
    module Stream
      module Raw
        class Decompressor < ADSP::Test::Stream::Raw::Decompressor
          Target = BZS::Stream::Raw::Decompressor
          Option = Test::Option
          String = BZS::String

          def test_invalid_read
            super

            decompressor = Target.new

            corrupted_compressed_text = String.compress("1111").reverse

            assert_raises DecompressorCorruptedSourceError do
              decompressor.read corrupted_compressed_text, &NOOP_PROC
            end
          end
        end

        Minitest << Decompressor
      end
    end
  end
end
