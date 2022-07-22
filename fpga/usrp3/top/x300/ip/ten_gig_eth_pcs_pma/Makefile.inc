#
# Copyright 2008-2013 Ettus Research LLC
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

TEN_GIGE_PHY_SRCS = \
$(IP_DIR)/ten_gig_eth_pcs_pma/ten_gige_phy_clk_gen.v \
$(IP_DIR)/ten_gig_eth_pcs_pma/ten_gige_phy.v \
$(IP_TEN_GIG_ETH_PCS_PMA_EXAMPLE_SRCS)

IP_TEN_GIG_ETH_PCS_PMA_EXAMPLE_SRCS = $(addprefix $(IP_BUILD_DIR)/ten_gig_eth_pcs_pma_ex/imports/, \
ten_gig_eth_pcs_pma_example_design.v \
ten_gig_eth_pcs_pma_ff_synchronizer_rst2.v \
ten_gig_eth_pcs_pma_gt_common.v \
ten_gig_eth_pcs_pma_shared_clock_and_reset.v \
ten_gig_eth_pcs_pma_support.v \
)

IP_TEN_GIG_ETH_PCS_PMA_SRCS = $(IP_BUILD_DIR)/ten_gig_eth_pcs_pma/ten_gig_eth_pcs_pma.xci

IP_TEN_GIG_ETH_PCS_PMA_OUTS = $(addprefix $(IP_BUILD_DIR)/ten_gig_eth_pcs_pma/, \
ten_gig_eth_pcs_pma.xci.out \
synth/ten_gig_eth_pcs_pma_block.v \
synth/ten_gig_eth_pcs_pma_gtwizard_10gbaser_multi_gt.v \
synth/ten_gig_eth_pcs_pma_cable_pull_logic.v \
synth/ten_gig_eth_pcs_pma_local_clock_and_reset.v \
synth/ten_gig_eth_pcs_pma_clocks.xdc \
synth/ten_gig_eth_pcs_pma_ooc.xdc \
synth/ten_gig_eth_pcs_pma_ff_synchronizer_rst.v \
synth/ten_gig_eth_pcs_pma_sim_speedup_controller.v \
synth/ten_gig_eth_pcs_pma_ff_synchronizer.v \
synth/ten_gig_eth_pcs_pma.v \
synth/ten_gig_eth_pcs_pma_gtwizard_10gbaser_gt.v \
synth/ten_gig_eth_pcs_pma.xdc \
)

$(IP_TEN_GIG_ETH_PCS_PMA_EXAMPLE_SRCS) : $(IP_TEN_GIG_ETH_PCS_PMA_OUTS)

.INTERMEDIATE: IP_TEN_GIG_ETH_PCS_PMA_TRGT
$(IP_TEN_GIG_ETH_PCS_PMA_SRCS) $(IP_TEN_GIG_ETH_PCS_PMA_OUTS): IP_TEN_GIG_ETH_PCS_PMA_TRGT
	@:

IP_TEN_GIG_ETH_PCS_PMA_TRGT: $(IP_DIR)/ten_gig_eth_pcs_pma/ten_gig_eth_pcs_pma.xci
	$(call BUILD_VIVADO_IP,ten_gig_eth_pcs_pma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),1)
