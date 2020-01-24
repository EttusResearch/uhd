#
# Copyright 2018 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Motherboard Pin Definitions for the E320.
#

## Power ###################################################################
##
############################################################################

# Power Enable, Bank 10
set_property PACKAGE_PIN   AK15             [get_ports {ENA_PAPWR}]
set_property IOSTANDARD    LVCMOS33         [get_ports {ENA_PAPWR}]

## SFP + ###################################################################
##
############################################################################

## Power
#set_property PACKAGE_PIN   AB9              [get_ports {+1V2_MGT_AVTT}]
#set_property IOSTANDARD    <IOSTD_BANK112>  [get_ports {+1V2_MGT_AVTT}]

## Clocks, Bank 112
set_property PACKAGE_PIN   R7               [get_ports {CLK_MGT_156_25M_N}]
set_property PACKAGE_PIN   R8               [get_ports {CLK_MGT_156_25M_P}]

set_property PACKAGE_PIN   N7               [get_ports {CLK_MGT_125M_N}]
set_property PACKAGE_PIN   N8               [get_ports {CLK_MGT_125M_P}]

## MGT, Bank 112
set_property PACKAGE_PIN   V5               [get_ports {SFP1_RX_N}]
set_property PACKAGE_PIN   V6               [get_ports {SFP1_RX_P}]
set_property PACKAGE_PIN   T1               [get_ports {SFP1_TX_N}]
set_property PACKAGE_PIN   T2               [get_ports {SFP1_TX_P}]

## SFP Sideband, Bank 10, 3.3V
set_property PACKAGE_PIN   AJ16             [get_ports {SFP1_RXLOS}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP1_RXLOS}]

set_property PACKAGE_PIN   AK13             [get_ports {SFP1_TXFAULT}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP1_TXFAULT}]

set_property PACKAGE_PIN   AK12             [get_ports {SFP1_TXDISABLE}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP1_TXDISABLE}]

set_property PACKAGE_PIN   AE17             [get_ports {SFP1_MOD_ABS}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP1_MOD_ABS}]

set_property PACKAGE_PIN   AJ18             [get_ports {SFP1_RS0}]
set_property PACKAGE_PIN   AK16             [get_ports {SFP1_RS1}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP1_RS*}]

set_property PACKAGE_PIN   AB17             [get_ports {LED_LINK1}]
set_property IOSTANDARD    LVCMOS33         [get_ports {LED_LINK1}]

set_property PACKAGE_PIN   AB16             [get_ports {LED_ACT1}]
set_property IOSTANDARD    LVCMOS33         [get_ports {LED_ACT1}]

## Used with N310 hardware
#set_property PACKAGE_PIN   U25              [get_ports PANEL_LED_GPS]
#set_property PACKAGE_PIN   T25              [get_ports PANEL_LED_LINK]
#set_property PACKAGE_PIN   W29              [get_ports PANEL_LED_PPS]
#set_property PACKAGE_PIN   V24              [get_ports PANEL_LED_REF]
#set_property IOSTANDARD    LVCMOS33         [get_ports PANEL_LED_*]
#set_property DRIVE         4                [get_ports PANEL_LED_*]
#set_property SLEW          SLOW             [get_ports PANEL_LED_*]

## XCVR ####################################################################
## Catalina AD9361 Connections
##   -- Data Buses
##   -- Clocks
##   -- SPI
##   -- TX Amplifier
##   -- LEDs
##
############################################################################

set_property PACKAGE_PIN   P25              [get_ports {XCVR_ENABLE}]
set_property PACKAGE_PIN   T25              [get_ports {XCVR_SYNC}]
set_property PACKAGE_PIN   P23              [get_ports {XCVR_TXNRX}]
set_property PACKAGE_PIN   N27              [get_ports {XCVR_ENA_AGC}]
set_property PACKAGE_PIN   P26              [get_ports {XCVR_RESET_N}]

## AD9361 SPI, Bank 13, 1.8V
set_property PACKAGE_PIN   T27              [get_ports {XCVR_SPI_CS_N}]
set_property PACKAGE_PIN   N26              [get_ports {XCVR_SPI_MISO}]
set_property PACKAGE_PIN   P24              [get_ports {XCVR_SPI_MOSI}]
set_property PACKAGE_PIN   T24              [get_ports {XCVR_SPI_CLK}]

