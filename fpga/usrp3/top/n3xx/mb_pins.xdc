#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#
# Motherboard Pin Definitions for the N3xx Product Family.
#

## Bank 12: 3.3V Logic : ################################################################
## Front-panel GPIO
## FPGA test outputs
#########################################################################################

set_property PACKAGE_PIN   AF25             [get_ports {FPGA_GPIO[0]}]
set_property PACKAGE_PIN   AE25             [get_ports {FPGA_GPIO[1]}]
set_property PACKAGE_PIN   AG26             [get_ports {FPGA_GPIO[2]}]
set_property PACKAGE_PIN   AG27             [get_ports {FPGA_GPIO[3]}]
set_property PACKAGE_PIN   AE26             [get_ports {FPGA_GPIO[4]}]
set_property PACKAGE_PIN   AB26             [get_ports {FPGA_GPIO[5]}]
set_property PACKAGE_PIN   AF27             [get_ports {FPGA_GPIO[6]}]
set_property PACKAGE_PIN   AA27             [get_ports {FPGA_GPIO[7]}]
set_property PACKAGE_PIN   AE27             [get_ports {FPGA_GPIO[8]}]
set_property PACKAGE_PIN   AC26             [get_ports {FPGA_GPIO[9]}]
set_property PACKAGE_PIN   AD25             [get_ports {FPGA_GPIO[10]}]
set_property PACKAGE_PIN   AD26             [get_ports {FPGA_GPIO[11]}]
set_property IOSTANDARD    LVCMOS33         [get_ports {FPGA_GPIO[*]}]
set_property DRIVE         4                [get_ports {FPGA_GPIO[*]}]
set_property SLEW          SLOW             [get_ports {FPGA_GPIO[*]}]

# These pins should be commented out for release hardware.
set_property PACKAGE_PIN   Y30              [get_ports {FPGA_TEST[0]}]
set_property PACKAGE_PIN   AA30             [get_ports {FPGA_TEST[1]}]
set_property IOSTANDARD    LVCMOS33         [get_ports {FPGA_TEST[*]}]

# set_property PACKAGE_PIN   AH27             [get_ports {MGMT-GPIO0}]
# set_property PACKAGE_PIN   AH26             [get_ports {MGMT-GPIO1}]
# set_property PACKAGE_PIN   AC27             [get_ports {MGMT-JTAG-TCK}]
# set_property PACKAGE_PIN   AF29             [get_ports {MGMT-JTAG-TDI}]
# set_property PACKAGE_PIN   AG29             [get_ports {MGMT-JTAG-TDO}]
# set_property PACKAGE_PIN   AB27             [get_ports {MGMT-JTAG-TMS}]
# set_property PACKAGE_PIN   Y28              [get_ports {MGMT-SPI-LE}]
# set_property PACKAGE_PIN   AD28             [get_ports {MGMT-SPI-MISO}]
# set_property PACKAGE_PIN   AA28             [get_ports {MGMT-SPI-MOSI}]
# set_property PACKAGE_PIN   AE28             [get_ports {MGMT-SPI-RESET}]
# set_property PACKAGE_PIN   AC28             [get_ports {MGMT-SPI-SCLK}]
# When implemented, the MGMT signals need DRIVE and SLEW attributes applied.

# NPIO and QSFP located elsewhere


## Bank 9, 2.5V Logic : #################################################################
## All of these are inputs and all require internal termination to meet voltage
## swing requirements at the pin. These ports and buffers should always be instantiated
## to meet the internal termination requirement.
#########################################################################################

set_property PACKAGE_PIN   AC18             [get_ports FPGA_REFCLK_P]
set_property PACKAGE_PIN   AC19             [get_ports FPGA_REFCLK_N]
set_property IOSTANDARD    LVDS_25          [get_ports FPGA_REFCLK_*]
set_property DIFF_TERM     TRUE             [get_ports FPGA_REFCLK_*]

set_property PACKAGE_PIN   AD18             [get_ports NETCLK_REF_P]
set_property PACKAGE_PIN   AD19             [get_ports NETCLK_REF_N]
set_property DIFF_TERM     TRUE             [get_ports NETCLK_REF_*]
set_property IOSTANDARD    LVDS_25          [get_ports NETCLK_REF_*]

