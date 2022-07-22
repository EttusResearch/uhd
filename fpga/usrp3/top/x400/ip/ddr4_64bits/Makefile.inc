#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_DDR4_64BITS_SRCS = \
$(IP_BUILD_DIR)/ddr4_64bits/ddr4_64bits.xci

IP_DDR4_64BITS_OUTS = $(addprefix $(IP_BUILD_DIR)/ddr4_64bits/, \
ddr4_64bits.xci.out \
)

.INTERMEDIATE: IP_DDR4_64BITS_TRGT
$(IP_DDR4_64BITS_SRCS) $(IP_DDR4_64BITS_OUTS): IP_DDR4_64BITS_TRGT
	@:

IP_DDR4_64BITS_TRGT: $(IP_DIR)/ddr4_64bits/ddr4_64bits.xci
	$(call BUILD_VIVADO_IP,ddr4_64bits,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
