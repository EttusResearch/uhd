UHD Firmware and FPGA Images Builder
===============================================================

The images directory contains the following:
* A Makefile for building firmware and FPGA images
* A CMake file for building an images package

This provides a clean and organized way to build all of the firmware and FPGA
images, the source code for which is in the `firmware` and `fpga` directories
one level above this.

The Makefile and build systems for the images are *probably* Unix-specific.
It's best to build the images on a Unix system with standard build tools.  The
CMake package target will create an installable images package for your system.

__To build the images (unix):__

1. `make clean`
2. `make images`

__To build the package (unix):__

1. `mkdir build`
2. `cd build`
3. `cmake -DCPACK_GENERATOR=<type> ../`
4. `make package`

The package generator types are described here:
http://www.cmake.org/Wiki/CMake:CPackPackageGenerators

__Fedora note:__

The sdcc binaries are prefixed with "sdcc-" which breaks the build.
However, /usr/libexec/sdcc contains properly named sdcc binaries.
`export PATH=${PATH}:/usr/libexec/sdcc`
