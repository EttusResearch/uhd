#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   QSFP28 Port 0 (Lane 3) pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs (QSFP28 ports)
###############################################################################

# Bank 131 (Quad X0Y4, Lanes X0Y16-X0Y19)
# Lane 3 (X0Y19)

set_property PACKAGE_PIN B36  [get_ports {QSFP0_3_RX_P}]
set_property PACKAGE_PIN B37  [get_ports {QSFP0_3_RX_N}]

set_property PACKAGE_PIN A33  [get_ports {QSFP0_3_TX_P}]
set_property PACKAGE_PIN A34  [get_ports {QSFP0_3_TX_N}]
