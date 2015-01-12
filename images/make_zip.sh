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

# Enter build dir
mkdir build
cd build

# Run the CPack process (ZIP file)
cmake .. -DCPACK_GENERATOR=ZIP -DUHD_RELEASE_MODE="$1" ..
make package
mv uhd-images*.zip ..

# Run the CPack process (tarball)
cmake .. -DCPACK_GENERATOR=TGZ -DUHD_RELEASE_MODE="$1" ..
make package
mv uhd-images*.tar.gz ..

# Move images to here and clean up after us:
cd ..
rm -r build
rm images/*.tag

TGZ_ARCHIVE_NAME=`ls *.tar.gz | tail -n1`

# CMake can't do xz, so do it by hand if possible
XZ_EXECUTABLE=`which xz`
if [ $? -eq 0 ]; then
	XZ_ARCHIVE_NAME=`echo $TGZ_ARCHIVE_NAME | sed "s/gz\>/xz/"`
	echo "Writing .xz tarball to $XZ_ARCHIVE_NAME ..."
	gunzip --to-stdout $TGZ_ARCHIVE_NAME | xz - > $XZ_ARCHIVE_NAME
fi

MD5_FILE_NAME=`echo $TGZ_ARCHIVE_NAME | sed "s/tar.gz\>/md5/"`
md5sum uhd-images* > $MD5_FILE_NAME
