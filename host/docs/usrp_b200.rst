========================================================================
UHD - USRP-B2X0 Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Comparative features list - B200
------------------------------------------------------------------------

* integrated RF frontend (RF coverage from 70 MHz - 6 GHz)
* 1 RX DDC chain in FPGA
* 1 TX DUC chain in FPGA
* Timed commands in FPGA
* Timed sampling in FPGA
* External PPS reference
* External 10MHz reference
* Configurable clock rate
* Internal GPSDO option

------------------------------------------------------------------------
Comparative features list - B210
------------------------------------------------------------------------

* integrated MIMO frontend (RF coverage from 70 MHz - 6 GHz)
* 2 RX DDC chains in FPGA
* 2 TX DUC chains in FPGA
* Timed commands in FPGA
* Timed sampling in FPGA
* External PPS reference
* External 10MHz reference
* Configurable clock rate
* Internal GPSDO option

------------------------------------------------------------------------
Specify a Non-standard Image
------------------------------------------------------------------------
UHD software will automatically select the USRP B2X0 images from the installed images package.
The image selection can be overridden with the **--fpga=** and **--fw=** device address parameters.

Example device address string representations to specify non-standard images:

::

    fpga=usrp_b200_fpga.bin

    -- OR --

    fw=usrp_b200_fw.hex

------------------------------------------------------------------------
Changing the Master Clock Rate
------------------------------------------------------------------------
The master clock rate feeds the RF frontends and the DSP chains.
Users may select non-default clock rates to acheive integer decimations or interpolations in the DSP chains.
The default master clock rate defaults to 32 MHz, but can be set to any rate between 5 MHz and 61.44 MHz.

The user can set the master clock rate through the usrp API call set_master_clock_rate(),
or the clock rate can be set through the device arguments, which many applications take:
::

    uhd_usrp_probe --args="master_clock_rate=52e6"

------------------------------------------------------------------------
RF Frontend Notes
------------------------------------------------------------------------
The B200 features and integrated RF frontend.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Frontend tuning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The RF frontend has individually tunable receive and transmit chains.
On the B200, there is one transmit and one receive RF frontend.
On the B210, both transmit and receive can be used in a MIMO configuration.
For the MIMO case, both receive frontends share the RX LO,
and both transmit frontends share the TX LO.
Each LO is tunable between 50 MHz and 6 GHz.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Frontend gain
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
All frontends have individual analog gain controls.
The receive frontends have 73 dB of available gain;
and the transmit frontends have 89 dB of available gain.
Gain settings are application specific,
but its recommended that users consider using at least
half of the available gain to get reasonable dynamic range.

------------------------------------------------------------------------
Hardware Reference
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
LED Indicators
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below is a table of the LED indicators and their meanings:

===============  ========================  ========================================================
Component ID     Description               Details
===============  ========================  ========================================================
 LED600          Power Indicator           | off = no power applied
                                           | red = power applied (external or USB)
 LED800          Channel 2 RX2 Activity    | off = no activity
                                           | green = receiving*
 LED801          Channel 2 TX/RX Activity  | off = no activity
                                           | green = receiving*
                                           | red = transmitting
                                           | orange = switching between transmitting and receiving
 LED802          Channel 1 TX/RX Activity  | off = no activity
                                           | green = receiving*
                                           | red = transmitting
                                           | orange = switching between transmitting and receiving
 LED803          Channel 1 RX2 Activity    | off = no activity
                                           | green = receiving*
 LED100          GPS lock indicator        | off = no lock
                                           | green = lock
===============  ========================  ========================================================

\* RX activity LED indicators will blink off in a receive overflow condition, indicating that the host is not receiving samples fast enough.  The host will be notified and output an "O" as well. 


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
External Connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below is a table showing the external connections and respective power information:

===============  ========================  ========================================================
Component ID     Description               Details
===============  ========================  ========================================================
 J601            External Power            | 6 V
                                           | 3 A
 J701            USB Connector
 J104            External PPS Input        | 3.3 V nominal
                                           | 3.6 V recommended max
                                           | 4.6 V absolute max
 J101            GPS Antenna               | GPSDO will supply nominal voltage to antenna.
 J100            External 10 MHz Input     | 15 dBm max (3.5 V into 50 ohms)
 J800            RF B: TX/RX               | TX power 10dBm max
                                           | RX power -15dBm max
 J802            RF B: RX2                 | RX power -15dBm max
 J803            RF A: RX2                 | RX power -15dBm max
 J801            RF A: TX/RX               | TX power 10dBm max
                                           | RX power -15dBm max
===============  ========================  ========================================================

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On-Board Connectors and Switches
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below is a table showing the on-board connectors and switches:

===============  ========================  ========================================================
Component ID     Description               Details
===============  ========================  ========================================================
 J502            Mictor Connector          | Interface to FPGA for I/O and inspection.
 J503            JTAG Header               | Interface to FPGA for programming and debugging.
 J400            Debug Header              | Pin 1 - serial data out (115200 8,N,1 @ 1.8V)
                                           | Pin 2 - ground
                                           | Pin 3 - serial data in (not connected) 
 S100            GPSDO ISP Enable Switch   | Not supported
 S700            FX3 Hard Reset Switch
===============  ========================  ========================================================

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Test Points
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Below is a table showing the test points:

===============  ==================================  ===================
Component ID     Description                         Details
===============  ==================================  ===================
 J603            Upstream Voltage Regulation         | 3.3 V supply
                 Test Point                          | Pin 1 - 3.7 V
                                                     | Pin 2 - gnd
 J604            Catalina Supply Test Point          | Pin 1 - 1.3 V
                                                     | Pin 2 - gnd
 J605            FPGA Supply Test Point              | Pin 1 - 1.2 V
                                                     | Pin 2 - gnd
 J606            FX3 Supply Test Point               | Pin 1 - 1.2V
                                                     | Pin 2 - gnd
 J609            Upstream Voltage Regulation         | 1.3 V supply
                 Test Point                          | Pin 1 - 1.8 V
                                                     | Pin 2 - gnd
 T600            External Voltage Supply
                 Test Point
 T601            1.3 V Catalina Power Good
                 Test Point
 T602            1.3 V Catalina Synthesizer
                 Power Good Test Point
 TP302           Catalina AUX DAC1 Test Point
 TP303           Catalina AUX DAC2 Test Point
 T700            Not connected
 T701            Not connected
 T702            FX3 External Clock In               | Not used
 T703            FX3 Charger Detect Out
===============  ==================================  ===================


