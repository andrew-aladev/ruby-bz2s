# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/writer_helpers"
require "bzs/stream/writer"
require "bzs/string"

require_relative "../minitest"
require_relative "../option"

module BZS
  module Test
    module Stream
      class WriterHelpers < ADSP::Test::Stream::WriterHelpers
        Target = BZS::Stream::Writer
        Option = Test::Option
        String = BZS::String
      end

      Minitest << WriterHelpers
    end
  end
end
