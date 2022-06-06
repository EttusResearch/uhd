#
# Copyright 2015-2017 Ettus Research
#

# -------------------------------------------------------------------
# Usage: BUILD_VIVADO_HLS_IP
# Args: $1 = HLS_IP_NAME (High level synthesis IP name)
#       $2 = PART_ID (<device>/<package>/<speedgrade>)
#       $3 = HLS_IP_SRCS (Absolute paths to the HLS IP source files)
#       $4 = HLS_IP_SRC_DIR (Absolute path to the top level HLS IP src dir)
#       $5 = HLS_IP_BUILD_DIR (Absolute path to the top level HLS IP build dir)
#       $6 = HLS_IP_INCLUDES (Absolute path to IP include dir)
# Prereqs:
# - TOOLS_DIR must be defined globally
# -------------------------------------------------------------------

BUILD_VIVADO_HLS_IP = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building HLS IP $(1)"; \
	echo "========================================================"; \
	export HLS_IP_NAME=$(1); \
	export PART_NAME=$(subst /,,$(2)); \
	export HLS_IP_SRCS=$(call RESOLVE_PATHS,$(3)); \
	export HLS_IP_INCLUDES=$(call RESOLVE_PATHS,$(6)); \
	echo "BUILDER: Staging HLS IP in build directory..."; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cp -rf $(4)/$(1)/* $(5)/$(1); \
	cd $(5); \
	echo "BUILDER: Building HLS IP..."; \
	export VIV_ERR=0; \
	vitis_hls -f $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_generate_hls_ip.tcl) -l $(1).log || export VIV_ERR=$$?; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	exit $$(($$VIV_ERR))

