#
# Copyright 2010-2011 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

all:
	@echo "Pick a specific target"

########################################################################
# Common Variables
########################################################################
TOP_DIR = $(shell pwd)
TOP_FW_DIR = $(TOP_DIR)/../firmware
TOP_FPGA_DIR = $(TOP_DIR)/../fpga
BUILT_IMAGES_DIR = $(TOP_DIR)/images
CMAKE_BUILD_DIR = $(TOP_DIR)/build

##filled in below
IMAGES_LIST =

##little rule to make the images directory
$(BUILT_IMAGES_DIR):
	mkdir $@
	mkdir $@/bit

GLOBAL_DEPS = $(BUILT_IMAGES_DIR)

########################################################################
# Utility Checks
########################################################################
ifeq ($(shell sdcc --help > /dev/null 2>&1 && echo $$?),0)
	HAS_SDCC=1
endif

ifeq ($(shell zpu-elf-gcc --help > /dev/null 2>&1 && echo $$?),0)
	HAS_ZPU_GCC=1
endif

ifeq ($(shell xtclsh -h > /dev/null 2>&1 && echo $$?),0)
	HAS_XTCLSH=1
endif

########################################################################
# USRP1 and B100 firmware
########################################################################
ifdef HAS_SDCC

_usrp1_fw_dir = $(TOP_FW_DIR)/fx2
_usrp1_fw_ihx = $(BUILT_IMAGES_DIR)/usrp1_fw.ihx
_usrp_b100_fw_ihx = $(BUILT_IMAGES_DIR)/usrp_b100_fw.ihx
IMAGES_LIST += $(_usrp1_fw_ihx) $(_usrp_b100_fw_ihx)

$(_usrp1_fw_ihx): $(GLOBAL_DEPS)
	cd $(_usrp1_fw_dir) && rm -rf build
	cd $(_usrp1_fw_dir) && mkdir build
	cd $(_usrp1_fw_dir)/build && cmake ../
	cd $(_usrp1_fw_dir)/build && make
	cp $(_usrp1_fw_dir)/build/usrp1/usrp1_fw.ihx $@

$(_usrp_b100_fw_ihx): $(_usrp1_fw_ihx)
	cp $(_usrp1_fw_dir)/build/b100/b100_fw.ihx $(_usrp_b100_fw_ihx)

endif

########################################################################
# USRP1 fpga
########################################################################
_usrp1_fpga_dir = $(TOP_FPGA_DIR)/usrp1/rbf/rev4
_usrp1_fpga_rbf = $(BUILT_IMAGES_DIR)/usrp1_fpga.rbf
_usrp1_fpga_4rx_rbf = $(BUILT_IMAGES_DIR)/usrp1_fpga_4rx.rbf
IMAGES_LIST += $(_usrp1_fpga_rbf) $(_usrp1_fpga_4rx_rbf)

$(_usrp1_fpga_rbf): $(GLOBAL_DEPS)
	cp $(_usrp1_fpga_dir)/std_2rxhb_2tx.rbf $@

$(_usrp1_fpga_4rx_rbf): $(GLOBAL_DEPS)
	cp $(_usrp1_fpga_dir)/std_4rx_0tx.rbf $@

########################################################################
# USRP-B100 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_b100_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/B100
_usrp_b100_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_b100_fpga.bin
_usrp_b100_fpga_2rx_bin = $(BUILT_IMAGES_DIR)/usrp_b100_fpga_2rx.bin
IMAGES_LIST += $(_usrp_b100_fpga_bin) $(_usrp_b100_fpga_2rx_bin)

$(_usrp_b100_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_b100_fpga_dir) && make -f Makefile.B100 clean
	cd $(_usrp_b100_fpga_dir) && make -f Makefile.B100 bin
	cp $(_usrp_b100_fpga_dir)/build-B100/B100.bin $@

$(_usrp_b100_fpga_2rx_bin): $(GLOBAL_DEPS)
	cd $(_usrp_b100_fpga_dir) && make -f Makefile.B100_2RX clean
	cd $(_usrp_b100_fpga_dir) && make -f Makefile.B100_2RX bin
	cp $(_usrp_b100_fpga_dir)/build-B100_2RX/B100.bin $@

