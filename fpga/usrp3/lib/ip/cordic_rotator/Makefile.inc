#
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CORDIC_ROTATOR_SRCS = $(IP_BUILD_DIR)/cordic_rotator/cordic_rotator.xci

LIB_IP_CORDIC_ROTATOR_OUTS = $(addprefix $(IP_BUILD_DIR)/cordic_rotator/, \
cordic_rotator.xci.out \
synth/cordic_rotator.vhd \
)

.INTERMEDIATE: LIB_IP_CORDIC_ROTATOR_TRGT
$(LIB_IP_CORDIC_ROTATOR_SRCS) $(LIB_IP_CORDIC_ROTATOR_OUTS): LIB_IP_CORDIC_ROTATOR_TRGT
	@:

LIB_IP_CORDIC_ROTATOR_TRGT: $(LIB_IP_DIR)/cordic_rotator/cordic_rotator.xci
	$(call BUILD_VIVADO_IP,cordic_rotator,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