set_property PACKAGE_PIN   AA18             [get_ports WB_20MHZ_P]
set_property PACKAGE_PIN   AA19             [get_ports WB_20MHZ_N]
set_property DIFF_TERM     TRUE             [get_ports WB_20MHZ_*]
set_property IOSTANDARD    LVDS_25          [get_ports WB_20MHZ_*]


## Bank 13, 3.3V : ######################################################################
## PPS In/Out (including MGMT PPS, unused)
## GPS PPS Raw/Disciplined
## RJ45 signaling (unused)
## Authentication IC
## Resets
## Rear-panel LEDs
#########################################################################################

set_property PACKAGE_PIN   U24              [get_ports REF_1PPS_IN]
set_property IOSTANDARD    LVCMOS33         [get_ports REF_1PPS_IN]

set_property PACKAGE_PIN   V29              [get_ports REF_1PPS_OUT]
set_property IOSTANDARD    LVCMOS33         [get_ports REF_1PPS_OUT]
set_property DRIVE         12               [get_ports REF_1PPS_OUT]
set_property SLEW          SLOW             [get_ports REF_1PPS_OUT]
set_property IOB           TRUE             [get_ports REF_1PPS_OUT]

# set_property PACKAGE_PIN   U29              [get_ports REF_1PPS_IN_MGMT]
# set_property IOSTANDARD    LVCMOS33         [get_ports REF_1PPS_IN_MGMT]

set_property PACKAGE_PIN   W30              [get_ports GPS_1PPS]
set_property IOSTANDARD    LVCMOS33         [get_ports GPS_1PPS]

# set_property PACKAGE_PIN   V28              [get_ports GPS_1PPS_RAW]
# set_property IOSTANDARD    LVCMOS33         [get_ports GPS_1PPS_RAW]

set_property PACKAGE_PIN   U26              [get_ports ENET0_CLK125]
set_property IOSTANDARD    LVCMOS33         [get_ports ENET0_CLK125]

# set_property PACKAGE_PIN   R25              [get_ports ENET0_PTP]
# set_property IOSTANDARD    LVCMOS33         [get_ports ENET0_PTP]

# set_property PACKAGE_PIN   R30              [get_ports ENET0_PTP_DIR]
# set_property IOSTANDARD    LVCMOS33         [get_ports ENET0_PTP_DIR]

# set_property PACKAGE_PIN   U30              [get_ports ATSHA204_SDA]
# set_property IOSTANDARD    LVCMOS33         [get_ports ATSHA204_SDA]

set_property PACKAGE_PIN   P26              [get_ports FPGA_PL_RESETN]
set_property IOSTANDARD    LVCMOS33         [get_ports FPGA_PL_RESETN]

set_property PACKAGE_PIN   U25              [get_ports PANEL_LED_GPS]
set_property PACKAGE_PIN   T25              [get_ports PANEL_LED_LINK]
set_property PACKAGE_PIN   W29              [get_ports PANEL_LED_PPS]
set_property PACKAGE_PIN   V24              [get_ports PANEL_LED_REF]
set_property IOSTANDARD    LVCMOS33         [get_ports PANEL_LED_*]
set_property DRIVE         4                [get_ports PANEL_LED_*]
set_property SLEW          SLOW             [get_ports PANEL_LED_*]

# SFP+ Sideband and White Rabbit DAC Control located elsewhere


## NanoPitch Interface : ################################################################
## Bank 12, 3.3V
#########################################################################################

# set_property PACKAGE_PIN   AD29             [get_ports {NPIO-GPIO0}]
# set_property PACKAGE_PIN   AC29             [get_ports {NPIO-GPIO1}]
# set_property PACKAGE_PIN   AE30             [get_ports {NPIO-GPIO2}]
# set_property PACKAGE_PIN   AD30             [get_ports {NPIO-GPIO3}]
# set_property PACKAGE_PIN   AH29             [get_ports {NPIO-GPIO4}]
# set_property PACKAGE_PIN   AH28             [get_ports {NPIO-GPIO5}]
# set_property PACKAGE_PIN   AF30             [get_ports {NPIO-GPIO6}]
# set_property PACKAGE_PIN   AG30             [get_ports {NPIO-GPIO7}]
# When implemented, the QSFP signals need DRIVE and SLEW attributes applied.