endif

########################################################################
# USRP2 and N Series firmware
########################################################################
ifdef HAS_ZPU_GCC

_usrp2_fw_dir = $(TOP_FW_DIR)/zpu
_usrp2_fw_bin = $(BUILT_IMAGES_DIR)/usrp2_fw.bin
_usrp_n200_fw_bin = $(BUILT_IMAGES_DIR)/usrp_n200_fw.bin
_usrp_n210_fw_bin = $(BUILT_IMAGES_DIR)/usrp_n210_fw.bin
IMAGES_LIST += $(_usrp2_fw_bin) $(_usrp_n200_fw_bin) $(_usrp_n210_fw_bin)

$(_usrp2_fw_bin): $(GLOBAL_DEPS)
	cd $(_usrp2_fw_dir) && rm -rf build
	cd $(_usrp2_fw_dir) && mkdir build
	cd $(_usrp2_fw_dir)/build && cmake ../
	cd $(_usrp2_fw_dir)/build && make
	cp $(_usrp2_fw_dir)/build/usrp2/usrp2_txrx_uhd.bin $@

$(_usrp_n200_fw_bin): $(_usrp2_fw_bin)
	cp $(_usrp2_fw_dir)/build/usrp2p/usrp2p_txrx_uhd.bin $@

$(_usrp_n210_fw_bin): $(_usrp2_fw_bin)
	cp $(_usrp2_fw_dir)/build/usrp2p/usrp2p_txrx_uhd.bin $@

endif

########################################################################
# USRP2 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp2_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/USRP2
_usrp2_fpga_bin = $(BUILT_IMAGES_DIR)/usrp2_fpga.bin
IMAGES_LIST += $(_usrp2_fpga_bin)

$(_usrp2_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp2_fpga_dir) && make clean
	cd $(_usrp2_fpga_dir) && make bin
	cp $(_usrp2_fpga_dir)/build/u2_rev3.bin $@

endif

########################################################################
# USRP-N200 R2/R3 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_n200_r3_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/N2x0
_usrp_n200_r3_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n200_r3_fpga.bin
_usrp_n200_r3_fpga_bit = $(BUILT_IMAGES_DIR)/bit/usrp_n200_r3_fpga.bit
IMAGES_LIST += $(_usrp_n200_r3_fpga_bin) $(_usrp_n200_r3_fpga_bit)

$(_usrp_n200_r3_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_n200_r3_fpga_dir) && make -f Makefile.N200R3 clean
	cd $(_usrp_n200_r3_fpga_dir) && make -f Makefile.N200R3 bin
	cp $(_usrp_n200_r3_fpga_dir)/build-N200R3/u2plus.bin $@

$(_usrp_n200_r3_fpga_bit): $(_usrp_n200_r3_fpga_bin)
	cp $(_usrp_n200_r3_fpga_dir)/build-N200R3/u2plus.bit $@

#its called r2 on the silkscreen, so make an r2
_usrp_n200_r2_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n200_r2_fpga.bin
IMAGES_LIST += $(_usrp_n200_r2_fpga_bin)
$(_usrp_n200_r2_fpga_bin): $(_usrp_n200_r3_fpga_bin)
	cp $< $@

endif

########################################################################
# USRP-N210 R2/R3 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_n210_r3_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/N2x0
_usrp_n210_r3_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n210_r3_fpga.bin
_usrp_n210_r3_fpga_bit = $(BUILT_IMAGES_DIR)/bit/usrp_n210_r3_fpga.bit
IMAGES_LIST += $(_usrp_n210_r3_fpga_bin) $(_usrp_n210_r3_fpga_bit)

$(_usrp_n210_r3_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_n210_r3_fpga_dir) && make -f Makefile.N210R3 clean
	cd $(_usrp_n210_r3_fpga_dir) && make -f Makefile.N210R3 bin
	cp $(_usrp_n210_r3_fpga_dir)/build-N210R3/u2plus.bin $@

$(_usrp_n210_r3_fpga_bit): $(_usrp_n210_r3_fpga_bin)
	cp $(_usrp_n210_r3_fpga_dir)/build-N210R3/u2plus.bit $@

