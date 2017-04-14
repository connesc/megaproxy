# megaproxy

> HTTP proxy for mega.nz

This is just a wrapper program. The core feature is actually implemented by the [official mega.nz SDK](https://github.com/meganz/sdk).
See [here](https://github.com/meganz/sdk/blob/v3.0.0/include/megaapi.h#L8719) for more details.

## Usage

```
Usage: megaproxy [options]

Generic options:
  -h [ --help ]          produce help message
  -c [ --config ] arg    configuration file

Configuration:
  -u [ --user ] arg      mega.nz username (required)
  -p [ --pass ] arg      mega.nz password (required)
  --local-only arg (=1)  listen on 127.0.0.1 only
  --port arg (=4443)     listening port
  --files arg (=1)       allow to serve files
  --folders arg (=1)     allow to serve folders
  --subtitles arg (=0)   enable subtitles support
  --buffer arg (=0)      maximum buffer size (in bytes)
  --output arg (=0)      maximum output size (in bytes)
```

## Dependencies

This project depends on the [mega.nz SDK](https://github.com/meganz/sdk) and Boost ([program_options](https://github.com/boostorg/program_options)).

Note: the mega.nz SDK must be built with libuv support. This can be done like this:

```
./autogen
./configure CXXFLAGS=-DHAVE_LIBUV=1 LDFLAGS=-luv
make
```

## Installation

Arch Linux users can install the [megaproxy package](https://aur.archlinux.org/packages/megaproxy) from the AUR.

Other users can simply build the binary with `make` (assuming the above dependencies are installed).

## License

[ISC](https://github.com/connesc/megaproxy/blob/master/LICENSE)
