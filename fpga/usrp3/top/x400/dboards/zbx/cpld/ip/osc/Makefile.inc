#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/quartus_ip_builder.mak

IP_OSC_SRCS = \
$(IP_BUILD_DIR)/osc/osc.qsys

IP_OSC_OUTS = \
$(IP_BUILD_DIR)/osc/osc.sopcinfo

$(IP_OSC_SRCS) $(IP_OSC_OUTS) : $(IP_DIR)/osc/osc.qsys
	$(call BUILD_QUARTUS_IP,osc,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR))
