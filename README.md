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

Read the [Blender](#blender) section for details about rendering with Cycles.

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

With the use of `.glb` files, Blender will automatically recognize the colors and properties of imported objects.

If you are using the Cycles renderer and need backface culling to see internal geometry, such as on the Chicken Universe hab, set up the following material nodes on the `default` texture:

![cycles_backface_culling](./demo/cycles_backface_culling.png)

If the object has light-emitting features, you may want to individually tweak these values to your liking.

To do this, select the object and open the materials tab on the sidebar.

![blender_materials](./demo/blender_emissive_materials.png)

Then, select a material, open `Surface > Emission`, and adjust the `Strength`.

## Compiling

You must have `rustc`/`cargo` installed.

```shell
$ git clone https://github.com/tylertms/rpotool.git
$ cd rpotool
$ make
```

## License

`rpotool` is released under the [MIT license](https://github.com/tylertms/rpotool/blob/main/LICENSE).
