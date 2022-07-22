#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_CORDIC_ROTATE_INT24_SRCS = $(IP_BUILD_DIR)/cordic_rotate_int24/cordic_rotate_int24.xci

LIB_IP_CORDIC_ROTATE_INT24_OUTS = $(addprefix $(IP_BUILD_DIR)/cordic_rotate_int24/, \
cordic_rotate_int24.xci.out \
synth/cordic_rotate_int24.vhd \
)

.INTERMEDIATE: LIB_IP_CORDIC_ROTATE_INT24_TRGT
$(LIB_IP_CORDIC_ROTATE_INT24_SRCS) $(LIB_IP_CORDIC_ROTATE_INT24_OUTS): LIB_IP_CORDIC_ROTATE_INT24_TRGT
	@:

LIB_IP_CORDIC_ROTATE_INT24_TRGT: $(LIB_IP_DIR)/cordic_rotate_int24/cordic_rotate_int24.xci
	$(call BUILD_VIVADO_IP,cordic_rotate_int24,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