set_property IOSTANDARD    LVCMOS18         [get_ports {XCVR_*}]

## Catalina TX Data Bus
set_property PACKAGE_PIN   F3               [get_ports {TX_CLK_N}]
set_property PACKAGE_PIN   F4               [get_ports {TX_CLK_P}]
set_property IOSTANDARD    LVDS             [get_ports {TX_CLK_*}]
#
set_property PACKAGE_PIN   B4               [get_ports {TX_FRAME_N}]
set_property PACKAGE_PIN   B5               [get_ports {TX_FRAME_P}]
set_property IOSTANDARD    LVDS             [get_ports {TX_FRAME_*}]
#
set_property PACKAGE_PIN   L2               [get_ports {TX_DATA_N[0]}]
set_property PACKAGE_PIN   L3               [get_ports {TX_DATA_P[0]}]
set_property PACKAGE_PIN   D5               [get_ports {TX_DATA_N[1]}]
set_property PACKAGE_PIN   E6               [get_ports {TX_DATA_P[1]}]
set_property PACKAGE_PIN   A4               [get_ports {TX_DATA_N[2]}]
set_property PACKAGE_PIN   A5               [get_ports {TX_DATA_P[2]}]
set_property PACKAGE_PIN   J3               [get_ports {TX_DATA_N[3]}]
set_property PACKAGE_PIN   J4               [get_ports {TX_DATA_P[3]}]
set_property PACKAGE_PIN   D1               [get_ports {TX_DATA_N[4]}]
set_property PACKAGE_PIN   E1               [get_ports {TX_DATA_P[4]}]
set_property PACKAGE_PIN   E2               [get_ports {TX_DATA_N[5]}]
set_property PACKAGE_PIN   E3               [get_ports {TX_DATA_P[5]}]
set_property IOSTANDARD    LVDS             [get_ports {TX_DATA_*[*]}]

## Catalina RX Data Bus
set_property PACKAGE_PIN   G4               [get_ports {RX_CLK_N}]
set_property PACKAGE_PIN   G5               [get_ports {RX_CLK_P}]
set_property IOSTANDARD    LVDS             [get_ports {RX_CLK_*}]
set_property DIFF_TERM     TRUE             [get_ports {RX_CLK_*}]
#
set_property PACKAGE_PIN   J6               [get_ports {RX_FRAME_N}]
set_property PACKAGE_PIN   K6               [get_ports {RX_FRAME_P}]
set_property IOSTANDARD    LVDS             [get_ports {RX_FRAME_*}]
set_property DIFF_TERM     TRUE             [get_ports {RX_FRAME_*}]
#
set_property PACKAGE_PIN   C3               [get_ports {RX_DATA_N[0]}]
set_property PACKAGE_PIN   C4               [get_ports {RX_DATA_P[0]}]
set_property PACKAGE_PIN   G6               [get_ports {RX_DATA_N[1]}]
set_property PACKAGE_PIN   H6               [get_ports {RX_DATA_P[1]}]
set_property PACKAGE_PIN   D3               [get_ports {RX_DATA_N[2]}]
set_property PACKAGE_PIN   D4               [get_ports {RX_DATA_P[2]}]
set_property PACKAGE_PIN   K1               [get_ports {RX_DATA_N[3]}]
set_property PACKAGE_PIN   L1               [get_ports {RX_DATA_P[3]}]
set_property PACKAGE_PIN   J5               [get_ports {RX_DATA_N[4]}]
set_property PACKAGE_PIN   K5               [get_ports {RX_DATA_P[4]}]
set_property PACKAGE_PIN   F2               [get_ports {RX_DATA_N[5]}]
set_property PACKAGE_PIN   G2               [get_ports {RX_DATA_P[5]}]
set_property IOSTANDARD    LVDS             [get_ports {RX_DATA_*[*]}]
set_property DIFF_TERM     TRUE             [get_ports {RX_DATA_*[*]}]

## TX Amp, Bank 13, 1.8V
set_property PACKAGE_PIN   P29              [get_ports {TX_HFAMP1_ENA}]
set_property PACKAGE_PIN   U29              [get_ports {TX_HFAMP2_ENA}]
set_property IOSTANDARD    LVCMOS18         [get_ports {TX_HFAMP*_ENA}]

