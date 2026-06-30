#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
IP_DIR = $(BASE_DIR)/x400/ip
# Reuse the shared x400 build-ip output (contains generated sim wrappers/mif).
BUILD_IP_DIR ?= $(BASE_DIR)/x400/build-ip
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies

include $(BASE_DIR)/../lib/rfnoc/Makefile.srcs
include ../full/Makefile.srcs
include ../common/Makefile.srcs
include ../x420/Makefile.srcs
include ../../ip/fir_dec3_4spc/Makefile.inc
include ../../ip/fir_inp3_2spc/Makefile.inc

DESIGN_SRCS +=  \
$(RFNOC_FRAMEWORK_SRCS) \
$(RF_FULL_SRCS) \
$(RF_COMMON_SRCS) \
$(RF_X420_COMMON_SRCS) \
$(IP_FIR_DEC3_4SPC_SRCS) \
$(IP_FIR_INP3_2SPC_SRCS) \
$(abspath rf_core_400m_x420.sv) \
$(abspath ../common/adc_gearbox_2x4.vhd) \
$(abspath rx_dec3.sv) \
$(abspath fir_dec3_4spc_wrapper.sv) \
$(abspath ../common/dac_gearbox_4x2.v) \
$(abspath ../common/dac_gearbox_12x4.sv) \
$(abspath ../common/dac_gearbox_6x12.vhd) \
$(abspath ../common/dac_gearbox_6x4.sv) \
$(abspath fir_inp3_2spc_wrapper.sv) \
$(abspath tx_inp3.sv) \

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------
# Native ModelSim flow compiles source files directly and does not elaborate
# Vivado .xci IP. Use generated simulation wrappers from build-ip.
MODELSIM_IP_SRCS = $(wildcard $(abspath \
$(IP_BUILD_DIR)/fir_dec3_4spc/sim/fir_dec3*.vhd \
$(IP_BUILD_DIR)/fir_inp3_2spc/sim/fir_inp3*.vhd \
$(VIVADO_PATH)/data/verilog/src/glbl.v \
))

MODELSIM_IP_DATA_SRCS = $(wildcard $(abspath \
$(IP_BUILD_DIR)/fir_dec3_4spc/fir_dec3*.mif \
$(IP_BUILD_DIR)/fir_inp3_2spc/fir_inp3*.mif \
))

MODELSIM_LIBS += secureip unisims_ver unimacro_ver xilinx_vip xpm fir_compiler_v7_2_76780
MODELSIM_ARGS += glbl -t 1fs

msim modelsim vlint : SIM_SRCS += $(MODELSIM_IP_SRCS)
msim modelsim vlint : modelsim_ip_data_prep

.PHONY: modelsim_ip_data_prep
modelsim_ip_data_prep: $(MODELSIM_IP_DATA_SRCS)
	@mkdir -p $(MODELSIM_PROJ_DIR)
	@if [ -n "$^" ]; then \
		cp -f $^ $(MODELSIM_PROJ_DIR)/; \
	else \
		echo "WARN: No FIR .mif files found under $(IP_BUILD_DIR)"; \
	fi

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rf_core_400m_x420_all_tb

SIM_SRCS = \
$(RF_X420_COMMON_TB_SRCS) \
$(abspath rx_dec3_tb.sv) \
$(abspath tx_inp3_tb.sv) \
$(abspath rf_core_400m_x420_tb.sv) \
$(abspath rf_core_400m_x420_all_tb.sv)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak
