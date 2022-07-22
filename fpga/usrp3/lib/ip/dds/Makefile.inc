#
# Copyright 2018 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

LIB_IP_DDS_SRCS = $(IP_BUILD_DIR)/dds/dds.xci

LIB_IP_DDS_OUTS = $(addprefix $(IP_BUILD_DIR)/dds/, \
dds.xci.out \
synth/dds.vhd \
)

.INTERMEDIATE: LIB_IP_DDS_TRGT
$(LIB_IP_DDS_SRCS) $(LIB_IP_DDS_OUTS): LIB_IP_DDS_TRGT
	@:

LIB_IP_DDS_TRGT: $(LIB_IP_DIR)/dds/dds.xci
	$(call BUILD_VIVADO_IP,dds,$(ARCH),$(PART_ID),$(LIB_IP_DIR),$(IP_BUILD_DIR),0)
