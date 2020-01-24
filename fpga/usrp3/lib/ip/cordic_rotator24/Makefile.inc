#
# Copyright 2016 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CORDIC_ROTATOR24_SRCS = $(IP_BUILD_DIR)/cordic_rotator24/cordic_rotator24.xci

LIB_IP_CORDIC_ROTATOR24_OUTS = $(addprefix $(IP_BUILD_DIR)/cordic_rotator24/, \
cordic_rotator24.xci.out \
synth/cordic_rotator24.vhd \
) 

$(LIB_IP_CORDIC_ROTATOR24_SRCS) $(LIB_IP_CORDIC_ROTATOR24_OUTS) : $(LIB_IP_DIR)/cordic_rotator24/cordic_rotator24.xci
	$(call BUILD_VIVADO_IP,cordic_rotator24,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
