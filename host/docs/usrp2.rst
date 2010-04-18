========================================================================
UHD - USRP2 App Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Building firmware and FPGA images
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^
FPGA Image
^^^^^^^^^^^^^^^^^^
Xilinx ISE 10.1 is required to build the FPGA image for the USRP2
(newer version of ISE are known to build buggy images).
The build requires that you have a unix-like environment with make.
Make sure that xtclsh from the Xilinx ISE bin directory is in your $PATH.

Run the following commands:
::

    cd <uhd-repo-path>/fpga/usrp2/top/u2_rev3
    make bin

*The image file will be ./build/u2_rev3.bin*

^^^^^^^^^^^^^^^^^^
Firmware Image
^^^^^^^^^^^^^^^^^^
The Microblaze GCC compiler from the Xilinx EDK is required to build the firmware.
The build requires that you have a unix-like environment with autotools and make.
Make sure that mb-gcc from the Xilinx EDK/microblaze directory is in your $PATH.

Run the following commands:
::

    cd <uhd-repo-path>/firmware/microblaze
    ./boostrap
    ./configure host=mb
    make

*The image file will be ./apps/txrx.bin*

------------------------------------------------------------------------
Load the images onto the SD card
------------------------------------------------------------------------
**Warning!**
Use the u2_flash_tool with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that --dev= specifies the SD card.

Load the FPGA image:

::

    cd <uhd-repo-path>/firmware/microblaze
    sudo ./u2_flash_tool --dev=/dev/sd<XXX> -t fpga -w <path_to_fpga_image>

Load the firmware image:

::

    cd <uhd-repo-path>/firmware/microblaze directory
    sudo ./u2_flash_tool --dev=/dev/sd<XXX> -t s/w -w <path_to_firmware_image>
