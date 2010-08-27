========================================================================
UHD - USRP1 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Addressing the device
------------------------------------------------------------------------
A USRP1 can be identified though its serial number,
designated by the "serial" key in the device address.

The device address string representation for a USRP1 with serial 1234

::

    serial=1234

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Change the USRP1's serial number
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
TODO

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