set_property PACKAGE_PIN   N29              [get_ports {TX_LFAMP1_ENA}]
set_property PACKAGE_PIN   T29              [get_ports {TX_LFAMP2_ENA}]
set_property IOSTANDARD    LVCMOS18         [get_ports {TX_LFAMP*_ENA}]

# CTRL_OUT, Bank 13, 1.8 V
set_property PACKAGE_PIN   V28              [get_ports {XCVR_CTRL_OUT[0]}]
set_property PACKAGE_PIN   V29              [get_ports {XCVR_CTRL_OUT[1]}]
set_property PACKAGE_PIN   W29              [get_ports {XCVR_CTRL_OUT[2]}]
set_property PACKAGE_PIN   W30              [get_ports {XCVR_CTRL_OUT[3]}]
set_property PACKAGE_PIN   V27              [get_ports {XCVR_CTRL_OUT[4]}]
set_property PACKAGE_PIN   W28              [get_ports {XCVR_CTRL_OUT[5]}]
set_property PACKAGE_PIN   W25              [get_ports {XCVR_CTRL_OUT[6]}]
set_property PACKAGE_PIN   W26              [get_ports {XCVR_CTRL_OUT[7]}]
set_property IOSTANDARD    LVCMOS18         [get_ports {XCVR_CTRL_OUT[*]}]
# Set pull-up on bits 6 and 7 to make the bitstream compatible with Rev A,
# where the signals don't exist.
set_property PULLUP        TRUE             [get_ports {XCVR_CTRL_OUT[6]}]
set_property PULLUP        TRUE             [get_ports {XCVR_CTRL_OUT[7]}]

## RX/TX LEDs, Bank 10, 3.3 V
set_property PACKAGE_PIN   AG14             [get_ports {RX1_GRN_ENA}]
set_property PACKAGE_PIN   AG17             [get_ports {RX2_GRN_ENA}]
set_property IOSTANDARD    LVCMOS33         [get_ports {RX*_GRN_ENA}]

set_property PACKAGE_PIN   AG16             [get_ports {TX1_RED_ENA}]
set_property PACKAGE_PIN   AF15             [get_ports {TX2_RED_ENA}]
set_property IOSTANDARD    LVCMOS33         [get_ports {TX*_RED_ENA}]

set_property PACKAGE_PIN   AG15             [get_ports {TXRX1_GRN_ENA}]
set_property PACKAGE_PIN   AF18             [get_ports {TXRX2_GRN_ENA}]
set_property IOSTANDARD    LVCMOS33         [get_ports {TXRX*_GRN_ENA}]


## PL DDR ####################################################################
##
############################################################################

## PL DDR, Bank 34,35

