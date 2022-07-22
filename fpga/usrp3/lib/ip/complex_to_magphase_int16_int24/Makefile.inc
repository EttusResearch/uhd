#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_SRCS = $(IP_BUILD_DIR)/complex_to_magphase_int16_int24/complex_to_magphase_int16_int24.xci

LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_OUTS = $(addprefix $(IP_BUILD_DIR)/complex_to_magphase_int16_int24/, \
complex_to_magphase_int16_int24.xci.out \
synth/complex_to_magphase_int16_int24.vhd \
)

.INTERMEDIATE: LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_TRGT
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_SRCS) $(LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_OUTS): LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_TRGT
	@:

LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_TRGT: $(LIB_IP_DIR)/complex_to_magphase_int16_int24/complex_to_magphase_int16_int24.xci
	$(call BUILD_VIVADO_IP,complex_to_magphase_int16_int24,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
