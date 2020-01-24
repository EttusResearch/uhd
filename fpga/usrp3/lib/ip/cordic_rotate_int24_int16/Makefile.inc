#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CORDIC_ROTATE_INT24_INT16_SRCS = $(IP_BUILD_DIR)/cordic_rotate_int24_int16/cordic_rotate_int24_int16.xci

LIB_IP_CORDIC_ROTATE_INT24_INT16_OUTS = $(addprefix $(IP_BUILD_DIR)/cordic_rotate_int24_int16/, \
cordic_rotate_int24_int16.xci.out \
synth/cordic_rotate_int24_int16.vhd \
) 

$(LIB_IP_CORDIC_ROTATE_INT24_INT16_SRCS) $(LIB_IP_CORDIC_ROTATE_INT24_INT16_OUTS) : $(LIB_IP_DIR)/cordic_rotate_int24_int16/cordic_rotate_int24_int16.xci
	$(call BUILD_VIVADO_IP,cordic_rotate_int24_int16,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
