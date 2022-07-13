# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/string"
require "bzs/string"

require_relative "minitest"
require_relative "option"

module BZS
  module Test
    class String < ADSP::Test::String
      Target = BZS::String
      Option = BZS::Test::Option

      def test_invalid_text
        corrupted_compressed_text = Target.compress("1111").reverse

        assert_raises DecompressorCorruptedSourceError do
          Target.decompress corrupted_compressed_text
        end
      end
    end

    Minitest << String
  end
end
