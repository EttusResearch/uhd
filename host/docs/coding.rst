========================================================================
UHD - Coding to the API
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Various API interfaces
------------------------------------------------------------------------
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Low-Level: The device API
^^^^^^^^^^^^^^^^^^^^^^^^^^^
A device is an abstraction for hardware that is connected to the host system.
For a USRP, this means that the motherboard and everything on it would be considered to be a "device".
The device API provides ways to:

* Discover devices that are physical connected to the host system.
* Create a device object for a particular physical device identified by address.
* Register a device driver into the discovery and factory sub-system.
* Streaming samples with metadata into and out of the device.
* Set and get properties on the device object.

See the documentation in *device.hpp* for reference.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
High-Level: The multi usrp
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Multi-USRP class provides a FAT interface to a single USRP with
one or more channels, or multiple USRPs in a homogeneous setup.
See the documentation in *usrp/multi_usrp.hpp* for reference.

------------------------------------------------------------------------
Integrating custom hardware
------------------------------------------------------------------------
Creators of custom hardware can create drivers that use the UHD API.
These drivers can be built as dynamically loadable modules that the UHD will load at runtime.

For a module to be loaded at runtime, it must be:

* found in the UHD_MODULE_PATH environment variable,
* installed into the <prefix>/share/uhd/modules directory,
* or installed into /usr/share/uhd/modules directory (unix only).

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Custom motherboard
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Create a new device driver when the driver in lib/usrp/
cannot support your custom FPGA or hardware modifications.
Make a copy of the relevant driver code in lib/usrp/, make mods, and rename the class.
The new device code should register itself into the discovery and factory system. 

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Custom daughterboard
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use code from an existing daughterboard in lib/usrp/dboard/* as an example.
Your daughterboard code should subclass an rx dboard, rx dboard, or xcvr dboard;
and it should respond to calls to get and set properties.
The new daughterboard code should register itself into the dboard manager
with a unique rx and/or tx 16 bit identification number.
