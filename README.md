# rpotool

[![Release](https://img.shields.io/github/release/tylertms/rpotool.svg?label=Release)](https://GitHub.com/tylertms/rpotool/releases/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/tylertms/rpotool/blob/main/LICENSE)

- [Installation](#installation)
- [Usage](#usage)
- [Compiling](#compiling)
- [License](#license)


## Installation

Get the latest version of `rpotool` from [GitHub releases](https://github.com/tylertms/rpotool/releases).

## Usage

To convert an .rpo or .rpoz file, use the `convert` command.

```
Convert an Egg, Inc. .rpo(z) file to the .obj format.

Usage:
  ./rpotool convert <file.rpo(z)> [flags]

Flags:
  -o, --output    Specify the name of the output file
  -h, --help      Display this help message
```

To browse shell .rpoz files, use the `fetch` command.

```
Interactively browse and download available RPOZ shells as OBJ files.

Usage:
  ./rpotool fetch [flags]

Flags:
  -h, --help      Display this help message
```

`fetch` allows you to filter shells, as well as download and convert in bulk.

## Compiling

### Requirements

`libcurl` and `zlib` installed on your system.

A compiler, such as `gcc` or `clang`

### Building

```shell
$ git clone https://github.com/tylertms/rpotool.git
$ cd rpotool
$ gcc -o rpotool src/*.c -lz -lcurl
```

## License

`rpotool` is released under the [MIT license](https://github.com/tylertms/rpotool/blob/main/LICENSE).
