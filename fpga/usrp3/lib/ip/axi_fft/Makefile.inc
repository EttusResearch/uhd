#
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_AXI_FFT_SRCS = $(IP_BUILD_DIR)/axi_fft/axi_fft.xci

LIB_IP_AXI_FFT_OUTS = $(addprefix $(IP_BUILD_DIR)/axi_fft/, \
axi_fft.xci.out \
synth/axi_fft.vhd \
)

.INTERMEDIATE: LIB_IP_AXI_FFT_TRGT
$(LIB_IP_AXI_FFT_SRCS) $(LIB_IP_AXI_FFT_OUTS): LIB_IP_AXI_FFT_TRGT
	@:

LIB_IP_AXI_FFT_TRGT: $(LIB_IP_DIR)/axi_fft/axi_fft.xci
	$(call BUILD_VIVADO_IP,axi_fft,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
