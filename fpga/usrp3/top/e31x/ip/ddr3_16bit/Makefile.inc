#
# Copyright 2022 Ettus Research, a National Instruments Brand
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_DDR3_16BIT_SRCS = $(IP_BUILD_DIR)/ddr3_16bit/ddr3_16bit.xci

IP_DDR3_16BIT_OUTS = $(addprefix $(IP_BUILD_DIR)/ddr3_16bit/, \
ddr3_16bit.xci.out \
ddr3_16bit/user_design/rtl/ddr3_16bit.v \
ddr3_16bit/user_design/rtl/ddr3_16bit_mig.v \
)

IP_DDR3_16BIT_TG_SRCS = $(addprefix $(IP_BUILD_DIR)/ddr3_16bit/, \
ddr3_16bit/example_design/rtl/example_top.v \
ddr3_16bit/example_design/rtl/traffic_gen/mig_7series_v4_2_axi4_tg.v \
ddr3_16bit/example_design/rtl/traffic_gen/mig_7series_v4_2_axi4_wrapper.v \
ddr3_16bit/example_design/rtl/traffic_gen/mig_7series_v4_2_cmd_prbs_gen_axi.v  \
ddr3_16bit/example_design/rtl/traffic_gen/mig_7series_v4_2_data_gen_chk.v \
ddr3_16bit/example_design/rtl/traffic_gen/mig_7series_v4_2_tg.v \
)

IP_DDR3_16BIT_SIM_OUTS = $(addprefix $(IP_BUILD_DIR)/ddr3_16bit/, \
ddr3_16bit/example_design/sim/ddr3_model.sv \
ddr3_16bit/example_design/sim/ddr3_model_parameters.vh \
)

$(IP_DDR3_16BIT_SRCS) $(IP_DDR3_16BIT_OUTS) : $(IP_DIR)/ddr3_16bit/ddr3_16bit.xci $(IP_DIR)/ddr3_16bit/mig_*.prj
	cp -f $(IP_DIR)/ddr3_16bit/mig_$(subst /,,$(PART_ID)).prj $(IP_DIR)/ddr3_16bit/mig_a.prj # Note: This won't allow parallel IP builds
	$(call BUILD_VIVADO_IP,ddr3_16bit,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
	rm -f $(IP_DIR)/ddr3_16bit/mig_a.prj
