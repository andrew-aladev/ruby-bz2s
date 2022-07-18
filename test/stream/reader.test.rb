# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/reader"
require "bzs/stream/reader"
require "bzs/string"
require "stringio"

require_relative "../minitest"
require_relative "../option"

module BZS
  module Test
    module Stream
      class Reader < ADSP::Test::Stream::Reader
        Target = BZS::Stream::Reader
        Option = Test::Option
        String = BZS::String

        def test_invalid_read
          super

          corrupted_compressed_text = String.compress("1111").reverse
          instance                  = target.new ::StringIO.new(corrupted_compressed_text)

          assert_raises DecompressorCorruptedSourceError do
            instance.read
          end
        end

        def test_invalid_readpartial_and_read_nonblock
          super

          corrupted_compressed_text = String.compress("1111").reverse

          instance = target.new ::StringIO.new(corrupted_compressed_text)

          assert_raises DecompressorCorruptedSourceError do
            instance.readpartial 1
          end

          instance = target.new ::StringIO.new(corrupted_compressed_text)

          assert_raises DecompressorCorruptedSourceError do
            instance.read_nonblock 1
          end
        end
      end

      Minitest << Reader
    end
  end
end
