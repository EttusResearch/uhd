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
UHD can dynamically reconfigure the clock rate though the set_master_clock_rate() API call.
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

For the correct clock settings, call usrp->set_master_clock_rate(61.44e6)
before any other parameters are set in your application.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set other rates - uses internal VCO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use other clock rates, the jumpers will need to be in the default position.

* J16 is a two pin header, move the jumper to (pin1, pin2)
* J15 is a three pin header, move the jumper to (pin2, pin3)

For the correct clock settings, call usrp->set_master_clock_rate(rate)
before any other parameters are set in your application.
