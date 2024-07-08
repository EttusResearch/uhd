#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# We first need to figure out our own path, because this file could be included
# from anywhere.
THIS_IP_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

LIB_IP_CMPLX_MUL_SRCS = $(IP_BUILD_DIR)/cmplx_mul/cmplx_mul.xci

LIB_IP_CMPLX_MUL_OUTS = $(addprefix $(IP_BUILD_DIR)/cmplx_mul/, \
cmplx_mul.xci.out \
synth/cmplx_mul.vhd \
)

.INTERMEDIATE: LIB_IP_CMPLX_MUL_TRGT
$(LIB_IP_CMPLX_MUL_SRCS) $(LIB_IP_CMPLX_MUL_OUTS): LIB_IP_CMPLX_MUL_TRGT
	@:

# This include provides BUILD_VIVADO_IP
include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CMPLX_MUL_TRGT: $(THIS_IP_DIR)/cmplx_mul.xci
	$(call BUILD_VIVADO_IP,cmplx_mul,$(ARCH),$(PART_ID),$(THIS_IP_DIR)/..,$(IP_BUILD_DIR),0)
