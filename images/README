The images directory contains the following:
    - a Makefile for building firmware and fpga images
    - a CMake file for building an images package

The Makefile and build systems for the images are probably Unix specific.
Its best to build the images on a Unix system with standard build tools.
The CMake package target will create an images package for your system.

To build the images (unix):
    make clean
    make images

To build the package (unix):
    mkdir build
    cd build
    cmake -DCPACK_GENERATOR=<type> ../
    make package

The package generator types are described here:
    http://www.cmake.org/Wiki/CMake:CPackPackageGenerators

Fedora note:
    The sdcc binaries are prefixed with "sdcc-" which breaks the build.
    However, /usr/libexec/sdcc contains properly named sdcc binaries.
    export PATH=${PATH}:/usr/libexec/sdcc
