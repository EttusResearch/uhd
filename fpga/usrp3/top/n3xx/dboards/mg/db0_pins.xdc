#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Daughterboard Pin Definitions for the N310.
#

## TDC : ################################################################################
## Bank 10, 2.5V (DB A)
#########################################################################################

set_property PACKAGE_PIN   AB15             [get_ports {UNUSED_PIN_TDCA_0}]
set_property PACKAGE_PIN   AB14             [get_ports {UNUSED_PIN_TDCA_1}]
set_property PACKAGE_PIN   AB16             [get_ports {UNUSED_PIN_TDCA_2}]
set_property PACKAGE_PIN   AB17             [get_ports {UNUSED_PIN_TDCA_3}]
set_property IOSTANDARD    LVCMOS25         [get_ports {UNUSED_PIN_TDCA_*}]
set_property IOB           TRUE             [get_ports {UNUSED_PIN_TDCA_*}]

## USRP IO A : ##########################################################################
## Banks 10/33
#########################################################################################

## HP GPIO, Bank 33, 1.8V

set_property PACKAGE_PIN   G1               [get_ports {DBA_CPLD_PS_SPI_LE}]
set_property PACKAGE_PIN   H2               [get_ports {DBA_CPLD_PS_SPI_SCLK}]
set_property PACKAGE_PIN   D1               [get_ports {DBA_CH1_TX_DSA_DATA[5]}]
# set_property PACKAGE_PIN   E1               [get_ports {nc}]
set_property PACKAGE_PIN   H1               [get_ports {DBA_CPLD_PS_SPI_ADDR[0]}]
set_property PACKAGE_PIN   J1               [get_ports {DBA_CPLD_PS_SPI_ADDR[1]}]
set_property PACKAGE_PIN   A5               [get_ports {DBA_CH1_TX_DSA_DATA[3]}]
set_property PACKAGE_PIN   A4               [get_ports {DBA_CH1_TX_DSA_DATA[4]}]
set_property PACKAGE_PIN   F5               [get_ports {DBA_CPLD_PS_SPI_SDO}]
set_property PACKAGE_PIN   E5               [get_ports {DBA_CPLD_PS_SPI_SDI}]
set_property PACKAGE_PIN   E3               [get_ports {DBA_CH1_RX_DSA_DATA[0]}]
set_property PACKAGE_PIN   E2               [get_ports {DBA_CH1_RX_DSA_DATA[1]}]
set_property PACKAGE_PIN   A3               [get_ports {DBA_CH1_TX_DSA_DATA[2]}]
set_property PACKAGE_PIN   A2               [get_ports {DBA_CH1_TX_DSA_DATA[1]}]
set_property PACKAGE_PIN   K1               [get_ports {DBA_ATR_RX_1}]
set_property PACKAGE_PIN   L1               [get_ports {DBA_ATR_TX_2}]
set_property PACKAGE_PIN   C4               [get_ports {DBA_CH1_TX_DSA_DATA[0]}]
set_property PACKAGE_PIN   C3               [get_ports {DBA_CH1_RX_DSA_DATA[5]}]
set_property PACKAGE_PIN   F4               [get_ports {DBA_ATR_TX_1}]
set_property PACKAGE_PIN   F3               [get_ports {DBA_ATR_RX_2}]
# set_property PACKAGE_PIN   B1               [get_ports {nc}]
set_property PACKAGE_PIN   B2               [get_ports {DBA_CH1_RX_DSA_DATA[3]}]
set_property PACKAGE_PIN   C1               [get_ports {DBA_CH1_RX_DSA_DATA[4]}]
set_property PACKAGE_PIN   C2               [get_ports {DBA_CH1_RX_DSA_DATA[2]}]

## HR GPIO, Bank 10, 2.5V