set_property PACKAGE_PIN   AE8              [get_ports {NPIO_RX0_P}]
set_property PACKAGE_PIN   AE7              [get_ports {NPIO_RX0_N}]
set_property PACKAGE_PIN   AG8              [get_ports {NPIO_RX1_P}]
set_property PACKAGE_PIN   AG7              [get_ports {NPIO_RX1_N}]
set_property PACKAGE_PIN   AK2              [get_ports {NPIO_TX0_P}]
set_property PACKAGE_PIN   AK1              [get_ports {NPIO_TX0_N}]
set_property PACKAGE_PIN   AJ4              [get_ports {NPIO_TX1_P}]
set_property PACKAGE_PIN   AJ3              [get_ports {NPIO_TX1_N}]


## QSFP : ###############################################################################
## Bank 12, 3.3V
#########################################################################################

set_property PACKAGE_PIN   AJ26             [get_ports {QSFP_I2C_SCL}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_I2C_SCL}]
set_property DRIVE         8                [get_ports {QSFP_I2C_SCL}]

set_property PACKAGE_PIN   AK26             [get_ports {QSFP_I2C_SDA}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_I2C_SDA}]
set_property DRIVE         8                [get_ports {QSFP_I2C_SDA}]

set_property PACKAGE_PIN   AK28             [get_ports {QSFP_INT_B}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_INT_B}]

set_property PACKAGE_PIN   AK30             [get_ports {QSFP_LED}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_LED}]

set_property PACKAGE_PIN   AJ29             [get_ports {QSFP_LPMODE}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_LPMODE}]

set_property PACKAGE_PIN   AK27             [get_ports {QSFP_PRESENT_B}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_PRESENT_B}]

set_property PACKAGE_PIN   AJ28             [get_ports {QSFP_MODSEL_B}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_MODSEL_B}]

set_property PACKAGE_PIN   AJ30             [get_ports {QSFP_RESET_B}]
set_property IOSTANDARD    LVCMOS33         [get_ports {QSFP_RESET_B}]

# When implemented, the QSFP signals need DRIVE and SLEW attributes applied.

set_property PACKAGE_PIN   AD5              [get_ports {QSFP_RX_N[0]}]
set_property PACKAGE_PIN   AD6              [get_ports {QSFP_RX_P[0]}]
set_property PACKAGE_PIN   AF5              [get_ports {QSFP_RX_N[1]}]
set_property PACKAGE_PIN   AF6              [get_ports {QSFP_RX_P[1]}]
set_property PACKAGE_PIN   AG3              [get_ports {QSFP_RX_N[2]}]
set_property PACKAGE_PIN   AG4              [get_ports {QSFP_RX_P[2]}]
set_property PACKAGE_PIN   AH5              [get_ports {QSFP_RX_N[3]}]
set_property PACKAGE_PIN   AH6              [get_ports {QSFP_RX_P[3]}]
set_property PACKAGE_PIN   AD1              [get_ports {QSFP_TX_N[0]}]
set_property PACKAGE_PIN   AD2              [get_ports {QSFP_TX_P[0]}]
set_property PACKAGE_PIN   AE3              [get_ports {QSFP_TX_N[1]}]
set_property PACKAGE_PIN   AE4              [get_ports {QSFP_TX_P[1]}]
set_property PACKAGE_PIN   AF1              [get_ports {QSFP_TX_N[2]}]
set_property PACKAGE_PIN   AF2              [get_ports {QSFP_TX_P[2]}]
set_property PACKAGE_PIN   AH1              [get_ports {QSFP_TX_N[3]}]
set_property PACKAGE_PIN   AH2              [get_ports {QSFP_TX_P[3]}]


## White Rabbit : #######################################################################
## Bank 13, 3.3V
#########################################################################################

