# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/reader_helpers"
require "bzs/stream/reader"
require "bzs/string"

require_relative "../minitest"
require_relative "../option"

module BZS
  module Test
    module Stream
      class ReaderHelpers < ADSP::Test::Stream::ReaderHelpers
        Target = BZS::Stream::Reader
        Option = Test::Option
        String = BZS::String
      end

      Minitest << ReaderHelpers
    end
  end
end
