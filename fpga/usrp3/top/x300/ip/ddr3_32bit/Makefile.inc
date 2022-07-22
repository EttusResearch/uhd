#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_DDR3_32BIT_SRCS = \
$(IP_BUILD_DIR)/ddr3_32bit/ddr3_32bit.xci \
$(IP_BUILD_DIR)/ddr3_32bit/ddr3_32bit/user_design/rtl/clocking/mig_7series_v4_2_tempmon.v

IP_DDR3_32BIT_OUTS = $(addprefix $(IP_BUILD_DIR)/ddr3_32bit/, \
ddr3_32bit.xci.out \
ddr3_32bit/user_design/rtl/ddr3_32bit.v \
ddr3_32bit/user_design/rtl/ddr3_32bit_mig.v \
)

IP_DDR3_32BIT_SIM_OUTS = $(addprefix $(IP_BUILD_DIR)/ddr3_32bit/, \
ddr3_32bit/example_design/sim/ddr3_model.sv \
ddr3_32bit/example_design/sim/ddr3_model_parameters.vh \
)

.INTERMEDIATE: IP_DDR3_32BIT_TRGT
$(IP_DDR3_32BIT_SRCS) $(IP_DDR3_32BIT_OUTS): IP_DDR3_32BIT_TRGT
	@:

IP_DDR3_32BIT_TRGT: $(IP_DIR)/ddr3_32bit/ddr3_32bit.xci $(IP_DIR)/ddr3_32bit/mig_*.prj
	cp -f $(IP_DIR)/ddr3_32bit/mig_$(subst /,,$(PART_ID)).prj $(IP_DIR)/ddr3_32bit/mig_a.prj # Note: This won't allow parallel IP builds
	$(call BUILD_VIVADO_IP,ddr3_32bit,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
	rm -f $(IP_DIR)/ddr3_32bit/mig_a.prj
