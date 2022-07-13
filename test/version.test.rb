# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/test/version"
require "bzs"

require_relative "minitest"

module BZS
  module Test
    class Version < ADSP::Test::Version
      def test_version
        refute_nil BZS::VERSION
        refute_nil BZS::LIBRARY_VERSION
      end
    end

    Minitest << Version
  end
end
