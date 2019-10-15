UHD Firmware and FPGA Images Builder
===============================================================

The images directory is an aid to prepare images packages.

Building the actual FPGA images is not handled with these tools.

## Making image packages

This functionality was moved to the FPGA repository in usrp3/tools/utils.

# Create image tarballs for releases

If you're on a release tag and want to create images packages for uploading to
github, run

    $ ./create_imgs_package.py

It'll take a few minutes to download and zip them all up.