set_property PACKAGE_PIN   T29              [get_ports {WB_DAC_DIN}]
set_property PACKAGE_PIN   T28              [get_ports {WB_DAC_NCLR}]
set_property PACKAGE_PIN   T30              [get_ports {WB_DAC_NLDAC}]
set_property PACKAGE_PIN   N29              [get_ports {WB_DAC_NSYNC}]
set_property PACKAGE_PIN   P29              [get_ports {WB_DAC_SCLK}]
set_property IOSTANDARD    LVCMOS33         [get_ports {WB_DAC_*}]
set_property DRIVE         4                [get_ports {WB_DAC_*}]
set_property SLEW          SLOW             [get_ports {WB_DAC_*}]


## SFP+ : ###############################################################################
##
#########################################################################################

## Clocks, Bank 109 and 110

# These need to have the internal buffer in the FPGA enabled at all times to avoid
# damage to the part. Therefore declare them here in the top level pins file.
set_property PACKAGE_PIN   AA8              [get_ports {MGT156MHZ_CLK1_P}]
set_property PACKAGE_PIN   AA7              [get_ports {MGT156MHZ_CLK1_N}]

set_property PACKAGE_PIN   AF10             [get_ports {NETCLK_P}]
set_property PACKAGE_PIN   AF9              [get_ports {NETCLK_N}]

# Swapping SFP_0 and SFP_1 pinout to match the label on the silkscreen.
# These FPGA pins are reversed with respect to the schematic now.
## MGTs, Bank 109

set_property PACKAGE_PIN   AJ7              [get_ports SFP_0_RX_N]
set_property PACKAGE_PIN   AJ8              [get_ports SFP_0_RX_P]
set_property PACKAGE_PIN   AK5              [get_ports SFP_0_TX_N]
set_property PACKAGE_PIN   AK6              [get_ports SFP_0_TX_P]

set_property PACKAGE_PIN   AH9              [get_ports SFP_1_RX_N]
set_property PACKAGE_PIN   AH10             [get_ports SFP_1_RX_P]
set_property PACKAGE_PIN   AK9              [get_ports SFP_1_TX_N]
set_property PACKAGE_PIN   AK10             [get_ports SFP_1_TX_P]

## SFP+ 0, Sideband, Bank 13 3.3V

set_property PACKAGE_PIN   T27              [get_ports {SFP_0_I2C_NPRESENT}]
set_property IOSTANDARD    LVCMOS33         [get_ports {SFP_0_I2C_NPRESENT}]

set_property PACKAGE_PIN   N27              [get_ports SFP_0_LED_A]
set_property PACKAGE_PIN   N28              [get_ports SFP_0_LED_B]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_0_LED_*]
set_property DRIVE         4                [get_ports SFP_0_LED_*]
set_property SLEW          SLOW             [get_ports SFP_0_LED_*]

set_property PACKAGE_PIN   R27              [get_ports SFP_0_LOS]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_0_LOS]

set_property PACKAGE_PIN   R26              [get_ports SFP_0_RS0]
set_property PACKAGE_PIN   P28              [get_ports SFP_0_RS1]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_0_RS*]
set_property DRIVE         4                [get_ports SFP_0_RS*]
set_property SLEW          SLOW             [get_ports SFP_0_RS*]

set_property PACKAGE_PIN   U27              [get_ports SFP_0_TXDISABLE]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_0_TXDISABLE]
set_property DRIVE         4                [get_ports SFP_0_TXDISABLE]
set_property SLEW          SLOW             [get_ports SFP_0_TXDISABLE]

set_property PACKAGE_PIN   V26              [get_ports SFP_0_TXFAULT]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_0_TXFAULT]

## SFP+ 1, Slow Speed, Bank 13 3.3V

# set_property PACKAGE_PIN   V23              [get_ports {SFP_1_I2C_NPRESENT}]
# set_property IOSTANDARD    LVCMOS33         [get_ports {SFP_1_I2C_NPRESENT}]

