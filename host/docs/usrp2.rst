========================================================================
UHD - USRP2 and N Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Load the images onto the SD card (USRP2 only)
------------------------------------------------------------------------
**Warning!**
Use the usrp2_card_burner.py with caution. If you specify the wrong device node,
you could overwrite your hard drive. Make sure that --dev= specifies the SD card.

**Warning!**
It is possible to use 3rd party SD cards with the USRP2.
However, certain types of SD cards will not interface with the CPLD:

* Cards can be SDHC, which is not a supported interface.
* Cards can have unexpected timing characteristics.

For these reasons, we recommend that you use the SD card that was supplied with the USRP2.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (unix)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    sudo <install-path>/share/uhd/utils/usrp2_card_burner_gui.py

    -- OR --

    cd <install-path>/share/uhd/utils
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fpga=<path_to_fpga_image>
    sudo ./usrp2_card_burner.py --dev=/dev/sd<XXX> --fw=<path_to_firmware_image>

Use the *--list* option to get a list of possible raw devices.
The list result will filter out disk partitions and devices too large to be the sd card.
The list option has been implemented on Linux, Mac OS X, and Windows.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the card burner tool (windows)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    <path_to_python.exe> <install-path>/share/uhd/utils/usrp2_card_burner_gui.py


------------------------------------------------------------------------
Load the images onto the on-board flash (USRP-N Series only)
------------------------------------------------------------------------
The USRP-N Series can be reprogrammed over the network
to update or change the firmware and FPGA images.
When updating images, always burn both the FPGA and firmware images before power cycling.
This ensures that when the device reboots, it has a compatible set of images to boot into.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the net burner tool (unix)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::

    sudo <install-path>/share/uhd/utils/usrp_n2xx_net_burner_gui.py

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
Device recovery and bricking
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Its possible to put the device into an unusable state by loading bad images.
Fortunately, the USRP-N Series can be booted into a safe (read-only) image.
Once booted into the safe image, the user can once again load images onto the device.

The safe-mode button is a pushbutton switch (S2) located inside the enclosure.
To boot into the safe image, hold-down the safe-mode button while power-cycling the device.
Continue to hold-down the button until the front-panel LEDs blink and remain solid.

When in safe-mode, the USRP-N device will always have the IP address 192.168.10.2

------------------------------------------------------------------------
Setup networking
------------------------------------------------------------------------
The USRP2 only supports gigabit ethernet,
and will not work with a 10/100 Mbps interface.
However, a 10/100 Mbps interface can be connected indirectly
to a USRP2 through a gigabit ethernet switch.

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
Multiple devices per host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For maximum throughput, one ethernet interface per USRP2 is recommended,
although multiple devices may be connected via a gigabit ethernet switch.
In any case, each ethernet interface should have its own subnet,
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

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=ip-addr --val=192.168.10.3

**Method 2 (Linux Only):**
This method assumes that you do not know the IP address of your USRP2.
It uses raw ethernet packets to bypass the IP/UDP layer to communicate with the USRP2.
Run the following commands:
::

    cd <install-path>/share/uhd/utils
    sudo ./usrp2_recovery.py --ifc=eth0 --new-ip=192.168.10.3

------------------------------------------------------------------------
Communication problems
------------------------------------------------------------------------
When setting up a development machine for the first time,
you may have various difficulties communicating with the USRP device.
The following tips are designed to help narrow-down and diagnose the problem.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Firewall issues
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When the IP address is not specified,
the device discovery sends broadcast UDP packets from each ethernet interface.
Many firewalls will block the replies to these broadcast packets.
If disabling your system's firewall,
or specifying the IP address yeilds a discovered device,
then your firewall may be blocking replies to UDP broadcast packets.
If this is the case, we recommend that you disable the firewall,
or create a rule to allow all incoming packets with UDP source port 49152.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Ping the device
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP will reply to icmp echo requests.
A successful ping response means that the device has booted properly,
and that it is using the expected IP address.

::

    ping 192.168.10.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Monitor the serial output
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Read the serial port to get debug verbose from the embedded microcontroller.
The microcontroller prints useful information about IP addresses,
MAC addresses, control packets, fast-path settings, and bootloading.
Use a standard USB to 3.3v-level serial converter at 230400 baud.
Connect GND to the converter ground, and connect TXD to the converter receive.
The RXD pin can be left unconnected as this is only a one-way communication.

* **USRP2:** Serial port located on the rear edge
* **N210:** Serial port located on the left side

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Monitor the host network traffic
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use wireshark to monitor packets sent to and received from the device.

------------------------------------------------------------------------
Addressing the device
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Single device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In a single-device configuration,
the USRP device must have a unique IPv4 address on the host computer.
The USRP can be identified through its IPv4 address, resolvable hostname, or by other means.
See the application notes on `device identification <./identification.html>`_.
Use this addressing scheme with the *single_usrp* interface.

Example device address string representation for a USRP2 with IPv4 address 192.168.10.2

