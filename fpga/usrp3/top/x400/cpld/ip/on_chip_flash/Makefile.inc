#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(TOOLS_DIR)/make/quartus_ip_builder.mak

IP_ON_CHIP_FLASH_SRCS = \
$(IP_BUILD_DIR)/on_chip_flash/on_chip_flash.qsys

IP_ON_CHIP_FLASH_OUTS = \
$(IP_BUILD_DIR)/on_chip_flash/on_chip_flash.sopcinfo

$(IP_ON_CHIP_FLASH_SRCS) $(IP_ON_CHIP_FLASH_OUTS) : $(IP_DIR)/on_chip_flash/on_chip_flash.qsys
	$(call BUILD_QUARTUS_IP,on_chip_flash,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR))
