#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_INPUT_SAMPLE_FIFO_SRCS = $(IP_BUILD_DIR)/input_sample_fifo/input_sample_fifo.xci

IP_INPUT_SAMPLE_FIFO_OUTS = $(addprefix $(IP_BUILD_DIR)/input_sample_fifo/, \
nput_sample_fifo.xci.out \
synth/input_sample_fifo.vhd \
) 

$(IP_INPUT_SAMPLE_FIFO_SRCS) $(IP_INPUT_SAMPLE_FIFO_OUTS) : $(IP_DIR)/input_sample_fifo/input_sample_fifo.xci
	$(call BUILD_VIVADO_IP,input_sample_fifo,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
