#
# Copyright 2011 Ettus Research LLC
#

all: B100 B100_2RX
	find -name "*.twr" | xargs grep constraint | grep met

clean:
	rm -rf build*

B100:
	make -f Makefile.$@ bin

B100_2RX:
	make -f Makefile.$@ bin

.PHONY: all clean
