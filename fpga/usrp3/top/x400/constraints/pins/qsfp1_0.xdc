#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   QSFP28 Port 1 (Lane 0) pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs (QSFP28 ports)
###############################################################################

# Bank 128 (Quad X0Y1, Lanes X0Y4-X0Y7)
# Lane 0 (X0Y4)

set_property PACKAGE_PIN AA38 [get_ports {QSFP1_0_RX_P}]
set_property PACKAGE_PIN AA39 [get_ports {QSFP1_0_RX_N}]

set_property PACKAGE_PIN Y35  [get_ports {QSFP1_0_TX_P}]
set_property PACKAGE_PIN Y36  [get_ports {QSFP1_0_TX_N}]

###############################################################################
# GTY_RCV_CLK_P can only be used with QSFP1
###############################################################################

set_property PACKAGE_PIN Y31 [get_ports {GTY_RCV_CLK_P}]
set_property PACKAGE_PIN Y32 [get_ports {GTY_RCV_CLK_N}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {GTY_RCV_CLK_*}]
