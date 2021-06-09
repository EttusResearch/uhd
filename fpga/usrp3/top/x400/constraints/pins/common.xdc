#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Common pin constraints for X410.
#

###############################################################################
# Pin constraints for the MGTs reference clocks
###############################################################################

set_property PACKAGE_PIN U33 [get_ports {MGT_REFCLK_LMK0_P}]
set_property PACKAGE_PIN U34 [get_ports {MGT_REFCLK_LMK0_N}]

set_property PACKAGE_PIN T31 [get_ports {MGT_REFCLK_LMK1_P}]
set_property PACKAGE_PIN T32 [get_ports {MGT_REFCLK_LMK1_N}]

set_property PACKAGE_PIN W33 [get_ports {MGT_REFCLK_LMK2_P}]
set_property PACKAGE_PIN W34 [get_ports {MGT_REFCLK_LMK2_N}]

set_property PACKAGE_PIN V31 [get_ports {MGT_REFCLK_LMK3_P}]
set_property PACKAGE_PIN V32 [get_ports {MGT_REFCLK_LMK3_N}]


###############################################################################
# Common pin constraints for the QSFP28 ports
###############################################################################

set_property PACKAGE_PIN AJ15 [get_ports {QSFP0_MODPRS_n}]
set_property PACKAGE_PIN AH16 [get_ports {QSFP0_RESET_n}]
set_property PACKAGE_PIN AH15 [get_ports {QSFP0_LPMODE_n}]

set_property PACKAGE_PIN AL11 [get_ports {QSFP1_MODPRS_n}]
set_property PACKAGE_PIN AR8  [get_ports {QSFP1_RESET_n}]
set_property PACKAGE_PIN AT9  [get_ports {QSFP1_LPMODE_n}]

set_property IOSTANDARD LVCMOS12 [get_ports {QSFP*_MODPRS_n QSFP*_RESET_n QSFP*_LPMODE_n}]
set_property SLEW       SLOW     [get_ports {QSFP*_RESET_n QSFP*_LPMODE_n}]


###############################################################################
# eCPRI future clocks
###############################################################################

# Input
set_property PACKAGE_PIN AK17 [get_ports {FPGA_AUX_REF}]
set_property IOSTANDARD LVCMOS12 [get_ports {FPGA_AUX_REF}]

# Output
set_property PACKAGE_PIN AG17 [get_ports {FABRIC_CLK_OUT_P}]
set_property PACKAGE_PIN AH17 [get_ports {FABRIC_CLK_OUT_N}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {FABRIC_CLK_OUT_*}]

# GTY_RCV_CLK_P is defined in qsfp_port1


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

set_property PACKAGE_PIN A7 [get_ports {DB1_SYNTH_SYNC}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB1_SYNTH_SYNC}]

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

set_property PACKAGE_PIN AP5 [get_ports {DB0_SYNTH_SYNC}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB0_SYNTH_SYNC}]

set_property PACKAGE_PIN A9   [get_ports {LMK_SYNC}]
set_property IOB         TRUE [get_ports {LMK_SYNC}]

set_property PACKAGE_PIN A10 [get_ports {TRIG_IO}]
set_property PACKAGE_PIN A6  [get_ports {PPS_IN}]
set_property PACKAGE_PIN AR7 [get_ports {PL_CPLD_SCLK}]
set_property PACKAGE_PIN AR6 [get_ports {PL_CPLD_MOSI}]
set_property PACKAGE_PIN AP6 [get_ports {PL_CPLD_MISO}]
set_property IOSTANDARD LVCMOS18 [get_ports {LMK_SYNC TRIG_IO PPS_IN PL_CPLD_SCLK PL_CPLD_MOSI PL_CPLD_MISO}]
set_property DRIVE      16       [get_ports {PL_CPLD_SCLK}]


###############################################################################
# Pin constraints for the other PL pins (1.2 V)
###############################################################################

set_property PACKAGE_PIN AL16 [get_ports {PLL_REFCLK_FPGA_P}]
set_property PACKAGE_PIN AL15 [get_ports {PLL_REFCLK_FPGA_N}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {PLL_REFCLK_FPGA_*}]

