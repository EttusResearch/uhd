========================================================================
UHD - USRP2 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Load the images onto the SD card
------------------------------------------------------------------------
**Warning!**
Use the usrp2_card_burner.py with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that --dev= specifies the SD card.

Use the *--list* option to get a list of possible raw devices.
The list result will filter out disk partitions and devices too large to be the sd card.
The list option has been implemented on Linux, Mac OS X, and Windows.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (unix)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    sudo <prefix>/share/uhd/utils/usrp2_card_burner_gui.py

    -- OR --

    cd <prefix>/share/uhd/utils
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fpga=<path_to_fpga_image>
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fw=<path_to_firmware_image>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    <path_to_python.exe> <prefix>/share/uhd/utils/usrp2_card_burner_gui.py


------------------------------------------------------------------------
Setup networking
------------------------------------------------------------------------
The USRP2 only supports gigabit ethernet, and
will not work with a 10/100 Mbps interface.
Because the USRP2 uses gigabit ethernet pause frames for flow control,
you cannot use multiple USRP2s with a switch or a hub.
It is recommended that each USRP2 be plugged directly into its own
dedicated gigabit ethernet interface on the host computer.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the host interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP2 communicates at the IP/UDP layer over the gigabit ethernet.
The default IP address of the USRP2 is **192.168.10.2**
You will need to configure the host's ethernet interface with a static IP address to enable communication.
An address of **192.168.10.1** and a subnet mask of **255.255.255.0** is recommended.

**Note:**
When using the UHD, if an IP address for the USRP2 is not specified,
the software will use UDP broadcast packets to locate the USRP2.
On some systems, the firewall will block UDP broadcast packets.
It is recommended that you change or disable your firewall settings. 

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Multiple device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As described above, you will need one ethernet interface per USRP2.
Each ethernet interface should have its own subnet,
and the corresponding USRP2 device should be assigned an address in that subnet.
Example:

**Configuration for USRP2 device 0:**

* Ethernet interface IPv4 address: 192.168.10.1
* Ethernet interface subnet mask: 255.255.255.0
* USRP2 device IPv4 address: 192.168.10.2

**Configuration for USRP2 device 1:**

* Ethernet interface IPv4 address: 192.168.20.1
* Ethernet interface subnet mask: 255.255.255.0
* USRP2 device IPv4 address: 192.168.20.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Change the USRP2's IP address
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
You may need to change the USRP2's IP address for several reasons:

* to satisfy your particular network configuration
* to use multiple USRP2s on the same host computer
* to set a known IP address into USRP2 (in case you forgot)

**Method 1:**
To change the USRP2's IP address
you must know the current address of the USRP2,
and the network must be setup properly as described above.
Run the following commands:
::

    cd <prefix>/share/uhd/utils
    ./usrp_addr_burner --addr=192.168.10.2 --new-ip=192.168.10.3

**Method 2 (Linux Only):**
This method assumes that you do not know the IP address of your USRP2.
It uses raw ethernet packets to bypass the IP/UDP layer to communicate with the USRP2.
Run the following commands:
::

    cd <prefix>/share/uhd/utils
    sudo ./usrp2_recovery.py --ifc=eth0 --new-ip=192.168.10.3

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Debugging networking problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
**Disable the firewall:**
If uhd_find_devices gives you nothing
but uhd_find_devices --args addr=192.168.10.2 yeilds a discovered device,
then your firewall may be blocking replies to UDP broadcast packets.

**Ping the USRP2:**
The USRP2 will reply to icmp echo requests.
::

    ping 192.168.10.2

**Monitor the USRP2:**
You can read the serial port on the rear of the USRP2
to get debug verbose from the embedded microcontroller.
Use a standard USB to 3.3v-level serial converter at 230400 baud.
The microcontroller prints useful information about IP addresses,
MAC addresses, control packets, and fast-path settings.

**Monitor the host network traffic:**
Use wireshark to monitor packets sent to and received from the USRP2.

------------------------------------------------------------------------
Addressing the device
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Single device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A USRP2 can be identified though its IPv4 address or resolvable hostname.
The USRP2 device is referenced through the "addr" key in the device address.
Use this addressing scheme with the *simple_usrp* interface.

The device address string representation for a USRP2 with IPv4 address 192.168.10.2

::

    addr=192.168.10.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Soft-MIMO configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In a soft-mimo configuration, each USRP2 must have a unique IPv4 address (per computer)
and be attached to its own dedicated network port.
The value for the addr key is a white-space separated list
of IPv4 addresses or resolvable hostnames.
The first address in the list will represent channel 0,
the second channel 1, and so on...
Use this addressing scheme with the *mimo_usrp* interface.

The device address string representation for 2 USRP2s with IPv4 addresses 192.168.10.2 and 192.168.20.2
::

    addr=192.168.10.2 192.168.20.2

------------------------------------------------------------------------
Resize the send and receive buffers
------------------------------------------------------------------------
It may be useful increase the size of the socket buffers to
move the burden of buffering samples into the kernel, or to
buffer incoming samples faster than they can be processed.
However, if you application cannot process samples fast enough,
no amount of buffering can save you.

By default, the UHD will try to resize both the send and receive buffer for optimum performance.
A warning will be printed on instantiation if the actual buffer size is insufficient.
See the OS specific notes below:

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OS specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On linux, the maximum buffer sizes are capped by the sysctl values
**net.core.rmem_max** and **net.core.wmem_max**.
To change the maximum values, run the following commands:
::

    sudo sysctl -w net.core.rmem_max=<new value>
    sudo sysctl -w net.core.wmem_max=<new value>

Set the values permanently by editing */etc/sysctl.conf*

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device address params
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To manually set the size of the buffers,
the usrp2 will accept two optional parameters in the device address.
Each parameter will accept a numeric value for the number of bytes.

* recv_buff_size
* send_buff_size

Example, set the args string to the following:
::

    addr=192.168.10.2, recv_buff_size=100e6

------------------------------------------------------------------------
Hardware setup notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Front panel LEDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The LEDs on the front panel can be useful in debugging hardware and software issues.
The LEDs reveal the following about the state of the device:

* **LED A:** transmitting
* **LED B:** undocumented
* **LED C:** receiving
* **LED D:** firmware loaded
* **LED E:** undocumented
* **LED F:** FPGA loaded


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Ref Clock - 10MHz
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using an external 10MHz reference clock requires a signal level between
+5dBm and +20dBm at 10MHz applied to the Ref Clock SMA port on the front panel.


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PPS - Pulse Per Second
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using a PPS signal for timestamp synchronization requires a 5Vpp square wave signal

Test the PPS input of the USRP2 with the following app:
::

    cd <prefix>/share/uhd/examples
    ./test_pps_input --args=<args>

* <args> are device address arguments (optional if only one USRP is on your machine)
