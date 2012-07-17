========================================================================
UHD - USRP-B100 Series Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Comparative features list
------------------------------------------------------------------------

* 1 transceiver card slot
* 1 RX DDC chain in FPGA
* 1 TX DUC chain in FPGA
* Timed commands in FPGA
* Timed sampling in FPGA
* External PPS reference
* External 10MHz reference
* Configurable clock rate (defaults 64 MHz)
* sc8 and sc16 sample modes

------------------------------------------------------------------------
Specify a Non-standard Image
------------------------------------------------------------------------
UHD will automatically select the USRP B-Series images from the installed images package.
The image selection can be overridden with the **--fpga=** and **--fw=** device address parameters.

Example device address string representations to specify non-standard images:

::

    fpga=usrp_b100_fpga_firmware.bin

    -- OR --

    fw=usrp_b100_fw_firmware.ihx

------------------------------------------------------------------------
Changing the Master Clock Rate
------------------------------------------------------------------------
The master clock rate of the USRP embedded feeds both the FPGA DSP and the codec chip.
Hundreds of rates between 32MHz and 64MHz are available.
A few notable rates are:

* **64MHz:** maximum rate of the codec chip
* **61.44MHz:** good for UMTS/WCDMA applications
* **52Mhz:** good for GSM applications

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set 61.44MHz - uses external VCXO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use the 61.44MHz clock rate, the USRP embedded will require one jumper to be moved,
and X4 must be populated with a 61.44 MHz oscillator.

* **J15** is a three pin header, move the jumper to (pin1, pin2)
* **357LB3I061M4400** is the recommended oscillator for X4

**Note:** See instructions below to communicate the desired clock rate into UHD.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set other rates - uses internal VCO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To use other clock rates, the jumper will need to be in the default position.

* **J15** is a three pin header, move the jumper to (pin2, pin3)

To communicate the desired clock rate into UHD,
specify the a special device address argument,
where the key is **master_clock_rate** and the value is a rate in Hz.
Example:
::

    uhd_usrp_probe --args="master_clock_rate=52e6"

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

------------------------------------------------------------------------
Miscellaneous
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Available Sensors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The following sensors are available;
they can be queried through the API.

* **ref_locked:** clock reference locked (internal/external)
