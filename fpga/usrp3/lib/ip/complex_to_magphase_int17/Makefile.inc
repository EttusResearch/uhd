#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_COMPLEX_TO_MAGPHASE_INT17_SRCS = $(IP_BUILD_DIR)/complex_to_magphase_int17/complex_to_magphase_int17.xci

LIB_IP_COMPLEX_TO_MAGPHASE_INT17_OUTS = $(addprefix $(IP_BUILD_DIR)/complex_to_magphase_int17/, \
complex_to_magphase_int17.xci.out \
synth/complex_to_magphase_int17.vhd \
)

.INTERMEDIATE: LIB_IP_COMPLEX_TO_MAGPHASE_INT17_TRGT
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT17_SRCS) $(LIB_IP_COMPLEX_TO_MAGPHASE_INT17_OUTS): LIB_IP_COMPLEX_TO_MAGPHASE_INT17_TRGT
	@:

LIB_IP_COMPLEX_TO_MAGPHASE_INT17_TRGT: $(LIB_IP_DIR)/complex_to_magphase_int17/complex_to_magphase_int17.xci
	$(call BUILD_VIVADO_IP,complex_to_magphase_int17,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
