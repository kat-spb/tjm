
# TJM Pilot Project

Utilities for tiff - JPEG2000 - MXF convetation

## What is it

Thits is a collection of tools for convertation digital cinema formats TIF -> JPEG 2000 codestream -> MXF.

## License

[![badge-license]][link-license]

As the TJM code is released under the BSD 2-clause "Simplified" License, anyone can use or modify the code, even for commercial applications.

## Tools

Convert TIFF images to DCI compliant jpeg2000 images

```
j2c -i <input_path> -o <output_path> [options ...]
```

Required:
* `-i, --input` path to a single TIFF image or to directory with the numbered TIFF images
* `-o, --output` path to a single TIFF image or to directory with the numbered TIFF images
Options:
* `-s, --start` start frame number
* `-f, --finish`finish frame number

Create MXF files from a sequence of j2k images

```
mxf -i <input_path> -o <output_path>
```

Required:
* `-i, --input` path to a directory with the numbered J2K images
* `-o, --output` path to MXF

## Dependencies

* OpenJPEG [openjpeg](https://github.com/uclouvain/openjpeg)
* AS-DCP Lib [asdcplib](https://github.com/cinecert/asdcplib)

## Step-by-step

How to build and use tools you can see on page [step by step](https://github.com/kat-spb/tjm/wiki/Step-by-step).

## Build sources

```
cd tjm
rm -rf build; mkdir build; cd build
cmake ..
make -j4
```

## Build Debian package

TODO:

```
dpkg-buildpackage -rfakeroot
```

## Instalation

```
sudo make install
```
TODO: OR
```
sudo dpkg -i tjm
```

## Scripts

Please, make all scripts executable with the command 
```
chmod +x scripts/*.sh
```
Clone template TIFF image to the <output_dir> with the indexes from <result_start_id> to <result_finish_id>
```
scripts/clone.sh <template.tif> <output_dir> <result_start_id> <result_finish_id>
```
You can use 
```
scripts/opjtoj2k.sh <input_dir>
```
and
```
scripts/as02tomxf.sh <input_dir> <output>
``` 
for get the same result with openjpeg and asdcp utilities 

Also you can try the process with TJM utilities:

```
scripts/test_process < data/input_40.txt (or input_450.txt)
```

## Docker

Please, prepare the images for copy to data/ (to container).

Build docker image (you can use debian:testing or ubuntu:20.04 or based on):

```
docker build --file Dockerfile --tag ${USER}/tjm .

```
Run it with bin/bash for tests
```
docker run --rm --name tjm -it ${USER}/tjm /bin/bash
```

## Documentation

More information about convering tools [here](https://github.com/kat-spb/tjm/wiki).

TODO: The offline documentation is available in the **doc/** directory.

## Examples

Please, put your TIF image into data directory 
```
scripts/test_process.sh < data/input_40.txt 
mediainfo data/output.mxf
```