set_property PACKAGE_PIN   AG12             [get_ports {DBA_MYK_SYNC_IN_n}]
set_property PACKAGE_PIN   AH12             [get_ports {DBA_CPLD_PL_SPI_ADDR[0]}]
set_property PACKAGE_PIN   AJ13             [get_ports {DBA_MYK_SPI_SDO}]
set_property PACKAGE_PIN   AJ14             [get_ports {DBA_MYK_SPI_SDIO}]
set_property PACKAGE_PIN   AG15             [get_ports {DBA_CPLD_PL_SPI_ADDR[1]}]
set_property PACKAGE_PIN   AF15             [get_ports {DBA_CH2_TX_DSA_DATA[5]}]
set_property PACKAGE_PIN   AH13             [get_ports {DBA_CPLD_JTAG_TDI}]
set_property PACKAGE_PIN   AH14             [get_ports {DBA_CPLD_JTAG_TDO}]
set_property PACKAGE_PIN   AK15             [get_ports {DBA_MYK_GPIO_1}]
set_property PACKAGE_PIN   AJ15             [get_ports {DBA_MYK_GPIO_4}]
set_property PACKAGE_PIN   AH16             [get_ports {DBA_CH2_TX_DSA_DATA[4]}]
set_property PACKAGE_PIN   AH17             [get_ports {DBA_CH2_TX_DSA_DATA[3]}]
set_property PACKAGE_PIN   AE12             [get_ports {DBA_MYK_SYNC_OUT_n}]
set_property PACKAGE_PIN   AF12             [get_ports {DBA_CPLD_PL_SPI_SDO}]
set_property PACKAGE_PIN   AK12             [get_ports {DBA_MYK_GPIO_13}]
set_property PACKAGE_PIN   AK13             [get_ports {DBA_MYK_GPIO_0}]
set_property PACKAGE_PIN   AK16             [get_ports {DBA_MYK_INTRQ}]
set_property PACKAGE_PIN   AJ16             [get_ports {DBA_CH2_TX_DSA_DATA[2]}]
set_property PACKAGE_PIN   AH18             [get_ports {DBA_CH2_TX_DSA_DATA[0]}]
set_property PACKAGE_PIN   AJ18             [get_ports {DBA_CH2_TX_DSA_DATA[1]}]
set_property PACKAGE_PIN   AF14             [get_ports {DBA_FPGA_CLK_P}]
set_property PACKAGE_PIN   AG14             [get_ports {DBA_FPGA_CLK_N}]
set_property PACKAGE_PIN   AG17             [get_ports {DBA_FPGA_SYSREF_P}]
set_property PACKAGE_PIN   AG16             [get_ports {DBA_FPGA_SYSREF_N}]
set_property PACKAGE_PIN   AD15             [get_ports {DBA_CH2_RX_DSA_DATA[3]}]
set_property PACKAGE_PIN   AD16             [get_ports {DBA_CH2_RX_DSA_DATA[5]}]
set_property PACKAGE_PIN   AE13             [get_ports {DBA_CPLD_JTAG_TMS}]
set_property PACKAGE_PIN   AF13             [get_ports {DBA_CPLD_JTAG_TCK}]
set_property PACKAGE_PIN   AE15             [get_ports {DBA_MYK_GPIO_15}]
set_property PACKAGE_PIN   AE16             [get_ports {DBA_MYK_SPI_CS_n}]
set_property PACKAGE_PIN   AF17             [get_ports {DBA_CH2_RX_DSA_DATA[1]}]
set_property PACKAGE_PIN   AF18             [get_ports {DBA_CH2_RX_DSA_DATA[2]}]
set_property PACKAGE_PIN   AC16             [get_ports {DBA_CPLD_PL_SPI_LE}]
set_property PACKAGE_PIN   AC17             [get_ports {DBA_CPLD_PL_SPI_SDI}]
set_property PACKAGE_PIN   AD13             [get_ports {DBA_MYK_GPIO_12}]
set_property PACKAGE_PIN   AD14             [get_ports {DBA_MYK_GPIO_14}]
set_property PACKAGE_PIN   AE17             [get_ports {DBA_MYK_SPI_SCLK}]
set_property PACKAGE_PIN   AE18             [get_ports {DBA_MYK_GPIO_3}]
set_property PACKAGE_PIN   AB12             [get_ports {DBA_CH2_RX_DSA_DATA[0]}]
set_property PACKAGE_PIN   AC12             [get_ports {DBA_CH2_RX_DSA_DATA[4]}]
set_property PACKAGE_PIN   AC13             [get_ports {DBA_CPLD_PL_SPI_ADDR[2]}]
set_property PACKAGE_PIN   AC14             [get_ports {DBA_CPLD_PL_SPI_SCLK}]

