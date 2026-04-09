#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_FIFO_32BIT_4K_SRCS = $(IP_BUILD_DIR)/fifo_32bit_4k/fifo_32bit_4k.xci

IP_FIFO_32BIT_4K_OUTS = $(addprefix $(IP_BUILD_DIR)/fifo_32bit_4k/, \
fifo_32bit_4k.xci.out \
synth/fifo_32bit_4k.vhd \
)

.INTERMEDIATE: IP_FIFO_32BIT_4K_TRGT
$(IP_FIFO_32BIT_4K_SRCS) $(IP_FIFO_32BIT_4K_OUTS): IP_FIFO_32BIT_4K_TRGT
	@:

IP_FIFO_32BIT_4K_TRGT: $(IP_DIR)/fifo_32bit_4k/fifo_32bit_4k.xci
	$(call BUILD_VIVADO_IP,fifo_32bit_4k,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
