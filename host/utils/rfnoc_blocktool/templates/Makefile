#
# Copyright 2019 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/erfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/erfnoc/utils/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath ${"\\"}
$(RFNOC_CORE_SRCS) ${"\\"}
$(RFNOC_UTIL_SRCS) ${"\\"}
$(RFNOC_BLOCK_${config.module_name.upper()}_SRCS) ${"\\"}
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
include $(BASE_DIR)/../sim/erfnoc/Makefile.srcs

SIM_TOP = rfnoc_block_${config.module_name}_tb

SIM_SRCS = ${"\\"}
$(abspath rfnoc_block_${config.module_name}_tb.sv) ${"\\"}
$(SIM_ERFNOC_SRCS)

# MODELSIM_USER_DO = $(abspath wave.do)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
