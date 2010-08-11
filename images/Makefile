#
# Copyright 2010 Ettus Research LLC
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

########################################################################
# USRP2 firmware
########################################################################
_usrp2_fw_dir = $(TOP_FW_DIR)/microblaze
_usrp2_fw_bin = $(BUILT_IMAGES_DIR)/usrp2_fw.bin
IMAGES_LIST += $(_usrp2_fw_bin)

$(_usrp2_fw_bin):
	cd $(_usrp2_fw_dir) && ./bootstrap
	cd $(_usrp2_fw_dir) && ./configure --host=mb
	make -C $(_usrp2_fw_dir) clean
	make -C $(_usrp2_fw_dir) all
	cp $(_usrp2_fw_dir)/usrp2/usrp2_txrx_uhd.bin $@

########################################################################
# USRP2 fpga
########################################################################
_usrp2_fpga_dir = $(TOP_FPGA_DIR)/usrp2/top/u2_rev3
_usrp2_fpga_bin = $(BUILT_IMAGES_DIR)/usrp2_fpga.bin
IMAGES_LIST += $(_usrp2_fpga_bin)

$(_usrp2_fpga_bin):
	cd $(_usrp2_fpga_dir) && make -f Makefile.udp clean
	cd $(_usrp2_fpga_dir) && make -f Makefile.udp bin
	cp $(_usrp2_fpga_dir)/build-udp/u2_rev3.bin $@

########################################################################
# Build rules
########################################################################
##little rule to make the images directory
$(BUILT_IMAGES_DIR):
	mkdir $@

images: $(BUILT_IMAGES_DIR) $(IMAGES_LIST)

clean:
	$(RM) -rf $(BUILT_IMAGES_DIR)
	$(RM) -rf $(CMAKE_BUILD_DIR)

#packages that a linux machine can build
linux-packages:
	mkdir -p $(CMAKE_BUILD_DIR)

	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=TGZ $(TOP_DIR)
	make -C $(CMAKE_BUILD_DIR) package

	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=ZIP $(TOP_DIR)
	make -C $(CMAKE_BUILD_DIR) package

	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=DEB $(TOP_DIR)
	make -C $(CMAKE_BUILD_DIR) package

	cd $(CMAKE_BUILD_DIR) && cmake -DCPACK_GENERATOR=RPM $(TOP_DIR)
	make -C $(CMAKE_BUILD_DIR) package
