FPGA Manual
===========

Welcome to the USRP FPGA HDL source code tree! This repository contains
free & open-source FPGA HDL for the Universal Software Radio Peripheral
(USRP&trade;) SDR platform, created and sold by Ettus Research. A large
percentage of the source code is written in Verilog.

## Product Generations

This repository contains the FPGA source for the following generations of
USRP devices.

### Generation 1

\li Directory: __usrp1__
\li Devices: USRP Classic Only
\li Tools: Quartus from Altera
\li \subpage md_usrp1_build_instructions "Build Instructions"


### Generation 2

\li Directory: __usrp2__
\li Devices: USRP N2X0, USRP B100, USRP E1X0, USRP2
\li Tools: ISE from Xilinx, GNU make
\li \subpage md_usrp2_build_instructions "Build Instructions"
\li \subpage md_usrp2_customize_signal_chain "Customization Instructions"

### Generation 3

\li Directory: __usrp3__
\li Devices: USRP B2X0, USRP X Series, USRP E3X0
\li Tools: ISE from Xilinx, GNU make
\li \subpage md_usrp3_build_instructions "Build Instructions"
\li \subpage md_usrp3_simulation "Simulation"

## Pre-built FPGA Images

Pre-built FPGA and Firmware images are not hosted here. Please visit \ref page_images
for instructions on downloading and using pre-built images. In most cases, running

    $ uhd_images_downloader

will do the right thing.

