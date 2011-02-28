========================================================================
UHD - USRP1 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Specify a non-standard image
------------------------------------------------------------------------
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
Missing and emulated features
------------------------------------------------------------------------
The USRP1 FPGA does not have the necessary space to support the advanced
streaming capabilities that are possible with the newer USRP devices.
Some of these features are emulated in software to support the API.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
List of emulated features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Setting the current device time
* Getting the current device time
* Transmitting at a specific time
* Transmitting a specific number of samples
* Receiving at a specific time
* Receiving a specific number of samples
* End of burst flags for transmit/receive

**Note:**
These emulated features rely on the host system's clock for timed operations,
and therefore may not have sufficient precision for the application.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
List of missing features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Notification on late stream command
* Notification on late transmit packet
* Notification on broken chain error
* Notification on underflow or overflow
* Start of burst flags for transmit/receive

------------------------------------------------------------------------
OS specific notes
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

------------------------------------------------------------------------
Hardware setup notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
External clock modification
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP can be modified to accept an external clock reference instead of the 64MHz onboard reference.
 * Solder SMA (LTI-SASF54GT) connector to J2001
 * Move 0 ohm 0603 resistor R2029 to R2930
 * Move 0.01uF 0603 capacitor C925 to C926
 * Remove 0.01uF 0603 capacitor C924

The new external clock needs to be a square wave between +7dBm and +15dBm

To configure UHD for the new reference clock, modify host/lib/usrp/usrp1/clock_ctrl.cpp:

::

    static const double master_clock_rate = <YOUR_NEW_REFERENCE_FREQUENCY>;

