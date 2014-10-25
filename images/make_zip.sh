#!/bin/sh
# Automatically run the make-zip-file process
# Check we're in the right directory and all is set:
if [ ! -e 'make_zip.sh' ]; then
    echo '[ERROR] Are you running this from the images/ directory?'
    exit 1
fi
if [ ! -e 'images' ]; then
    echo 'images subdirectory does not exist. Please create it and put all the images you want to package in there.'
    exit 1
fi

# Remove cruft before proceeding:
if [ -e 'build' ]; then
    echo 'Please remove build subdirectory before proceeding.'
    exit 1
fi
if [ -e "images/LICENSE" ]; then
	rm images/LICENSE
fi
TAGFILES=`ls images/*.tag 2>/dev/null`
if [ -n "$TAGFILES" ]; then
	rm $TAGFILES
fi

# Run the CPack process:
mkdir build
cd build
cmake .. -DCPACK_GENERATOR=ZIP -DUHD_RELEASE_MODE="$1" ..
make package
mv uhd-images*.zip ..
cmake .. -DCPACK_GENERATOR=TGZ -DUHD_RELEASE_MODE="$1" ..
make package

# Move images to here and clean up after us:
mv uhd-images*.tar.gz ..
cd ..
rm -r build
rm images/*.tag
