========================================================================
UHD - USRP2 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Building firmware and FPGA images
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^
FPGA Image
^^^^^^^^^^^^^^^^^^
Xilinx ISE 10.1 is required to build the FPGA image for the USRP2
(newer version of ISE are known to build buggy images).
The build requires that you have a unix-like environment with make.
Make sure that xtclsh from the Xilinx ISE bin directory is in your $PATH.

Run the following commands:
::

    cd <uhd-repo-path>/fpga/usrp2/top/u2_rev3
    make bin

*The image file will be ./build/u2_rev3.bin*

^^^^^^^^^^^^^^^^^^
Firmware Image
^^^^^^^^^^^^^^^^^^
The Microblaze GCC compiler from the Xilinx EDK is required to build the firmware.
The build requires that you have a unix-like environment with autotools and make.
Make sure that mb-gcc from the Xilinx EDK/microblaze directory is in your $PATH.

Run the following commands:
::

    cd <uhd-repo-path>/firmware/microblaze
    ./boostrap
    ./configure --host=mb
    make

*The image file will be ./apps/txrx_uhd.bin*

------------------------------------------------------------------------
Load the images onto the SD card
------------------------------------------------------------------------
**Warning!**
Use the usrp2_card_burner.py with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that --dev= specifies the SD card.

Use the card burner tool (linux):
::

    cd <prefix>/share/uhd/utils
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fpga=<path_to_fpga_image>
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fw=<path_to_firmware_image>

Use the card burner tool (windows):
::

    <path_to_python.exe> <prefix>/share/uhd/utils/usrp2_card_burner.py --gui


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
An address of **192.168.10.1** is recommended.

**Note:**
When using the UHD, if an IP address for the USRP2 is not specified,
the software will use UDP broadcast packets to locate the USRP2.
On some systems, the firewall will block UDP broadcast packets.
It is recommended that you change or disable your firewall settings. 

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
    ./usrp2_recovery.py --ifc=eth0 --new-ip=192.168.10.3

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
Use a standard USB to tty-level serial converter at 230400 baud.
The microcontroller prints useful information about IP addresses,
MAC addresses, control packets, and fast-path settings.

**Monitor the host network traffic:**
Use wireshark to monitor packets sent to and received from the USRP2.

------------------------------------------------------------------------
Resize the send and receive buffers
------------------------------------------------------------------------
It may be useful increase the size of the socket buffers to
move the burden of buffering samples into the kernel, or to
buffer incoming samples faster than they can be processed.
However, if you application cannot process samples fast enough,
no amount of buffering can save you.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device address params
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To set the size of the buffers,
the usrp2 will accept two optional parameters in the device address.
Each parameter will accept a numeric value for the number of bytes.

* recv_buff_size
* send_buff_size

Example, set the args string to the following:
::

    addr=192.168.10.2, recv_buff_size=100e6

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
OS specific notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On linux, the maximum buffer sizes are capped by the sysctl values
**net.core.rmem_max** and **net.core.wmem_max**.
To change the maximum values, run the following commands:
::

    sudo sysctl -w net.core.rmem_max=<new value>
    sudo sysctl -w net.core.wmem_max=<new value>
