#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_COMPLEX_TO_MAGPHASE_INT32_SRCS = $(IP_BUILD_DIR)/complex_to_magphase_int32/complex_to_magphase_int32.xci

LIB_IP_COMPLEX_TO_MAGPHASE_INT32_OUTS = $(addprefix $(IP_BUILD_DIR)/complex_to_magphase_int32/, \
complex_to_magphase_int32.xci.out \
synth/complex_to_magphase_int32.vhd \
) 

$(LIB_IP_COMPLEX_TO_MAGPHASE_INT32_SRCS) $(LIB_IP_COMPLEX_TO_MAGPHASE_INT32_OUTS) : $(LIB_IP_DIR)/complex_to_magphase_int32/complex_to_magphase_int32.xci
	$(call BUILD_VIVADO_IP,complex_to_magphase_int32,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
