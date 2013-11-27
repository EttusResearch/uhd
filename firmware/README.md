Firmware for USRP Devices
========================================================

## fx2/

__Description:__ This is the firmware for the FX2 USB PHY.

__Devices:__ USRP1 and B100 devices.

__Tools:__ sdcc, cmake

__Build Instructions:__

1. mkdir `<build directory>`
2. cd `<build directory>`
3. cmake `<source directory>`
4. make

## octoclock/

__Description:__ Firmware for the Octoclock device.

__Devices:__ Octoclock.

__Tools:__ avrtools, cmake

__Build Instructions:__ Run `make` from the Octoclock source directory.

## zpu/

__Description:__ Firmware for the soft CPUs in the UHD FPGA images.

__Devices:__ USRP2 and N-Series devices.

__Tools:__ zpu-gcc, cmake

This code requires the gcc-zpu tool-chain which can be found here:

http://opensource.zylin.com/zpudownload.html

zpu-elf-gcc should be in your `$PATH`

__Build Instructions:__

1. mkdir `<build directory>`
2. cd `<build directory>`
3. cmake `<source directory>`
4. make
