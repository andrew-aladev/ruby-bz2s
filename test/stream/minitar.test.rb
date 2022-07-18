# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/stream/minitar"
require "bzs/stream/reader"
require "bzs/stream/writer"

require_relative "../minitest"

module BZS
  module Test
    module Stream
      class MinitarTest < ADSP::Test::Stream::MinitarTest
        Reader = BZS::Stream::Reader
        Writer = BZS::Stream::Writer
      end

      Minitest << MinitarTest
    end
  end
end
