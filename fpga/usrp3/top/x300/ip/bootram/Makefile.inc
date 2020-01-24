#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_BOOTRAM_SRCS = $(IP_BUILD_DIR)/bootram/bootram.xci

IP_BOOTRAM_OUTS = $(addprefix $(IP_BUILD_DIR)/bootram/, \
bootram.xci.out \
synth/bootram.vhd \
) 

$(IP_BOOTRAM_SRCS) $(IP_BOOTRAM_OUTS) : $(IP_DIR)/bootram/bootram.xci $(IP_DIR)/bootram/bootram.coe
	$(call BUILD_VIVADO_IP,bootram,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
