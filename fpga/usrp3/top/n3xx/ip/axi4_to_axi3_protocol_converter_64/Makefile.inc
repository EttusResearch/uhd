#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI4_TO_AXI3_PROTOCOL_CONVERTER_64_SRCS = $(IP_BUILD_DIR)/axi4_to_axi3_protocol_converter_64/axi4_to_axi3_protocol_converter_64.xci

IP_AXI4_TO_AXI3_PROTOCOL_CONVERTER_64_OUTS = $(addprefix $(IP_BUILD_DIR)/axi4_to_axi3_protocol_converter_64/, \
axi4_to_axi3_protocol_converter_64.xci.out \
synth/axi4_to_axi3_protocol_converter_64.v \
)

$(IP_AXI4_TO_AXI3_PROTOCOL_CONVERTER_64_SRCS) $(IP_AXI4_TO_AXI3_PROTOCOL_CONVERTER_64_OUTS) : $(IP_DIR)/axi4_to_axi3_protocol_converter_64/axi4_to_axi3_protocol_converter_64.xci
	$(call BUILD_VIVADO_IP,axi4_to_axi3_protocol_converter_64,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
