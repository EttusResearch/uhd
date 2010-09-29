========================================================================
UHD - USRP1 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Addressing the device
------------------------------------------------------------------------
A USRP1 can be identified though its 8 digit serial number,
designated by the "serial" key in the device address.

The device address string representation for a USRP1 with serial 12345678:

::

    serial=12345678

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Change the serial number
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP1 serial number can be changed to any 8 byte string. Examples:

::

    cd <prefix>/share/uhd/utils
    ./usrp1_serial_burner --new=87654321

    -- OR --

    ./usrp1_serial_burner --new=Beatrice

    -- OR --

    ./usrp1_serial_burner --old=12345678 --new=87654321

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
Change USB transfer parameters
------------------------------------------------------------------------
The advanced user may manipulate parameters of the usb bulk transfers
for various reasons, such as lowering latency or increasing buffer size.
By default, the UHD will use values for these parameters
that are known to perform well on a variety of systems.

The following device address can be used to manipulate USB bulk transfers:

* **recv_xfer_size:** the size of each receive bulk transfer in bytes
* **recv_num_xfers:** the number of simultaneous receive bulk transfers
* **send_xfer_size:** the size of each send bulk transfer in bytes
* **send_num_xfers:** the number of simultaneous send bulk transfers

Example, set the args string to the following:
::

    serial=12345678, recv_num_xfers=16

------------------------------------------------------------------------
OS Specific Notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup Udev on Linux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Linux, Udev handles USB plug and unplug events.
The following command creates a Udev rule for the USRP1
so that non-root users may access the device:

::

    echo 'ACTION=="add", BUS=="usb", SYSFS{idVendor}=="fffe", SYSFS{idProduct}=="0002", MODE:="0666"' > tmpfile
    sudo chown root.root tmpfile
    sudo mv tmpfile /etc/udev/rules.d/10-usrp.rules
    sudo udevadm control --reload-rules

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Install libusb driver on Windows
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Windows, a driver must be installed the first time the USRP1 is attached to the host computer.
A download link for this driver can be found on the Ettus Research UHD wiki page.
Download and unpack the driver, and direct the Windows driver install wizard to the *.inf file.
