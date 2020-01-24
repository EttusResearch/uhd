#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Daughterboard Pin Definitions for the N310.
#

## TDC : ################################################################################
## Bank 11, 2.5V (DB B)
#########################################################################################

set_property PACKAGE_PIN   W21              [get_ports {UNUSED_PIN_TDCB_0}]
set_property PACKAGE_PIN   Y21              [get_ports {UNUSED_PIN_TDCB_1}]
set_property PACKAGE_PIN   Y22              [get_ports {UNUSED_PIN_TDCB_2}]
set_property PACKAGE_PIN   Y23              [get_ports {UNUSED_PIN_TDCB_3}]
set_property IOSTANDARD    LVCMOS25         [get_ports {UNUSED_PIN_TDCB_*}]
set_property IOB           TRUE             [get_ports {UNUSED_PIN_TDCB_*}]

### USRP IO B : #########################################################################
## Bank 11/33
#########################################################################################

## HP GPIO, Bank 33, 1.8V

set_property PACKAGE_PIN   J4               [get_ports {DBB_CPLD_PS_SPI_LE}]
set_property PACKAGE_PIN   J3               [get_ports {DBB_CPLD_PS_SPI_SCLK}]
set_property PACKAGE_PIN   D4               [get_ports {DBB_CH1_TX_DSA_DATA[5]}]
# set_property PACKAGE_PIN   D3               [get_ports {nc}]
set_property PACKAGE_PIN   K2               [get_ports {DBB_CPLD_PS_SPI_ADDR[0]}]
set_property PACKAGE_PIN   K3               [get_ports {DBB_CPLD_PS_SPI_ADDR[1]}]
set_property PACKAGE_PIN   B5               [get_ports {DBB_CH1_TX_DSA_DATA[3]}]
set_property PACKAGE_PIN   B4               [get_ports {DBB_CH1_TX_DSA_DATA[4]}]
set_property PACKAGE_PIN   G5               [get_ports {DBB_CPLD_PS_SPI_SDO}]
set_property PACKAGE_PIN   G4               [get_ports {DBB_CPLD_PS_SPI_SDI}]
set_property PACKAGE_PIN   J5               [get_ports {DBB_CH1_RX_DSA_DATA[0]}]
set_property PACKAGE_PIN   K5               [get_ports {DBB_CH1_RX_DSA_DATA[1]}]
set_property PACKAGE_PIN   D5               [get_ports {DBB_CH1_TX_DSA_DATA[2]}]
set_property PACKAGE_PIN   E6               [get_ports {DBB_CH1_TX_DSA_DATA[1]}]
set_property PACKAGE_PIN   L3               [get_ports {DBB_ATR_RX_1}]
set_property PACKAGE_PIN   L2               [get_ports {DBB_ATR_TX_2}]
set_property PACKAGE_PIN   G6               [get_ports {DBB_CH1_TX_DSA_DATA[0]}]
set_property PACKAGE_PIN   H6               [get_ports {DBB_CH1_RX_DSA_DATA[5]}]
set_property PACKAGE_PIN   H4               [get_ports {DBB_ATR_TX_1}]
set_property PACKAGE_PIN   H3               [get_ports {DBB_ATR_RX_2}]
# set_property PACKAGE_PIN   F2               [get_ports {nc}]
set_property PACKAGE_PIN   G2               [get_ports {DBB_CH1_RX_DSA_DATA[3]}]
set_property PACKAGE_PIN   J6               [get_ports {DBB_CH1_RX_DSA_DATA[4]}]
set_property PACKAGE_PIN   K6               [get_ports {DBB_CH1_RX_DSA_DATA[2]}]

## HR GPIO, Bank 10, 2.5V

