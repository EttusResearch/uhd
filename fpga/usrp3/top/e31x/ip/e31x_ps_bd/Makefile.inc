#
# Copyright 2018 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak
include $(LIB_DIR)/rfnoc/utils/Makefile.srcs

IP_E31X_PS_ORIG_SRCS = $(addprefix $(IP_DIR)/e31x_ps_bd/, \
e31x_ps_bd.tcl \
chdr_dma_rx.tcl \
chdr_dma_tx.tcl \
chdr_dma_frame_size.tcl \
chdr_dma_top.tcl \
)

IP_E31X_PS_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/e31x_ps_bd/, \
e31x_ps_bd.tcl \
chdr_dma_rx.tcl \
chdr_dma_tx.tcl \
chdr_dma_frame_size.tcl \
chdr_dma_top.tcl \
)

IP_E31X_PS_HDL_SRCS = $(RFNOC_UTIL_SRCS)

IP_E31X_PS_BD_SRCS = $(IP_BUILD_DIR)/e31x_ps_bd/e31x_ps_bd/e31x_ps_bd.bd

BD_E31X_PS_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/e31x_ps_bd/, \
e31x_ps_bd.bd.out \
e31x_ps_bd/e31x_ps_bd_ooc.xdc \
)

$(IP_E31X_PS_BD_SRCS) $(BD_E31X_PS_BD_OUTS) $(IP_E31X_PS_BDTCL_SRCS): $(IP_E31X_PS_ORIG_SRCS) $(IP_E31X_PS_HDL_SRCS)
	$(call BUILD_VIVADO_BDTCL,e31x_ps_bd,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi,$(IP_E31X_PS_HDL_SRCS))

