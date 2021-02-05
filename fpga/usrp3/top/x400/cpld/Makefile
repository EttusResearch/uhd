#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

GIT_HASH = $(shell ../../../tools/scripts/git-hash.sh)

build: ip
	@echo -ne "\n---- Make: Synthesis ...\n\n";
	@quartus_map mb_cpld --verilog_macro="GIT_HASH=32'h$(GIT_HASH)";
	@echo -ne "\n---- Make: Partition Merge ...\n\n";
	@quartus_cdb mb_cpld --merge=on --incremental_compilation_import
	@echo -ne "\n\n---- Make: Implementation ...\n\n";
	@quartus_fit mb_cpld;
	@echo -ne "\n\n---- Make: Analyzing timing ...\n\n";
	@quartus_sta mb_cpld;
	@# grep for unconstrained path warning
	@grep "332102" output_files/mb_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# grep for timing closure critical warning
	@grep "332148" output_files/mb_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# expect no warnings
	@grep -iw "warning" output_files/mb_cpld.sta.rpt; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# expect no critical warning except "review power analyzer report file"
	@grep -i "critical warning" output_files/* | grep -v 16562; \
		if [ $$? -eq 0 ]; then false; else true; fi
	@# PS chip select analysis
	@quartus_sta -t scripts/ps_cs_analysis.tcl
	@echo -ne "\n\n---- Make: Generating bitfile...\n\n";
	@quartus_asm mb_cpld;
	@echo -ne "\n\n---- Make: Converting bitfile to svf format (ISP enabled)...\n\n";
	@quartus_cpf --convert \
	  --frequency 10.0MHz \
	  --voltage 2.5       \
	  --operation p       \
	  ./output_files/mb_cpld.pof ./output_files/mb_cpld_isp_on.svf -o background_programming=on;
	@echo -ne "\n\n---- Make: Converting bitfile to svf format (ISP disabled)...\n\n";
	@quartus_cpf --convert \
	  --frequency 10.0MHz \
	  --voltage 2.5       \
	  --operation p       \
	  ./output_files/mb_cpld.pof ./output_files/mb_cpld_isp_off.svf;
	@echo -ne "\n\n---- Make: Converting bitfile to rdp format...\n\n";
	@quartus_cpf -c raw_conversion.cof
	@echo -ne "\n\n---- Make: Copy final files...\n\n";
	@mkdir -p build
	@cp output_files/mb_cpld.pof                     build/usrp_x410_cpld.pof
	@cp output_files/mb_cpld_isp_off.svf             build/usrp_x410_cpld_isp_off.svf
	@cp output_files/mb_cpld_isp_on.svf              build/usrp_x410_cpld.svf
	@cp output_files/mb_cpld_converted_cfm0_auto.rpd build/usrp_x410_cpld.rpd
	@echo -ne "\n\n---- Make: MB CPLD ready!\n";
	@echo -ne "     Use build/usrp_x410_cpld.pof via JTAG programmer or\n"
	@echo -ne "         build/usrp_x410_cpld.svf (ISP on) via PS JTAG-engine (background programming) or\n"
	@echo -ne "         build/usrp_x410_cpld.rpd via reconfig engine or\n"
	@echo -ne "         build/usrp_x410_cpld_isp_off.svf via JTAG test points (initial programming)\n"

clean:
	@echo -ne "\nCleaning MB CPLD...\n";
	@git clean -Xdf

QSYS_PATH=$(subst \,/,$(QSYS_ROOTDIR))

ip: ip/flash/on_chip_flash/simulation/on_chip_flash.v \
    ip/clkctrl/clkctrl/simulation/clkctrl.v

ip/flash/on_chip_flash/simulation/on_chip_flash.v:
	$(QSYS_PATH)/qsys-generate ip/flash/on_chip_flash.qsys --simulation=VERILOG

ip/clkctrl/clkctrl/simulation/clkctrl.v:
	$(QSYS_PATH)/qsys-generate ip/clkctrl/clkctrl.qsys --simulation=VERILOG

all: build

.PHONY: all build clean ip
