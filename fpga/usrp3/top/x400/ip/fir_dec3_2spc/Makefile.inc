#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIR_DEC3_2SPC_SRCS = \
$(IP_BUILD_DIR)/fir_dec3_2spc/fir_dec3_2spc.xci

IP_FIR_DEC3_2SPC_OUTS = $(addprefix $(IP_BUILD_DIR)/fir_dec3_2spc/, \
fir_dec3_2spc.xci.out \
)

.INTERMEDIATE: IP_FIR_DEC3_2SPC_TRGT
$(IP_FIR_DEC3_2SPC_SRCS) $(IP_FIR_DEC3_2SPC_OUTS): IP_FIR_DEC3_2SPC_TRGT
	@:

IP_FIR_DEC3_2SPC_TRGT: $(IP_DIR)/fir_dec3_2spc/fir_dec3_2spc.xci
	$(call BUILD_VIVADO_IP,fir_dec3_2spc,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
