#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   DB GPIO pin constraints for X410.
#

###############################################################################
# Pin constraints for the other PL pins (1.8 V)
###############################################################################
set_property PACKAGE_PIN F6  [get_ports {DB1_GPIO[0]}]
set_property PACKAGE_PIN E6  [get_ports {DB1_GPIO[1]}]
set_property PACKAGE_PIN E9  [get_ports {DB1_GPIO[2]}]
set_property PACKAGE_PIN E8  [get_ports {DB1_GPIO[3]}]
set_property PACKAGE_PIN E7  [get_ports {DB1_GPIO[4]}]
set_property PACKAGE_PIN D6  [get_ports {DB1_GPIO[5]}]
set_property PACKAGE_PIN D10 [get_ports {DB1_GPIO[6]}]
set_property PACKAGE_PIN C10 [get_ports {DB1_GPIO[7]}]
set_property PACKAGE_PIN C8  [get_ports {DB1_GPIO[8]}]
set_property PACKAGE_PIN C7  [get_ports {DB1_GPIO[9]}]
set_property PACKAGE_PIN D9  [get_ports {DB1_GPIO[10]}]
set_property PACKAGE_PIN D8  [get_ports {DB1_GPIO[11]}]
set_property PACKAGE_PIN B8  [get_ports {DB1_GPIO[12]}]
set_property PACKAGE_PIN B7  [get_ports {DB1_GPIO[13]}]
set_property PACKAGE_PIN B10 [get_ports {DB1_GPIO[14]}]
set_property PACKAGE_PIN B9  [get_ports {DB1_GPIO[15]}]
set_property PACKAGE_PIN C6  [get_ports {DB1_GPIO[16]}]
set_property PACKAGE_PIN C5  [get_ports {DB1_GPIO[17]}]
set_property PACKAGE_PIN B5  [get_ports {DB1_GPIO[18]}]
set_property PACKAGE_PIN A5  [get_ports {DB1_GPIO[19]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB1_GPIO[*]}]
set_property PULLDOWN   TRUE     [get_ports {DB1_GPIO[*]}]
set_property IOB        TRUE     [get_ports {DB1_GPIO[*]}]

set_property PACKAGE_PIN AW6 [get_ports {DB0_GPIO[0]}]
set_property PACKAGE_PIN AW5 [get_ports {DB0_GPIO[1]}]
set_property PACKAGE_PIN AW4 [get_ports {DB0_GPIO[2]}]
set_property PACKAGE_PIN AW3 [get_ports {DB0_GPIO[3]}]
set_property PACKAGE_PIN AV3 [get_ports {DB0_GPIO[4]}]
set_property PACKAGE_PIN AV2 [get_ports {DB0_GPIO[5]}]
set_property PACKAGE_PIN AU2 [get_ports {DB0_GPIO[6]}]
set_property PACKAGE_PIN AU1 [get_ports {DB0_GPIO[7]}]
set_property PACKAGE_PIN AV6 [get_ports {DB0_GPIO[8]}]
set_property PACKAGE_PIN AV5 [get_ports {DB0_GPIO[9]}]
set_property PACKAGE_PIN AU4 [get_ports {DB0_GPIO[10]}]
set_property PACKAGE_PIN AU3 [get_ports {DB0_GPIO[11]}]
set_property PACKAGE_PIN AT5 [get_ports {DB0_GPIO[12]}]
set_property PACKAGE_PIN AU5 [get_ports {DB0_GPIO[13]}]
set_property PACKAGE_PIN AT7 [get_ports {DB0_GPIO[14]}]
set_property PACKAGE_PIN AT6 [get_ports {DB0_GPIO[15]}]
set_property PACKAGE_PIN AU8 [get_ports {DB0_GPIO[16]}]
set_property PACKAGE_PIN AV8 [get_ports {DB0_GPIO[17]}]
set_property PACKAGE_PIN AU7 [get_ports {DB0_GPIO[18]}]
set_property PACKAGE_PIN AV7 [get_ports {DB0_GPIO[19]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB0_GPIO[*]}]
set_property PULLDOWN   TRUE     [get_ports {DB0_GPIO[*]}]
set_property IOB        TRUE     [get_ports {DB0_GPIO[*]}]
