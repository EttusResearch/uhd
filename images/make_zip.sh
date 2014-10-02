#!/bin/sh
# Automatically run the make-zip-file process
if [ ! -e 'make_zip.sh' ]; then
    echo 'Are you running this from the images/ directory?'
    exit 1
fi
if [ -e 'build' ]; then
    echo 'Please remove build subdirectory before proceeding.'
    exit 1
fi
mkdir build
cd build
cmake .. -DCPACK_GENERATOR=ZIP -DUHD_RELEASE_MODE="$1" ..
make package
mv uhd-images*.zip ..
cd ..
rm -r build
