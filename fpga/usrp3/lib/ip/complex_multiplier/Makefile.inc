#
# Copyright 2015 Ettus Research
# Copyright 2016 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_COMPLEX_MULTIPLIER_SRCS = $(IP_BUILD_DIR)/complex_multiplier/complex_multiplier.xci

LIB_IP_COMPLEX_MULTIPLIER_OUTS = $(addprefix $(IP_BUILD_DIR)/complex_multiplier/, \
complex_multiplier.xci.out \
synth/complex_multiplier.vhd \
)

.INTERMEDIATE: LIB_IP_COMPLEX_MULTIPLIER_TRGT
$(LIB_IP_COMPLEX_MULTIPLIER_SRCS) $(LIB_IP_COMPLEX_MULTIPLIER_OUTS): LIB_IP_COMPLEX_MULTIPLIER_TRGT
	@:

LIB_IP_COMPLEX_MULTIPLIER_TRGT: $(LIB_IP_DIR)/complex_multiplier/complex_multiplier.xci
	$(call BUILD_VIVADO_IP,complex_multiplier,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
