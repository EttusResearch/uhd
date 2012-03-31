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
For a USRP, this means that the motherboard and everything on it would be
considered to be a "device".  The device API provides ways to:

* Discover devices that are physically connected to the host system.
* Create a device object for a particular device identified by address.
* Register a device driver into the discovery and factory sub-system.
* Streaming samples with metadata into and out of the device.
* Set and get properties on the device object.

See the documentation in *device.hpp* for reference.

^^^^^^^^^^^^^^^^^^^^^^^^^^^
High-Level: The Multi-USRP
^^^^^^^^^^^^^^^^^^^^^^^^^^^
The Multi-USRP class provides a fat interface to a single USRP with
one or more channels, or multiple USRPs in a homogeneous setup.
See the documentation in *usrp/multi_usrp.hpp* for reference.
