#UHD images downloader configuration
FILE(READ ${CURRENT_DIR}/../../images/manifest.txt CMAKE_MANIFEST_CONTENTS)
CONFIGURE_FILE(
    ${CURRENT_DIR}/uhd_images_downloader.py.in
    ${INSTALL_DIR}/uhd_images_downloader.py
@ONLY)
