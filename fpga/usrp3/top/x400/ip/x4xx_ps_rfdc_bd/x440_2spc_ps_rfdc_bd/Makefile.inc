#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

IP_X4XX_2SPC_PS_RFDC_ORIG_SRCS = \
$(IP_BUILD_DIR)/x440_2spc_ps_rfdc_bd_base/x440_2spc_ps_rfdc_bd_base.tcl \
$(IP_DIR)/x4xx_ps_rfdc_bd/x440_2spc_ps_rfdc_bd/x440_2spc_ps_rfdc_bd.tcl

IP_X4XX_2SPC_PS_RFDC_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/x440_2spc_ps_rfdc_bd/, \
x440_2spc_ps_rfdc_bd.tcl \
)

IP_X4XX_2SPC_PS_RFDC_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/x440_2spc_ps_rfdc_bd/, \
x440_2spc_ps_rfdc_bd/x440_2spc_ps_rfdc_bd.bd \
)

BD_X4XX_2SPC_PS_RFDC_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/x440_2spc_ps_rfdc_bd/, \
x440_2spc_ps_rfdc_bd.bd.out \
x440_2spc_ps_rfdc_bd/x440_2spc_ps_rfdc_bd_ooc.xdc \
x440_2spc_ps_rfdc_bd/synth/x440_2spc_ps_rfdc_bd.v \
)

.INTERMEDIATE: IP_X4XX_2SPC_PS_RFDC_BD_TRGT
$(IP_X4XX_2SPC_PS_RFDC_BD_SRCS) $(BD_X4XX_2SPC_PS_RFDC_BD_OUTS) $(IP_X4XX_2SPC_PS_RFDC_BDTCL_SRCS): IP_X4XX_2SPC_PS_RFDC_BD_TRGT
	@:

IP_X4XX_2SPC_PS_RFDC_BD_TRGT: $(IP_X4XX_2SPC_PS_RFDC_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,x440_2spc_ps_rfdc_bd,$(ARCH),$(PART_ID),$(IP_DIR)/x4xx_ps_rfdc_bd,$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi,$(IP_X4XX_PS_RFDC_HDL_SRCS))

# derive the base diagram by adding "2spc" to all strings
# This ensures that generated clock domain naming like "x440_2spc_ps_rfdc_bd_pll_ref_clk_in" still
# works fine after renaming.
$(IP_BUILD_DIR)/x440_2spc_ps_rfdc_bd_base/x440_2spc_ps_rfdc_bd_base.tcl: $(IP_DIR)/x4xx_ps_rfdc_bd/x440_ps_rfdc_bd/x440_ps_rfdc_bd.tcl
	mkdir -p $(dir $@)
	sed 's/x440_ps_rfdc_bd/x440_2spc_ps_rfdc_bd/g' $< > $@
