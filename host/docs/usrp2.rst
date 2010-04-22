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

*The image file will be ./apps/txrx.bin*

------------------------------------------------------------------------
Load the images onto the SD card
------------------------------------------------------------------------
**Warning!**
Use the u2_flash_tool with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that --dev= specifies the SD card.

Load the FPGA image:

::

    cd <uhd-repo-path>/firmware/microblaze
    sudo ./u2_flash_tool --dev=/dev/sd<XXX> -t fpga -w <path_to_fpga_image>

Load the firmware image:

::

    cd <uhd-repo-path>/firmware/microblaze directory
    sudo ./u2_flash_tool --dev=/dev/sd<XXX> -t s/w -w <path_to_firmware_image>

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
    ./usrp_burner --addr=192.168.10.2 --new-ip=192.168.10.3

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
**Monitor the USRP2:**
You can read the serial port on the rear of the USRP2
to get debug verbose from the embedded microcontroller.
Use a standard USB to tty-level serial converter at 230400 baud.
The microcontroller prints useful information about IP addresses,
MAC addresses, control packets, and fast-path settings.

**Monitor the host network traffic:**
Use wireshark to monitor packets sent to and received from the USRP2.
