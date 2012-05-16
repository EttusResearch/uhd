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
UDP Transport (Sockets)
------------------------------------------------------------------------
The UDP transport is implemented with user-space sockets.
This means standard Berkeley sockets API using send()/recv().

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transport parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following parameters can be used to alter the transport's default behavior:

* **recv_frame_size:** The size of a single receive buffer in bytes
* **num_recv_frames:** The number of receive buffers to allocate
* **send_frame_size:** The size of a single send buffer in bytes
* **num_send_frames:** The number of send buffers to allocate

**Note1:**
**num_recv_frames** does not affect performance.

**Note2:**
**num_send_frames** does not affect performance.

**Note3:**
**recv_frame_size** and **send_frame_size** can be used to
increase or decrease the maximum number of samples per packet.
The frame sizes default to an MTU of 1472 bytes per IP/UDP packet
and may be increased if permitted by your network hardware.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Flow control parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The host-based flow control expects periodic update packets from the device.
These update packets inform the host of the last packet consumed by the device,
which allows the host to determine throttling conditions for the transmission of packets.
The following mechanisms affect the transmission of periodic update packets:

* **ups_per_fifo:** The number of update packets for each FIFO's worth of bytes sent into the device
* **ups_per_sec:** The number of update packets per second (defaults to 20 updates per second)

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Resize socket buffers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
It may be useful to increase the size of the socket buffers to
move the burden of buffering samples into the kernel or to
buffer incoming samples faster than they can be processed.
However, if your application cannot process samples fast enough,
no amount of buffering can save you.
The following parameters can be used to alter socket's buffer sizes:

* **recv_buff_size:** The desired size of the receive buffer in bytes
* **send_buff_size:** The desired size of the send buffer in bytes

**Note:** Large send buffers tend to decrease transmit performance.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Latency Optimization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Latency is a measurement of the time it takes a sample to travel between the host and device.
Most computer hardware and software is bandwidth optimized, which may negatively affect latency.
If your application has strict latency requirements, please consider the following notes:

**Note1:**
The time taken by the device to populate a packet is proportional to the sample rate.
Therefore, to improve receive latency, configure the transport for a smaller frame size.

**Note2:**
For overall latency improvements,
look for "Interrupt Coalescing" settings for your OS and ethernet chipset.
It seems the Intel ethernet chipsets offer fine-grained control in Linux.
Also, consult:

* http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.aix.prftungd/doc/prftungd/interrupt_coal.htm

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Linux specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Linux, the maximum buffer sizes are capped by the sysctl values
**net.core.rmem_max** and **net.core.wmem_max**.
To change the maximum values, run the following commands:
::

    sudo sysctl -w net.core.rmem_max=<new value>
    sudo sysctl -w net.core.wmem_max=<new value>

Set the values permanently by editing **/etc/sysctl.conf**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Windows specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
**UDP send fast-path:**
It is important to change the default UDP behavior such that
1500 byte packets still travel through the fast path of the sockets stack.
This can be adjusted with the FastSendDatagramThreshold registry key:

* FastSendDatagramThreshold registry key documented here:

  * http://www.microsoft.com/windows/windowsmedia/howto/articles/optimize_web.aspx#appendix_e

* Double click and run <install-path>/share/uhd/FastSendDatagramThreshold.reg
* A system reboot is recommended after the registry key change.

**Power profile:**
The Windows power profile can seriously impact instantaneous bandwidth.
Application can take time to ramp-up to full performance capability.
It is recommended that users set the power profile to "high performance".

------------------------------------------------------------------------
USB Transport (LibUSB)
------------------------------------------------------------------------
The USB transport is implemented with LibUSB.
LibUSB provides an asynchronous API for USB bulk transfers.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Transport parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following parameters can be used to alter the transport's default behavior:

* **recv_frame_size:** The size of a single receive transfers in bytes
* **num_recv_frames:** The number of simultaneous receive transfers
* **send_frame_size:** The size of a single send transfers in bytes
* **num_send_frames:** The number of simultaneous send transfers

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup Udev for USB (Linux)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On Linux, Udev handles USB plug and unplug events.
The following commands install a Udev rule
so that non-root users may access the device:

::

    cd <install-path>/share/uhd/utils
    sudo cp uhd-usrp.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Install USB driver (Windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A driver package must be installed to use a USB-based product with UHD:

* Download the driver from the UHD wiki page `here <http://files.ettus.com/binaries/misc/erllc_uhd_winusb_driver.zip>`_.
* Unzip the file into a known location. We will refer to this as the **<directory>**.
* Open the device manager and plug in the USRP. You will see an unrecognized USB device in the device manager.
* Right click on the unrecognized USB device and select update/install driver software (may vary for your OS).
* In the driver installation wizard, select "browse for driver", browse to the **<directory>**, and select the **.inf** file.
* Continue through the installation wizard until the driver is installed.
