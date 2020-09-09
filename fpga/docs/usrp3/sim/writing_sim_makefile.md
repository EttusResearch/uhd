# Writing a Simulation Makefile

The testbench Makefile tells the build tools what to build, where to build it,
dependency information, and runtime information. The build infrastructure will
handle the how-to part for each supported simulation tool.

An example makefile for a testbench is shown below. If creating a testbench for
RFNoC, it is recommended to use the RFNoC ModTool to generate a Makefile
template for your RFNoC block.

    #
    # Copyright 2020 Ettus Research, a National Instruments Brand
    #
    # SPDX-License-Identifier: LGPL-3.0-or-later
    #

    #-------------------------------------------------
    # Top-of-Makefile
    #-------------------------------------------------
    # Define BASE_DIR to point to the "top" dir. Note:
    # UHD_FPGA_DIR must be passed into this Makefile.
    ifndef UHD_FPGA_DIR
    $(error "UHD_FPGA_DIR is not set! Must point to UHD FPGA repository!")
    endif
    BASE_DIR = $(UHD_FPGA_DIR)/usrp3/top
    # Include viv_sim_preample after defining BASE_DIR
    include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

    #-------------------------------------------------
    # Design Specific
    #-------------------------------------------------
    # Define part using PART_ID (<device>/<package>/<speedgrade>)
    ARCH = kintex7
    PART_ID = xc7k410t/ffg900/-2

    # Include makefiles and sources for the DUT and its dependencies
    include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
    include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
    include $(BASE_DIR)/../lib/fifo/Makefile.srcs
    include $(BASE_DIR)/../lib/axi/Makefile.srcs
    include $(BASE_DIR)/../lib/control/Makefile.srcs

    DESIGN_SRCS += $(abspath \
    $(RFNOC_CORE_SRCS) \
    $(RFNOC_UTIL_SRCS) \
    $(RFNOC_OOT_SRCS) \
    $(FIFO_SRCS) \
    $(AXI_SRCS) \
    $(CONTROL_LIB_SRCS) \
    )

    #-------------------------------------------------
    # IP Specific
    #-------------------------------------------------
    # If simulation contains IP, define the IP_DIR and point
    # it to the base level IP directory
    IP_DIR = $(BASE_DIR)/x300/ip
    LIB_IP_DIR = $(BASE_DIR)/../lib/ip

    # Include makefiles and sources for all IP components
    # *after* defining the IP_DIR
    include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
    include $(LIB_IP_DIR)/axi_fft/Makefile.inc

    DESIGN_SRCS += $(abspath \
    $(IP_FIFO_4K_2CLK_SRCS) \
    $(LIB_IP_AXI_FFT_SRCS) \
    rfnoc_block_example.v \
    )

    #-------------------------------------------------
    # Testbench Specific
    #-------------------------------------------------
    include $(BASE_DIR)/../sim/general/Makefile.srcs
    include $(BASE_DIR)/../sim/axi/Makefile.srcs

    # Define only one top-level module
    SIM_TOP = rfnoc_block_example_tb

    # Simulation runtime in microseconds
    SIM_RUNTIME_US = 1000

    SIM_SRCS = \
    $(abspath rfnoc_block_example_tb.sv) \

    #-------------------------------------------------
    # Bottom-of-Makefile
    #-------------------------------------------------
    # Include all simulator specific makefiles here
    # Each should define a unique target to simulate
    # e.g. xsim, vsim, etc and a common "clean" target
    include $(BASE_DIR)/../tools/make/viv_simulator.mak

You will notice that the Makefile has 5 distinct sections.

## Section 1: Boilerplate

    #-------------------------------------------------
    # Top-of-Makefile
    #-------------------------------------------------
    # Define BASE_DIR to point to the "top" dir. Note:
    # UHD_FPGA_DIR must be passed into this Makefile.
    ifndef UHD_FPGA_DIR
    $(error "UHD_FPGA_DIR is not set! Must point to UHD FPGA repository!")
    endif
    BASE_DIR = $(UHD_FPGA_DIR)/usrp3/top
    # Include viv_sim_preample after defining BASE_DIR
    include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

Before declaring any variables or using any recipes, the following must be
performed (in order):

- Define `BASE_DIR` to tell the build system where the `<repo>/fpga/usrp3/top`
  directory is relative to the current testbench directory
- Include `viv_sim_preamble.mak` to initialize boilerplate variables and
  functions

This example uses the make variable UHD_FPGA_DIR to specify the location of the
FPGA repository. This is useful for out-of-tree modules. UHD_FPGA_DIR is the
variable used by CMake for the RFNoC examples.

