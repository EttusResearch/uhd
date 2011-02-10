========================================================================
UHD - Transport Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Introduction
------------------------------------------------------------------------
A transport is the layer between the packet interface and a device IO interface.
The advanced user can pass optional parameters
into the underlying transport layer through the device address.
These optional parameters control how the transport object allocates memory,
resizes kernel buffers, spawns threads, etc.
When not spcified, the transport layer will use values for these parameters
that are known to perform well on a variety of systems.
The transport parameters are defined below for the various transports in the UHD:

------------------------------------------------------------------------
UDP transport (ASIO)
------------------------------------------------------------------------
The UDP transport is implemented with Boost's ASIO library.
ASIO provides an asynchronous API for user-space sockets.
The transport implementation allocates a number of buffers
and submits asynchronous requests for send and receive.
IO service threads run in the background to process these requests.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transport parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following parameters can be used to alter the transport's default behavior:

* **recv_frame_size:** The size of a single receive buffer in bytes
* **num_recv_frames:** The number of receive buffers to allocate
* **send_frame_size:** The size of a single send buffer in bytes
* **num_send_frames:** The number of send buffers to allocate

**Note:** num_recv_frames and num_send_frames will not have an effect
as the asynchronous send implementation is currently unimplemented.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Flow control parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The host-based flow control expects periodic update packets from the device.
These update packets inform the host of the last packet consumed by the device,
which allows the host to determine throttling conditions for the transmission of packets.
The following mechanisms affect the transmission of periodic update packets:

* **ups_per_fifo:** The number of update packets for each FIFO's worth of bytes sent into the device
* **ups_per_sec:** The number of update packets per second (disabled by default)

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Resize socket buffers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
It may be useful increase the size of the socket buffers to
move the burden of buffering samples into the kernel, or to
buffer incoming samples faster than they can be processed.
However, if your application cannot process samples fast enough,
no amount of buffering can save you.
The following parameters can be used to alter socket's buffer sizes:

* **recv_buff_size:** The desired size of the receive buffer in bytes
* **send_buff_size:** The desired size of the send buffer in bytes

**Note:** Large send buffers tend to decrease transmit performance.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Linux specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On linux, the maximum buffer sizes are capped by the sysctl values
**net.core.rmem_max** and **net.core.wmem_max**.
To change the maximum values, run the following commands:
::

    sudo sysctl -w net.core.rmem_max=<new value>
    sudo sysctl -w net.core.wmem_max=<new value>

Set the values permanently by editing */etc/sysctl.conf*

------------------------------------------------------------------------
USB transport (libusb)
------------------------------------------------------------------------
The USB transport is implemented with libusb.
Libusb provides an asynchronous API for USB bulk transfers.
The transport implementation allocates a number of buffers
and submits asynchronous requests through libusb.
Event handler threads run in the background to process these requests.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transport parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following parameters can be used to alter the transport's default behavior:

* **recv_frame_size:** The size of a single receive transfers in bytes
* **num_recv_frames:** The number of simultaneous receive transfers
* **send_frame_size:** The size of a single send transfers in bytes
* **num_send_frames:** The number of simultaneous send transfers
* **concurrency_hint:** The number of threads to run the event handler
