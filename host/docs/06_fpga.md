\page page_fpga FPGA Manual


Welcome to the USRP FPGA HDL source code tree! This repository contains
free & open-source FPGA HDL for the Universal Software Radio Peripheral
(USRP&trade;) SDR platform, created and sold by Ettus Research. A large
percentage of the source code is written in Verilog.

# Product Generations

This repository contains the FPGA source for the following generations of
USRP devices.

## Generation 1

\li Directory: __fpga/usrp1__
\li Devices: USRP Classic Only
\li Tools: Quartus from Altera
\li \subpage gen1_fpga_build "Build Instructions"


## Generation 2

\li Directory: __fpga/usrp2__
\li Devices: USRP N2X0, USRP B100, USRP E1X0, USRP2
\li Tools: ISE from Xilinx, GNU make
\li \subpage gen2_fpga_build "Build and Customization Instructions"

## Generation 3

\li Directory: __fpga/usrp3__
\li Devices: USRP B2XX, USRP E3XX, USRP N3XX, USRP X3XX, USRP X4XX
\li Tools: Vivado/ISE from Xilinx, GNU make
\li \subpage gen3_fpga_build_sim "Generation 3 USRP Build Documentation"

# Pre-built FPGA Images

Pre-built FPGA and Firmware images are not hosted here. Please visit \ref page_images
for instructions on downloading and using pre-built images. In most cases, running

    $ uhd_images_downloader

will do the right thing.

