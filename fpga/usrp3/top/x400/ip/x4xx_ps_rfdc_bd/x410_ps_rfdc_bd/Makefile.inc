#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_X4XX_PS_RFDC_ORIG_SRCS = $(addprefix $(IP_DIR)/x4xx_ps_rfdc_bd/x410_ps_rfdc_bd/, \
x410_ps_rfdc_bd.tcl \
)

IP_X4XX_PS_RFDC_HDL_SRCS = $(addprefix $(BASE_DIR)/x400/rf/common/, \
rfdc_info_pkg.sv \
) \
$(addprefix $(BASE_DIR)/x400/rf/x410/, \
x410_rf_reset_controller.vhd \
x410_clock_gates.vhd \
x410_rfdc_memory_content_pkg.sv \
) \
$(addprefix $(BASE_DIR)/x400/rf/common/, \
capture_sysref.v \
rf_nco_reset_wrapper.v \
rf_nco_reset_controller.sv \
rf_reset.vhd \
sync_wrapper.v \
axis_mux.vhd \
gpio_to_axis_mux.vhd \
rfdc_info_memory.sv \
axi_rfdc_info_memory.v \
) \
$(addprefix $(BASE_DIR)/../lib/control/, \
synchronizer.v \
synchronizer_impl.v \
reset_sync.v \
) \
$(addprefix $(BASE_DIR)/../lib/rfnoc/core/, \
ctrlport.vh \
) \
$(addprefix $(BASE_DIR)/x400/regmap/x410/, PkgRFDC_REGS_REGMAP.vhd )

IP_X4XX_PS_RFDC_BDTCL_SRCS = $(addprefix $(IP_BUILD_DIR)/x410_ps_rfdc_bd/, \
x410_ps_rfdc_bd.tcl \
)

IP_X4XX_PS_RFDC_BD_SRCS = $(addprefix $(IP_BUILD_DIR)/x410_ps_rfdc_bd/, \
x410_ps_rfdc_bd/x410_ps_rfdc_bd.bd \
)

BD_X4XX_PS_RFDC_BD_OUTS = $(addprefix $(IP_BUILD_DIR)/x410_ps_rfdc_bd/, \
x410_ps_rfdc_bd.bd.out \
x410_ps_rfdc_bd/x410_ps_rfdc_bd_ooc.xdc \
x410_ps_rfdc_bd/synth/x410_ps_rfdc_bd.v \
)

EMPTY_IP_SRCS =

.INTERMEDIATE: IP_X4XX_PS_RFDC_BD_TRGT
$(IP_X4XX_PS_RFDC_BD_SRCS) $(BD_X4XX_PS_RFDC_BD_OUTS) $(IP_X4XX_PS_RFDC_BDTCL_SRCS): IP_X4XX_PS_RFDC_BD_TRGT
	@:

IP_X4XX_PS_RFDC_BD_TRGT: $(IP_X4XX_PS_RFDC_ORIG_SRCS)
	$(call BUILD_VIVADO_BDTCL,x410_ps_rfdc_bd,$(ARCH),$(PART_ID),$(IP_DIR)/x4xx_ps_rfdc_bd,$(IP_BUILD_DIR),$(LIB_DIR)/vivado_ipi,$(IP_X4XX_PS_RFDC_HDL_SRCS))
