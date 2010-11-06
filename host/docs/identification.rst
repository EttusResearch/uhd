========================================================================
UHD - Device Identification Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Identifying USRPs
------------------------------------------------------------------------
Every device has several ways of identifying it on the host system:

* **Serial:** A globally unique identifier.
* **Address:** A unique identifier on a network.
* **Name:** An optional user-set identifier.

The address is only applicable for network-based devices.
See the USRP2 application notes.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device discovery via command line
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A "find devices" utility application comes bundled with the UHD.
The find devices application will search for all devices on the host system and print the results.

::

    uhd_find_devices

Device address arguments can be supplied to narrow the scope of the search.

::

    uhd_find_devices --args="type=usrp1"

    -- OR --

    uhd_find_devices --args="serial=12345678"

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device discovery through the API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The device::find() API call searches for devices and returns a list of discovered devices.

::

    uhd::device_addr_t hint; //an empty hint discovers all devices
    uhd::device_addrs_t dev_addrs = uhd::device::find(hint);

The hint argument can be populated to narrow the scope of the search.

::

    uhd::device_addr_t hint;
    hint["type"] = "usrp1";
    uhd::device_addrs_t dev_addrs = uhd::device::find(hint);

    -- OR --

    uhd::device_addr_t hint;
    hint["serial"] = "12345678";
    uhd::device_addrs_t dev_addrs = uhd::device::find(hint);

------------------------------------------------------------------------
Naming a USRP
------------------------------------------------------------------------
For convenience purposes, users may assign a custom name to their USRPs.
The USRP can then be identified via name, rather than a difficult to remember serial or address.

A name has the following properties:

* is composed of ASCII characters
* is between 0 and 20 characters
* is not required to be unique

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set a custom name
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Run the following commands:
::

    cd <prefix>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=name --val=lab1_xcvr

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Discovery via name
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The keyword "name" can be used to narrow the scope of the search.
Example with the find devices utility:
::

    uhd_find_devices --args="name=lab1_xcvr"

    -- OR --

    uhd_find_devices --args="type=usrp1, name=lab1_xcvr"
