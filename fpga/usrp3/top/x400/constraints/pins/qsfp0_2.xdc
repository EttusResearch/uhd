#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   QSFP28 Port 0 (Lane 2) pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs (QSFP28 ports)
###############################################################################

# Bank 131 (Quad X0Y4, Lanes X0Y16-X0Y19)
# Lane 2 (X0Y18)

set_property PACKAGE_PIN C38  [get_ports {QSFP0_2_RX_P}]
set_property PACKAGE_PIN C39  [get_ports {QSFP0_2_RX_N}]

set_property PACKAGE_PIN B31  [get_ports {QSFP0_2_TX_P}]
set_property PACKAGE_PIN B32  [get_ports {QSFP0_2_TX_N}]