## Section 2: Design Specific

    #-------------------------------------------------
    # Design Specific
    #-------------------------------------------------
    # Define part using PART_ID (<device>/<package>/<speedgrade>)
    ARCH = kintex7
    PART_ID = xc7k410t/ffg900/-2

    # Include makefiles and sources for the DUT and its dependencies
    include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
    include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
    include $(BASE_DIR)/../lib/fifo/Makefile.srcs
    include $(BASE_DIR)/../lib/axi/Makefile.srcs
    include $(BASE_DIR)/../lib/control/Makefile.srcs

    DESIGN_SRCS += $(abspath \
    $(RFNOC_CORE_SRCS) \
    $(RFNOC_UTIL_SRCS) \
    $(RFNOC_OOT_SRCS) \
    $(FIFO_SRCS) \
    $(AXI_SRCS) \
    $(CONTROL_LIB_SRCS) \
    )

This section contains pointers to sources and other variables needed for the
DUT to function. In the example above, we are including all sources from the
`lib/fifo`, `lib/axi`, `lib/control` directories. These may not be needed for
your simulation, in which case they can be removed to reduce compile times.

The following makefile variables are special and should be defined:

- `ARCH`: The architecture targeted for the simulation.
- `PART_ID`: The exact part targeted for the simulation. Format:
  `<device>/<package>/<speedgrade>`
- `DESIGN_SRCS`: Space-separated paths to the DUT and all of its dependencies.

If `ARCH` and `PART_ID` are not specified, a default part will be assumed. This
may be fine if your DUT doesn't have any architecture-specific IP.

## Section 3: IP Specific

    #-------------------------------------------------
    # IP Specific
    #-------------------------------------------------
    # If simulation contains IP, define the IP_DIR and point
    # it to the base level IP directory
    IP_DIR = $(BASE_DIR)/x300/ip
    LIB_IP_DIR = $(BASE_DIR)/../lib/ip

    # Include makefiles and sources for all IP components
    # *after* defining the IP_DIR
    include $(IP_DIR)/fifo_4k_2clk/Makefile.inc
    include $(LIB_IP_DIR)/axi_fft/Makefile.inc

    DESIGN_SRCS += $(abspath \
    $(IP_FIFO_4K_2CLK_SRCS) \
    $(LIB_IP_AXI_FFT_SRCS) \
    rfnoc_block_example.v \
    )

If the DUT depends on any Xilinx IP then this section is required. It tells the
tools which IP cores need to be built in order to run the simulation. The IP
specific Makefile `include` statements handle the "how" part of building IP. As
long as the correct Makefile is included and the IP XCI sources are added to
`DESIGN_SRCS`, the IP intermediates will be built correctly.

The `IP_DIR` variable shown here is defined to point to the base IP directory
that contains target-specific IP sources. The `IP_LIB_DIR` variable shown here
is defined to point to the base IP directory that contains target-independent
IP sources.

The IP included in this example may not be needed for your testbench, in which
case it can be removed to reduce compile times.

## Section 4: Testbench Specific

    #-------------------------------------------------
    # Testbench Specific
    #-------------------------------------------------
    include $(BASE_DIR)/../sim/general/Makefile.srcs
    include $(BASE_DIR)/../sim/axi/Makefile.srcs

    # Define only one top-level module
    SIM_TOP = rfnoc_block_example_tb

    # Simulation runtime in microseconds
    SIM_RUNTIME_US = 1000

    SIM_SRCS = \
    $(abspath rfnoc_block_example_tb.sv) \

This section contains all sources and parameters for the actual testbench. Any simulation
dependency makefiles can be included here.

The following variables should be defined:

- `SIM_TOP`: The top-level module name for the simulation.
- `SIM_RUNTIME_US`: The maximum runtime of the simulation in microseconds. At
  this time `$finish` will be called to terminate the testbench.
- `SIM_SRCS`: This is similar to DESIGN_SRCS except that it should contain a
  path to `SIM_TOP` and all of its dependencies.

## Section 5: Tool Support

    #-------------------------------------------------
    # Bottom-of-Makefile
    #-------------------------------------------------
    # Include all simulator specific makefiles here
    # Each should define a unique target to simulate
    # e.g. xsim, vsim, etc and a common "clean" target
    include $(BASE_DIR)/../tools/make/viv_simulator.mak

Now that the Makefile knows all the basic information about the testbench,
include tool-specific makefiles to implement simulation targets. Currently the
following simulator makefiles exits:

- `<repo>/fpga/tools/make/viv_simulator.mak`<br>
  This makefile supports both Vivado and ModelSim simulators.
