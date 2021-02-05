#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

GIT_HASH = $(shell ../../../../../tools/scripts/git-hash.sh)

build: cpld_defaults ip
	@echo -ne "\n---- Make: Synthesis ...\n\n";
	@quartus_map zbx_top_cpld --verilog_macro="GIT_HASH=32'h$(GIT_HASH)";
	@echo -ne "\n\n---- Make: Implementation ...\n\n";
	@quartus_fit zbx_top_cpld;
	@echo -ne "\n\n---- Make: Analyzing timing ...\n\n";
	@quartus_sta zbx_top_cpld;
	@# grep for unconstrained path warning
	@grep "332102" output_files/zbx_top_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# grep for timing closure critical warning
	@grep "332148" output_files/zbx_top_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# expect no warnings
	@grep -iw "warning" output_files/zbx_top_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# expect no critical warning except "review power analyzer report file"
	@grep -i "critical warning" output_files/* | grep -v 16562; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@echo -ne "\n\n---- Make: Generating bitfile...\n\n";
	@quartus_asm zbx_top_cpld;
	@echo -ne "\n\n---- Make: Converting bitfile to svf format (ISP enabled)...\n\n";
	@quartus_cpf --convert \
	  --frequency 12.5MHz \
	  --voltage 2.5       \
	  --operation p       \
	  ./output_files/zbx_top_cpld.pof ./output_files/zbx_top_cpld_isp_on.svf -o background_programming=on;
	@echo -ne "\n\n---- Make: Converting bitfile to svf format (ISP disabled)...\n\n";
	@quartus_cpf --convert \
	  --frequency 12.5MHz \
	  --voltage 2.5       \
	  --operation p       \
	  ./output_files/zbx_top_cpld.pof ./output_files/zbx_top_cpld_isp_off.svf;
	@echo -ne "\n\n---- Make: Converting bitfile to rdp format...\n\n";
	@quartus_cpf -c raw_conversion.cof
	@echo -ne "\n\n---- Make: Copy final files...\n\n";
	@mkdir -p build
	@cp output_files/zbx_top_cpld.pof                     build/usrp_zbx_cpld.pof
	@cp output_files/zbx_top_cpld_isp_off.svf             build/usrp_zbx_cpld.svf
	@cp output_files/zbx_top_cpld_isp_on.svf              build/usrp_zbx_cpld_isp_on.svf
	@cp output_files/zbx_top_cpld_converted_cfm0_auto.rpd build/usrp_zbx_cpld.rpd
	@echo -ne "\n\n---- Make: ZBX CPLD ready!\n";
	@echo -ne "     Use build/usrp_zbx_cpld.pof via JTAG programmer or\n"
	@echo -ne "         build/usrp_zbx_cpld.svf (ISP off) via MB CPLD JTAG engine or\n"
	@echo -ne "         build/usrp_zbx_cpld.rpd via reconfig engine or\n"
	@echo -ne "         build/usrp_zbx_cpld_isp_on.rpd via MB CPLD JTAG engine.\n"

clean:
	@echo -ne "\nCleaning ZBX CPLD...\n";
	@git clean -Xdf

QSYS_PATH=$(subst \,/,$(QUARTUS_ROOTDIR))/sopc_builder/bin

ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

REGS_PY_FILE=$(ROOT_DIR)/../../../../../../../host/lib/ic_reg_maps/gen_zbx_cpld_regs.py
REGS_PY_MODULE=register_endpoints/memory_init_files/zbx_cpld_regs_t.py

$(REGS_PY_MODULE): $(REGS_PY_FILE)
	    @python3 $(REGS_PY_FILE) $(REGS_PY_MODULE)

# Using one of the files as a dependency (all files are generated at the same time)
INIT_FILES := $(ROOT_DIR)/register_endpoints/memory_init_files/rx0_path_defaults.hex

$(INIT_FILES): register_endpoints/memory_init_files/gen_defaults.py $(REGS_PY_MODULE)
	@python3 $(ROOT_DIR)/register_endpoints/memory_init_files/gen_defaults.py

cpld_defaults: $(INIT_FILES)

ip: ip/flash/on_chip_flash/simulation/on_chip_flash.v \
    ip/osc/osc/simulation/osc.v
	@make -C ../../../cpld ip

ip/flash/on_chip_flash/simulation/on_chip_flash.v:
	$(QSYS_PATH)/qsys-generate ip/flash/on_chip_flash.qsys --simulation=VERILOG

ip/osc/osc/simulation/osc.v:
	$(QSYS_PATH)/qsys-generate ip/osc/osc.qsys --simulation=VERILOG

all: build

.PHONY: all build clean ip
