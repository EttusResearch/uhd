# Customizing the Signal Chain in Generation 2 products

## Adding DSP logic to Generation 2 products

As part of the USRP FPGA build-framework, there are several convenient places
for users to insert custom DSP modules into the transmit and receive chains.

- Before the DDC module
- After the DDC module
- Replace the DDC module
- Before the DUC module
- After the DUC module
- Replace the DUC module
- As an RX packet engine
- As an TX packet engine

### Customizing the top level makefile

Each USRP device has a makefile associated with it. This makefile contains all
of the necessary build rules. When making a customized FPGA design, start by
copying the current makefile for your device. Makefiles can be found in
`usrp2/top/{product}/Makefile`

Edit your new makefile:
- Set BUILD_DIR to a unique directory name
- Set CUSTOM_SRCS for your verilog sources
- Set CUSTOM_DEFS (see section below)

### Inserting custom modules

CUSTOM_DEFS is a string of space-separate key-value pairs. Set the CUSTOM_DEFS
variable so the FPGA fabric glue will substitute your custom modules into the
DSP chain.

Example:

    CUSTOM_DEFS = "TX_ENG0_MODULE=my_tx_engine RX_ENG0_MODULE=my_rx_engine"

Where `my_tx_engine` and `my_rx_engine` are the names of custom verilog modules.

The following module definition keys are possible (X is a DSP number):

- `TX_ENG<X>_MODULE`: Set the module for the transmit chain engine.
- `RX_ENG<X>_MODULE`: Set the module for the receive chain engine.
- `RX_DSP<X>_MODULE`: Set the module for the transmit dsp chain.
- `TX_DSP<X>_MODULE`: Set the module for the receive dsp chain.

Examples of custom modules can be found in the Verilog files in `usrp2/custom/`.

