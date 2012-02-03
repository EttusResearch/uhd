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

########################################################################
## Customizing the DSP
########################################################################

As part of the USRP FPGA build-framework,
there are several convenient places for users to insert
custom DSP modules into the transmit and receive chains.

* before the DDC module
* after the DDC module
* replace the DDC module
* before the DUC module
* after the DUC module
* replace of the DUC module
* as an RX packet engine
* as an TX packet engine

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Customizing the top level makefile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Each USRP device has a makefile associated with it.
This makefile contains all of the necessary build rules.
When making a customized FPGA design,
start by copying the current makefile for your device.
Makefiles can be found in the usrp2/top/<dir>/Makefile.*

Edit your new makefile:
* set BUILD_DIR to a unique directory name
* set CUSTOM_SRCS for your verilog sources
* set CUSTOM_DEFS (see section below)

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Inserting custom modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
CUSTOM_DEFS is a string of space-separate key-value pairs.
Set the CUSTOM_DEFS variable so the FPGA fabric glue
will substitute your custom modules into the DSP chain.

Example:
CUSTOM_DEFS = "TX_ENG0_MODULE=my_tx_engine RX_ENG0_MODULE=my_rx_engine"
Where my_tx_engine and my_rx_engine are the names of custom verilog modules.

The following module definition keys are possible (X is a DSP number):

* TX_ENG<X>_MODULE: set the module for the transmit chain engine.
* RX_ENG<X>_MODULE: set the module for the receive chain engine.
* RX_DSP<X>_MODULE: set the module for the transmit dsp chain.
* TX_DSP<X>_MODULE: set the module for the receive dsp chain.

Examples of custom modules can be found in usrp2/custom/*.v
