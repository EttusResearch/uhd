#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   100 GbE Placement Constraints for QSFP port 0.
#

set 100g_mac_0 [get_cells -hierarchical -filter { PRIMITIVE_TYPE == ADVANCED.MAC.CMACE4 && PARENT =~  "*eth_qsfp0*" } ]
set_property LOC CMACE4_X0Y1 $100g_mac_0
