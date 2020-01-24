#
# Copyright 2018 Ettus Research, a National Instruments Company
#

.PHONY: all clean

SRCS=TopCpld.qpf TopCpld.qsf Timing.sdc PkgMgCpld.vhd PkgSetup.vhd TopCpld.vhd

all: output_files/TopCpld.svf

cpld-magnesium-revc.svf: output_files/TopCpld.pof
	quartus_cpf --convert --frequency 10.0MHz --voltage 3.3 --operation p $? $@

output_files/TopCpld.pof: $(SRCS)
	quartus_map TopCpld
	quartus_fit TopCpld
	quartus_asm TopCpld
	quartus_sta TopCpld

clean:
	rm -rf db incremental_db output_files simulation cpld-magnesium-revc.svf

