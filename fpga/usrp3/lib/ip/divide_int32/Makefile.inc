#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DIVIDE_INT32_SRCS = $(IP_BUILD_DIR)/divide_int32/divide_int32.xci

LIB_IP_DIVIDE_INT32_OUTS = $(addprefix $(IP_BUILD_DIR)/divide_int32/, \
divide_int32.xci.out \
synth/divide_int32.vhd \
) 

$(LIB_IP_DIVIDE_INT32_SRCS) $(LIB_IP_DIVIDE_INT32_OUTS) : $(LIB_IP_DIR)/divide_int32/divide_int32.xci
	$(call BUILD_VIVADO_IP,divide_int32,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
