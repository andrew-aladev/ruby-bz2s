# Ruby bindings for bzip2 library

| Github actions | Codecov | Gem  |
| :------------: | :-----: | :--: |
| [![Github Actions test status](https://github.com/andrew-aladev/ruby-bzs/workflows/test/badge.svg?branch=main)](https://github.com/andrew-aladev/ruby-bzs/actions) | [![Codecov](https://codecov.io/gh/andrew-aladev/ruby-bzs/branch/main/graph/badge.svg)](https://codecov.io/gh/andrew-aladev/ruby-bzs) | [![Gem](https://img.shields.io/gem/v/ruby-bzs.svg)](https://rubygems.org/gems/ruby-bzs) |

See [bzip2 library](https://gitlab.com/bzip2/bzip2).

Other bindings: [lzw](https://github.com/andrew-aladev/ruby-lzws), [brotli](https://github.com/andrew-aladev/ruby-brs), [zstd](https://github.com/andrew-aladev/ruby-zstds).

## Installation

Operating systems: GNU/Linux, FreeBSD, OSX.

Dependencies: [bzip2](https://gitlab.com/bzip2/bzip2) 1.0.0+ version.

| Popular OS | Dependencies              |
|------------|---------------------------|
| Ubuntu     | `libbz2-dev`              |
| CentOS     | `bzip2-devel`             |
| ArchLinux  | `bzip2`                   |
| OSX        | `bzip2`                   |

```sh
gem install ruby-bzs
```

You can build it from source.

```sh
rake gem
gem install pkg/ruby-bzs-*.gem
```

You can also use [overlay](https://github.com/andrew-aladev/overlay) for gentoo.

### Installation in macOS on Apple Silicon

On M1 Macs, Homebrew installs to /opt/homebrew, so you'll need to specify its
include and lib paths when building the native extension for bzip2.

```sh
brew install bzip2
gem install ruby-bzs -- --with-opt-include=/opt/homebrew/include --with-opt-lib=/opt/homebrew/lib
```

You can also configure Bundler to use those options when installing:

```sh
bundle config set build.ruby-bzs "--with-opt-include=/opt/homebrew/include --with-opt-lib=/opt/homebrew/lib"
```

## Usage

There are simple APIs: `String` and `File`. Also you can use generic streaming API: `Stream::Writer` and `Stream::Reader`.

```ruby
require "bzs"

data = BZS::String.compress "sample string"
puts BZS::String.decompress(data)

BZS::File.compress "file.txt", "file.txt.bz2"
BZS::File.decompress "file.txt.bz2", "file.txt"

BZS::Stream::Writer.open("file.txt.bz2") { |writer| writer << "sample string" }
puts BZS::Stream::Reader.open("file.txt.bz2") { |reader| reader.read }

writer = BZS::Stream::Writer.new output_socket
begin
  bytes_written = writer.write_nonblock "sample string"
  # handle "bytes_written"
rescue IO::WaitWritable
  # handle wait
ensure
  writer.close
end

reader = BZS::Stream::Reader.new input_socket
begin
  puts reader.read_nonblock(512)
rescue IO::WaitReadable
  # handle wait
rescue ::EOFError
  # handle eof
ensure
  reader.close
end
```

You can create and read `tar.bz2` archives with [minitar](https://github.com/halostatue/minitar).

```ruby
require "bzs"
require "minitar"

BZS::Stream::Writer.open "file.tar.bz2" do |writer|
  Minitar::Writer.open writer do |tar|
    tar.add_file_simple "file", :data => "sample string"
  end
end

BZS::Stream::Reader.open "file.tar.bz2" do |reader|
  Minitar::Reader.open reader do |tar|
    tar.each_entry do |entry|
      puts entry.name
      puts entry.read
    end
  end
end
```

All functionality (including streaming) can be used inside multiple threads with [parallel](https://github.com/grosser/parallel).
This code will provide heavy load for your CPU.

```ruby
require "bzs"
require "parallel"

Parallel.each large_datas do |large_data|
  BZS::String.compress large_data
end
```

# Docs

Please review [rdoc generated docs](https://andrew-aladev.github.io/ruby-bzs).

## Options

| Option                          | Values         | Default    | Description |
|---------------------------------|----------------|------------|-------------|
| `source_buffer_length`          | 0 - inf        | 0 (auto)   | internal buffer length for source data |
| `destination_buffer_length`     | 0 - inf        | 0 (auto)   | internal buffer length for description data |
| `gvl`                           | true/false     | false      | enables global VM lock where possible |
| `block_size`                    | 1 - 9          | 9          | block size to be used for compression |
| `work_factor`                   | 0 - 250        | 0          | controls threshold for switching from standard to fallback algorithm |
| `small`                         | true/false     | true       | enables alternative decompression algorithm with less memory |
| `quiet`                         | true/false     | false      | disables bzip2 library logging |

There are internal buffers for compressed and decompressed data.
For example you want to use 1 KB as `source_buffer_length` for compressor - please use 256 B as `destination_buffer_length`.
You want to use 256 B as `source_buffer_length` for decompressor - please use 1 KB as `destination_buffer_length`.

`gvl` is disabled by default, this mode allows running multiple compressors/decompressors in different threads simultaneously.
Please consider enabling `gvl` if you don't want to launch processors in separate threads.
If `gvl` is enabled ruby won't waste time on acquiring/releasing VM lock.

You can also read bzs docs for more info about options.

Possible compressor options:
```
:source_buffer_length
:destination_buffer_length
:gvl
:block_size
:work_factor
:quiet
```

Possible decompressor options:
```
:source_buffer_length
:destination_buffer_length
:gvl
:small
:quiet
```

Example:

```ruby
require "bzs"

data = BZS::String.compress "sample string", :block_size => 1
puts BZS::String.decompress(data, :block_size => 1)
```

## String

String maintains destination buffer only, so it accepts `destination_buffer_length` option only.

```
::compress(source, options = {})
::decompress(source, options = {})
```

`source` is a source string.

## File

File maintains both source and destination buffers, it accepts both `source_buffer_length` and `destination_buffer_length` options.

```
::compress(source, destination, options = {})
::decompress(source, destination, options = {})
```

`source` and `destination` are file pathes.

## Stream::Writer

Its behaviour is similar to builtin [`Zlib::GzipWriter`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipWriter.html).

Writer maintains destination buffer only, so it accepts `destination_buffer_length` option only.

```
::open(file_path, options = {}, :external_encoding => nil, :transcode_options => {}, &block)
```

Open file path and create stream writer associated with opened file.
Data will be transcoded to `:external_encoding` using `:transcode_options` before compressing.

It may be tricky to use both `:pledged_size` and `:transcode_options`. You have to provide size of transcoded input.

```
::new(destination_io, options = {}, :external_encoding => nil, :transcode_options => {})
```

Create stream writer associated with destination io.
Data will be transcoded to `:external_encoding` using `:transcode_options` before compressing.

It may be tricky to use both `:pledged_size` and `:transcode_options`. You have to provide size of transcoded input.

```
#set_encoding(external_encoding, nil, transcode_options)
```

Set another encodings, `nil` is just for compatibility with `IO`.

```
#io
#to_io
#stat
#external_encoding
#transcode_options
#pos
#tell
```

See [`IO`](https://ruby-doc.org/core/IO.html) docs.

```
#write(*objects)
#flush
#rewind
#close
#closed?
```

See [`Zlib::GzipWriter`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipWriter.html) docs.

```
#write_nonblock(object, *options)
#flush_nonblock(*options)
#rewind_nonblock(*options)
#close_nonblock(*options)
```

Special asynchronous methods missing in `Zlib::GzipWriter`.
`rewind` wants to `close`, `close` wants to `write` something and `flush`, `flush` want to `write` something.
So it is possible to have asynchronous variants for these synchronous methods.
Behaviour is the same as `IO#write_nonblock` method.

```
#<<(object)
#print(*objects)
#printf(*args)
#putc(object, :encoding => 'ASCII-8BIT')
#puts(*objects)
```

Typical helpers, see [`Zlib::GzipWriter`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipWriter.html) docs.

## Stream::Reader

Its behaviour is similar to builtin [`Zlib::GzipReader`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipReader.html).

Reader maintains both source and destination buffers, it accepts both `source_buffer_length` and `destination_buffer_length` options.

```
::open(file_path, options = {}, :external_encoding => nil, :internal_encoding => nil, :transcode_options => {}, &block)
```

Open file path and create stream reader associated with opened file.
Data will be force encoded to `:external_encoding` and transcoded to `:internal_encoding` using `:transcode_options` after decompressing.

```
::new(source_io, options = {}, :external_encoding => nil, :internal_encoding => nil, :transcode_options => {})
```

Create stream reader associated with source io.
Data will be force encoded to `:external_encoding` and transcoded to `:internal_encoding` using `:transcode_options` after decompressing.

```
#set_encoding(external_encoding, internal_encoding, transcode_options)
```

Set another encodings.

```
#io
#to_io
#stat
#external_encoding
#internal_encoding
#transcode_options
#pos
#tell
```

See [`IO`](https://ruby-doc.org/core/IO.html) docs.

```
#read(bytes_to_read = nil, out_buffer = nil)
#eof?
#rewind
#close
#closed?
```

See [`Zlib::GzipReader`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipReader.html) docs.

```
#readpartial(bytes_to_read = nil, out_buffer = nil)
#read_nonblock(bytes_to_read, out_buffer = nil, *options)
```

See [`IO`](https://ruby-doc.org/core/IO.html) docs.

```
#getbyte
#each_byte(&block)
#readbyte
#ungetbyte(byte)

#getc
#readchar
#each_char(&block)
#ungetc(char)

#lineno
#lineno=
#gets(separator = $OUTPUT_RECORD_SEPARATOR, limit = nil)
#readline
#readlines
#each(&block)
#each_line(&block)
#ungetline(line)
```

Typical helpers, see [`Zlib::GzipReader`](https://ruby-doc.org/stdlib/libdoc/zlib/rdoc/Zlib/GzipReader.html) docs.

## Thread safety

`:gvl` option is disabled by default, you can use bindings effectively in multiple threads.
Please be careful: bindings are not thread safe.
You should lock all shared data between threads.

For example: you should not use same compressor/decompressor inside multiple threads.
Please verify that you are using each processor inside single thread at the same time.

## CI

Please visit [scripts/test-images](scripts/test-images).
See universal test script [scripts/ci_test.sh](scripts/ci_test.sh) for CI.

## License

MIT license, see [LICENSE](LICENSE) and [AUTHORS](AUTHORS).