set_property PACKAGE_PIN G17  [get_ports {BASE_REFCLK_FPGA_P}]
set_property PACKAGE_PIN F17  [get_ports {BASE_REFCLK_FPGA_N}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {BASE_REFCLK_FPGA_*}]

set_property PACKAGE_PIN AF17 [get_ports {SYSREF_FABRIC_P}]
set_property PACKAGE_PIN AF16 [get_ports {SYSREF_FABRIC_N}]
set_property IOSTANDARD DIFF_SSTL12 [get_ports {SYSREF_FABRIC_*}]

set_property PACKAGE_PIN J15  [get_ports {DIOA_FPGA[0]}]
set_property PACKAGE_PIN H15  [get_ports {DIOA_FPGA[1]}]
set_property PACKAGE_PIN L17  [get_ports {DIOA_FPGA[2]}]
set_property PACKAGE_PIN K17  [get_ports {DIOA_FPGA[3]}]
set_property PACKAGE_PIN K16  [get_ports {DIOA_FPGA[4]}]
set_property PACKAGE_PIN J16  [get_ports {DIOA_FPGA[5]}]
set_property PACKAGE_PIN K19  [get_ports {DIOA_FPGA[6]}]
set_property PACKAGE_PIN K18  [get_ports {DIOA_FPGA[7]}]
set_property PACKAGE_PIN H17  [get_ports {DIOA_FPGA[8]}]
set_property PACKAGE_PIN H16  [get_ports {DIOA_FPGA[9]}]
set_property PACKAGE_PIN J19  [get_ports {DIOA_FPGA[10]}]
set_property PACKAGE_PIN J18  [get_ports {DIOA_FPGA[11]}]
set_property PACKAGE_PIN M18  [get_ports {DIOB_FPGA[0]}]
set_property PACKAGE_PIN H18  [get_ports {DIOB_FPGA[1]}]
set_property PACKAGE_PIN G18  [get_ports {DIOB_FPGA[2]}]
set_property PACKAGE_PIN G15  [get_ports {DIOB_FPGA[3]}]
set_property PACKAGE_PIN F15  [get_ports {DIOB_FPGA[4]}]
set_property PACKAGE_PIN G19  [get_ports {DIOB_FPGA[5]}]
set_property PACKAGE_PIN F19  [get_ports {DIOB_FPGA[6]}]
set_property PACKAGE_PIN F16  [get_ports {DIOB_FPGA[7]}]
set_property PACKAGE_PIN E16  [get_ports {DIOB_FPGA[8]}]
set_property PACKAGE_PIN E18  [get_ports {DIOB_FPGA[9]}]
set_property PACKAGE_PIN E17  [get_ports {DIOB_FPGA[10]}]
set_property PACKAGE_PIN E19  [get_ports {DIOB_FPGA[11]}]
set_property IOSTANDARD LVCMOS12 [get_ports {DIO*_FPGA[*]}]
set_property PULLDOWN   true     [get_ports {DIO*_FPGA[*]}]

set_property PACKAGE_PIN AW13 [get_ports {PPS_LED}]
set_property IOSTANDARD LVCMOS12 [get_ports {PPS_LED}]

set_property PACKAGE_PIN B23  [get_ports {PL_CPLD_JTAGEN}]
set_property PACKAGE_PIN N21  [get_ports {PL_CPLD_CS0_n}]
set_property PACKAGE_PIN J24  [get_ports {PL_CPLD_CS1_n}]
set_property PACKAGE_PIN AN12 [get_ports {CPLD_JTAG_OE_n}]
set_property IOSTANDARD LVCMOS12 [get_ports {PL_CPLD_JTAGEN PL_CPLD_CS*_n CPLD_JTAG_OE_n}]


###############################################################################
# Unused pins
###############################################################################

# set_property PACKAGE_PIN D19  [get_ports {PL_CPLD_IRQ}]
# set_property PACKAGE_PIN AF15 [get_ports {FPGA_TEST}]
# set_property IOSTANDARD LVCMOS12 [get_ports {FPGA_TEST PL_CPLD_IRQ}]

# set_property PACKAGE_PIN AK16 [get_ports {TDC_SPARE_0}]
# set_property PACKAGE_PIN AJ16 [get_ports {TDC_SPARE_1}]
# set_property IOSTANDARD LVCMOS12 [get_ports {TDC_SPARE_*}]