::

    addr=192.168.10.2

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Multiple device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In a multi-device configuration,
each USRP device must have a unique IPv4 address on the host computer.
The device address parameter keys must be suffixed with the device index.
Each parameter key should be of the format <key><index>.
Use this addressing scheme with the *multi_usrp* interface.

* The order in which devices are indexed corresponds to the indexing of the transmit and receive channels.
* The key indexing provides the same granularity of device identification as in the single device case.

Example device address string representation for 2 USRP2s with IPv4 addresses 192.168.10.2 and 192.168.20.2
::

    addr0=192.168.10.2, addr1=192.168.20.2

------------------------------------------------------------------------
Using the MIMO Cable
------------------------------------------------------------------------
The MIMO cable allows two USRP devices to share reference clocks,
time synchronization, and the ethernet interface.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Shared ethernet mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In shared ethernet mode,
only one device in the configuration can be attached to the ethernet.
This device will be referred to as the master, and the other device, the slave.

* The master provides reference clock and time synchronization to the slave.
* All data passing between the host and the slave is routed over the MIMO cable.
* Both master and slave must have different IPv4 addresses in the same subnet.
* The master and slave may be used individually or in a multi-device configuration.
* External clocking is optional, and should only be supplied to the master device.
* The role of slave and master may be switched with the "mimo_mode" device address (see dual ethernet mode).

Example device address string representation for 2 USRP2s with IPv4 addresses 192.168.10.2 (master) and 192.168.10.3 (slave)
::

    -- Multi-device example --

    addr0=192.168.10.2, addr1=192.168.10.3

    -- Two single devices example --

    addr=192.168.10.2

    addr=192.168.10.3

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Dual ethernet mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In dual ethernet mode,
both devices in the configuration must be attached to the ethernet.
One of the devices in the configuration will be configured to provide synchronization.
This device will be referred to as the master, and the other device, the slave.

* The master provides reference clock and time synchronization to the slave.
* The devices require the special device address argument "mimo_mode" set.
* Both master and slave must have different IPv4 addresses in different subnets.
* The master and slave may be used individually or in a multi-device configuration.
* External clocking is optional, and should only be supplied to the master device.

Example device address string representation for 2 USRP2s with IPv4 addresses 192.168.10.2 (master) and 192.168.20.2 (slave)
::

    -- Multi-device example --

    addr0=192.168.10.2, mimo_mode0=master, addr1=192.168.20.2, mimo_mode1=slave

    -- Two single devices example --

    addr=192.168.10.2, mimo_mode=master

    addr=192.168.20.2, mimo_mode=slave

------------------------------------------------------------------------
Hardware setup notes
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
Using an external 10MHz reference clock, square wave will offer the best phase
noise performance, but sinusoid is acceptable.  The reference clock requires the following power level:

* **USRP2** 5 to 15dBm
* **N2XX** 0 to 15dBm


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PPS - Pulse Per Second
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Using a PPS signal for timestamp synchronization requires a square wave signal with the following amplitude:

* **USRP2** 5Vpp
* **N2XX** 3.3 to 5Vpp

Test the PPS input with the following app:

* <args> are device address arguments (optional if only one USRP is on your machine)

::

    cd <install-path>/share/uhd/examples
    ./test_pps_input --args=<args>

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Internal GPSDO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
USRP-N2XX models can have an optional internal GPSDO.
To use the GPSDO with UHD, you must burn an EEPROM setting
so that UHD knows that the internal GPSDO was installed.

**Installation instructions:**

1. Remove the daughterboard.
2. Move J510 jumper on the motherboard from 1-2 to 2-3 in order to switch from external 10 MHz Ref Clock to GPSDO’s 10 MHz Ref Clock
3. Screw the GPSDO module in place with the screws provided. The screws are treated to avoid loosening with vibration.
4. Connect the GPSDO power cable to J509 on the motherboard, and then to connector D on the GPSDO module
5. Connect an SMB to SMA cable between connectors B and J506 (PPS2)
6. Connect an SMB to SMA cable between connectors C and J507 (CLK REF2)
7. Connect the serial cable between connectors A and J312 (RS232-3) on the motherboard. If J312 on your USRP isn’t a keyed connector, please ensure to connect pin1 (TX) of connector A to pin3 (RX) on J312.
8. Remove the washer and nut from the MMCX to SMA-Bulkhead cable. Connect it to connector E and then insert SMA-Bulkhead connector through the hole in the rear panel. Tighten nut to fasten in place.
9. Replace the daughterboard pushing all the cables underneath.

Then run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=gpsdo --val=internal

**Removal instructions:**

Restore the jumper setting, disconnect the cables, and unscrew the GPSDO unit.
Then run the following commands:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=gpsdo --val=none

**Antenna Types:**

The GPSDO is capable of supplying a 3V for active GPS antennas or supporting passive antennas

------------------------------------------------------------------------
Miscellaneous
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available Sensors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following sensors are available for the USRP2/N-Series motherboards;
they can be queried through the API.

* mimo_locked - clock reference locked over the MIMO cable
* ref_locked - clock reference locked (internal/external)
* gps_time - GPS seconds (available when GPSDO installed)
