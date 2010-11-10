========================================================================
UHD - USRP1 Application Notes
========================================================================

.. contents:: Table of Contents

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Specify a non-standard image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The standard USRP1 images installer comes with two FPGA images:
 * **usrp1_fpga.rbf:** 2 DDCs + 2 DUCs
 * **usrp1_fpga_4rx.rbf:** 4 DDCs + 0 DUCs

By default, the USRP1 uses the FPGA image with 2 DDCs and 2 DUCs.
However, a device address parameter can be used to override
the FPGA image selection to use an alternate or a custom FPGA image.
See the images application notes for installing custom images.

Example device address string representations to specify non-standard firmware and/or FPGA images:

::

    fpga=usrp1_fpga_4rx.rbf

    -- OR --

    fw=usrp1_fw_custom.ihx

    -- OR --

    fpga=usrp1_fpga_4rx.rbf, fw=usrp1_fw_custom.ihx

------------------------------------------------------------------------
Specifying the subdevice to use
------------------------------------------------------------------------
The USRP1 has multiple daughterboard slots, known as slot A and slot B.
The subdevice specification can be used to select
the daughterboard and subdevice for each channel.
For daughterboards with one one subdevice,
the subdevice name may be left blank for automatic selection.

Ex: The subdev spec markup string to select a WBX on slot B.
Notice the use of the blank subdevice name for automatic selection.

::

    B:

    -- OR --

    B:0

Ex: The subdev spec markup string to select a BasicRX on slot B.
Notice that the subdevice name is always specified in the 3 possible cases.

::

    B:AB

    -- OR --

    B:A

    -- OR --

    B:B

------------------------------------------------------------------------
OS Specific Notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Linux - setup udev
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Linux, udev handles USB plug and unplug events.
The following commands create a udev rule for the USRP1
so that non-root users may access the device:

::

    echo 'ACTION=="add", BUS=="usb", SYSFS{idVendor}=="fffe", SYSFS{idProduct}=="0002", MODE:="0666"' > tmpfile
    sudo chown root.root tmpfile
    sudo mv tmpfile /etc/udev/rules.d/10-usrp.rules
    sudo udevadm control --reload-rules

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Windows - install driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Windows, a driver must be installed the first time the USRP1 is attached to the host computer.
A download link for this driver can be found on the UHD wiki page.
Download and unpack the driver, and direct the Windows driver install wizard to the .inf file.
