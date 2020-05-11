#
# Copyright 2015 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_MIG_7SERIES_0_SRCS = $(IP_BUILD_DIR)/mig_7series_0/mig_7series_0.xci

IP_MIG_7SERIES_0_OUTS = $(addprefix $(IP_BUILD_DIR)/mig_7series_0/, \
mig_7series_0.xci.out \
mig_7series_0/user_design/rtl/mig_7series_0.v \
mig_7series_0/user_design/rtl/mig_7series_0_mig.v \
)

IP_MIG_7SERIES_TG_SRCS = $(addprefix $(IP_BUILD_DIR)/mig_7series_0/, \
mig_7series_0/example_design/rtl/example_top.v \
mig_7series_0/example_design/rtl/traffic_gen/mig_7series_v4_2_axi4_tg.v \
mig_7series_0/example_design/rtl/traffic_gen/mig_7series_v4_2_axi4_wrapper.v \
mig_7series_0/example_design/rtl/traffic_gen/mig_7series_v4_2_cmd_prbs_gen_axi.v  \
mig_7series_0/example_design/rtl/traffic_gen/mig_7series_v4_2_data_gen_chk.v \
mig_7series_0/example_design/rtl/traffic_gen/mig_7series_v4_2_tg.v \
)

IP_MIG_7SERIES_0_SIM_OUTS = $(addprefix $(IP_BUILD_DIR)/mig_7series_0/, \
mig_7series_0/example_design/sim/ddr3_model.sv \
mig_7series_0/example_design/sim/ddr3_model_parameters.vh \
)

$(IP_MIG_7SERIES_0_SRCS) $(IP_MIG_7SERIES_0_OUTS) : $(IP_DIR)/mig_7series_0/mig_7series_0.xci $(IP_DIR)/mig_7series_0/mig_*.prj
	cp -f $(IP_DIR)/mig_7series_0/mig_$(subst /,,$(PART_ID)).prj $(IP_DIR)/mig_7series_0/mig_a.prj # Note: This won't allow parallel IP builds
	$(call BUILD_VIVADO_IP,mig_7series_0,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
	rm -f $(IP_DIR)/mig_7series_0/mig_a.prj
