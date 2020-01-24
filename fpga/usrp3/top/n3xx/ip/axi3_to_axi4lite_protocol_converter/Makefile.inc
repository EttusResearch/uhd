#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI3_TO_AXI4LITE_PROTOCOL_CONVERTER_SRCS = $(IP_BUILD_DIR)/axi3_to_axi4lite_protocol_converter/axi3_to_axi4lite_protocol_converter.xci

IP_AXI3_TO_AXI4LITE_PROTOCOL_CONVERTER_OUTS = $(addprefix $(IP_BUILD_DIR)/axi3_to_axi4lite_protocol_converter/, \
axi3_to_axi4lite_protocol_converter.xci.out \
synth/axi3_to_axi4lite_protocol_converter.v \
)

$(IP_AXI3_TO_AXI4LITE_PROTOCOL_CONVERTER_SRCS) $(IP_AXI3_TO_AXI4LITE_PROTOCOL_CONVERTER_OUTS) : $(IP_DIR)/axi3_to_axi4lite_protocol_converter/axi3_to_axi4lite_protocol_converter.xci
	$(call BUILD_VIVADO_IP,axi3_to_axi4lite_protocol_converter,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
