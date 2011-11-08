#
# Copyright 2011 Ettus Research LLC
#

all: E100 E110
	find -name "*.twr" | xargs grep constraint | grep met

clean:
	rm -rf build*

E100:
	make -f Makefile.$@ bin

E110:
	make -f Makefile.$@ bin

.PHONY: all clean
