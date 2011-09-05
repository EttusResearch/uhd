========================================================================
UHD - USRP-B1XX Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Specify a non-standard image
------------------------------------------------------------------------
The UHD will automatically select the USRP B-Series images from the installed images package.
The image selection can be overridden with the "fpga" and "fw" device address parameters.

Example device address string representations to specify non-standard images:

::

    fpga=usrp_b100_fpga_firmware.bin

    -- OR --

    fw=usrp_b100_fw_firmware.ihx

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
