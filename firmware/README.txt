########################################################################
# Firmware for USRP devices
########################################################################

fx2/

    Description: firmware for FX2 device

    Devices: USRP1 and B100

    Tools: sdcc, cmake

    Build Instructions:
        1) mkdir <build directory>
        2) cd <build directory>
        3) cmake <source directory>
        4) make

zpu/

    Description: firmware for soft CPU in FPGA

    Devices: USRP2 and N Series

    Tools: zpu-gcc, cmake

    Build Instructions:
        1) mkdir <build directory>
        2) cd <build directory>
        3) cmake <source directory>
        4) make
