#
# Copyright 2012-2013 Ettus Research LLC
#

# Uncomment the following lines to build radio's with no DSP's
#OPTIONS += DELETE_DSP0=1
#OPTIONS += DELETE_DSP1=1
# Uncomment the following line to add a debug UART on GPIO 10 & 11
#OPTIONS += DEBUG_UART=1

CREATE_LVBITX=python ../../lib/io_port2/create-lvbitx.py

GIGE_DEFS=BUILD_1G=1 $(OPTIONS)
HYBRID_DEFS=ETH10G_PORT1=1 BUILD_1G=1 BUILD_10G=1 $(OPTIONS)
XGIGE_DEFS=ETH10G_PORT0=1 ETH10G_PORT1=1 BUILD_10G=1 $(OPTIONS)
HYBRID_SRAM_DEFS=ETH10G_PORT1=1 BUILD_1G=1 BUILD_10G=1 NO_DRAM_FIFOS=1 SRAM_FIFO_SIZE=16 $(OPTIONS)
XGIGE_SRAM_DEFS=ETH10G_PORT0=1 ETH10G_PORT1=1 BUILD_10G=1 NO_DRAM_FIFOS=1 SRAM_FIFO_SIZE=16 $(OPTIONS)

X300: X300_HGS X300_HG X300_XGS X300_XG

X310: X310_HGS X310_HG X310_XGS X310_XG

all: X300 X310

clean:
	rm -rf build-*_*
	rm -rf build

print_report = \
	echo "========================================================================"; \
	cat $(1) | grep "Design Summary:" -A 124; \
	echo "========================================================================"; \
	echo "Timing Summary:\n"; \
	cat $(1) | grep constraint | grep met | grep -v "*"; \
	echo "========================================================================";

# post_build($1=Device, $2=Flavor)
post_build = \
	@$(call print_report,build-$(1)_$(2)/build.log) \
	mkdir -p build; \
	echo "Exporting bitstream files..."; \
	cp build-$(1)_$(2)/x300.bin build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).bin; \
	cp build-$(1)_$(2)/x300.bit build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).bit; \
	echo "Generating LVBITX..."; \
	$(CREATE_LVBITX) --input-bin=build-$(1)_$(2)/x300.bin --output-lvbitx=build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).lvbitx --device="USRP $(1)" x3x0_base.lvbitx; \
	cp -f x3x0_base.lvbitx build/`echo $(1) | tr A-Z a-z`.lvbitx_base; \
	echo "Exporting logs..."; \
	cp build-$(1)_$(2)/build.log build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).log; \
	cp build-$(1)_$(2)/x300.twr build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).twr; \
	echo "\nDONE ... $(1)_$(2)\n";


#1Gig on both ports
X310_1G:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(GIGE_DEFS) EXTRA_DEFS="$(GIGE_DEFS)"
	$(call post_build,X310,1G)

X300_1G:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(GIGE_DEFS) EXTRA_DEFS="$(GIGE_DEFS)"
	$(call post_build,X300,1G)

#1Gig on port0, 10Gig on port1
X310_HG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(HYBRID_DEFS) EXTRA_DEFS="$(HYBRID_DEFS)"
	$(call post_build,X310,HG)

X300_HG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(HYBRID_DEFS) EXTRA_DEFS="$(HYBRID_DEFS)"
	$(call post_build,X300,HG)

#10Gig on both ports
X310_XG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(XGIGE_DEFS) EXTRA_DEFS="$(XGIGE_DEFS)"
	$(call post_build,X310,XG)

X300_XG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(XGIGE_DEFS) EXTRA_DEFS="$(XGIGE_DEFS)"
	$(call post_build,X300,XG)

# 1Gig on port0, 10Gig on port1, SRAM Tx FIFO's
X310_HGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(HYBRID_SRAM_DEFS) EXTRA_DEFS="$(HYBRID_SRAM_DEFS)"
	$(call post_build,X310,HGS)

X300_HGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(HYBRID_SRAM_DEFS) EXTRA_DEFS="$(HYBRID_SRAM_DEFS)"
	$(call post_build,X300,HGS)

# 1Gig on both ports, SRAM Tx FIFO's
X310_XGS: 
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(XGIGE_SRAM_DEFS) EXTRA_DEFS="$(XGIGE_SRAM_DEFS)"
	$(call post_build,X310,XGS)

X300_XGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(XGIGE_SRAM_DEFS) EXTRA_DEFS="$(XGIGE_SRAM_DEFS)"
	$(call post_build,X300,XGS)


.PHONY: all clean
