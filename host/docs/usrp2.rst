========================================================================
UHD - USRP2 and N2X0 Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Comparative features list
------------------------------------------------------------------------

* 1 transceiver card slot
* 2 RX DDC chains in FPGA
* 1 TX DUC chain in FPGA
* Timed commands in FPGA (N2x0 only)
* Timed sampling in FPGA
* External PPS reference
* External 10MHz reference
* MIMO cable shared reference
* Fixed 100 MHz clock rate
* Internal GPSDO option (N2x0 only)
* sc8 and sc16 sample modes

------------------------------------------------------------------------
Load the Images onto the SD card (USRP2 only)
------------------------------------------------------------------------
**Warning!**
Use **usrp2_card_burner.py** with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that **--dev=** specifies the SD card.

**Warning!**
It is possible to use 3rd party SD cards with the USRP2.
However, certain types of SD cards will not interface with the CPLD:

* Cards can be SDHC, which is not a supported interface.
* Cards can have unexpected timing characteristics.

For these reasons, we recommend that you use the SD card that was supplied with the USRP2.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (UNIX)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    sudo <install-path>/share/uhd/utils/usrp2_card_burner_gui.py

    -- OR --

    cd <install-path>/share/uhd/utils
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fpga=<path_to_fpga_image>
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fw=<path_to_firmware_image>

