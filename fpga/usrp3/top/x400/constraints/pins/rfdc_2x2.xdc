#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   RF data converters pin constraints for X410.
#   Note: commented constraints are left for documentation purposes.
#

# SYSREF input for data converters.
set_property PACKAGE_PIN U4  [get_ports {SYSREF_RF_N}]
set_property PACKAGE_PIN U5  [get_ports {SYSREF_RF_P}]

###############################################################################
# Pin constraints for the ADCs
###############################################################################

# ADC Reference Clocks for Slot 0 (DBA)
set_property PACKAGE_PIN AF5 [get_ports {ADC_CLK_P[0]}]
set_property PACKAGE_PIN AF4 [get_ports {ADC_CLK_N[0]}]
set_property PACKAGE_PIN AD5 [get_ports {ADC_CLK_P[1]}]
set_property PACKAGE_PIN AD4 [get_ports {ADC_CLK_N[1]}]

# ADC Reference Clocks for Slot 1 (DBB)
set_property PACKAGE_PIN AB5 [get_ports {ADC_CLK_P[2]}]
set_property PACKAGE_PIN AB4 [get_ports {ADC_CLK_N[2]}]
set_property PACKAGE_PIN Y5  [get_ports {ADC_CLK_P[3]}]
set_property PACKAGE_PIN Y4  [get_ports {ADC_CLK_N[3]}]

# ADC Inputs for Slot 0 (DBA)
# Note: numbering here does NOT match schematic, but it is the right order
# according to RF BD Connection spec.
# set_property PACKAGE_PIN AH2 [get_ports {DB0_RX_P[3]}]
# set_property PACKAGE_PIN AH1 [get_ports {DB0_RX_N[3]}]
# set_property PACKAGE_PIN AK2 [get_ports {DB0_RX_P[2]}]
# set_property PACKAGE_PIN AK1 [get_ports {DB0_RX_N[2]}]
set_property PACKAGE_PIN AM2 [get_ports {DB0_RX_P[1]}]
set_property PACKAGE_PIN AM1 [get_ports {DB0_RX_N[1]}]
set_property PACKAGE_PIN AP2 [get_ports {DB0_RX_P[0]}]
set_property PACKAGE_PIN AP1 [get_ports {DB0_RX_N[0]}]

# ADC Inputs for Slot 1 (DBB)
# Note: numbering here does NOT match schematic, but it is the right order
# according to RF BD Connection spec.
# set_property PACKAGE_PIN Y2  [get_ports {DB1_RX_P[3]}]
# set_property PACKAGE_PIN Y1  [get_ports {DB1_RX_N[3]}]
# set_property PACKAGE_PIN AB2 [get_ports {DB1_RX_P[2]}]
# set_property PACKAGE_PIN AB1 [get_ports {DB1_RX_N[2]}]
set_property PACKAGE_PIN AD2 [get_ports {DB1_RX_P[1]}]
set_property PACKAGE_PIN AD1 [get_ports {DB1_RX_N[1]}]
set_property PACKAGE_PIN AF2 [get_ports {DB1_RX_P[0]}]
set_property PACKAGE_PIN AF1 [get_ports {DB1_RX_N[0]}]


###############################################################################
# Pin constraints for the DACs
###############################################################################

# DAC Reference Clock for Slot 0 (DBA)
set_property PACKAGE_PIN R5  [get_ports {DAC_CLK_P[0]}]
set_property PACKAGE_PIN R4  [get_ports {DAC_CLK_N[0]}]

# DAC Reference Clock for Slot 1 (DBB)
set_property PACKAGE_PIN N5  [get_ports {DAC_CLK_P[1]}]
set_property PACKAGE_PIN N4  [get_ports {DAC_CLK_N[1]}]

# DAC Outputs for Slot 0 (DBA)
set_property PACKAGE_PIN U2  [get_ports {DB0_TX_P[0]}]
set_property PACKAGE_PIN U1  [get_ports {DB0_TX_N[0]}]
set_property PACKAGE_PIN R2  [get_ports {DB0_TX_P[1]}]
set_property PACKAGE_PIN R1  [get_ports {DB0_TX_N[1]}]
# set_property PACKAGE_PIN N2  [get_ports {DB0_TX_P[2]}]
# set_property PACKAGE_PIN N1  [get_ports {DB0_TX_N[2]}]
# set_property PACKAGE_PIN L2  [get_ports {DB0_TX_P[3]}]
# set_property PACKAGE_PIN L1  [get_ports {DB0_TX_N[3]}]

# DAC Outputs for Slot 1 (DBB)
set_property PACKAGE_PIN J2  [get_ports {DB1_TX_P[0]}]
set_property PACKAGE_PIN J1  [get_ports {DB1_TX_N[0]}]
set_property PACKAGE_PIN G2  [get_ports {DB1_TX_P[1]}]
set_property PACKAGE_PIN G1  [get_ports {DB1_TX_N[1]}]
# set_property PACKAGE_PIN E2  [get_ports {DB1_TX_P[2]}]
# set_property PACKAGE_PIN E1  [get_ports {DB1_TX_N[2]}]
# set_property PACKAGE_PIN C2  [get_ports {DB1_TX_P[3]}]
# set_property PACKAGE_PIN C1  [get_ports {DB1_TX_N[3]}]
