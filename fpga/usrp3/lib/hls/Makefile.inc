#
# Copyright 2015-2017 Ettus Research
#
include $(TOOLS_DIR)/make/viv_hls_ip_builder.mak

# Add HLS IP to build
include $(HLS_IP_DIR)/addsub_hls/Makefile.inc

LIB_HLS_IP_SRCS = \
$(HLS_IP_ADDSUB_HLS_OUTS)
