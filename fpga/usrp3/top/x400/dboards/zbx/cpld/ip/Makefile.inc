#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(IP_DIR)/on_chip_flash/Makefile.inc
include $(IP_DIR)/osc/Makefile.inc
include $(IP_DIR)/clkctrl/Makefile.inc

IP_SRCS = \
$(IP_ON_CHIP_FLASH_SRCS) \
$(IP_OSC_SRCS) \
$(IP_CLKCTRL_SRCS)


IP_OUTPUTS = \
$(IP_ON_CHIP_FLASH_OUTS) \
$(IP_OSC_OUTS) \
$(IP_CLKCTRL_OUTS)


ip: $(IP_OUTPUTS)

.PHONY: ip
