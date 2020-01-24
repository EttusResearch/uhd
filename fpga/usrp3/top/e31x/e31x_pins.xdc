###############################################################################
# Pin mapping
###############################################################################
## RF board connector pins

# Pin 1
set_property PACKAGE_PIN H19 [get_ports {TX_BANDSEL[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {TX_BANDSEL[2]}]

# Pin 2
# 3.3v DB

# Pin 3
set_property PACKAGE_PIN F19 [get_ports {RX1B_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1B_BANDSEL[0]}]

#Pin 4
# 3.3v DB

#Pin 5
set_property PACKAGE_PIN G19 [get_ports {RX1B_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1B_BANDSEL[1]}]

#Pin 6
set_property PACKAGE_PIN E19 [get_ports {RX1_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1_BANDSEL[0]}]

#Pin 7
set_property PACKAGE_PIN E20 [get_ports VCTXRX2_V2]
set_property IOSTANDARD LVCMOS33 [get_ports VCTXRX2_V2]

#Pin 8
set_property PACKAGE_PIN G21 [get_ports {RX1_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1_BANDSEL[1]}]

#Pin 9
set_property PACKAGE_PIN G22 [get_ports TX_ENABLE1A]
set_property IOSTANDARD LVCMOS33 [get_ports TX_ENABLE1A]

#Pin 10
set_property PACKAGE_PIN G20 [get_ports {RX1_BANDSEL[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1_BANDSEL[2]}]

#Pin 11
set_property PACKAGE_PIN H22 [get_ports TX_ENABLE2A]
set_property IOSTANDARD LVCMOS33 [get_ports TX_ENABLE2A]

#Pin 12
set_property PACKAGE_PIN F22 [get_ports {TX_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {TX_BANDSEL[0]}]

#Pin 13
set_property PACKAGE_PIN A17 [get_ports TX_ENABLE1B]
set_property IOSTANDARD LVCMOS33 [get_ports TX_ENABLE1B]

#Pin 14
set_property PACKAGE_PIN F21 [get_ports {TX_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {TX_BANDSEL[1]}]

#Pin 15
set_property PACKAGE_PIN B16 [get_ports TX_ENABLE2B]
set_property IOSTANDARD LVCMOS33 [get_ports TX_ENABLE2B]

#Pin 16 -- Not used
#set_property PACKAGE_PIN J21 [get_ports DB_SCL]
#set_property IOSTANDARD LVCMOS18 [get_ports DB_SCL]

#Pin 17
set_property PACKAGE_PIN A19 [get_ports {RX1C_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1C_BANDSEL[0]}]

#Pin 18 -- Not used
#set_property PACKAGE_PIN J22 [get_ports DB_SDA]
#set_property IOSTANDARD LVCMOS18 [get_ports DB_SDA]

#Pin 19
set_property PACKAGE_PIN B15 [get_ports {RX1C_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RX1C_BANDSEL[1]}]

#Pin 20
set_property PACKAGE_PIN K21 [get_ports TCXO_DAC_SYNC_N]
set_property IOSTANDARD LVCMOS18 [get_ports TCXO_DAC_SYNC_N]

#Pin 21
set_property PACKAGE_PIN A16 [get_ports VCTXRX2_V1]
set_property IOSTANDARD LVCMOS33 [get_ports VCTXRX2_V1]

#Pin 22
set_property PACKAGE_PIN L22 [get_ports TCXO_DAC_SCLK]
set_property IOSTANDARD LVCMOS18 [get_ports TCXO_DAC_SCLK]

#Pin 23
set_property PACKAGE_PIN B17 [get_ports VCTXRX1_V2]
set_property IOSTANDARD LVCMOS33 [get_ports VCTXRX1_V2]

#Pin 24
set_property PACKAGE_PIN L21 [get_ports TCXO_DAC_SDIN]
set_property IOSTANDARD LVCMOS18 [get_ports TCXO_DAC_SDIN]

#Pin 25
set_property PACKAGE_PIN C15 [get_ports VCTXRX1_V1]
set_property IOSTANDARD LVCMOS33 [get_ports VCTXRX1_V1]

#Pin 26
set_property PACKAGE_PIN R18 [get_ports {DB_EXP_1_8V[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[5]}]

#Pin 27
set_property PACKAGE_PIN E18 [get_ports VCRX1_V1]
set_property IOSTANDARD LVCMOS33 [get_ports VCRX1_V1]

#Pin 28
set_property PACKAGE_PIN T18 [get_ports {DB_EXP_1_8V[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[6]}]

#Pin 29
set_property PACKAGE_PIN F18 [get_ports VCRX1_V2]
set_property IOSTANDARD LVCMOS33 [get_ports VCRX1_V2]

#Pin 30
set_property PACKAGE_PIN M20 [get_ports TCXO_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports TCXO_CLK]
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets TCXO_CLK]

#Pin 31
set_property PACKAGE_PIN F17 [get_ports VCRX2_V1]
set_property IOSTANDARD LVCMOS33 [get_ports VCRX2_V1]

#Pin 32
set_property PACKAGE_PIN M15 [get_ports {DB_EXP_1_8V[8]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[8]}]

#Pin 33
set_property PACKAGE_PIN G17 [get_ports VCRX2_V2]
set_property IOSTANDARD LVCMOS33 [get_ports VCRX2_V2]

#Pin 34
set_property PACKAGE_PIN J18 [get_ports {DB_EXP_1_8V[9]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[9]}]

#Pin 35
set_property PACKAGE_PIN U5 [get_ports {CAT_CTRL_IN[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_IN[2]}]

#Pin 36
set_property PACKAGE_PIN J20 [get_ports {DB_EXP_1_8V[10]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[10]}]

#Pin 37
set_property PACKAGE_PIN U6 [get_ports {CAT_CTRL_IN[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_IN[3]}]

#Pin 38
set_property PACKAGE_PIN K19 [get_ports {DB_EXP_1_8V[11]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[11]}]

#Pin 39
set_property PACKAGE_PIN AB5 [get_ports {CAT_CTRL_OUT[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[0]}]

#Pin 40
set_property PACKAGE_PIN K20 [get_ports {CAT_CTRL_OUT[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[4]}]

#Pin 41
set_property PACKAGE_PIN AB6 [get_ports {CAT_CTRL_OUT[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[1]}]

#Pin 42
set_property PACKAGE_PIN L19 [get_ports {CAT_CTRL_OUT[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[5]}]

#Pin 43
set_property PACKAGE_PIN AB7 [get_ports {CAT_CTRL_OUT[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[2]}]

#Pin 44
set_property PACKAGE_PIN V12 [get_ports {CAT_CTRL_OUT[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[6]}]

#Pin 45
set_property PACKAGE_PIN AA4 [get_ports {CAT_CTRL_OUT[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[3]}]

#Pin 46
set_property PACKAGE_PIN W12 [get_ports {CAT_CTRL_OUT[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_OUT[7]}]

#Pin 47
set_property PACKAGE_PIN T6 [get_ports {DB_EXP_1_8V[31]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[31]}]

#Pin 48
set_property PACKAGE_PIN U11 [get_ports CAT_RESET]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_RESET]

#Pin 49
# 1.8V

#Pin 50
set_property PACKAGE_PIN W6 [get_ports CAT_CS]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_CS]

#Pin 51
# 1.8V
#Pin 52
set_property PACKAGE_PIN W5 [get_ports CAT_SCLK]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_SCLK]

#Pin 53
# 5V

#Pin 54
set_property PACKAGE_PIN V7 [get_ports CAT_MOSI]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_MOSI]

#Pin 55
# 5V

#Pin 56
set_property PACKAGE_PIN W7 [get_ports CAT_MISO]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_MISO]

#Pin 57
# 5V

#Pin 58
set_property PACKAGE_PIN V4 [get_ports {CAT_CTRL_IN[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_IN[0]}]

#Pin 59
# 5V

#Pin 60
set_property PACKAGE_PIN V5 [get_ports {CAT_CTRL_IN[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_CTRL_IN[1]}]

#Pin 61
# 1.8V

#Pin 62
set_property PACKAGE_PIN U4 [get_ports {DB_EXP_1_8V[33]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[33]}]

#Pin 63
# 1.8V

#Pin 64
set_property PACKAGE_PIN T4 [get_ports {DB_EXP_1_8V[34]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[34]}]
#Pin 65
# GND

#Pin 66
set_property PACKAGE_PIN R6 [get_ports {DB_EXP_1_8V[32]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[32]}]

#Pin 67
set_property PACKAGE_PIN AB1 [get_ports CAT_TXNRX]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_TXNRX]

#Pin 68
# GND

#Pin 69
set_property PACKAGE_PIN AB4 [get_ports CAT_ENABLE]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_ENABLE]

#Pin 70
set_property PACKAGE_PIN M19 [get_ports CAT_BBCLK_OUT]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_BBCLK_OUT]

#Pin 71
set_property PACKAGE_PIN AB2 [get_ports CAT_ENAGC]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_ENAGC]

#Pin 72
# GND

#Pin 73
# GND

#Pin 74
set_property PACKAGE_PIN T16 [get_ports CAT_SYNC]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_SYNC]

#Pin 78
set_property PACKAGE_PIN N15 [get_ports {CAT_P1_D[11]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[11]}]

#Pin 76
# GND

#Pin 100
set_property PACKAGE_PIN N22 [get_ports {CAT_P1_D[10]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[10]}]

#Pin 93
set_property PACKAGE_PIN M17 [get_ports {CAT_P0_D[11]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[11]}]

#Pin 96
set_property PACKAGE_PIN T17 [get_ports {CAT_P1_D[9]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[9]}]

#Pin 95
set_property PACKAGE_PIN N17 [get_ports {CAT_P0_D[10]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[10]}]

#Pin 98
set_property PACKAGE_PIN M22 [get_ports {CAT_P1_D[8]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[8]}]

#Pin 81
set_property PACKAGE_PIN K15 [get_ports {CAT_P0_D[9]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[9]}]

#Pin 92
set_property PACKAGE_PIN P21 [get_ports {CAT_P1_D[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[7]}]

#Pin 97
set_property PACKAGE_PIN N20 [get_ports {CAT_P0_D[8]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[8]}]

#Pin 94
set_property PACKAGE_PIN R20 [get_ports {CAT_P1_D[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[6]}]

#Pin 77
set_property PACKAGE_PIN J16 [get_ports {CAT_P0_D[7]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[7]}]

#Pin 86
set_property PACKAGE_PIN P18 [get_ports {CAT_P1_D[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[5]}]

#Pin 85
set_property PACKAGE_PIN K16 [get_ports {CAT_P0_D[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[6]}]

#Pin 90
set_property PACKAGE_PIN P17 [get_ports {CAT_P1_D[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[4]}]

#Pin 75
set_property PACKAGE_PIN J15 [get_ports {CAT_P0_D[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[5]}]

#Pin 82
set_property PACKAGE_PIN P15 [get_ports {CAT_P1_D[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[3]}]

#Pin 91
set_property PACKAGE_PIN M16 [get_ports {CAT_P0_D[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[4]}]

#Pin 88
set_property PACKAGE_PIN P20 [get_ports {CAT_P1_D[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[2]}]

#Pin 79
set_property PACKAGE_PIN J17 [get_ports {CAT_P0_D[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[3]}]

#Pin 80
set_property PACKAGE_PIN M21 [get_ports {CAT_P1_D[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[1]}]

#Pin 89
set_property PACKAGE_PIN L17 [get_ports {CAT_P0_D[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[2]}]

#Pin 84
set_property PACKAGE_PIN N19 [get_ports {CAT_P1_D[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P1_D[0]}]

#Pin 83
set_property PACKAGE_PIN K18 [get_ports {CAT_P0_D[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[1]}]

#Pin 102
set_property PACKAGE_PIN P22 [get_ports CAT_TX_FRAME]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_TX_FRAME]

#Pin 87
set_property PACKAGE_PIN L16 [get_ports {CAT_P0_D[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {CAT_P0_D[0]}]

#Pin 104
set_property PACKAGE_PIN R21 [get_ports CAT_FB_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_FB_CLK]

#Pin 99
set_property PACKAGE_PIN N18 [get_ports CAT_RX_FRAME]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_RX_FRAME]

#Pin 103
# GND

#Pin 101
set_property PACKAGE_PIN L18 [get_ports CAT_DATA_CLK]
set_property IOSTANDARD LVCMOS18 [get_ports CAT_DATA_CLK]
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets CAT_DATA_CLK]

#Pin 105
# 1.8V

#Pin 106
# GND

#Pin 107
set_property PACKAGE_PIN AA8 [get_ports {RX2_BANDSEL[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2_BANDSEL[2]}]

#Pin 108
set_property PACKAGE_PIN Y11 [get_ports LED_TXRX1_TX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_TXRX1_TX]

#Pin 109
set_property PACKAGE_PIN AA9 [get_ports {RX2_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2_BANDSEL[1]}]

#Pin 110
set_property PACKAGE_PIN AB10 [get_ports LED_TXRX1_RX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_TXRX1_RX]

#Pin 111
set_property PACKAGE_PIN AB9 [get_ports {RX2_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2_BANDSEL[0]}]

#Pin 112
set_property PACKAGE_PIN AA12 [get_ports LED_RX1_RX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_RX1_RX]

#Pin 113
set_property PACKAGE_PIN U10 [get_ports {RX2C_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2C_BANDSEL[1]}]

#Pin 114
set_property PACKAGE_PIN U12 [get_ports LED_TXRX2_TX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_TXRX2_TX]

#Pin 115
set_property PACKAGE_PIN Y10 [get_ports {RX2C_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2C_BANDSEL[0]}]

#Pin 116
set_property PACKAGE_PIN AB11 [get_ports LED_TXRX2_RX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_TXRX2_RX]

#Pin 117
set_property PACKAGE_PIN U9 [get_ports {RX2B_BANDSEL[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2B_BANDSEL[1]}]

#Pin 118
set_property PACKAGE_PIN AA11 [get_ports LED_RX2_RX]
set_property IOSTANDARD LVCMOS18 [get_ports LED_RX2_RX]

#Pin 119
set_property PACKAGE_PIN Y4 [get_ports {RX2B_BANDSEL[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {RX2B_BANDSEL[0]}]

#Pin 120
set_property PACKAGE_PIN AB12 [get_ports {DB_EXP_1_8V[24]}]
set_property IOSTANDARD LVCMOS18 [get_ports {DB_EXP_1_8V[24]}]

### Other I/O
set_property PACKAGE_PIN A22 [get_ports AVR_CS_R]
set_property IOSTANDARD LVCMOS33 [get_ports AVR_CS_R]
set_property PACKAGE_PIN B22 [get_ports AVR_IRQ]
set_property IOSTANDARD LVCMOS33 [get_ports AVR_IRQ]
set_property PACKAGE_PIN C22 [get_ports AVR_MISO_R]
set_property IOSTANDARD LVCMOS33 [get_ports AVR_MISO_R]
set_property PACKAGE_PIN A21 [get_ports AVR_MOSI_R]
set_property IOSTANDARD LVCMOS33 [get_ports AVR_MOSI_R]
set_property PACKAGE_PIN D22 [get_ports AVR_SCK_R]
set_property IOSTANDARD LVCMOS33 [get_ports AVR_SCK_R]

set_property PACKAGE_PIN E21 [get_ports ONSWITCH_DB]
set_property IOSTANDARD LVCMOS33 [get_ports ONSWITCH_DB]

set_property PACKAGE_PIN Y9 [get_ports GPS_PPS]
set_property IOSTANDARD LVCMOS18 [get_ports GPS_PPS]

set_property PACKAGE_PIN D18 [get_ports PPS_EXT_IN]
set_property IOSTANDARD LVCMOS33 [get_ports PPS_EXT_IN]

set_property PACKAGE_PIN E16 [get_ports {PL_GPIO[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[0]}]
set_property PACKAGE_PIN C18 [get_ports {PL_GPIO[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[1]}]
set_property PACKAGE_PIN D17 [get_ports {PL_GPIO[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[2]}]
set_property PACKAGE_PIN D16 [get_ports {PL_GPIO[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[3]}]
set_property PACKAGE_PIN D15 [get_ports {PL_GPIO[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[4]}]
set_property PACKAGE_PIN E15 [get_ports {PL_GPIO[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {PL_GPIO[5]}]
set_property PULLDOWN true [get_ports {PL_GPIO[0]}]
set_property PULLDOWN true [get_ports {PL_GPIO[1]}]
set_property PULLDOWN true [get_ports {PL_GPIO[2]}]
set_property PULLDOWN true [get_ports {PL_GPIO[3]}]
set_property PULLDOWN true [get_ports {PL_GPIO[4]}]
set_property PULLDOWN true [get_ports {PL_GPIO[5]}]

