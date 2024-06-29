# rpotool

[![Release](https://img.shields.io/github/release/tylertms/rpotool.svg?label=Release)](https://GitHub.com/tylertms/rpotool/releases/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/tylertms/rpotool/blob/main/LICENSE)

- [Installation](#installation)
- [Usage](#usage)
  - [Examples](#examples)
- [Blender](#blender)
- [Compiling](#compiling)
- [License](#license)

Note: All assets and converted files are property of Auxbrain Inc. and should be used accordingly.

## Installation

1. Download the latest version of `rpotool` for your system from [GitHub releases](https://github.com/tylertms/rpotool/releases).

2. Extract the `.zip` archive and `cd` into the folder.

3. On macOS/Linux:
```
chmod +x rpotool
```

To obtain the default `.rpo` assets, unzip a `.apk` or `.ipa` game file and open the `/rpos` directory.

To obtain `.rpoz` shell assets, see the `search` command below.

## Usage
```
Usage: ./rpotool <input.rpo(z)?> [-s|--search <term>] [-o|--output <output>]
Options:
  -s, --search <term>       Search for shells and download as .glb 
  -o, --output <path>       Output file or folder for the converted file(s)
```

On Windows, you can simply drag a .rpo file onto the .exe to convert it.

Read the [Blender](#blender) section for instructions on importing to Blender.

### Examples:

Convert `coop.rpo` to `coop.glb`.
```
./rpotool coop.rpo
```

Convert the folder `rpos` to a new folder `glbs`, using the `-o`/`--output` flag.
```
./rpotool rpos/ -o glbs/
```

![convert_demo](./demo/rpotool_convert_demo.gif)

To browse, download, and convert shell files, use the `-s`/`--search` flag.

![search_demo](./demo/rpotool_search_demo.gif)

## Blender

While the default Blender glTF 2.0 importer will work for the converted `.glb` files, it does not support the vertex emission that Egg, Inc. uses.

To import with emission and Cycles backface culling, use the `egg_inc_gltf_import.py` addon bundled with each release.

1. In Blender, open `Edit > Preferences`, select `Add-ons`, and click `Install...`. 

2. Navigate to the `egg_inc_gltf_import.py` file and click the checkbox to enable the addon.

3. When importing a `.glb` file, select `Egg, Inc. glTF (.glb)` option from the dropdown menu, *not* the default `glTF 2.0` importer.

To adjust the emission strength, open the `Shading` tab, select the `Multiply` node, and adjust the value.

## Compiling

You must have `rustc`/`cargo` installed.

```shell
$ git clone https://github.com/tylertms/rpotool.git
$ cd rpotool
$ make
```

## License

`rpotool` is released under the [MIT license](https://github.com/tylertms/rpotool/blob/main/LICENSE).