set_property PACKAGE_PIN   G9               [get_ports {SYS_CLK_N}]
set_property PACKAGE_PIN   H9               [get_ports {SYS_CLK_P}]
set_property PACKAGE_PIN   D8               [get_ports {DDR3_ADDR[0]}]
set_property PACKAGE_PIN   F9               [get_ports {DDR3_ADDR[1]}]
set_property PACKAGE_PIN   A7               [get_ports {DDR3_ADDR[2]}]
set_property PACKAGE_PIN   D9               [get_ports {DDR3_ADDR[3]}]
set_property PACKAGE_PIN   H7               [get_ports {DDR3_ADDR[4]}]
set_property PACKAGE_PIN   E8               [get_ports {DDR3_ADDR[5]}]
set_property PACKAGE_PIN   F8               [get_ports {DDR3_ADDR[6]}]
set_property PACKAGE_PIN   E7               [get_ports {DDR3_ADDR[7]}]
set_property PACKAGE_PIN   C6               [get_ports {DDR3_ADDR[8]}]
set_property PACKAGE_PIN   C7               [get_ports {DDR3_ADDR[9]}]
set_property PACKAGE_PIN   B7               [get_ports {DDR3_ADDR[10]}]
set_property PACKAGE_PIN   J9               [get_ports {DDR3_ADDR[11]}]
set_property PACKAGE_PIN   D11              [get_ports {DDR3_ADDR[12]}]
set_property PACKAGE_PIN   F7               [get_ports {DDR3_ADDR[13]}]
set_property PACKAGE_PIN   D6               [get_ports {DDR3_ADDR[14]}]
set_property PACKAGE_PIN   B6               [get_ports {DDR3_ADDR[15]}]
set_property PACKAGE_PIN   C9               [get_ports {DDR3_BA[0]}]
set_property PACKAGE_PIN   G7               [get_ports {DDR3_BA[1]}]
set_property PACKAGE_PIN   B9               [get_ports {DDR3_BA[2]}]
set_property PACKAGE_PIN   D10              [get_ports {DDR3_CAS_N}]
set_property PACKAGE_PIN   E11              [get_ports {DDR3_CKE[0]}]
set_property PACKAGE_PIN   H8               [get_ports {DDR3_CK_N[0]}]
set_property PACKAGE_PIN   J8               [get_ports {DDR3_CK_P[0]}]
set_property PACKAGE_PIN   E10              [get_ports {DDR3_CS_N[0]}]
set_property PACKAGE_PIN   D16              [get_ports {DDR3_DM[0]}]
set_property PACKAGE_PIN   C12              [get_ports {DDR3_DM[1]}]
set_property PACKAGE_PIN   J13              [get_ports {DDR3_DM[2]}]
set_property PACKAGE_PIN   F14              [get_ports {DDR3_DM[3]}]
set_property PACKAGE_PIN   B16              [get_ports {DDR3_DQ[0]}]
set_property PACKAGE_PIN   A17              [get_ports {DDR3_DQ[1]}]
set_property PACKAGE_PIN   C16              [get_ports {DDR3_DQ[2]}]
set_property PACKAGE_PIN   E15              [get_ports {DDR3_DQ[3]}]
set_property PACKAGE_PIN   C17              [get_ports {DDR3_DQ[4]}]
set_property PACKAGE_PIN   D14              [get_ports {DDR3_DQ[5]}]
set_property PACKAGE_PIN   B17              [get_ports {DDR3_DQ[6]}]
set_property PACKAGE_PIN   D15              [get_ports {DDR3_DQ[7]}]
set_property PACKAGE_PIN   B12              [get_ports {DDR3_DQ[8]}]
set_property PACKAGE_PIN   A13              [get_ports {DDR3_DQ[9]}]
set_property PACKAGE_PIN   C11              [get_ports {DDR3_DQ[10]}]
set_property PACKAGE_PIN   C14              [get_ports {DDR3_DQ[11]}]
set_property PACKAGE_PIN   A12              [get_ports {DDR3_DQ[12]}]
set_property PACKAGE_PIN   B14              [get_ports {DDR3_DQ[13]}]
set_property PACKAGE_PIN   B11              [get_ports {DDR3_DQ[14]}]
set_property PACKAGE_PIN   A14              [get_ports {DDR3_DQ[15]}]
set_property PACKAGE_PIN   J16              [get_ports {DDR3_DQ[16]}]
set_property PACKAGE_PIN   J14              [get_ports {DDR3_DQ[17]}]
set_property PACKAGE_PIN   L15              [get_ports {DDR3_DQ[18]}]
set_property PACKAGE_PIN   H14              [get_ports {DDR3_DQ[19]}]
set_property PACKAGE_PIN   K15              [get_ports {DDR3_DQ[20]}]
set_property PACKAGE_PIN   H13              [get_ports {DDR3_DQ[21]}]
set_property PACKAGE_PIN   L14              [get_ports {DDR3_DQ[22]}]
set_property PACKAGE_PIN   J15              [get_ports {DDR3_DQ[23]}]
set_property PACKAGE_PIN   E13              [get_ports {DDR3_DQ[24]}]
set_property PACKAGE_PIN   F15              [get_ports {DDR3_DQ[25]}]
set_property PACKAGE_PIN   F13              [get_ports {DDR3_DQ[26]}]
set_property PACKAGE_PIN   G16              [get_ports {DDR3_DQ[27]}]
set_property PACKAGE_PIN   E12              [get_ports {DDR3_DQ[28]}]
set_property PACKAGE_PIN   G15              [get_ports {DDR3_DQ[29]}]
set_property PACKAGE_PIN   D13              [get_ports {DDR3_DQ[30]}]
set_property PACKAGE_PIN   G14              [get_ports {DDR3_DQ[31]}]
set_property PACKAGE_PIN   E17              [get_ports {DDR3_DQS_N[0]}]
set_property PACKAGE_PIN   F17              [get_ports {DDR3_DQS_P[0]}]
set_property PACKAGE_PIN   A15              [get_ports {DDR3_DQS_N[1]}]
set_property PACKAGE_PIN   B15              [get_ports {DDR3_DQS_P[1]}]
set_property PACKAGE_PIN   K13              [get_ports {DDR3_DQS_N[2]}]
set_property PACKAGE_PIN   L13              [get_ports {DDR3_DQS_P[2]}]
set_property PACKAGE_PIN   F12              [get_ports {DDR3_DQS_N[3]}]
set_property PACKAGE_PIN   G12              [get_ports {DDR3_DQS_P[3]}]
set_property PACKAGE_PIN   G11              [get_ports {DDR3_ODT[0]}]
set_property PACKAGE_PIN   A10              [get_ports {DDR3_RAS_N}]
set_property PACKAGE_PIN   B10              [get_ports {DDR3_RESET_N}]
set_property PACKAGE_PIN   A9               [get_ports {DDR3_WE_N}]

