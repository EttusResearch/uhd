#!/bin/bash
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
TAGFILES=`ls images/*.tag 2>/dev/null`
if [ -n "$TAGFILES" ]; then
	rm $TAGFILES
fi

# Copy LICENSE file
cp ../host/LICENSE images/

ARCHIVE_SUFFIX=$1
ARCHIVE_NAME="uhd-images_${ARCHIVE_SUFFIX}"
echo "Creating images archive: ${ARCHIVE_NAME}"

cp -r images $ARCHIVE_NAME

# Now zip 'em up:
echo "Creating ZIP archive..."
zip -r $ARCHIVE_NAME.zip $ARCHIVE_NAME
echo "Creating tar.gz archive..."
tar zcvf $ARCHIVE_NAME.tar.gz $ARCHIVE_NAME
echo "Creating tar.xz archive..."
tar Jcvf $ARCHIVE_NAME.tar.xz $ARCHIVE_NAME
ALL_ARCHIVES=`ls $ARCHIVE_NAME.*`
sha256sum $ALL_ARCHIVES > $ARCHIVE_NAME.sha256
md5sum $ALL_ARCHIVES > $ARCHIVE_NAME.md5

# Clean up archive directory
rm -r $ARCHIVE_NAME