set_property PACKAGE_PIN   AK17             [get_ports {DBB_MYK_SYNC_IN_n}]
set_property PACKAGE_PIN   AK18             [get_ports {DBB_CPLD_PL_SPI_ADDR[0]}]
set_property PACKAGE_PIN   AK21             [get_ports {DBB_MYK_SPI_SDO}]
set_property PACKAGE_PIN   AJ21             [get_ports {DBB_MYK_SPI_SDIO}]
set_property PACKAGE_PIN   AF19             [get_ports {DBB_CPLD_PL_SPI_ADDR[1]}]
set_property PACKAGE_PIN   AG19             [get_ports {DBB_CH2_TX_DSA_DATA[5]}]
set_property PACKAGE_PIN   AH19             [get_ports {DBB_CPLD_JTAG_TDI}]
set_property PACKAGE_PIN   AJ19             [get_ports {DBB_CPLD_JTAG_TDO}]
set_property PACKAGE_PIN   AK22             [get_ports {DBB_MYK_GPIO_1}]
set_property PACKAGE_PIN   AK23             [get_ports {DBB_MYK_GPIO_4}]
set_property PACKAGE_PIN   AF20             [get_ports {DBB_CH2_TX_DSA_DATA[4]}]
set_property PACKAGE_PIN   AG20             [get_ports {DBB_CH2_TX_DSA_DATA[3]}]
set_property PACKAGE_PIN   AF23             [get_ports {DBB_MYK_SYNC_OUT_n}]
set_property PACKAGE_PIN   AF24             [get_ports {DBB_CPLD_PL_SPI_SDO}]
set_property PACKAGE_PIN   AK20             [get_ports {DBB_MYK_GPIO_13}]
set_property PACKAGE_PIN   AJ20             [get_ports {DBB_MYK_GPIO_0}]
set_property PACKAGE_PIN   AJ23             [get_ports {DBB_MYK_INTRQ}]
set_property PACKAGE_PIN   AJ24             [get_ports {DBB_CH2_TX_DSA_DATA[2]}]
set_property PACKAGE_PIN   AG24             [get_ports {DBB_CH2_TX_DSA_DATA[0]}]
set_property PACKAGE_PIN   AG25             [get_ports {DBB_CH2_TX_DSA_DATA[1]}]
set_property PACKAGE_PIN   AG21             [get_ports {DBB_FPGA_CLK_P}]
set_property PACKAGE_PIN   AH21             [get_ports {DBB_FPGA_CLK_N}]
set_property PACKAGE_PIN   AE22             [get_ports {DBB_FPGA_SYSREF_P}]
set_property PACKAGE_PIN   AF22             [get_ports {DBB_FPGA_SYSREF_N}]
set_property PACKAGE_PIN   AJ25             [get_ports {DBB_CH2_RX_DSA_DATA[3]}]
set_property PACKAGE_PIN   AK25             [get_ports {DBB_CH2_RX_DSA_DATA[5]}]
set_property PACKAGE_PIN   AB21             [get_ports {DBB_CPLD_JTAG_TMS}]
set_property PACKAGE_PIN   AB22             [get_ports {DBB_CPLD_JTAG_TCK}]
set_property PACKAGE_PIN   AD23             [get_ports {DBB_MYK_GPIO_15}]
set_property PACKAGE_PIN   AE23             [get_ports {DBB_MYK_SPI_CS_n}]
set_property PACKAGE_PIN   AB24             [get_ports {DBB_CH2_RX_DSA_DATA[1]}]
set_property PACKAGE_PIN   AA24             [get_ports {DBB_CH2_RX_DSA_DATA[2]}]
set_property PACKAGE_PIN   AG22             [get_ports {DBB_CPLD_PL_SPI_LE}]
set_property PACKAGE_PIN   AH22             [get_ports {DBB_CPLD_PL_SPI_SDI}]
set_property PACKAGE_PIN   AD21             [get_ports {DBB_MYK_GPIO_12}]
set_property PACKAGE_PIN   AE21             [get_ports {DBB_MYK_GPIO_14}]
set_property PACKAGE_PIN   AC22             [get_ports {DBB_MYK_SPI_SCLK}]
set_property PACKAGE_PIN   AC23             [get_ports {DBB_MYK_GPIO_3}]
set_property PACKAGE_PIN   AC24             [get_ports {DBB_CH2_RX_DSA_DATA[0]}]
set_property PACKAGE_PIN   AD24             [get_ports {DBB_CH2_RX_DSA_DATA[4]}]
set_property PACKAGE_PIN   AH23             [get_ports {DBB_CPLD_PL_SPI_ADDR[2]}]
set_property PACKAGE_PIN   AH24             [get_ports {DBB_CPLD_PL_SPI_SCLK}]