# set_property PACKAGE_PIN   AB25             [get_ports {DBA_SWITCHER_CLOCK}]
# set_property IOSTANDARD    LVCMOS33         [get_ports {DBA_SWITCHER_CLOCK}]
# set_property DRIVE         4                [get_ports {DBA_SWITCHER_CLOCK}]
# set_property SLEW          SLOW             [get_ports {DBA_SWITCHER_CLOCK}]

# During SI measurements with default drive strength, many of the FPGA-driven lines to
# the DB were showing high over/undershoot. Therefore for single-ended lines to the DBs
# we are decreasing the drive strength to the minimum value (4mA) and explicitly
# declaring the (default) slew rate as SLOW.

set UsrpIoAHpPinsSe [get_ports {DBA_CPLD_PS_* \
                                DBA_CH1_* \
                                DBA_ATR*}]
set_property IOSTANDARD    LVCMOS18         $UsrpIoAHpPinsSe
set_property DRIVE         4                $UsrpIoAHpPinsSe
set_property SLEW          SLOW             $UsrpIoAHpPinsSe

set UsrpIoAHrPinsSe [get_ports {DBA_MYK_SPI_* \
                                DBA_MYK_INTRQ \
                                DBA_MYK_SYNC* \
                                DBA_MYK_GPIO* \
                                DBA_CPLD_PL_* \
                                DBA_CPLD_JTAG_* \
                                DBA_CH2*}]
set_property IOSTANDARD    LVCMOS25         $UsrpIoAHrPinsSe
set_property DRIVE         4                $UsrpIoAHrPinsSe
set_property SLEW          SLOW             $UsrpIoAHrPinsSe

set UsrpIoAHrPinsDiff [get_ports {DBA_FPGA_CLK_* \
                                  DBA_FPGA_SYSREF_*}]
set_property IOSTANDARD    LVDS_25          $UsrpIoAHrPinsDiff
set_property DIFF_TERM     TRUE             $UsrpIoAHrPinsDiff

# Do not allow the DSA lines to float... give them a weak pull if undriven.
set_property PULLUP TRUE [get_ports {DBA_CH*_*X_DSA_DATA[*]}]


### MGTs, Bank 112

set_property PACKAGE_PIN   N8               [get_ports {USRPIO_A_MGTCLK_P}]
set_property PACKAGE_PIN   N7               [get_ports {USRPIO_A_MGTCLK_N}]

# This mapping uses the TX pins as the "master" and mimics RX off of them so Vivado
# places the transceivers in the correct places. The mixup in lanes is accounted for
# in the Mykonos lane crossbar settings.
set_property PACKAGE_PIN   V6               [get_ports {USRPIO_A_RX_P[0]}]
set_property PACKAGE_PIN   V5               [get_ports {USRPIO_A_RX_N[0]}]
set_property PACKAGE_PIN   U4               [get_ports {USRPIO_A_RX_P[1]}]
set_property PACKAGE_PIN   U3               [get_ports {USRPIO_A_RX_N[1]}]
set_property PACKAGE_PIN   T6               [get_ports {USRPIO_A_RX_P[2]}]
set_property PACKAGE_PIN   T5               [get_ports {USRPIO_A_RX_N[2]}]
set_property PACKAGE_PIN   P6               [get_ports {USRPIO_A_RX_P[3]}]
set_property PACKAGE_PIN   P5               [get_ports {USRPIO_A_RX_N[3]}]

set_property PACKAGE_PIN   T2               [get_ports {USRPIO_A_TX_P[0]}]
set_property PACKAGE_PIN   T1               [get_ports {USRPIO_A_TX_N[0]}]
set_property PACKAGE_PIN   R4               [get_ports {USRPIO_A_TX_P[1]}]
set_property PACKAGE_PIN   R3               [get_ports {USRPIO_A_TX_N[1]}]
set_property PACKAGE_PIN   P2               [get_ports {USRPIO_A_TX_P[2]}]
set_property PACKAGE_PIN   P1               [get_ports {USRPIO_A_TX_N[2]}]
set_property PACKAGE_PIN   N4               [get_ports {USRPIO_A_TX_P[3]}]
set_property PACKAGE_PIN   N3               [get_ports {USRPIO_A_TX_N[3]}]
