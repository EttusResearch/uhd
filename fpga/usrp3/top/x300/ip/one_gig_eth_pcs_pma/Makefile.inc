#
# Copyright 2008-2013 Ettus Research LLC
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

ONE_GIGE_PHY_SRCS = \
$(IP_DIR)/one_gig_eth_pcs_pma/one_gige_phy_clk_gen.v \
$(IP_DIR)/one_gig_eth_pcs_pma/one_gige_phy.v \
$(IP_ONE_GIG_ETH_PCS_PMA_EXAMPLE_SRCS)

IP_ONE_GIG_ETH_PCS_PMA_EXAMPLE_SRCS = $(addprefix $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/, \
one_gig_eth_pcs_pma_example_design.v \
one_gig_eth_pcs_pma_reset_sync_ex.v \
one_gig_eth_pcs_pma_sync_block_ex.v \
one_gig_eth_pcs_pma_tx_elastic_buffer.v \
one_gig_eth_pcs_pma_clocking.v \
one_gig_eth_pcs_pma_gt_common.v \
one_gig_eth_pcs_pma_resets.v \
one_gig_eth_pcs_pma_support.v \
)

IP_ONE_GIG_ETH_PCS_PMA_SRCS = $(IP_BUILD_DIR)/one_gig_eth_pcs_pma/one_gig_eth_pcs_pma.xci

IP_ONE_GIG_ETH_PCS_PMA_OUTS = $(addprefix $(IP_BUILD_DIR)/one_gig_eth_pcs_pma/, \
one_gig_eth_pcs_pma.xci.out \
synth/one_gig_eth_pcs_pma_block.v \
synth/one_gig_eth_pcs_pma_reset_sync.v \
synth/one_gig_eth_pcs_pma.v \
synth/one_gig_eth_pcs_pma_ooc.xdc \
synth/one_gig_eth_pcs_pma_sync_block.v \
synth/one_gig_eth_pcs_pma.xdc \
synth/transceiver/one_gig_eth_pcs_pma_gtwizard_gt.v \
synth/transceiver/one_gig_eth_pcs_pma_gtwizard.v \
synth/transceiver/one_gig_eth_pcs_pma_transceiver.v \
synth/transceiver/one_gig_eth_pcs_pma_gtwizard_init.v \
synth/transceiver/one_gig_eth_pcs_pma_reset_wtd_timer.v \
synth/transceiver/one_gig_eth_pcs_pma_tx_startup_fsm.v \
synth/transceiver/one_gig_eth_pcs_pma_gtwizard_multi_gt.v \
synth/transceiver/one_gig_eth_pcs_pma_rx_startup_fsm.v \
)

$(IP_ONE_GIG_ETH_PCS_PMA_EXAMPLE_SRCS) : $(IP_ONE_GIG_ETH_PCS_PMA_OUTS)

$(IP_ONE_GIG_ETH_PCS_PMA_SRCS) $(IP_ONE_GIG_ETH_PCS_PMA_OUTS) : $(IP_DIR)/one_gig_eth_pcs_pma/one_gig_eth_pcs_pma.xci
	$(call BUILD_VIVADO_IP,one_gig_eth_pcs_pma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),1)
	cp $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_clocking.v $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_clocking.v.orig
	patch $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_clocking.v $(IP_DIR)/one_gig_eth_pcs_pma/one_gig_eth_pcs_pma_clocking.v.patch
	cp $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_support.v $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_support.v.orig
	patch $(IP_BUILD_DIR)/one_gig_eth_pcs_pma_ex/imports/one_gig_eth_pcs_pma_support.v $(IP_DIR)/one_gig_eth_pcs_pma/one_gig_eth_pcs_pma_support.v.patch