#its called r2 on the silkscreen, so make an r2
_usrp_n210_r2_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n210_r2_fpga.bin
IMAGES_LIST += $(_usrp_n210_r2_fpga_bin)
$(_usrp_n210_r2_fpga_bin): $(_usrp_n210_r3_fpga_bin)
	cp $< $@

endif

########################################################################
# USRP-N200 R4 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_n200_r4_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/N2x0
_usrp_n200_r4_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n200_r4_fpga.bin
_usrp_n200_r4_fpga_bit = $(BUILT_IMAGES_DIR)/bit/usrp_n200_r4_fpga.bit
IMAGES_LIST += $(_usrp_n200_r4_fpga_bin) $(_usrp_n200_r4_fpga_bit)

$(_usrp_n200_r4_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_n200_r4_fpga_dir) && make -f Makefile.N200R4 clean
	cd $(_usrp_n200_r4_fpga_dir) && make -f Makefile.N200R4 bin
	cp $(_usrp_n200_r4_fpga_dir)/build-N200R4/u2plus.bin $@

$(_usrp_n200_r4_fpga_bit): $(_usrp_n200_r4_fpga_bin)
	cp $(_usrp_n200_r4_fpga_dir)/build-N200R4/u2plus.bit $@

endif

########################################################################
# USRP-N210 R4 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_n210_r4_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/N2x0
_usrp_n210_r4_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_n210_r4_fpga.bin
_usrp_n210_r4_fpga_bit = $(BUILT_IMAGES_DIR)/bit/usrp_n210_r4_fpga.bit
IMAGES_LIST += $(_usrp_n210_r4_fpga_bin) $(_usrp_n210_r4_fpga_bit)

$(_usrp_n210_r4_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_n210_r4_fpga_dir) && make -f Makefile.N210R4 clean
	cd $(_usrp_n210_r4_fpga_dir) && make -f Makefile.N210R4 bin
	cp $(_usrp_n210_r4_fpga_dir)/build-N210R4/u2plus.bin $@

$(_usrp_n210_r4_fpga_bit): $(_usrp_n210_r4_fpga_bin)
	cp $(_usrp_n210_r4_fpga_dir)/build-N210R4/u2plus.bit $@

endif

########################################################################
# USRP-E100 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_e100_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/E1x0
_usrp_e100_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_e100_fpga_v2.bin
IMAGES_LIST += $(_usrp_e100_fpga_bin)

$(_usrp_e100_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_e100_fpga_dir) && make -f Makefile.E100 clean
	cd $(_usrp_e100_fpga_dir) && make -f Makefile.E100 bin
	cp $(_usrp_e100_fpga_dir)/build-E100/E1x0.bin $@

endif

########################################################################
# USRP-E110 fpga
########################################################################
ifdef HAS_XTCLSH

_usrp_e110_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/E1x0
_usrp_e110_fpga_bin = $(BUILT_IMAGES_DIR)/usrp_e110_fpga.bin
IMAGES_LIST += $(_usrp_e110_fpga_bin)

$(_usrp_e110_fpga_bin): $(GLOBAL_DEPS)
	cd $(_usrp_e110_fpga_dir) && make -f Makefile.E110 clean
	cd $(_usrp_e110_fpga_dir) && make -f Makefile.E110 bin
	cp $(_usrp_e110_fpga_dir)/build-E110/E1x0.bin $@

endif

########################################################################
# Build rules
########################################################################
images: $(IMAGES_LIST)
	find $(BUILT_IMAGES_DIR) -type f | xargs chmod -x
	find $(TOP_FPGA_DIR) -name "*.twr" | xargs grep constraint | grep met

package:
	mkdir -p $(CMAKE_BUILD_DIR)
	cd $(CMAKE_BUILD_DIR) && cmake -DUHD_RELEASE_MODE=OFF ..
	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=TGZ .. && make package
	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=ZIP .. && make package

clean:
	$(RM) -rf $(BUILT_IMAGES_DIR)
	$(RM) -rf $(CMAKE_BUILD_DIR)
