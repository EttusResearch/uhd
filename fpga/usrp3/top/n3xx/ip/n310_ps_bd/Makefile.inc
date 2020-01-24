#
# Copyright 2016 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak
include $(LIB_DIR)/rfnoc/utils/Makefile.srcs

IP_N310_PS_ORIG_SRCS = $(addprefix $(IP_DIR)/n310_ps_bd/, \
n310_ps_bd.tcl \
chdr_dma_rx.tcl \
chdr_dma_tx.tcl \
chdr_dma_frame_size.tcl \
chdr_dma_top.tcl \
)

IP_N310_PS_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/n310_ps_bd/, \
n310_ps_bd.tcl \
chdr_dma_rx.tcl \
chdr_dma_tx.tcl \
chdr_dma_frame_size.tcl \
chdr_dma_top.tcl \
)

IP_N310_PS_HDL_SRCS = $(RFNOC_UTIL_SRCS)

IP_N310_PS_BD_SRCS = $(IP_BUILD_DIR)/n310_ps_bd/n310_ps_bd/n310_ps_bd.bd

BD_N310_PS_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/n310_ps_bd/, \
n310_ps_bd.bd.out \
n310_ps_bd/n310_ps_bd_ooc.xdc \
)

$(IP_N310_PS_BD_SRCS) $(BD_N310_PS_BD_OUTS) $(IP_N310_PS_BDTCL_SRCS): $(IP_N310_PS_ORIG_SRCS) $(IP_N310_PS_HDL_SRCS)
	$(call BUILD_VIVADO_BDTCL,n310_ps_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi,$(IP_N310_PS_HDL_SRCS))

