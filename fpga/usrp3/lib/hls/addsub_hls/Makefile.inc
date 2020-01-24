#
# Copyright 2015-2017 Ettus Research
# Copyright 2016 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# Add C/C++/tcl files relative to usrp3/lib/hls/<ip> directory
HLS_IP_ADDSUB_HLS_SRCS = \
addsub_hls.cpp \
addsub_hls.tcl

HLS_IP_ADDSUB_HLS_OUTS = $(addprefix $(IP_BUILD_DIR)/addsub_hls/, \
solution/impl/verilog/addsub_hls.v \
)

# Sources in lib directory
HLS_IP_ADDSUB_HLS_LIB_SRCS = $(addprefix $(HLS_IP_DIR)/addsub_hls/, $(HLS_IP_ADDSUB_HLS_SRCS))

# Build with HLS
$(HLS_IP_ADDSUB_HLS_OUTS) : $(HLS_IP_ADDSUB_HLS_LIB_SRCS)
	$(call BUILD_VIVADO_HLS_IP,addsub_hls,$(PART_ID),$(HLS_IP_ADDSUB_HLS_LIB_SRCS),$(HLS_IP_DIR),$(IP_BUILD_DIR),)
