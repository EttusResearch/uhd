#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CMPLX_MUL_SRCS = $(IP_BUILD_DIR)/cmplx_mul/cmplx_mul.xci

LIB_IP_CMPLX_MUL_OUTS = $(addprefix $(IP_BUILD_DIR)/cmplx_mul/, \
cmplx_mul.xci.out \
synth/cmplx_mul.vhd \
)

.INTERMEDIATE: LIB_IP_CMPLX_MUL_TRGT
$(LIB_IP_CMPLX_MUL_SRCS) $(LIB_IP_CMPLX_MUL_OUTS): LIB_IP_CMPLX_MUL_TRGT
	@:

LIB_IP_CMPLX_MUL_TRGT: $(OOT_FPGA_DIR)/ip/cmplx_mul/cmplx_mul.xci
	$(call BUILD_VIVADO_IP,cmplx_mul,$(ARCH),$(PART_ID),$(OOT_FPGA_DIR)/ip,$(IP_BUILD_DIR),0)