Use the **--list** option to get a list of possible raw devices.
The list result will filter out disk partitions and devices too large to be the sd card.
The list option has been implemented on Linux, Mac OS X, and Windows.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (Windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    <path_to_python.exe> <install-path>/share/uhd/utils/usrp2_card_burner_gui.py

------------------------------------------------------------------------
Load the Images onto the On-board Flash (USRP-N Series only)
------------------------------------------------------------------------
The USRP-N Series can be reprogrammed over the network
to update or change the firmware and FPGA images.
When updating images, always burn both the FPGA and firmware images before power cycling.
This ensures that when the device reboots, it has a compatible set of images to boot into.

**Note:**
Different hardware revisions require different FPGA images.
Determine the revision number from the sticker on the rear of the chassis.
Use this number to select the correct FPGA image for your device.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the net burner tool (UNIX)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    <install-path>/share/uhd/utils/usrp_n2xx_net_burner_gui.py

    -- OR --

    cd <install-path>/share/uhd/utils
    ./usrp_n2xx_net_burner.py --addr=<ip address> --fw=<path for firmware image>
    ./usrp_n2xx_net_burner.py --addr=<ip address> --fpga=<path to FPGA image>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the net burner tool (Windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    <path_to_python.exe> <install-path>/share/uhd/utils/usrp_n2xx_net_burner_gui.py

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Burning images without Python
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For users who do not wish to install Python, a new script is available in UHD 003.005.000:
the USRP N2XX Simple Net Burner. It provides the same functionality as its Python
counterpart, but by default, it automatically installs the default images without the user needing
to specify their location on the command line.

The utility can be found at: **<install-path>/share/uhd/utils/usrp_n2xx_simple_net_burner**

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Device recovery and bricking
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Its possible to put the device into an unusable state by loading bad images.
Fortunately, the USRP-N Series can be booted into a safe (read-only) image.
Once booted into the safe image, the user can once again load images onto the device.

The safe-mode button is a pushbutton switch (S2) located inside the enclosure.
To boot into the safe image, hold-down the safe-mode button while power-cycling the device.
Continue to hold-down the button until the front-panel LEDs blink and remain solid.

When in safe-mode, the USRP-N device will always have the IP address **192.168.10.2**.

------------------------------------------------------------------------
Setup Networking
------------------------------------------------------------------------
The USRP2 only supports Gigabit Ethernet
and will not work with a 10/100 Mbps interface.
However, a 10/100 Mbps interface can be connected indirectly
to a USRP2 through a Gigabit Ethernet switch.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Setup the host interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP2 communicates at the IP/UDP layer over the gigabit ethernet.
The default IP address of the USRP2 is **192.168.10.2**.
You will need to configure the host's Ethernet interface with a static IP
address to enable communication.  An address of **192.168.10.1** and a subnet
mask of **255.255.255.0** is recommended.

On a Linux system, you can set a static IP address very easily by using the
'ifconfig' command:
::

    sudo ifconfig <interface> 192.168.10.1

Note that **<interface>** is usually something like **eth0**.  You can discover the
names of the network interfaces in your computer by running **ifconfig** without
any parameters:
::

    ifconfig -a

**Note:**
When using UHD, if an IP address for the USRP2 is not specified,
the software will use UDP broadcast packets to locate the USRP2.
On some systems, the firewall will block UDP broadcast packets.
It is recommended that you change or disable your firewall settings.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Multiple devices per host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For maximum throughput, one Ethernet interface per USRP2 is recommended,
although multiple devices may be connected via a Gigabit Ethernet switch.
In any case, each Ethernet interface should have its own subnet,
and the corresponding USRP2 device should be assigned an address in that subnet.
Example:

**Configuration for USRP2 device 0:**

* Ethernet interface IPv4 address: **192.168.10.1**
* Ethernet interface subnet mask: **255.255.255.0**
* USRP2 device IPv4 address: **192.168.10.2**

**Configuration for USRP2 device 1:**

* Ethernet interface IPv4 address: **192.168.20.1**
* Ethernet interface subnet mask: **255.255.255.0**
* USRP2 device IPv4 address: **192.168.20.2**

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Change the USRP2's IP address
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
You may need to change the USRP2's IP address for several reasons:

* to satisfy your particular network configuration
* to use multiple USRP2s on the same host computer
* to set a known IP address into USRP2 (in case you forgot)

**Method 1:**
To change the USRP2's IP address,
you must know the current address of the USRP2,
and the network must be setup properly as described above.
Run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=ip-addr --val=192.168.10.3

**Method 2 (Linux Only):**
This method assumes that you do not know the IP address of your USRP2.
It uses raw Ethernet packets to bypass the IP/UDP layer to communicate with the USRP2.
Run the following commands:
::

    cd <install-path>/share/uhd/utils
    sudo ./usrp2_recovery.py --ifc=eth0 --new-ip=192.168.10.3

------------------------------------------------------------------------
Communication Problems
------------------------------------------------------------------------
When setting up a development machine for the first time,
you may have various difficulties communicating with the USRP device.
The following tips are designed to help narrow down and diagnose the problem.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
RuntimeError: no control response
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This is a common error that occurs when you have set the subnet of your network
interface to a different subnet than the network interface of the USRP.  For
example, if your network interface is set to **192.168.20.1**, and the USRP is
**192.168.10.2** (note the difference in the third numbers of the IP addresses), you
will likely see a 'no control response' error message.

Fixing this is simple - just set the your host PC's IP address to the same
subnet as that of your USRP. Instructions for setting your IP address are in the
previous section of this documentation.


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Firewall issues
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When the IP address is not specified,
the device discovery broadcasts UDP packets from each ethernet interface.
Many firewalls will block the replies to these broadcast packets.
If disabling your system's firewall
or specifying the IP address yields a discovered device,
then your firewall may be blocking replies to UDP broadcast packets.
If this is the case, we recommend that you disable the firewall
or create a rule to allow all incoming packets with UDP source port **49152**.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Ping the device
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP will reply to ICMP echo requests.
A successful ping response means that the device has booted properly
and that it is using the expected IP address.

::

    ping 192.168.10.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Monitor the serial output
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Read the serial port to get debug verbose output from the embedded microcontroller.
The microcontroller prints useful information about IP addresses,
MAC addresses, control packets, fast-path settings, and bootloading.
Use a standard USB to 3.3v-level serial converter at 230400 baud.
Connect **GND** to the converter ground, and connect **TXD** to the converter receive.
The **RXD** pin can be left unconnected as this is only a one-way communication.

* **USRP2:** Serial port located on the rear edge
* **N210:** Serial port located on the left side

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Monitor the host network traffic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use Wireshark to monitor packets sent to and received from the device.

------------------------------------------------------------------------
Addressing the Device
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Single device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In a single-device configuration,
the USRP device must have a unique IPv4 address on the host computer.
The USRP can be identified through its IPv4 address, resolvable hostname, or by other means.
See the application notes on `device identification <./identification.html>`_.
Use this addressing scheme with the **single_usrp** interface.

Example device address string representation for a USRP2 with IPv4 address **192.168.10.2**:

::

    addr=192.168.10.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Multiple device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In a multi-device configuration,
each USRP device must have a unique IPv4 address on the host computer.
The device address parameter keys must be suffixed with the device index.
Each parameter key should be of the format <key><index>.
Use this addressing scheme with the **multi_usrp** interface.

* The order in which devices are indexed corresponds to the indexing of the transmit and receive channels.
* The key indexing provides the same granularity of device identification as in the single device case.

Example device address string representation for 2 USRP2s with IPv4 addresses **192.168.10.2** and **192.168.20.2**:
::

    addr0=192.168.10.2, addr1=192.168.20.2

------------------------------------------------------------------------
Using the MIMO Cable
------------------------------------------------------------------------
The MIMO cable allows two USRP devices to share reference clocks,
time synchronization, and the Ethernet interface.
One of the devices will sync its clock and time references to the MIMO cable.
This device will be referred to as the slave, and the other device, the master.

* The slave device acquires the clock and time references from the master device.
* The master and slave may be used individually or in a multi-device configuration.
* External clocking is optional and should only be supplied to the master device.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Shared ethernet mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In shared Ethernet mode,
only one device in the configuration can be attached to the Ethernet.

* Clock reference, time reference, and data are communicated over the MIMO cable.
* Master and slave must have different IPv4 addresses in the same subnet.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Dual ethernet mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In dual Ethernet mode,
both devices in the configuration must be attached to the Ethernet.

* Only clock reference and time reference are communicated over the MIMO cable.
* The master and slave must have different IPv4 addresses in different subnets.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Configuring the slave
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In order for the slave to synchronize to the master over MIMO cable,
the following clock configuration must be set on the slave device:
::

    uhd::clock_config_t clock_config;
    clock_config.ref_source = uhd::clock_config_t::REF_MIMO;
    clock_config.pps_source = uhd::clock_config_t::PPS_MIMO;
    usrp->set_clock_config(clock_config, slave_index);


------------------------------------------------------------------------
Alternative stream destination
------------------------------------------------------------------------
It is possible to program the USRP to send RX packets to an alternative IP/UDP destination.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set the subnet and gateway
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use an alternative streaming destination,
the device needs to be able to determine if the destination address
is within its subnet, and ARP appropriately.
Therefore, the user should ensure that subnet and gateway addresses
have been programmed into the device's EEPROM.

Run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=subnet --val=255.255.255.0
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=gateway --val=192.168.10.1

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Create a receive streamer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set the stream args "addr" and "port" values to the alternative destination.
Packets will be sent to this destination when the user issues a stream command.

::

    //create a receive streamer, host type does not matter
    uhd::stream_args_t stream_args("fc32");

    //resolvable address and port for a remote udp socket
    stream_args.args["addr"] = "192.168.10.42";
    stream_args.args["port"] = "12345";

    //create the streamer
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    //issue stream command
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = total_num_samps;
    stream_cmd.stream_now = true;
    usrp->issue_stream_cmd(stream_cmd);

**Note:**
Calling recv() on this streamer object should yield a timeout.

------------------------------------------------------------------------
Hardware Setup Notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Front panel LEDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The LEDs on the front panel can be useful in debugging hardware and software issues.
The LEDs reveal the following about the state of the device:

* **LED A:** transmitting
* **LED B:** mimo cable link
* **LED C:** receiving
* **LED D:** firmware loaded
* **LED E:** reference lock
* **LED F:** CPLD loaded


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Ref Clock - 10MHz
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using an external 10MHz reference clock, a square wave will offer the best phase
noise performance, but a sinusoid is acceptable.  The reference clock requires the following power level:

* **USRP2** 5 to 15dBm
* **N2XX** 0 to 15dBm


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PPS - Pulse Per Second
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using a PPS signal for timestamp synchronization requires a square wave signal with the following amplitude:

* **USRP2** 5Vpp
* **N2XX** 3.3 to 5Vpp

Test the PPS input with the following app:

* **<args>** are device address arguments (optional if only one USRP is on your machine)

::

    cd <install-path>/share/uhd/examples
    ./test_pps_input --args=<args>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Internal GPSDO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Please see the `Internal GPSDO Application Notes <./gpsdo.html>`_
for information on configuring and using the internal GPSDO.

------------------------------------------------------------------------
Miscellaneous
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available Sensors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following sensors are available for the USRP2/N-Series motherboards;
they can be queried through the API.

* **mimo_locked** - clock reference locked over the MIMO cable
* **ref_locked** - clock reference locked (internal/external)
* other sensors are added when the GPSDO is enabled

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Multiple RX channels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
There are two complete DDC chains in the FPGA.
In the single channel case, only one chain is ever used.
To receive from both channels,
the user must set the **RX** subdevice specification.
This hardware has only one daughterboard slot,
which has been aptly named slot **A**.

In the following example, a TVRX2 is installed.
Channel 0 is sourced from subdevice **RX1**,
and channel 1 is sourced from subdevice **RX2**:
::

    usrp->set_rx_subdev_spec("A:RX1 A:RX2");
