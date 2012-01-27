########################################################################
## Welcome to the USRP FPGA source code tree
########################################################################

usrp1/

    Description: generation 1 products

    Devices: USRP classic only

    Tools: Quartus from Altera

    Project file: usrp1/toplevel/usrp_std/

usrp2/

    Description: generation 2 products

    Devices: USRP2, N2XX, B100, E1XX

    Tools: ISE from Xilinx, GNU make

    Build Instructions:
        1) ensure that xtclsh is in the $PATH
        2) cd usrp2/top/<project-directory>
        3) make -f Makefile.<device> bin
        4) bin file in build-<device>/*.bin

    Customize the DSP:
        Implement design in usrp2/custom/custom_*.v
        Instructions are included in the module.
