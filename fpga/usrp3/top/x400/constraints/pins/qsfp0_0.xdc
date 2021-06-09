#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   QSFP28 Port 0 (Lane 0) pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs (QSFP28 ports)
###############################################################################

# Bank 131 (Quad X0Y4, Lanes X0Y16-X0Y19)
# Lane 0 (X0Y16)

set_property PACKAGE_PIN E38  [get_ports {QSFP0_0_RX_P}]
set_property PACKAGE_PIN E39  [get_ports {QSFP0_0_RX_N}]

set_property PACKAGE_PIN D31  [get_ports {QSFP0_0_TX_P}]
set_property PACKAGE_PIN D32  [get_ports {QSFP0_0_TX_N}]
