# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/file"
require "bzs/file"

require_relative "minitest"
require_relative "option"

module BZS
  module Test
    class File < ADSP::Test::File
      Target = BZS::File
      Option = BZS::Test::Option
    end

    Minitest << File
  end
end
