UHD Firmware and FPGA Images Builder
===============================================================

The images directory contains the following:
* A Makefile for building firmware and FPGA images
* Scripts to load all the binaries for this current commit, as
  well as create a new images package from the images in subdir
  images/

This provides a clean and organized way to build all of the firmware and FPGA
images, the source code for which is in the `firmware` and `fpga` directories
one level above this, and also maintains a linkage between images and git commits.

Building the binaries
---------------------

The Makefile and build systems for the images are *probably* Unix-specific.
It's best to build the images on a Unix system with standard build tools.  The
CMake package target will create an installable images package for your system.

__To build the images (unix):__

1. `make clean`
2. `make images`

__Fedora note:__

The sdcc binaries are prefixed with "sdcc-" which breaks the build.
However, /usr/libexec/sdcc contains properly named sdcc binaries.
`export PATH=${PATH}:/usr/libexec/sdcc`


Updating binaries
-----------------

This goes two ways:

1. Loading the correct binaries for this commit
2. Updating the binary package on this branch


### Loading the correct binaries ###

If you check out a branch or commit, you might want to use the exact same
binaries that were used when this branch or commit was generated.
To do this, run `populate_images.py`. This will either download the correct
images package from a web server or from a local directory if
`UHD_IMAGES_BASE_URL` is set.

### Updating the binaries ###

If you have commited changes to this branch that require new images, you
should probably update those.
Simply copy the new image binaries into images/. If necessary, run
`populate_images.py` before you do any of this, so all the untouched
images are the correct version. Then, run `create_imgs_package.py --commit "COMMIT MSG"`
to create a new ZIP file and commit the info.
If `UHD_IMAGES_BASE_URL` is set and is a local directory, it will move
the ZIP file to this directory after creating it.

### Updating the binaries ###

Typical workflow:

1. Check out a branch or commit (git checkout FOO)
2. Update the images/ subdir (`populate_images.py`)
3. Do some coding, and commit those changes (`git commit`)
4. Copy new binaries to images/
5. Commit the new binaries: `create_imgs_package.py --commit "Updated images on branch X"`

### The CPack system ###

Underlying `create_imgs_package.py` is a CPack system, which can be manually
invoked by:

1. `mkdir build`
2. `cd build`
3. `cmake -DCPACK_GENERATOR=<type> ../`
4. `make package`

The package generator types are described here:
http://www.cmake.org/Wiki/CMake:CPackPackageGenerators

