========================================================================
UHD - General Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Finding devices
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device addressing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Deviced are addressed through key/value string pairs.
These string pairs can be used to narrow down the search for a specific device or group of devices.
Most UHD utility applications and examples have a --args parameter that takes a device address;
where the device address is expressed as a delimited string.

* See the documentation in types/device_addr.hpp for reference.
* See device-specific application notes for usage.

**Example:**
::

    serial=0x1234, type=usrpx

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device discovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Devices attached to your system can be discovered using the "uhd_find_devices" program.
The find devices program scans your system for supported devices and prints
out an enumerated list of discovered devices and their addresses.
The list of discovered devices can be narrowed down by specifying device address args.

**Usage:**
::

    uhd_find_devices

    -- OR --

    uhd_find_devices --args <device-specific-address-args>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Properties of devices attached to your system can be probed with the "uhd_usrp_probe" program.
The usrp probe program contructs an instance of the device and prints out its properties;
properties such as detected daughter-boards, frequency range, gain ranges, etc...

**Usage:**
::

    uhd_usrp_probe --args <device-specific-address-args>

------------------------------------------------------------------------
Misc notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Process scheduling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The UHD will try to automatically boost the process's scheduling priority.
Currently, this is only supported on platforms with *sched.h*.

When setting the priority fails, the UHD prints out an error.
This error is harmless, it simply means that your process will have a normal scheduling priority.

**Linux Notes:**

Non-privileged users need special permission to change the scheduling priority.
Add the following line to */etc/security/limits.conf*:
::

    @<my_group>    -    rtprio    99

Replace <my_group> with a group to which your user belongs.
Settings will not take effect until the user has logged in and out.
