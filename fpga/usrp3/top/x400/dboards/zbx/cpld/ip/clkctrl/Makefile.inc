#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/quartus_ip_builder.mak

IP_CLKCTRL_SRCS = \
$(IP_BUILD_DIR)/clkctrl/clkctrl.qsys

IP_CLKCTRL_OUTS = \
$(IP_BUILD_DIR)/clkctrl/clkctrl.sopcinfo

$(IP_CLKCTRL_SRCS) $(IP_CLKCTRL_OUTS) : $(IP_DIR)/clkctrl/clkctrl.qsys
	$(call BUILD_QUARTUS_IP,clkctrl,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR))