## Front Panel GPIO ########################################################
##
############################################################################

# GPIO, Bank 12
set_property PACKAGE_PIN   AC26             [get_ports {EN_GPIO_2V5}]
set_property IOSTANDARD    LVCMOS18         [get_ports {EN_GPIO_2V5}]

set_property PACKAGE_PIN   AA29             [get_ports {EN_GPIO_3V3}]
set_property IOSTANDARD    LVCMOS18         [get_ports {EN_GPIO_3V3}]

set_property PACKAGE_PIN   AB30             [get_ports {EN_GPIO_VAR_SUPPLY}]
set_property IOSTANDARD    LVCMOS18         [get_ports {EN_GPIO_VAR_SUPPLY}]

set_property PACKAGE_PIN   AF27             [get_ports {GPIO_PREBUFF[0]}]
set_property PACKAGE_PIN   AF29             [get_ports {GPIO_PREBUFF[1]}]
set_property PACKAGE_PIN   AE25             [get_ports {GPIO_PREBUFF[2]}]
set_property PACKAGE_PIN   AF25             [get_ports {GPIO_PREBUFF[3]}]
set_property PACKAGE_PIN   AJ30             [get_ports {GPIO_PREBUFF[4]}]
set_property PACKAGE_PIN   AK30             [get_ports {GPIO_PREBUFF[5]}]
set_property PACKAGE_PIN   AJ28             [get_ports {GPIO_PREBUFF[6]}]
set_property PACKAGE_PIN   AJ29             [get_ports {GPIO_PREBUFF[7]}]

set_property PACKAGE_PIN   AA27             [get_ports {GPIO_DIR[0]}]
set_property PACKAGE_PIN   AA28             [get_ports {GPIO_DIR[1]}]
set_property PACKAGE_PIN   AB25             [get_ports {GPIO_DIR[2]}]
set_property PACKAGE_PIN   AB26             [get_ports {GPIO_DIR[3]}]
set_property PACKAGE_PIN   AE26             [get_ports {GPIO_DIR[4]}]
set_property PACKAGE_PIN   AB27             [get_ports {GPIO_DIR[5]}]
set_property PACKAGE_PIN   AC27             [get_ports {GPIO_DIR[6]}]
set_property PACKAGE_PIN   Y25              [get_ports {GPIO_DIR[7]}]

set_property PACKAGE_PIN   AD26             [get_ports {GPIO_OE_N}]

set_property IOSTANDARD    LVCMOS18         [get_ports {GPIO_*}]

## GPSDO ###################################################################
##
############################################################################

# GPSDO, Bank 13
set_property PACKAGE_PIN   R28              [get_ports {CLK_GPS_PWR_EN}]
set_property IOSTANDARD    LVCMOS18         [get_ports {CLK_GPS_PWR_EN}]

# GPSDO, Bank 10
set_property PACKAGE_PIN   AD14             [get_ports {GPS_ALARM}]
set_property PACKAGE_PIN   AD13             [get_ports {GPS_INITSURV_N}]
set_property PACKAGE_PIN   AH13             [get_ports {GPS_LOCK}]
set_property PACKAGE_PIN   AE12             [get_ports {GPS_PHASELOCK}]
set_property PACKAGE_PIN   AG12             [get_ports {GPS_RST_N}]
set_property PACKAGE_PIN   AH14             [get_ports {GPS_SURVEY}]
set_property PACKAGE_PIN   AF12             [get_ports {GPS_WARMUP}]