set_property PACKAGE_PIN   N26              [get_ports SFP_1_LED_A]
set_property PACKAGE_PIN   P30              [get_ports SFP_1_LED_B]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_1_LED_*]
set_property DRIVE         4                [get_ports SFP_1_LED_*]
set_property SLEW          SLOW             [get_ports SFP_1_LED_*]

set_property PACKAGE_PIN   R28              [get_ports SFP_1_LOS]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_1_LOS]

set_property PACKAGE_PIN   T24              [get_ports SFP_1_RS0]
set_property PACKAGE_PIN   P25              [get_ports SFP_1_RS1]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_1_RS*]
set_property DRIVE         4                [get_ports SFP_1_RS*]
set_property SLEW          SLOW             [get_ports SFP_1_RS*]

set_property PACKAGE_PIN   V27              [get_ports SFP_1_TXDISABLE]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_1_TXDISABLE]
set_property DRIVE         4                [get_ports SFP_1_TXDISABLE]
set_property SLEW          SLOW             [get_ports SFP_1_TXDISABLE]

set_property PACKAGE_PIN   W24              [get_ports SFP_1_TXFAULT]
set_property IOSTANDARD    LVCMOS33         [get_ports SFP_1_TXFAULT]


## PL DDR : #############################################################################
##
#########################################################################################

# This port must be always enabled due to a Xilinx bug in the silicon.
# https://www.xilinx.com/support/answers/63950.html
set_property PACKAGE_PIN   A8               [get_ports FPGA_PUDC_B]
set_property IOSTANDARD    LVDCI_15         [get_ports FPGA_PUDC_B]

set_property PACKAGE_PIN   D8               [get_ports {ddr3_addr[0]}]
set_property PACKAGE_PIN   A7               [get_ports {ddr3_addr[1]}]
set_property PACKAGE_PIN   C7               [get_ports {ddr3_addr[2]}]
set_property PACKAGE_PIN   D9               [get_ports {ddr3_addr[3]}]
set_property PACKAGE_PIN   J9               [get_ports {ddr3_addr[4]}]
set_property PACKAGE_PIN   E8               [get_ports {ddr3_addr[5]}]
set_property PACKAGE_PIN   G7               [get_ports {ddr3_addr[6]}]
set_property PACKAGE_PIN   E7               [get_ports {ddr3_addr[7]}]
set_property PACKAGE_PIN   G11              [get_ports {ddr3_addr[8]}]
set_property PACKAGE_PIN   C6               [get_ports {ddr3_addr[9]}]
set_property PACKAGE_PIN   B6               [get_ports {ddr3_addr[10]}]
set_property PACKAGE_PIN   H7               [get_ports {ddr3_addr[11]}]
set_property PACKAGE_PIN   B7               [get_ports {ddr3_addr[12]}]
set_property PACKAGE_PIN   F7               [get_ports {ddr3_addr[13]}]
set_property PACKAGE_PIN   F8               [get_ports {ddr3_addr[14]}]
set_property PACKAGE_PIN   F9               [get_ports {ddr3_addr[15]}]

set_property PACKAGE_PIN   C9               [get_ports {ddr3_ba[0]}]
set_property PACKAGE_PIN   E10              [get_ports {ddr3_ba[1]}]
set_property PACKAGE_PIN   B9               [get_ports {ddr3_ba[2]}]

set_property PACKAGE_PIN   A10              [get_ports ddr3_cas_n]
set_property PACKAGE_PIN   E11              [get_ports {ddr3_cke[0]}]
set_property PACKAGE_PIN   H8               [get_ports {ddr3_ck_n[0]}]
set_property PACKAGE_PIN   J8               [get_ports {ddr3_ck_p[0]}]
set_property PACKAGE_PIN   D11              [get_ports {ddr3_cs_n[0]}]

set_property PACKAGE_PIN   B16              [get_ports {ddr3_dm[0]}]
set_property PACKAGE_PIN   B11              [get_ports {ddr3_dm[1]}]
set_property PACKAGE_PIN   H13              [get_ports {ddr3_dm[2]}]
set_property PACKAGE_PIN   G15              [get_ports {ddr3_dm[3]}]

