# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "adsp/validation"

module BZS
  # BZS::Validation class.
  class Validation < ADSP::Validation
    # Raises error when +value+ is not boolean.
    def self.validate_bool(value)
      raise ValidateError, "invalid bool" unless value.is_a?(::TrueClass) || value.is_a?(::FalseClass)
    end
  end
end
