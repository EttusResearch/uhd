#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = ../../../../top
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its
# dependencies.
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include $(LIB_IP_DIR)/complex_multiplier_dds/Makefile.inc
include $(LIB_IP_DIR)/dds_sin_cos_lut_only/Makefile.inc
include $(BASE_DIR)/x300/coregen_dsp/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(RFNOC_UTIL_SRCS) \
$(LIB_IP_COMPLEX_MULTIPLIER_DDS_SRCS) \
$(LIB_IP_DDS_SIN_COS_LUT_ONLY_SRCS) \
$(COREGEN_DSP_SRCS) \
$(RFNOC_BLOCK_DDC_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
MODELSIM_LIBS += unimacro_ver
SIM_TOP = rfnoc_block_ddc_tb glbl
SIM_SRCS = \
$(abspath $(IP_BUILD_DIR)/dds_sin_cos_lut_only/sim/dds_sin_cos_lut_only.vhd) \
$(abspath $(IP_BUILD_DIR)/complex_multiplier_dds/sim/complex_multiplier_dds.vhd) \
$(abspath rfnoc_block_ddc_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