set_property PACKAGE_PIN   B17              [get_ports {ddr3_dq[0]}]
set_property PACKAGE_PIN   A17              [get_ports {ddr3_dq[1]}]
set_property PACKAGE_PIN   D15              [get_ports {ddr3_dq[2]}]
set_property PACKAGE_PIN   D14              [get_ports {ddr3_dq[3]}]
set_property PACKAGE_PIN   C17              [get_ports {ddr3_dq[4]}]
set_property PACKAGE_PIN   E15              [get_ports {ddr3_dq[5]}]
set_property PACKAGE_PIN   C16              [get_ports {ddr3_dq[6]}]
set_property PACKAGE_PIN   D16              [get_ports {ddr3_dq[7]}]
set_property PACKAGE_PIN   A13              [get_ports {ddr3_dq[8]}]
set_property PACKAGE_PIN   A12              [get_ports {ddr3_dq[9]}]
set_property PACKAGE_PIN   C14              [get_ports {ddr3_dq[10]}]
set_property PACKAGE_PIN   B12              [get_ports {ddr3_dq[11]}]
set_property PACKAGE_PIN   B14              [get_ports {ddr3_dq[12]}]
set_property PACKAGE_PIN   C12              [get_ports {ddr3_dq[13]}]
set_property PACKAGE_PIN   A14              [get_ports {ddr3_dq[14]}]
set_property PACKAGE_PIN   C11              [get_ports {ddr3_dq[15]}]
set_property PACKAGE_PIN   J15              [get_ports {ddr3_dq[16]}]
set_property PACKAGE_PIN   L14              [get_ports {ddr3_dq[17]}]
set_property PACKAGE_PIN   L15              [get_ports {ddr3_dq[18]}]
set_property PACKAGE_PIN   J13              [get_ports {ddr3_dq[19]}]
set_property PACKAGE_PIN   J14              [get_ports {ddr3_dq[20]}]
set_property PACKAGE_PIN   K15              [get_ports {ddr3_dq[21]}]
set_property PACKAGE_PIN   J16              [get_ports {ddr3_dq[22]}]
set_property PACKAGE_PIN   H14              [get_ports {ddr3_dq[23]}]
set_property PACKAGE_PIN   F15              [get_ports {ddr3_dq[24]}]
set_property PACKAGE_PIN   G16              [get_ports {ddr3_dq[25]}]
set_property PACKAGE_PIN   F14              [get_ports {ddr3_dq[26]}]
set_property PACKAGE_PIN   E13              [get_ports {ddr3_dq[27]}]
set_property PACKAGE_PIN   G14              [get_ports {ddr3_dq[28]}]
set_property PACKAGE_PIN   D13              [get_ports {ddr3_dq[29]}]
set_property PACKAGE_PIN   F13              [get_ports {ddr3_dq[30]}]
set_property PACKAGE_PIN   E12              [get_ports {ddr3_dq[31]}]

set_property PACKAGE_PIN   F17              [get_ports {ddr3_dqs_p[0]}]
set_property PACKAGE_PIN   E17              [get_ports {ddr3_dqs_n[0]}]
set_property PACKAGE_PIN   B15              [get_ports {ddr3_dqs_p[1]}]
set_property PACKAGE_PIN   A15              [get_ports {ddr3_dqs_n[1]}]
set_property PACKAGE_PIN   L13              [get_ports {ddr3_dqs_p[2]}]
set_property PACKAGE_PIN   K13              [get_ports {ddr3_dqs_n[2]}]
set_property PACKAGE_PIN   F12              [get_ports {ddr3_dqs_n[3]}]
set_property PACKAGE_PIN   G12              [get_ports {ddr3_dqs_p[3]}]

set_property PACKAGE_PIN   D10              [get_ports {ddr3_odt[0]}]
set_property PACKAGE_PIN   B10              [get_ports ddr3_ras_n]
set_property PACKAGE_PIN   D6               [get_ports ddr3_reset_n]
set_property PACKAGE_PIN   G9               [get_ports sys_clk_n]
set_property PACKAGE_PIN   H9               [get_ports sys_clk_p]
set_property PACKAGE_PIN   A9               [get_ports ddr3_we_n]
