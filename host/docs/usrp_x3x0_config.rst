========================================================================
UHD - System Configuration for USRP X3x0 Series
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Configuring your Host PC
------------------------------------------------------------------------

The USRP X3x0 is capable of delivering very fast sample rates to the host PC,
and even high-powered desktops can have trouble keeping up at the higher rates.
You can improve the performance of your host by configuring a number of
settings that affect the performance of your computer.

These are:

 * Kernel Version
 * Network Configuration
 * Power Management Configuration
 * Real-Time & Priority Scheduling
 * Building with ORC & Volk

These items are covered in more detail, below.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Kernel Version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Performance issues may be encountered with Linux kernels earlier than 3.11.
Ettus Research strongly recommends using kernel version 3.11 or higher for high
sample rates.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Network Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When using Ethernet interfaces to communicate with the device, it is necessary
to configure a number of facets regarding your network connection.

Configuring NetworkManager
-------------------------------------
Fedora and Ubuntu both use NetworkManager to manage network connections.
Unfortunately, NetworkManager often tries to take control of a connection and
will disconnect the interface.

You should open your NetworkManager configuration and tell it to ignore the
network interface you are using. **This is not the same as simply setting
a static IP address.** You *must* tell NetworkManager to ignore the interface.

Configuring the Socket Buffers
-------------------------------------
It is necessary to increase the maximum size of the socket buffers to avoid
potential overflows and underruns at high sample rates.  Add the following
entries into /etc/sysctl.conf (root privileges required):

::

    net.core.rmem_max=33554432
    net.core.wmem_max=33554432

Either restart the system or issue the following commands:

::

    sudo sysctl -w net.core.rmem_max=33554432
    sudo sysctl -w net.core.wmem_max=33554432


Configuring the MTU
-------------------------------------
In order to achieve maximum performance, we recommend setting the MTU size to
9000 for 10 GigE and 1500 for 1 GigE. It is possible to use smaller MTUs, but this
can affect performance. With some NICs, setting the MTU too high can also cause issues.
To set the MTU to 9000, you can use the following command:

::

    sudo ifconfig <interface> mtu 9000 # For 10 GigE
    sudo ifconfig <interface> mtu 1500 # For 1 GigE

Using these MTUs will set the frame sizes for UHD communication to 8000 and 1472,
respectively.

In some cases, specifying the frame size manually by adding the argument
"<send/recv>_frame_size=1472" can solve issues. Note that a frame size of 1472 will limit
the available sampling rate, although this is not a problem on 1 GigE.


Configuring the Firewall
-------------------------------------
Many Linux distributions come installed with a Firewall, by default. The
Firewall will often interfere with your ability to communicate with your USRP.
You should configure your firewall to "trust" the interface you are using.
Setting this properly depends on your OS and firewall configuration method.

Interface Configuration File (Fedora)
-------------------------------------
On Fedora systems, you can configure the network interface mostly from one
place (with the exception of the socket buffers). Each interface on your system
should have a file in:

::

    /etc/sysconfig/network-scripts/

As an example, if your 1GigE interface is "em1", your "ifcfg-em1" configuration
file should look something like this, when configured for use with a USRP X3xx:

::

    TYPE="Ethernet"
    BOOTPROTO="none"
    IPADDR0="192.168.10.1"
    DEFROUTE="yes"
    IPV4_FAILURE_FATAL="no"
    IPV6INIT="no"
    IPV6_FAILURE_FATAL="no"
    NAME="em1"
    UUID="<specific to your device>"
    ONBOOT="no"
    HWADDR"<specific to your device>"
    PEERDNS="yes"
    PEERROUTES="yes"
    ZONE="trusted"
    MTU="9000"
    NM_MANAGED="no"

The above file was generated and modified on a "Fedora 20" system.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Power Management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Power management on the host system attempts to save power by reducing clock
frequencies or even powering off devices while not in use.  This can lead to
significant performance issues when trying to operate at high sample rates.
Ettus Research strongly recommends disabling all power management.


Setting the CPU Governors
-------------------------------------
In Linux, the CPU governors dictate the frequency at which the CPU operates and
attempt to reduce the CPU frequencies at certain times to save power.  When
running at high sample rates, reduction of CPU frequencies can cause
significant performance issues.  To prevent those issues, set the governor to
"performance".

**Ubuntu:**
1. Install cpufrequtils:

::

    sudo apt-get install cpufrequtils

2. Edit /etc/init.d/cpufrequtils and set GOVERNOR="performance" on the appropriate line (run as root):

::

    sed s/^GOVERNOR=.*$/GOVERNOR=\"performance\"/g /etc/init.d/cpufrequtils > /etc/init.d/cpufrequtils

3. Restart cpufrequtils:

::

    sudo /etc/init.d/cpufrequtils restart