# set_property PACKAGE_PIN   AA25             [get_ports DBB_SWITCHER_CLOCK]
# set_property IOSTANDARD    LVCMOS33         [get_ports DBB_SWITCHER_CLOCK]
# set_property DRIVE         4                [get_ports DBB_SWITCHER_CLOCK]
# set_property SLEW          SLOW             [get_ports DBB_SWITCHER_CLOCK]

# During SI measurements with default drive strength, many of the FPGA-driven lines to
# the DB were showing high over/undershoot. Therefore for single-ended lines to the DBs
# we are decreasing the drive strength to the minimum value (4mA) and explicitly
# declaring the (default) slew rate as SLOW.

set UsrpIoBHpPinsSe [get_ports {DBB_CPLD_PS_* \
                                DBB_CH1_* \
                                DBB_ATR*}]
set_property IOSTANDARD    LVCMOS18         $UsrpIoBHpPinsSe
set_property DRIVE         4                $UsrpIoBHpPinsSe
set_property SLEW          SLOW             $UsrpIoBHpPinsSe

set UsrpIoBHrPinsSe [get_ports {DBB_MYK_SPI_* \
                                DBB_MYK_INTRQ \
                                DBB_MYK_SYNC* \
                                DBB_MYK_GPIO* \
                                DBB_CPLD_PL_* \
                                DBB_CPLD_JTAG_* \
                                DBB_CH2*}]
set_property IOSTANDARD    LVCMOS25         $UsrpIoBHrPinsSe
set_property DRIVE         4                $UsrpIoBHrPinsSe
set_property SLEW          SLOW             $UsrpIoBHrPinsSe

set UsrpIoBHrPinsDiff [get_ports {DBB_FPGA_CLK_* \
                                  DBB_FPGA_SYSREF_*}]
set_property IOSTANDARD    LVDS_25          $UsrpIoBHrPinsDiff
set_property DIFF_TERM     TRUE             $UsrpIoBHrPinsDiff

# Do not allow the DSA lines to float... give them a weak pull if undriven.
set_property PULLUP TRUE [get_ports {DBB_CH*_*X_DSA_DATA[*]}]


### MGTs, Bank 112

set_property PACKAGE_PIN   W8               [get_ports {USRPIO_B_MGTCLK_P}]
set_property PACKAGE_PIN   W7               [get_ports {USRPIO_B_MGTCLK_N}]

# This mapping uses the TX pins as the "master" and mimics RX off of them so Vivado
# places the transceivers in the correct places. The mixup in lanes is accounted for
# in the Mykonos lane crossbar settings.
set_property PACKAGE_PIN   AC4              [get_ports {USRPIO_B_RX_P[0]}]
set_property PACKAGE_PIN   AC3              [get_ports {USRPIO_B_RX_N[0]}]
set_property PACKAGE_PIN   AB6              [get_ports {USRPIO_B_RX_P[1]}]
set_property PACKAGE_PIN   AB5              [get_ports {USRPIO_B_RX_N[1]}]
set_property PACKAGE_PIN   Y6               [get_ports {USRPIO_B_RX_P[2]}]
set_property PACKAGE_PIN   Y5               [get_ports {USRPIO_B_RX_N[2]}]
set_property PACKAGE_PIN   AA4              [get_ports {USRPIO_B_RX_P[3]}]
set_property PACKAGE_PIN   AA3              [get_ports {USRPIO_B_RX_N[3]}]

set_property PACKAGE_PIN   AB2              [get_ports {USRPIO_B_TX_P[0]}]
set_property PACKAGE_PIN   AB1              [get_ports {USRPIO_B_TX_N[0]}]
set_property PACKAGE_PIN   Y2               [get_ports {USRPIO_B_TX_P[1]}]
set_property PACKAGE_PIN   Y1               [get_ports {USRPIO_B_TX_N[1]}]
set_property PACKAGE_PIN   W4               [get_ports {USRPIO_B_TX_P[2]}]
set_property PACKAGE_PIN   W3               [get_ports {USRPIO_B_TX_N[2]}]
set_property PACKAGE_PIN   V2               [get_ports {USRPIO_B_TX_P[3]}]
set_property PACKAGE_PIN   V1               [get_ports {USRPIO_B_TX_N[3]}]
