#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_XFFT_32K_16B_SRCS = $(IP_BUILD_DIR)/xfft_32k_16b/xfft_32k_16b.xci

LIB_IP_XFFT_32K_16B_OUTS = $(addprefix $(IP_BUILD_DIR)/xfft_32k_16b/, \
xfft_32k_16b.xci.out \
synth/xfft_32k_16b.vhd \
)

.INTERMEDIATE: LIB_IP_XFFT_32K_16B_TRGT
$(LIB_IP_XFFT_32K_16B_SRCS) $(LIB_IP_XFFT_32K_16B_OUTS): LIB_IP_XFFT_32K_16B_TRGT
	@:

LIB_IP_XFFT_32K_16B_TRGT : $(LIB_IP_DIR)/xfft_32k_16b/xfft_32k_16b.xci
	$(call BUILD_VIVADO_IP,xfft_32k_16b,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