**Fedora:**

::

    sudo cpupower frequency-set -g performance

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Real-Time & Priority Scheduling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Enabling real-time and priority scheduling can improve the total processing
throughput of your application. Priority scheduling should be enabled for UHD,
and real-time scheduling can be enabled by your application.

Thread Priority Scheduling with UHD
-------------------------------------
For information regarding how to enable priority scheduling for UHD on your
system, please see the `General UHD Notes <./general.html#threading-notes>`_.

Real-Time Scheduling in your Application
----------------------------------------
Please note that turning on real-time scheduling in your application **may lock
up your computer** if the processor cannot keep up with the application. You
should generally avoid using real-time scheduling unless you need to.

Real-time scheduling is enabled via different methods depending on your
application and operating system. In GNU Radio Companion, it can be turned on in
each individual flowgraph.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Building with ORC & Volk
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Especially when running high-performance applications, processing performance
can be dramatically improved by SIMD instructions. UHD uses ORC to provide SIMD
capability, and GNU Radio includes a SIMD library called "Volk". These should
both be used to guarantee optimum performance.

Compiling UHD with ORC
-------------------------------------
ORC, the `Oil Runtime Compiler <http://code.entropywave.com/orc/>`_, is
a third-party compiler that UHD uses to create efficient SIMD code for your
particular computer. ORC is generally easily installed from your OS's package
manager.

On Fedora:

::

    $ sudo yum update; sudo yum install orc-compiler orc-devel

On Ubuntu:

::

    $ sudo apt-get update; sudo apt-get install liborc-<version> liborc-<version>-dev

After installing ORC, when building UHD from source, you should see "ORC" as
one of the configured UHD components.

::

    -- ######################################################
    -- # UHD enabled components                              
    -- ######################################################
    --   * LibUHD
         <cut for brevity>
    --   * ORC

Compiling GNURadio with Volk
-------------------------------------
If you are using GNURadio to build applications, you should compile GNURadio
with Volk. For instructions on how to do this, `refer to the GNURadio wiki
<http://gnuradio.org/redmine/projects/gnuradio/wiki/Volk>`_.


------------------------------------------------------------------------
Host PC Hardware Selection
------------------------------------------------------------------------
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Motherboard
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Testing has shown that some motherboards do not provide enough PCIe bus
bandwidth to support higher sample rates.  Motherboards with PCIe 3.0 are
required and the PCIe architecture of the motherboard should be carefully
considered.  Slots with dedicated PCIe lanes should be used for PCIe or 10GbE
cards that will be connected to the X3x0 device.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
10GbE NIC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Intel or Myricom 10GbE NICs are recommended.  Mellanox, SolarFlare, and Chelsio
10GbE NICs are not currently recommended.  The Ethernet card should be plugged
into the slot that has the most direct connection with the CPU (PCIe lanes are
not shared with another slot).  Refer to the motherboard manual for more
information on PCIe architecture.

------------------------------------------------------------------------
Troubleshooting Performance Issues
------------------------------------------------------------------------
The output on the host console provides indicators of performance issues in the
form of single upper-case letters.  The following table lists the letters,
their meanings, and possible causes:

========= ====================== ====================================================================
Indicator Meaning                Possible Causes
========= ====================== ====================================================================
O         Overflow on RX         - Data is not being consumed by user's application fast enough.
                                 - CPU governor or other power management not configured correctly.
D         Dropped packet on RX   - Network hardware failure.  (Check host NIC, cable, switch, etc...)
                                 - PCIe bus on host cannot sustain throughput. (Check ethtool -S <interface>).
                                 - CPU governor or other power management not configured correctly.
                                 - Frame size might not work with the current NIC's MTU.
U         Underflow on TX        - Samples are not being produced by user's application fast enough.
                                 - CPU governor or other power management not configured correctly.
L         Late packet            - Samples are not being produced by user's application fast enough.
          (usually on MIMO TX)   - CPU governor or other power management not configured correctly.
                                 - Incorrect/invalid time_spec provided.
S         Sequence error on TX   - Network hardware failure.  (Check host NIC, cable, switch, etc...)
                                 - Frame size might not work with the current NIC's MTU.
========= ====================== ====================================================================

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Troubleshooting Ethernet Issues
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
1. First, check 'ifconfig <interface>' to see if there are any errors reported
   on the interface.  If there are errors, it is most likely a network hardware
   problem.
2. Next, check the output of 'ethtool -S <interface>'.  The output is
   driver-specific, but may give important clues as to what may be happening.
   For example, a high value on rx_missed_errors for an Intel NIC indicates
   that the bus (i.e. PCIe) is not keeping up.
3. Finally, Wireshark can be used to validate the traffic between the host and
   device and make sure there is no unwanted traffic on the interface.

