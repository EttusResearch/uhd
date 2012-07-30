========================================================================
UHD - Device streaming
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Introduction to Streaming
------------------------------------------------------------------------
The concept of streaming refers to the transportation of samples between host and device.
A stream is an object that facilitates streaming between host application and device.
An RX stream allows the user to receive samples from the device.
A TX stream allows the user to transmit samples to the device.

------------------------------------------------------------------------
Link Layer Encapsulation
------------------------------------------------------------------------
The VITA49 standard provides encapsulation for sample data across a link layer.
On all generation2 hardware, samples are encapsulated into VRT IF data packets.
These packets also provide sample decoration such as stream time and burst flags.
Sample decoration is exposed to the user in the form of RX and TX metadata structs.

The length of an IF data packet can be limited by several factors:

* **MTU of the link layer:** network card, network switch
* **Buffering on the host:** frame size in a ring buffer
* **Buffering on the device:** size of BRAM FIFOs

------------------------------------------------------------------------
Data Types
------------------------------------------------------------------------
There are two important data types to consider when streaming:

* The data type of the samples used on the host for processing
* The data type of the samples sent through the link-layer

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The host/CPU data type
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The host data type refers to the format of samples used in the host for baseband processing.
Typically, the data type is complex baseband such as normalized **complex-float32** or **complex-int16**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The link-layer data type
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The link-layer or "over-the-wire" data type refers to the format of the samples sent through the link.
Typically, this data type is **complex-int16**.
However, to increase throughput over the link-layer,
at the expense of precision, **complex-int8** may be used.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Conversion
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The user may request arbitrary combinations of host and link data types;
however, not all combinations are supported.
The user may register custom data type formats and conversion routines.
See **uhd/convert.hpp** for futher documentation.

TODO: provide example of convert API
