#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   zHD+iPASS ports (0 and 1) pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs (zHD+iPASS ports)
###############################################################################

# Quad 129
set_property PACKAGE_PIN N38 [get_ports {IPASS1_RX_P[0]}]
set_property PACKAGE_PIN N39 [get_ports {IPASS1_RX_N[0]}]
set_property PACKAGE_PIN M36 [get_ports {IPASS1_RX_P[1]}]
set_property PACKAGE_PIN M37 [get_ports {IPASS1_RX_N[1]}]
set_property PACKAGE_PIN L38 [get_ports {IPASS1_RX_P[2]}]
set_property PACKAGE_PIN L39 [get_ports {IPASS1_RX_N[2]}]
set_property PACKAGE_PIN K36 [get_ports {IPASS1_RX_P[3]}]
set_property PACKAGE_PIN K37 [get_ports {IPASS1_RX_N[3]}]

set_property PACKAGE_PIN P35 [get_ports {IPASS1_TX_P[0]}]
set_property PACKAGE_PIN P36 [get_ports {IPASS1_TX_N[0]}]
set_property PACKAGE_PIN N33 [get_ports {IPASS1_TX_P[1]}]
set_property PACKAGE_PIN N34 [get_ports {IPASS1_TX_N[1]}]
set_property PACKAGE_PIN L33 [get_ports {IPASS1_TX_P[2]}]
set_property PACKAGE_PIN L34 [get_ports {IPASS1_TX_N[2]}]
set_property PACKAGE_PIN J33 [get_ports {IPASS1_TX_P[3]}]
set_property PACKAGE_PIN J34 [get_ports {IPASS1_TX_N[3]}]

# Quad 130
set_property PACKAGE_PIN J38 [get_ports {IPASS0_RX_P[0]}]
set_property PACKAGE_PIN J39 [get_ports {IPASS0_RX_N[0]}]
set_property PACKAGE_PIN H36 [get_ports {IPASS0_RX_P[1]}]
set_property PACKAGE_PIN H37 [get_ports {IPASS0_RX_N[1]}]
set_property PACKAGE_PIN G38 [get_ports {IPASS0_RX_P[2]}]
set_property PACKAGE_PIN G39 [get_ports {IPASS0_RX_N[2]}]
set_property PACKAGE_PIN F36 [get_ports {IPASS0_RX_P[3]}]
set_property PACKAGE_PIN F37 [get_ports {IPASS0_RX_N[3]}]

set_property PACKAGE_PIN H31 [get_ports {IPASS0_TX_P[0]}]
set_property PACKAGE_PIN H32 [get_ports {IPASS0_TX_N[0]}]
set_property PACKAGE_PIN G33 [get_ports {IPASS0_TX_P[1]}]
set_property PACKAGE_PIN G34 [get_ports {IPASS0_TX_N[1]}]
set_property PACKAGE_PIN F31 [get_ports {IPASS0_TX_P[2]}]
set_property PACKAGE_PIN F32 [get_ports {IPASS0_TX_N[2]}]
set_property PACKAGE_PIN E33 [get_ports {IPASS0_TX_P[3]}]
set_property PACKAGE_PIN E34 [get_ports {IPASS0_TX_N[3]}]


###############################################################################
# Pin constraints for PCIe-related signals
###############################################################################
set_property PACKAGE_PIN F22  [get_ports {IPASS_SIDEBAND[0]}]
set_property PACKAGE_PIN D20  [get_ports {IPASS_SIDEBAND[1]}]
set_property PACKAGE_PIN AG14 [get_ports {PCIE_RESET}]

set_property IOSTANDARD  LVCMOS12 [get_ports {IPASS_SIDEBAND[*] PCIE_RESET}]
