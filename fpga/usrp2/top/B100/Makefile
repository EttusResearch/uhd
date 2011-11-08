#
# Copyright 2011 Ettus Research LLC
#

all: B100
	find -name "*.twr" | xargs grep constraint | grep met

clean:
	rm -rf build*

B100:
	make -f Makefile.$@ bin

.PHONY: all clean
