#
# Copyright 2012-2013 Ettus Research LLC
#

all: B200 B210
	find -name "*.twr" | xargs grep constraint | grep met
	mkdir -p build
	cp build-B200/b200.bin build/usrp_b200_fpga.bin
	cp build-B210/b200.bin build/usrp_b210_fpga.bin

clean:
	rm -rf build*

B200:
	make -f Makefile.b200.inc bin NAME=B200 DEVICE=XC6SLX75

B210:
	make -f Makefile.b200.inc bin NAME=B210 DEVICE=XC6SLX150 EXTRA_DEFS="B200_CAN_HAZ_R1=1"

.PHONY: all clean
