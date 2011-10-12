========================================================================
UHD - USRP-E1XX Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Specify a non-standard image
------------------------------------------------------------------------
The UHD will automatically select the USRP embedded FPGA image from the installed images package.
The FPGA image selection can be overridden with the "fpga" device address parameter.

Example device address string representations to specify non-standard FPGA image:

::

    fpga=usrp_e100_custom.bin

------------------------------------------------------------------------
Changing the master clock rate
------------------------------------------------------------------------
The master clock rate of the USRP embedded feeds both the FPGA DSP and the codec chip.
Hundreds of rates between 32MHz and 64MHz are available.
A few notable rates are:

* 64MHz - maximum rate of the codec chip
* 61.44MHz - good for UMTS/WCDMA applications
* 52Mhz - good for GSM applications

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set 61.44MHz - uses external VCXO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use the 61.44MHz clock rate, the USRP embedded will require two jumpers to be moved.

* J16 is a two pin header, remove the jumper (or leave it on pin1 only)
* J15 is a three pin header, move the jumper to (pin1, pin2)

**Note:** See instructions below to communicate the desired clock rate into the UHD.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set other rates - uses internal VCO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use other clock rates, the jumpers will need to be in the default position.

* J16 is a two pin header, move the jumper to (pin1, pin2)
* J15 is a three pin header, move the jumper to (pin2, pin3)

To communicate the desired clock rate into the UHD,
specify the a special device address argument,
where the key is "master_clock_rate" and the value is a rate in Hz.
Example:
::

    uhd_usrp_probe --args="master_clock_rate=52e6"

------------------------------------------------------------------------
Clock Synchronization
------------------------------------------------------------------------


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Ref Clock - 10MHz
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The E1xx has a 10MHz TCXO which can be used to discipline the flexible clocking by 
selecting REF_INT for the clock_config_t.

Alternately, an external 10MHz reference clock can be supplied by soldering a connector.

* Connector J10 (REF_IN) needs MCX connector WM5541-ND or similar
* Square wave will offer the best phase noise performance, but sinusoid is acceptable
* Power level: 0 to 15dBm
* Select REF_SMA in clock_config_t


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
PPS - Pulse Per Second
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
An exteral PPS signal for timestamp synchronization can be supplied by soldering a connector.

* Connector J13 (PPS) needs MCX connector WM5541-ND or similar
* Requires a square wave signal
* Amplitude: 3.3 to 5Vpp

Test the PPS input with the following app:

* <args> are device address arguments (optional if only one USRP is on your machine)

::

    cd <install-path>/share/uhd/examples
    ./test_pps_input --args=<args>

------------------------------------------------------------------------
Hardware setup notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Front panel LEDs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The LEDs on the front panel can be useful in debugging hardware and software issues.
The LEDs reveal the following about the state of the device:

* **LED A:** transmitting
* **LED B:** fpga loaded
* **LED C:** receiving
* **LED D:** fpga loaded
* **LED E:** reference lock
* **LED F:** board power
