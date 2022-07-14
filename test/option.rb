# Ruby bindings for bzip2 library.
# Copyright (c) 2022 AUTHORS, MIT License.

require "bzs/option"
require "ocg"

require_relative "validation"

module BZS
  module Test
    module Option
      INVALID_BLOCK_SIZES = (
        Validation::INVALID_NOT_NEGATIVE_INTEGERS - [nil] +
        [
          BZS::Option::MIN_BLOCK_SIZE - 1,
          BZS::Option::MAX_BLOCK_SIZE + 1
        ]
      )
      .freeze

      INVALID_WORK_FACTORS = (
        Validation::INVALID_NOT_NEGATIVE_INTEGERS - [nil] +
        [
          BZS::Option::MIN_WORK_FACTOR - 1,
          BZS::Option::MAX_WORK_FACTOR + 1
        ]
      )
      .freeze

      def self.get_invalid_decompressor_options(buffer_length_names, &block)
        Validation::INVALID_HASHES.each(&block)

        buffer_length_names.each do |name|
          (Validation::INVALID_NOT_NEGATIVE_INTEGERS - [nil]).each do |invalid_integer|
            yield({ name => invalid_integer })
          end
        end

        Validation::INVALID_BOOLS.each do |invalid_bool|
          yield({ :gvl => invalid_bool })
        end

        (Validation::INVALID_BOOLS - [nil]).each do |invalid_bool|
          yield({ :small => invalid_bool })
          yield({ :quiet => invalid_bool })
        end
      end

      def self.get_invalid_compressor_options(buffer_length_names, &block)
        Validation::INVALID_HASHES.each(&block)

        buffer_length_names.each do |name|
          (Validation::INVALID_NOT_NEGATIVE_INTEGERS - [nil]).each do |invalid_integer|
            yield({ name => invalid_integer })
          end
        end

        Validation::INVALID_BOOLS.each do |invalid_bool|
          yield({ :gvl => invalid_bool })
        end

        INVALID_BLOCK_SIZES.each do |invalid_block_size|
          yield({ :block_size => invalid_block_size })
        end

        INVALID_WORK_FACTORS.each do |invalid_work_factor|
          yield({ :work_factor => invalid_work_factor })
        end

        (Validation::INVALID_BOOLS - [nil]).each do |invalid_bool|
          yield({ :quiet => invalid_bool })
        end
      end

      # -----

      # "0" means default buffer length.
      # "512" bytes is the minimal buffer length for compressor and decompressor.
      BUFFER_LENGTHS = [
        0,
        512
      ]
      .freeze

      BOOLS = [
        true,
        false
      ]
      .freeze

      BLOCK_SIZES = Range.new(
        BZS::Option::MIN_BLOCK_SIZE,
        BZS::Option::MAX_BLOCK_SIZE
      )
      .freeze

      WORK_FACTORS = Range.new(
        BZS::Option::MIN_WORK_FACTOR,
        BZS::Option::MAX_WORK_FACTOR
      )
      .freeze

      private_class_method def self.get_buffer_length_option_generator(buffer_length_names)
        OCG.new(
          buffer_length_names.to_h { |name| [name, BUFFER_LENGTHS] }
        )
      end

      def self.get_compressor_options_generator(buffer_length_names)
        buffer_length_generator = get_buffer_length_option_generator buffer_length_names

        # main

        main_generator = OCG.new(
          :block_size  => BLOCK_SIZES,
          :work_factor => WORK_FACTORS
        )

        # thread

        thread_generator = OCG.new(
          :gvl => BOOLS
        )

        # other

        other_generator = OCG.new(
          :quiet => BOOLS
        )

        # complete

        buffer_length_generator.mix(main_generator).mix(thread_generator).mix other_generator
      end

      def self.get_compatible_decompressor_options(compressor_options, buffer_length_name_mapping, &_block)
        decompressor_options = {
          :gvl   => compressor_options[:gvl],
          :quiet => compressor_options[:quiet]
        }

        buffer_length_name_mapping.each do |compressor_name, decompressor_name|
          decompressor_options[decompressor_name] = compressor_options[compressor_name]
        end

        other_generator = OCG.new(
          :small => BOOLS
        )

        other_generator.each do |other_options|
          yield decompressor_options.merge(other_options)
        end
      end
    end
  end
end
