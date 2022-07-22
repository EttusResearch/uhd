#
# Copyright 2017 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXIS_FIFO_TO_AXI4LITE_SRCS = $(IP_BUILD_DIR)/axis_fifo_to_axi4lite/axis_fifo_to_axi4lite.xci

IP_AXIS_FIFO_TO_AXI4LITE_OUTS = $(addprefix $(IP_BUILD_DIR)/axis_fifo_to_axi4lite/, \
axis_fifo_to_axi4lite.xci.out \
)

.INTERMEDIATE: IP_AXIS_FIFO_TO_AXI4LITE_TRGT
$(IP_AXIS_FIFO_TO_AXI4LITE_SRCS) $(IP_AXIS_FIFO_TO_AXI4LITE_OUTS) : IP_AXIS_FIFO_TO_AXI4LITE_TRGT
	@:

IP_AXIS_FIFO_TO_AXI4LITE_TRGT: $(IP_DIR)/axis_fifo_to_axi4lite/axis_fifo_to_axi4lite.xci
	$(call BUILD_VIVADO_IP,axis_fifo_to_axi4lite,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
