#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AXI4_BRAM_SRCS = $(IP_BUILD_DIR)/axi4_dualport_sram/axi4_dualport_sram.xci

IP_AXI4_BRAM_OUTS = $(addprefix $(IP_BUILD_DIR)/axi4_dualport_sram/, \
axi4_dualport_sram.xci.out \
synth/axi4_dualport_sram.vhd \
) 

$(IP_AXI4_BRAM_SRCS) $(IP_AXI4_BRAM_OUTS) : $(IP_DIR)/axi4_dualport_sram/axi4_dualport_sram.xci
	$(call BUILD_VIVADO_IP,axi4_dualport_sram,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
