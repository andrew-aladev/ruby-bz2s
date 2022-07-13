# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "bzs_ext"

require_relative "error"
require_relative "validation"

module BZS
  # BZS::Option module.
  module Option
    # Current default buffer length.
    DEFAULT_BUFFER_LENGTH = 0

    # Current compressor defaults.
    COMPRESSOR_DEFAULTS = {
      # Enables global VM lock where possible.
      :gvl                  => false,
      # Block size to be used for compression.
      :block_size           => nil,
      # Controls threshold for switching from standard to fallback algorithm.
      :work_factor          => nil,
      # Disables bzip2 library logging.
      :quiet                => nil
    }
    .freeze

    # Current decompressor defaults.
    DECOMPRESSOR_DEFAULTS = {
      # Enables global VM lock where possible.
      :gvl                  => false,
      # Enables alternative decompression algorithm with less memory.
      :small                => nil,
      # Disables bzip2 library logging.
      :quiet                => nil
    }
    .freeze

    # Processes compressor +options+ and +buffer_length_names+.
    # Option: +:source_buffer_length+ source buffer length.
    # Option: +:destination_buffer_length+ destination buffer length.
    # Option: +:gvl+ enables global VM lock where possible.
    # Option: +:block_size+ block size to be used for compression.
    # Option: +:work_factor+ controls threshold for switching from standard to fallback algorithm.
    # Option: +:quiet+ disables bzip2 library logging.
    # Returns processed compressor options.
    def self.get_compressor_options(options, buffer_length_names)
      Validation.validate_hash options

      buffer_length_defaults = buffer_length_names.each_with_object({}) do |name, defaults|
        defaults[name] = DEFAULT_BUFFER_LENGTH
      end

      options = COMPRESSOR_DEFAULTS.merge(buffer_length_defaults).merge options

      buffer_length_names.each { |name| Validation.validate_not_negative_integer options[name] }

      Validation.validate_bool options[:gvl]

      block_size = options[:block_size]
      Validation.validate_positive_integer block_size unless block_size.nil?

      work_factor = options[:work_factor]
      Validation.validate_positive_integer work_factor unless work_factor.nil?

      quiet = options[:quiet]
      Validation.validate_bool quiet unless quiet.nil?

      options
    end

    # Processes decompressor +options+ and +buffer_length_names+.
    # Option: +:source_buffer_length+ source buffer length.
    # Option: +:destination_buffer_length+ destination buffer length.
    # Option: +:gvl+ enables global VM lock where possible.
    # Option: +:small+ enables alternative decompression algorithm with less memory.
    # Option: +:quiet+ disables bzip2 library logging.
    # Returns processed decompressor options.
    def self.get_decompressor_options(options, buffer_length_names)
      Validation.validate_hash options

      buffer_length_defaults = buffer_length_names.each_with_object({}) do |name, defaults|
        defaults[name] = DEFAULT_BUFFER_LENGTH
      end

      options = DECOMPRESSOR_DEFAULTS.merge(buffer_length_defaults).merge options

      buffer_length_names.each { |name| Validation.validate_not_negative_integer options[name] }

      Validation.validate_bool options[:gvl]

      small = options[:small]
      Validation.validate_bool small unless small.nil?

      quiet = options[:quiet]
      Validation.validate_bool quiet unless quiet.nil?

      options
    end
  end
end
