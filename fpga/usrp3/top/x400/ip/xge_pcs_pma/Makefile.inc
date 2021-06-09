#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

XGE_PCS_PMA_SRCS = \
$(IP_DIR)/xge_pcs_pma/ten_gige_phy.v \
$(IP_DIR)/xge_pcs_pma/eth_10g.sv \
$(IP_XGE_PCS_PMA_EXAMPLE_SRCS)

IP_XGE_PCS_PMA_HDL_SIM_SRCS = $(IP_DIR)/xge_pcs_pma/model_10gbe.sv \
$(wildcard $(addprefix $(IP_BUILD_DIR)/xge_pcs_pma/, \
xge_pcs_pma.v \
hdl/xxv_ethernet_v3_0_vl_rfs.sv \
ip_0/hdl/*.v \
ip_0/sim/*.v \
xxv_ethernet_v3_0_1/*.v \
))

IP_XGE_PCS_PMA_EXAMPLE_SRCS = \
$(IP_BUILD_DIR)/xge_pcs_pma_ex/imports/xge_pcs_pma_common_wrapper.v \
$(IP_BUILD_DIR)/xge_pcs_pma_ex/imports/xge_pcs_pma_gt_gtye4_common_wrapper.v \
$(IP_BUILD_DIR)/xge_pcs_pma_ex/imports/gtwizard_ultrascale_v1_7_gtye4_common.v \

# Describe the paths for the patch file, the file to be patched, and the
# patched copy of the file.
IP_XGE_FILE_PATCH = $(IP_DIR)/xge_pcs_pma/xge_pcs_pma_wrapper.v.patch
IP_XGE_FILE_TO_PATCH = $(IP_BUILD_DIR)/xge_pcs_pma/xxv_ethernet_v3_0_1/xge_pcs_pma_wrapper.v
IP_XGE_PATCHED_FILE = $(IP_BUILD_DIR)/xge_pcs_pma_wrapper.v.patched

IP_XGE_PCS_PMA_SRCS = $(IP_BUILD_DIR)/xge_pcs_pma/xge_pcs_pma.xci $(IP_XGE_PATCHED_FILE)

IP_XGE_PCS_PMA_OUTS = \
$(IP_BUILD_DIR)/xge_pcs_pma/xge_pcs_pma.xci.out \
$(IP_BUILD_DIR)/xge_pcs_pma/xge_pcs_pma.v \

$(IP_XGE_PCS_PMA_EXAMPLE_SRCS) : $(IP_XGE_PCS_PMA_OUTS) 

$(IP_XGE_PCS_PMA_SRCS) $(IP_XGE_PCS_PMA_OUTS) : $(IP_DIR)/xge_pcs_pma/xge_pcs_pma.xci $(IP_XGE_FILE_PATCH)
	$(call BUILD_VIVADO_IP,xge_pcs_pma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),1)
	cp $(IP_XGE_FILE_TO_PATCH) $(IP_XGE_FILE_TO_PATCH).orig
	cp $(IP_XGE_FILE_TO_PATCH) $(IP_XGE_PATCHED_FILE)
	patch $(IP_XGE_PATCHED_FILE) $(IP_XGE_FILE_PATCH)
	$(call REBUILD_VIVADO_IP_WITH_PATCH,xge_pcs_pma,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0,$(call RESOLVE_PATH,$(IP_XGE_PATCHED_FILE)),$(call RESOLVE_PATH,$(IP_XGE_FILE_TO_PATCH)))
