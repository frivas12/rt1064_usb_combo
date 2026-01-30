LUT Builder
===========
A firmware-integrated tool to convert XMLs of look-up table
    data structures into the LUT file format (lutc) and the one-wire
    file format (owc).

Requirements
------------
- A C++20-compatible compiler

Building
--------
Both the `owc` and `lutc` are valid targets, but `all` can be
    used to build both.
```shell
make all
```

`clean` will also cleanup files.

## LUTC Usage
```shell
bin/lutc.exe [opts] <supported xml>...`
```
### Arguments
- A variable amount (at least 1) of supported XMLs.
### Default Behavior
- All of the supported XMLs will be compiled to a single LUT with 0 levels of indirection.
- The key widths will be 32 bits.
### Options
- `-w <cs-int>`, `--width=<cs-int>` Comma-separated list for the number of bytes that each key takes up.
  - Eg) `--width=1,2,4` defines a LUT with 1 level of indirection, an indirection key of size 1, a structure key of size 2, and a discriminator key of size 4.
- `-s <int>`, `--size=<int>` Sets the size of the LUT pages in bytes
- `-s auto`, `--size=auto` Sets the size of the LUT pages by optimizing for size.
- `-o <path>`, `--output=<path>` Sets the output path for output object.
- `-c` Compiles the XMLs into an intermediate format.
  - Cannot use this option with `-o` is more than one supported XML is compiled.

Remarks
-------
The `tinyxml2.cpp` file had some macro nonsense to select the FSEEK
    to use.
This was not happy on Git for Windows, MSYS2, or MINGW64.
I edited lines 103 to 115 to select the build that is working
    on MINGW64.