set_property IOSTANDARD    LVCMOS33         [get_ports {GPS_*}]

## Daughterboard Connections ###############################################
##
############################################################################

# Switches, Bank 9
set_property PACKAGE_PIN   Y20              [get_ports {FE1_SEL[0]}]
set_property PACKAGE_PIN   AA20             [get_ports {FE1_SEL[1]}]
set_property PACKAGE_PIN   AC18             [get_ports {FE1_SEL[2]}]
set_property PACKAGE_PIN   AA18             [get_ports {FE2_SEL[0]}]
set_property PACKAGE_PIN   AA19             [get_ports {FE2_SEL[1]}]
set_property PACKAGE_PIN   AB19             [get_ports {FE2_SEL[2]}]
set_property IOSTANDARD    LVCMOS33         [get_ports {FE*_SEL[*]}]

set_property PACKAGE_PIN   AC19             [get_ports {RX1_SEL[0]}]
set_property PACKAGE_PIN   AD18             [get_ports {RX1_SEL[1]}]
set_property PACKAGE_PIN   AB20             [get_ports {RX2_SEL[0]}]
set_property PACKAGE_PIN   AD20             [get_ports {RX2_SEL[1]}]
set_property IOSTANDARD    LVCMOS33         [get_ports {RX*_SEL[*]}]

# Switches, Bank 11
set_property PACKAGE_PIN   AB21             [get_ports {RX1_BSEL[0]}]
set_property PACKAGE_PIN   AB22             [get_ports {RX1_BSEL[1]}]
set_property PACKAGE_PIN   W21              [get_ports {RX1_BSEL[2]}]
set_property PACKAGE_PIN   Y21              [get_ports {RX1_BSEL[3]}]
set_property PACKAGE_PIN   AA24             [get_ports {RX1_BSEL[4]}]
set_property PACKAGE_PIN   AB24             [get_ports {RX1_BSEL[5]}]
set_property IOSTANDARD    LVCMOS18         [get_ports {RX1_BSEL[*]}]

set_property PACKAGE_PIN   AC24             [get_ports {RX2_BSEL[0]}]
set_property PACKAGE_PIN   AD24             [get_ports {RX2_BSEL[1]}]
set_property PACKAGE_PIN   AG24             [get_ports {RX2_BSEL[2]}]
set_property PACKAGE_PIN   AG25             [get_ports {RX2_BSEL[3]}]
set_property PACKAGE_PIN   AD21             [get_ports {RX2_BSEL[4]}]
set_property PACKAGE_PIN   AE21             [get_ports {RX2_BSEL[5]}]
set_property IOSTANDARD    LVCMOS18         [get_ports {RX2_BSEL[*]}]

set_property PACKAGE_PIN   AK17             [get_ports {TX1_BSEL[0]}]
set_property PACKAGE_PIN   AK18             [get_ports {TX1_BSEL[1]}]
set_property PACKAGE_PIN   AH19             [get_ports {TX1_BSEL[2]}]
set_property PACKAGE_PIN   AJ19             [get_ports {TX1_BSEL[3]}]
set_property PACKAGE_PIN   AF19             [get_ports {TX1_BSEL[4]}]
set_property PACKAGE_PIN   AG19             [get_ports {TX1_BSEL[5]}]
set_property IOSTANDARD    LVCMOS18         [get_ports {TX1_BSEL[*]}]

set_property PACKAGE_PIN   AK23             [get_ports {TX2_BSEL[0]}]
set_property PACKAGE_PIN   AJ23             [get_ports {TX2_BSEL[1]}]
set_property PACKAGE_PIN   AJ24             [get_ports {TX2_BSEL[2]}]
set_property PACKAGE_PIN   AH23             [get_ports {TX2_BSEL[3]}]
set_property PACKAGE_PIN   AH24             [get_ports {TX2_BSEL[4]}]
set_property PACKAGE_PIN   AG22             [get_ports {TX2_BSEL[5]}]
set_property IOSTANDARD    LVCMOS18         [get_ports {TX2_BSEL[*]}]

## PPS, REFCLK #############################################################
##
############################################################################

# RefClk, Bank 10
set_property PACKAGE_PIN   AF14             [get_ports {CLK_REF_RAW}]
set_property PACKAGE_PIN   AH17             [get_ports {CLK_REF_SEL}]
set_property IOSTANDARD    LVCMOS33         [get_ports {CLK_REF_*}]

