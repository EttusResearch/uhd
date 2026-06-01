#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIR_INP3_1SPC_SRCS = \
$(IP_BUILD_DIR)/fir_inp3_1spc/fir_inp3_1spc.xci

IP_FIR_INP3_1SPC_OUTS = $(addprefix $(IP_BUILD_DIR)/fir_inp3_1spc/, \
fir_inp3_1spc.xci.out \
)

.INTERMEDIATE: IP_FIR_INP3_1SPC_TRGT
$(IP_FIR_INP3_1SPC_SRCS) $(IP_FIR_INP3_1SPC_OUTS): IP_FIR_INP3_1SPC_TRGT
	@:

IP_FIR_INP3_1SPC_TRGT: $(IP_DIR)/fir_inp3_1spc/fir_inp3_1spc.xci
	$(call BUILD_VIVADO_IP,fir_inp3_1spc,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
