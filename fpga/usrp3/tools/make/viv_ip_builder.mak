#
# Copyright 2014 Ettus Research
#

ifeq ($(SIMULATION),1)
SYNTH_IP=0
else
SYNTH_IP=1
endif

# -------------------------------------------------------------------
# Usage: BUILD_VIVADO_IP
# Args: $1 = IP_NAME (IP name)
#       $2 = ARCH (zynq, kintex7, etc)
#       $3 = PART_ID (<device>/<package>/<speedgrade>[/<tempgrade>[/<silicon revision>]])
#       $4 = IP_SRC_DIR (Absolute path to the top level ip src dir)
#       $5 = IP_BUILD_DIR (Absolute path to the top level ip build dir)
#       $6 = GENERATE_EXAMPLE (0 or 1)
# Prereqs:
# - TOOLS_DIR must be defined globally
# -------------------------------------------------------------------
BUILD_VIVADO_IP = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building IP $(1)"; \
	echo "========================================================"; \
	export XCI_FILE=$(call RESOLVE_PATH,$(5)/$(1)/$(1).xci); \
	export PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(2)/$(3)`; \
	export GEN_EXAMPLE=$(6); \
	export SYNTH_IP=$(SYNTH_IP); \
	echo "BUILDER: Staging IP in build directory..."; \
	rm -rf $(5)/$(1); \
	mkdir -p $(5)/$(1); \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cp -rf $(4)/$(1)/* $(5)/$(1); \
	echo "BUILDER: Retargeting IP to part $(2)/$(3)..."; \
	python3 $(TOOLS_DIR)/scripts/viv_ip_xci_editor.py --output_dir=$(5)/$(1) --target=$(2)/$(3) retarget $(4)/$(1)/$(1).xci; \
	cd $(5); \
	echo "BUILDER: Building IP..."; \
	export VIV_ERR=0; \
	$(TOOLS_DIR)/scripts/launch_vivado.py -mode batch -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_generate_ip.tcl) -log $(1).log -nojournal || export VIV_ERR=$$?; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	exit $$VIV_ERR

# -------------------------------------------------------------------
# Usage: REBUILD_VIVADO_IP_WITH_PATCH
# Args: $1 = IP_NAME (IP name)
#       $2 = ARCH (zynq, kintex7, etc)
#       $3 = PART_ID (<device>/<package>/<speedgrade>[/<tempgrade>[/<silicon revision>]])
#       $4 = IP_SRC_DIR (Absolute path to the top level ip src dir)
#       $5 = IP_BUILD_DIR (Not used here, but kept for consistency with BUILD_VIVADO_IP)
#       $6 = GENERATE_EXAMPLE (0 or 1)
#       $7 = PATCHED_FILE (Patched version of the file from previous IP build)
#       $8 = FILE_TO_PATCH (Path to file that needs to be patched)
# Prereqs: 
# - TOOLS_DIR must be defined globally
# - This assumes BUILD_VIVADO_IP has been run once
# -------------------------------------------------------------------
REBUILD_VIVADO_IP_WITH_PATCH = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building Patched IP $(1)"; \
	echo "========================================================"; \
	export XCI_FILE=$(call RESOLVE_PATH,$(5)/$(1)/$(1).xci); \
	export PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(2)/$(3)`; \
	export GEN_EXAMPLE=$(6); \
	export SYNTH_IP=$(SYNTH_IP); \
	export PATCHED_FILE=$(7); \
	export FILE_TO_PATCH=$(8); \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cd $(5); \
	echo "BUILDER: Building Patched IP..."; \
	export VIV_ERR=0; \
	$(TOOLS_DIR)/scripts/launch_vivado.py -mode batch -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_generate_patch_ip.tcl) -log $(1).log -nojournal || export VIV_ERR=$$?; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	exit $$VIV_ERR

# -------------------------------------------------------------------
# Usage: BUILD_VIVADO_BD
# Args: $1 = BD_NAME (IP name)
#       $2 = ARCH (zynq, kintex7, etc)
#       $3 = PART_ID (<device>/<package>/<speedgrade>[/<tempgrade>[/<silicon revision>]])
#       $4 = BD_SRC_DIR (Absolute path to the top level ip src dir)
#       $5 = BD_BUILD_DIR (Absolute path to the top level ip build dir)
# Prereqs:
# - TOOLS_DIR must be defined globally
# -------------------------------------------------------------------
BUILD_VIVADO_BD = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building BD $(1)"; \
	echo "========================================================"; \
	export BD_FILE=$(call RESOLVE_PATH,$(5)/$(1)/$(1).bd); \
	export PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(2)/$(3)`; \
	echo "BUILDER: Staging BD in build directory..."; \
	rm -rf $(5)/$(1); \
	mkdir -p $(5)/$(1); \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cp -rf $(4)/$(1)/* $(5)/$(1); \
	echo "BUILDER: Retargeting BD to part $(2)/$(3)..."; \
	cd $(5)/$(1); \
	echo "BUILDER: Building BD..."; \
	export VIV_ERR=0; \
	$(TOOLS_DIR)/scripts/launch_vivado.py -mode batch -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_generate_bd.tcl) -log $(1).log -nojournal || export VIV_ERR=$$?; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	exit $$VIV_ERR

# -------------------------------------------------------------------
# Usage: BUILD_VIVADO_BDTCL
# Args: $1 = BD_NAME (IP name)
#       $2 = ARCH (zynq, kintex7, etc)
#       $3 = PART_ID (<device>/<package>/<speedgrade>[/<tempgrade>[/<silicon revision>]])
#       $4 = BDTCL_SRC_DIR (Absolute path to the top level ip src dir)
#       $5 = BDTCL_BUILD_DIR (Absolute path to the top level ip build dir)
#       $6 = BD_IP_REPOS (space-separated list of absolute paths to IP repos)
#       $7 = BD_HDL_SRCS (space-separated list of absolute paths to HDL sources)
# Prereqs:
# - TOOLS_DIR must be defined globally
# -------------------------------------------------------------------
BUILD_VIVADO_BDTCL = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Generating BD from Tcl $(1)"; \
	echo "========================================================"; \
	export BD_FILE=$(call RESOLVE_PATH,$(5)/$(1)/$(1).tcl); \
	export PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(2)/$(3)`; \
	export BD_IP_REPOS=$(call RESOLVE_PATH,$(6)); \
	export BD_HDL_SRCS=$(call RESOLVE_PATHS,$(7)); \
	echo "BUILDER: Staging BD Tcl in build directory..."; \
	rm -rf $(5)/$(1); \
	mkdir -p $(5)/$(1); \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cp -rf $(4)/$(1)/* $(5)/$(1); \
	echo "BUILDER: Retargeting BD to part $(2)/$(3)..."; \
	cd $(5)/$(1); \
	echo "BUILDER: Generating BD..."; \
	export VIV_ERR=0; \
	$(TOOLS_DIR)/scripts/launch_vivado.py -mode batch -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_generate_bd.tcl) -log $(1).log -nojournal || export VIV_ERR=$$?; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	exit $$VIV_ERR
