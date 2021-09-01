#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = ../../..
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = zynquplusRFSOC
PART_ID = xczu28dr/ffvg1517/-1/e

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/axi/Makefile.srcs
include $(BASE_DIR)/../lib/control/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(FIFO_SRCS) \
$(AXI_SRCS) \
$(CONTROL_LIB_SRCS) \
)

include ../common/Makefile.srcs
include ../100m/Makefile.srcs
include ../200m/Makefile.srcs
include ../400m/Makefile.srcs

DESIGN_SRCS += $(abspath \
../../regmap/PkgRFDC_REGS_REGMAP.vhd \
$(RF_COMMON_SRCS) \
$(RF_100M_SRCS) \
$(RF_200M_SRCS) \
$(RF_400M_SRCS) \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = $(BASE_DIR)/x400/ip
LIB_IP_DIR = $(BASE_DIR)/../lib/ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
#
# These TBs don't use any IP yet :)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
include $(BASE_DIR)/../sim/general/Makefile.srcs

# Define only one top-level module
SIM_TOP = rf_all_tb

# Simulation runtime in microseconds
SIM_RUNTIME_US = 1000

SIM_SRCS = \
$(abspath tb_adc_gearbox_2x1.vhd    ) \
$(abspath tb_adc_gearbox_2x4.vhd    ) \
$(abspath tb_adc_gearbox_8x4.vhd    ) \
$(abspath tb_capture_sysref.vhd     ) \
$(abspath tb_dac_gearbox_12x8.vhd   ) \
$(abspath tb_dac_gearbox_4x2.vhd    ) \
$(abspath tb_dac_gearbox_6x12.vhd   ) \
$(abspath tb_ddc_400m_saturate.vhd  ) \
$(abspath tb_duc_400m_saturate.vhd  ) \
$(abspath tb_rf_nco_reset.vhd       ) \
$(abspath tb_rf_reset_controller.vhd) \
$(abspath rf_all_tb.sv              ) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