# PPS, Bank 10
set_property PACKAGE_PIN   AD15             [get_ports {CLK_SYNC_EXT}]
set_property PACKAGE_PIN   AC14             [get_ports {CLK_SYNC_INT}]
set_property PACKAGE_PIN   AA15             [get_ports {CLK_SYNC_INT_RAW}]
set_property IOSTANDARD    LVCMOS33         [get_ports {CLK_SYNC_*}]

## REF CLK PLL, Bank 10
set_property PACKAGE_PIN   AE15             [get_ports {CLK_PLL_SCLK}]
set_property PACKAGE_PIN   AE16             [get_ports {CLK_PLL_SDATA}]
set_property PACKAGE_PIN   AE18             [get_ports {CLK_PLL_SLE}]
set_property IOSTANDARD    LVCMOS33         [get_ports {CLK_PLL_*}]
#
# Set pull-up on CLK_MUX_OUT to make the bitstream compatible with Rev A, where
# the signal doesn't exist.
set_property PACKAGE_PIN   AH18             [get_ports {CLK_MUX_OUT}]
set_property IOSTANDARD    LVCMOS33         [get_ports {CLK_MUX_OUT}]
set_property PULLUP        TRUE             [get_ports {CLK_MUX_OUT}]


############################################################################
##FIXME: Remove this?

