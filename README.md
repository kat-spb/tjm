
# TJM Pilot Project

Utilities for tiff - JPEG2000 - MXF convetation

## What is it

Thits is a collection of tools for convertation digital cinema formats TIF -> JPEG 2000 codestream -> MXF.

## License

[![badge-license]][link-license]

As the TJM code is released under the BSD 2-clause "Simplified" License, anyone can use or modify the code, even for commercial applications.

## Tools

Convert TIFF images to DCI compliant jpeg2000 images

```j2c -i <input_path> -o <output_path> [options ...]```

Required:
* `-i, --input` path to a single TIFF image or to directory with the numbered TIFF images
* `-o, --output` path to a single TIFF image or to directory with the numbered TIFF images
Options:
* `-s, --start` start frame number
* `-f, --finish`finish frame number

Create MXF files from a sequence of j2k images

```mxf -i <input_path> -o <output_path>```

Required:
* `-i, --input` path to a directory with the numbered J2K images
* `-o, --output` path to MXF

## Dependencies

* OpenJPEG [openjpeg](https://github.com/uclouvain/openjpeg)
* AS-DCP Lib [asdcplib](https://github.com/cinecert/asdcplib)
* FFMpeg [ffmpef](https://github.com/FFmpeg/FFmpeg)

## Step-by-step

How to build and use tools you can see on page [step by step](https://github.com/kat-spb/tjm/wiki/Step-by-step).

## Build sources

TODO:

```cd tjm
rm -rf build && mkdir build
cd build
cmake ..
make
```

## Build Debian package

TODO:

```dpkg-buildpackage -rfakeroot```

## Instalation

TODO:

```make install```

```sudo dpkg -i tjm```

## Documentation

More information about convering tools [here](https://github.com/kat-spb/tjm/wiki).

TODO: The offline documentation is available in the **doc/** directory.

## Examples

TODO:

