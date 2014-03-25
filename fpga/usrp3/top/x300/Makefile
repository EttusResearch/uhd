#
# Copyright 2012-2013 Ettus Research LLC
#

# Uncomment following line to build with internal SRAM FIFOS instead of DRAM based FIFO's
#OPTIONS += NO_DRAM_FIFOS=1
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

all: X300 X310
	find -name "*.twr" | xargs grep constraint | grep met

clean:
	rm -rf build*

#Build X300_HGS and X300_XGS
X300: X300_HGS X300_XGS

#Build X310_HGS and X310_XGS
X310: X310_HGS X310_XGS

#Build DRAM Hybrid images
DRAM: X300_HG X310_HG

#Build X300_HGS and X310_HGS
HGS: X300_HGS X310_HGS

#Build X300_HGS and X310_HGS
XGS: X300_XGS X310_XGS

#1Gig on both ports
X310_1G:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(GIGE_DEFS) EXTRA_DEFS="$(GIGE_DEFS)"
	mkdir -p build
	cp build-X310/x300.bin build/usrp_x310_fpga_1G.bin
	cp build-X310/x300.bit build/usrp_x310_fpga_1G.bit
	$(CREATE_LVBITX) --input-bin=build-X310/x300.bin --output-lvbitx=build/usrp_x310_fpga_1G.lvbitx --device="USRP X310" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x310.lvbitx_base


X300_1G:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(GIGE_DEFS) EXTRA_DEFS="$(GIGE_DEFS)"
	mkdir -p build
	cp build-X300/x300.bin build/usrp_x300_fpga_1G.bin
	cp build-X300/x300.bit build/usrp_x300_fpga_1G.bit
	$(CREATE_LVBITX) --input-bin=build-X300/x300.bin --output-lvbitx=build/usrp_x300_fpga_1G.lvbitx --device="USRP X300" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x300.lvbitx_base

#1Gig on port0, 10Gig on port1
X310_HG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(HYBRID_DEFS) EXTRA_DEFS="$(HYBRID_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X310_HG/x300.bin build/usrp_x310_fpga_HG.bin
	cp build-X310_HG/x300.bit build/usrp_x310_fpga_HG.bit
	$(CREATE_LVBITX) --input-bin=build-X310_HG/x300.bin --output-lvbitx=build/usrp_x310_fpga_HG.lvbitx --device="USRP X310" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x310.lvbitx_base

X300_HG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(HYBRID_DEFS) EXTRA_DEFS="$(HYBRID_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X300_HG/x300.bin build/usrp_x300_fpga_HG.bin
	cp build-X300_HG/x300.bit build/usrp_x300_fpga_HG.bit
	$(CREATE_LVBITX) --input-bin=build-X300_HG/x300.bin --output-lvbitx=build/usrp_x300_fpga_HG.lvbitx --device="USRP X300" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x300.lvbitx_base

#10Gig on both ports
X310_XG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(XGIGE_DEFS) EXTRA_DEFS="$(XGIGE_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X310_XG/x300.bin build/usrp_x310_fpga_XG.bin
	cp build-X310_XG/x300.bit build/usrp_x310_fpga_XG.bit
	$(CREATE_LVBITX) --input-bin=build-X310_XG/x300.bin --output-lvbitx=build/usrp_x310_fpga_XG.lvbitx --device="USRP X310" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x310.lvbitx_base

X300_XG:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(XGIGE_DEFS) EXTRA_DEFS="$(XGIGE_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X300_XG/x300.bin build/usrp_x300_fpga_XG.bin
	cp build-X300_XG/x300.bit build/usrp_x300_fpga_XG.bit
	$(CREATE_LVBITX) --input-bin=build-X300_XG/x300.bin --output-lvbitx=build/usrp_x300_fpga_XG.lvbitx --device="USRP X300" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x300.lvbitx_base

# 1Gig on port0, 10Gig on port1, SRAM Tx FIFO's
X310_HGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(HYBRID_SRAM_DEFS) EXTRA_DEFS="$(HYBRID_SRAM_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X310_HGS/x300.bin build/usrp_x310_fpga_HGS.bin
	cp build-X310_HGS/x300.bit build/usrp_x310_fpga_HGS.bit
	$(CREATE_LVBITX) --input-bin=build-X310_HGS/x300.bin --output-lvbitx=build/usrp_x310_fpga_HGS.lvbitx --device="USRP X310" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x310.lvbitx_base

X300_HGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(HYBRID_SRAM_DEFS) EXTRA_DEFS="$(HYBRID_SRAM_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X300_HGS/x300.bin build/usrp_x300_fpga_HGS.bin
	cp build-X300_HGS/x300.bit build/usrp_x300_fpga_HGS.bit
	$(CREATE_LVBITX) --input-bin=build-X300_HGS/x300.bin --output-lvbitx=build/usrp_x300_fpga_HGS.lvbitx --device="USRP X300" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x300.lvbitx_base

# 1Gig on both ports, SRAM Tx FIFO's
X310_XGS: 
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K410T $(XGIGE_SRAM_DEFS) EXTRA_DEFS="$(XGIGE_SRAM_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X310_XGS/x300.bin build/usrp_x310_fpga_XGS.bin
	cp build-X310_XGS/x300.bit build/usrp_x310_fpga_XGS.bit
	$(CREATE_LVBITX) --input-bin=build-X310_XGS/x300.bin --output-lvbitx=build/usrp_x310_fpga_XGS.lvbitx --device="USRP X310" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x310.lvbitx_base

X300_XGS:
	make -f Makefile.x300.inc bin NAME=$@ DEVICE=XC7K325T $(XGIGE_SRAM_DEFS) EXTRA_DEFS="$(XGIGE_SRAM_DEFS)" FLOORPLAN="$(FLOORPLAN)"
	mkdir -p build
	cp build-X300_XGS/x300.bin build/usrp_x300_fpga_XGS.bin
	cp build-X300_XGS/x300.bit build/usrp_x300_fpga_XGS.bit
	$(CREATE_LVBITX) --input-bin=build-X300_XGS/x300.bin --output-lvbitx=build/usrp_x300_fpga_XGS.lvbitx --device="USRP X300" x3x0_base.lvbitx
	cp -f x3x0_base.lvbitx build/x300.lvbitx_base



.PHONY: all clean