## PS DDR
#set_property PACKAGE_PIN   K25              [get_ports DDR_MODCLK_P]
#set_property PACKAGE_PIN   J25              [get_ports DDR_MODCLK_N]
#set_property PACKAGE_PIN   M22              [get_ports PS_DDR3_CKE]
#set_property PACKAGE_PIN   F25              [get_ports PS_DDR3_RESET_N]
#set_property PACKAGE_PIN   J24              [get_ports PS_DDR3_ADDR[14]]
#set_property PACKAGE_PIN   H23              [get_ports PS_DDR3_ADDR[13]]
#set_property PACKAGE_PIN   K23              [get_ports PS_DDR3_ADDR[12]]
#set_property PACKAGE_PIN   H24              [get_ports PS_DDR3_ADDR[11]]
#set_property PACKAGE_PIN   G26              [get_ports PS_DDR3_ADDR[10]]
#set_property PACKAGE_PIN   J23              [get_ports PS_DDR3_ADDR[9]]
#set_property PACKAGE_PIN   F27              [get_ports PS_DDR3_ADDR[8]]
#set_property PACKAGE_PIN   K22              [get_ports PS_DDR3_ADDR[7]]
#set_property PACKAGE_PIN   H26              [get_ports PS_DDR3_ADDR[6]]
#set_property PACKAGE_PIN   G24              [get_ports PS_DDR3_ADDR[5]]
#set_property PACKAGE_PIN   J26              [get_ports PS_DDR3_ADDR[4]]
#set_property PACKAGE_PIN   G25              [get_ports PS_DDR3_ADDR[3]]
#set_property PACKAGE_PIN   L27              [get_ports PS_DDR3_ADDR[2]]
#set_property PACKAGE_PIN   K26              [get_ports PS_DDR3_ADDR[1]]
#set_property PACKAGE_PIN   L25              [get_ports PS_DDR3_ADDR[0]]
#set_property PACKAGE_PIN   K28              [get_ports PS_DDR3_DM[3]]
#set_property PACKAGE_PIN   H29              [get_ports PS_DDR3_DM[2]]
#set_property PACKAGE_PIN   B30              [get_ports PS_DDR3_DM[1]]
#set_property PACKAGE_PIN   C27              [get_ports PS_DDR3_DM[0]]
#set_property PACKAGE_PIN   M25              [get_ports PS_DDR3_BA[2]]
#set_property PACKAGE_PIN   M26              [get_ports PS_DDR3_BA[1]]
#set_property PACKAGE_PIN   M27              [get_ports PS_DDR3_BA[0]]
#set_property PACKAGE_PIN   L28              [get_ports PS_DDR3_DQS_P[3]]
#set_property PACKAGE_PIN   G29              [get_ports PS_DDR3_DQS_P[2]]
#set_property PACKAGE_PIN   C29              [get_ports PS_DDR3_DQS_P[1]]
#set_property PACKAGE_PIN   C26              [get_ports PS_DDR3_DQS_P[0]]
#set_property PACKAGE_PIN   L29              [get_ports PS_DDR3_DQS_N[3]]
#set_property PACKAGE_PIN   F29              [get_ports PS_DDR3_DQS_N[2]]
#set_property PACKAGE_PIN   B29              [get_ports PS_DDR3_DQS_N[1]]
#set_property PACKAGE_PIN   B26              [get_ports PS_DDR3_DQS_N[0]]
#set_property PACKAGE_PIN   M30              [get_ports PS_DDR3_DQ[31]]
#set_property PACKAGE_PIN   L30              [get_ports PS_DDR3_DQ[30]]
#set_property PACKAGE_PIN   M29              [get_ports PS_DDR3_DQ[29]]
#set_property PACKAGE_PIN   K30              [get_ports PS_DDR3_DQ[28]]
#set_property PACKAGE_PIN   J28              [get_ports PS_DDR3_DQ[27]]
#set_property PACKAGE_PIN   J30              [get_ports PS_DDR3_DQ[26]]
#set_property PACKAGE_PIN   K27              [get_ports PS_DDR3_DQ[25]]
#set_property PACKAGE_PIN   J29              [get_ports PS_DDR3_DQ[24]]
#set_property PACKAGE_PIN   F30              [get_ports PS_DDR3_DQ[23]]
#set_property PACKAGE_PIN   G30              [get_ports PS_DDR3_DQ[22]]
#set_property PACKAGE_PIN   F28              [get_ports PS_DDR3_DQ[21]]
#set_property PACKAGE_PIN   E30              [get_ports PS_DDR3_DQ[20]]
#set_property PACKAGE_PIN   E28              [get_ports PS_DDR3_DQ[19]]
#set_property PACKAGE_PIN   H28              [get_ports PS_DDR3_DQ[18]]
#set_property PACKAGE_PIN   G27              [get_ports PS_DDR3_DQ[17]]
#set_property PACKAGE_PIN   H27              [get_ports PS_DDR3_DQ[16]]
#set_property PACKAGE_PIN   D29              [get_ports PS_DDR3_DQ[15]]
#set_property PACKAGE_PIN   D28              [get_ports PS_DDR3_DQ[14]]
#set_property PACKAGE_PIN   D30              [get_ports PS_DDR3_DQ[13]]
#set_property PACKAGE_PIN   C28              [get_ports PS_DDR3_DQ[12]]
#set_property PACKAGE_PIN   A28              [get_ports PS_DDR3_DQ[11]]
#set_property PACKAGE_PIN   A30              [get_ports PS_DDR3_DQ[10]]
#set_property PACKAGE_PIN   A27              [get_ports PS_DDR3_DQ[9]]
#set_property PACKAGE_PIN   A29              [get_ports PS_DDR3_DQ[8]]
#set_property PACKAGE_PIN   E27              [get_ports PS_DDR3_DQ[7]]
#set_property PACKAGE_PIN   D26              [get_ports PS_DDR3_DQ[6]]
#set_property PACKAGE_PIN   E26              [get_ports PS_DDR3_DQ[5]]
#set_property PACKAGE_PIN   B25              [get_ports PS_DDR3_DQ[4]]
#set_property PACKAGE_PIN   D25              [get_ports PS_DDR3_DQ[3]]
#set_property PACKAGE_PIN   B27              [get_ports PS_DDR3_DQ[2]]
#set_property PACKAGE_PIN   E25              [get_ports PS_DDR3_DQ[1]]
#set_property PACKAGE_PIN   A25              [get_ports PS_DDR3_DQ[0]]
#set_property PACKAGE_PIN   L23              [get_ports PS_DDR3_ODT]
#set_property PACKAGE_PIN   N21              [get_ports PS_DDR3_VRN]
#set_property PACKAGE_PIN   M21              [get_ports PS_DDR3_VRP]
#set_property PACKAGE_PIN   N23              [get_ports PS_DDR3_WE_N]
#set_property PACKAGE_PIN   N22              [get_ports PS_DDR3_CS_N]
#set_property PACKAGE_PIN   M24              [get_ports PS_DDR3_CAS_N]
#set_property PACKAGE_PIN   N24              [get_ports PS_DDR3_RAS_N]

