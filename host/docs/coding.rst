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
High-Level: The single usrp
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The goal of the single usrp API is to wrap high level functions around the device properties.
The single usrp provides a fat interface to access the most common properties.
The single usrp provides ways to:

* Set and get daughterboard gains.
* Set and get daughterboard antennas.
* Set and get the streaming rates.
* Tune the DSPs and daughterboards.
* Issue stream commands.
* Set the clock configuration.
* Set the usrp time registers.
* Get the underlying device (as discussed above).

See the documentation in *usrp/single_usrp.hpp* for reference.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
High-Level: The multi usrp
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The multi usrp API provides a wrapper around a device that represents several motherboards.
This API provides convenience calls just like the single usrp,
however the calls either work across all channels in the configuration,
or take a channel argument to specify which channel to configure.
The multi usrp provides ways to:

* Set and get the sample rate across all channels.
* Issue a stream command across all channels.
* Set the time registers across all channels.
* Set and get individual daughterboard gains.
* Set and get individual daughterboard antennas.
* Tune individual DSPs and daughterboards.
* Get the underlying device (as discussed above).

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
