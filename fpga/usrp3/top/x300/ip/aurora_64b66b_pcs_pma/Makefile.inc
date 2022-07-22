#
# Copyright 2016 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_AURORA_64B66B_PCS_PMA_EXAMPLE_SRCS = $(addprefix $(IP_BUILD_DIR)/aurora_64b66b_pcs_pma_ex/, \
aurora_64b66b_pcs_pma_ex.srcs/shared_logic/aurora_64b66b_pcs_pma_clock_module.v \
aurora_64b66b_pcs_pma_ex.srcs/shared_logic/aurora_64b66b_pcs_pma_gt_common_wrapper.v \
aurora_64b66b_pcs_pma_ex.srcs/shared_logic/aurora_64b66b_pcs_pma_support_reset_logic.v \
aurora_64b66b_pcs_pma_ex.srcs/shared_logic/aurora_64b66b_pcs_pma_support.v \
imports/aurora_64b66b_pcs_pma_cdc_sync_exdes.v \
imports/aurora_64b66b_pcs_pma_example_axi_to_ll.v \
imports/aurora_64b66b_pcs_pma_example_ll_to_axi.v \
imports/aurora_64b66b_pcs_pma_exdes.v \
imports/aurora_64b66b_pcs_pma_exdes.xdc \
imports/aurora_64b66b_pcs_pma_frame_check.v \
imports/aurora_64b66b_pcs_pma_frame_gen.v \
)

AURORA_PHY_SRCS = \
$(IP_DIR)/aurora_64b66b_pcs_pma/aurora_phy_x1.v \
$(IP_DIR)/aurora_64b66b_pcs_pma/aurora_phy_clk_gen.v \
$(IP_DIR)/aurora_64b66b_pcs_pma/aurora_axis_mac.v \
$(IP_AURORA_64B66B_PCS_PMA_EXAMPLE_SRCS)

IP_AURORA_64B66B_PCS_PMA_SRCS = $(IP_BUILD_DIR)/aurora_64b66b_pcs_pma/aurora_64b66b_pcs_pma.xci

IP_AURORA_64B66B_PCS_PMA_OUTS = $(addprefix $(IP_BUILD_DIR)/aurora_64b66b_pcs_pma/, \
aurora_64b66b_pcs_pma.xci.out \
)

.INTERMEDIATE: IP_AURORA_64B66B_PCS_PMA_TRGT
$(IP_AURORA_64B66B_PCS_PMA_SRCS) $(IP_AURORA_64B66B_PCS_PMA_OUTS) $(IP_AURORA_64B66B_PCS_PMA_EXAMPLE_SRCS): IP_AURORA_64B66B_PCS_PMA_TRGT
	@:

IP_AURORA_64B66B_PCS_PMA_TRGT: $(IP_DIR)/aurora_64b66b_pcs_pma/aurora_64b66b_pcs_pma.xci
	$(call BUILD_VIVADO_IP,aurora_64b66b_pcs_pma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),1)
