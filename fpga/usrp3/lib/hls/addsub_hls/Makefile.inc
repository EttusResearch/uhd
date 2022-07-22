#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# Add C/C++/tcl files relative to usrp3/lib/hls/<ip> directory
HLS_IP_ADDSUB_HLS_LIB_SRCS = $(addprefix $(HLS_IP_DIR)/addsub_hls/, \
addsub_hls.cpp \
addsub_hls.tcl \
)

# HLS output artifact points to the ip/hdl/verilog folder. The build process
# will glob all the files in this directory, including *.dat files.
HLS_IP_ADDSUB_HLS_OUTS = $(IP_BUILD_DIR)/addsub_hls/solution/impl/ip/hdl/verilog

.INTERMEDIATE: HLS_IP_ADDSUB_HLS_TRGT
$(HLS_IP_ADDSUB_HLS_OUTS): HLS_IP_ADDSUB_HLS_TRGT
	@:

# Build with HLS
HLS_IP_ADDSUB_HLS_TRGT: $(HLS_IP_ADDSUB_HLS_LIB_SRCS)
	$(call BUILD_VIVADO_HLS_IP,addsub_hls,$(PART_ID),$(HLS_IP_ADDSUB_HLS_LIB_SRCS),$(HLS_IP_DIR),$(IP_BUILD_DIR),)
