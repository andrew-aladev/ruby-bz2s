# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/writer"
require "bzs/stream/writer"
require "bzs/string"

require_relative "../minitest"
require_relative "../option"

module BZS
  module Test
    module Stream
      class Writer < ADSP::Test::Stream::Writer
        Target = BZS::Stream::Writer
        Option = Test::Option
        String = BZS::String
      end

      Minitest << Writer
    end
  end
end
