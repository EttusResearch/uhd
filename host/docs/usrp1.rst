========================================================================
UHD - USRP1 Application Notes
========================================================================

.. contents:: Table of Contents

------------------------------------------------------------------------
Comparative features list
------------------------------------------------------------------------

* 2 transceiver card slots
* 2 RX DDC chains in FPGA
* 2 TX DUC chains in FPGA (no TX CORDIC -> uses DAC)
* 64 MHz fixed clock rate
* sc16 sample modes
* sc8 sample mode - RX only

------------------------------------------------------------------------
Specify a Non-standard Image
------------------------------------------------------------------------
The standard USRP1 images installer comes with two FPGA images:
 * **usrp1_fpga.rbf:** 2 DDCs + 2 DUCs
 * **usrp1_fpga_4rx.rbf:** 4 DDCs + 0 DUCs

By default, the USRP1 uses the FPGA image with 2 DDCs and 2 DUCs.
However, a device address parameter can be used to override
the FPGA image selection to use an alternate or a custom FPGA image.
See the images application notes for installing custom images.

Example device address string representations to specify non-standard firmware and/or FPGA images:

::

    fpga=usrp1_fpga_4rx.rbf

    -- OR --

    fw=usrp1_fw_custom.ihx

    -- OR --

    fpga=usrp1_fpga_4rx.rbf, fw=usrp1_fw_custom.ihx

------------------------------------------------------------------------
Missing and Emulated Features
------------------------------------------------------------------------
The USRP1 FPGA does not have the necessary space to support the advanced
streaming capabilities that are possible with the newer USRP devices.
Some of these features are emulated in software to support the API.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
List of emulated features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Setting the current device time
* Getting the current device time
* Transmitting at a specific time
* Transmitting a specific number of samples
* Receiving at a specific time
* Receiving a specific number of samples
* End of burst flags for transmit/receive
* Notification on late stream command
* Notification on late transmit packet
* Notification on underflow or overflow
* Notification on broken chain error

**Note:**
These emulated features rely on the host system's clock for timed operations
and therefore may not have sufficient precision for the application.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
List of missing features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Start of burst flags for transmit/receive

------------------------------------------------------------------------
Hardware Setup Notes
------------------------------------------------------------------------

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
External clock modification
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The USRP can be modified to accept an external clock reference instead of the 64MHz onboard reference.
 * Solder SMA (**LTI-SASF54GT**) connector to **J2001**.
 * Move 0 ohm 0603 resistor **R2029** to **R2030**.
 * Move 0.01uF 0603 capacitor **C925** to **C926**.
 * Remove 0.01uF 0603 capacitor **C924**.

The new external clock needs to be a square wave between +7dBm and +15dBm

After the hardware modification,
the user should burn the setting into the EEPROM,
so UHD can initialize with the correct clock rate.
Run the following commands to record the setting into the EEPROM:
::

    cd <install-path>/share/uhd/utils
    ./usrp_burn_mb_eeprom --args=<optional device args> --key=mcr --val=<rate>

The user may override the clock rate specified in the EEPROM by using a device address:
Example:
::

    uhd_usrp_probe --args="mcr=52e6"
