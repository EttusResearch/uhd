#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   DRAM pin constraints for X410.
#

###############################################################################
# Pin constraints for DRAM controller 0
###############################################################################

set_property PACKAGE_PIN AH20 [get_ports DRAM0_ACT_n]
set_property PACKAGE_PIN AN22 [get_ports {DRAM0_ADDR[0]}]
set_property PACKAGE_PIN AJ18 [get_ports {DRAM0_ADDR[10]}]
set_property PACKAGE_PIN AN21 [get_ports {DRAM0_ADDR[11]}]
set_property PACKAGE_PIN AF20 [get_ports {DRAM0_ADDR[12]}]
set_property PACKAGE_PIN AJ20 [get_ports {DRAM0_ADDR[13]}]
set_property PACKAGE_PIN AG18 [get_ports {DRAM0_ADDR[14]}]
set_property PACKAGE_PIN AK18 [get_ports {DRAM0_ADDR[15]}]
set_property PACKAGE_PIN AN18 [get_ports {DRAM0_ADDR[16]}]
set_property PACKAGE_PIN AL20 [get_ports {DRAM0_ADDR[1]}]
set_property PACKAGE_PIN AK22 [get_ports {DRAM0_ADDR[2]}]
set_property PACKAGE_PIN AM19 [get_ports {DRAM0_ADDR[3]}]
set_property PACKAGE_PIN AG20 [get_ports {DRAM0_ADDR[4]}]
set_property PACKAGE_PIN AT20 [get_ports {DRAM0_ADDR[5]}]
set_property PACKAGE_PIN AL19 [get_ports {DRAM0_ADDR[6]}]
set_property PACKAGE_PIN AK21 [get_ports {DRAM0_ADDR[7]}]
set_property PACKAGE_PIN AL21 [get_ports {DRAM0_ADDR[8]}]
set_property PACKAGE_PIN AP21 [get_ports {DRAM0_ADDR[9]}]
set_property PACKAGE_PIN AK19 [get_ports {DRAM0_BA[0]}]
set_property PACKAGE_PIN AM18 [get_ports {DRAM0_BA[1]}]
set_property PACKAGE_PIN AJ19 [get_ports {DRAM0_BG[0]}]
set_property PACKAGE_PIN AL22 [get_ports {DRAM0_CLK_P[0]}]
set_property PACKAGE_PIN AM22 [get_ports {DRAM0_CLK_N[0]}]
set_property PACKAGE_PIN AF19 [get_ports {DRAM0_CKE[0]}]
set_property PACKAGE_PIN AH18 [get_ports {DRAM0_CS_n[0]}]
set_property PACKAGE_PIN AT17 [get_ports {DRAM0_ODT[0]}]
set_property PACKAGE_PIN AP15 [get_ports DRAM0_RESET_n]
set_property PACKAGE_PIN AM20 [get_ports DRAM0_REFCLK_P]
set_property PACKAGE_PIN AN20 [get_ports DRAM0_REFCLK_N]
set_property PACKAGE_PIN AR17 [get_ports {DRAM0_DM_n[0]}]
set_property PACKAGE_PIN AR22 [get_ports {DRAM0_DQS_p[0]}]
set_property PACKAGE_PIN AT22 [get_ports {DRAM0_DQS_n[0]}]
set_property PACKAGE_PIN AR18 [get_ports {DRAM0_DQ[0]}]
set_property PACKAGE_PIN AR19 [get_ports {DRAM0_DQ[1]}]
set_property PACKAGE_PIN AT19 [get_ports {DRAM0_DQ[2]}]
set_property PACKAGE_PIN AT21 [get_ports {DRAM0_DQ[3]}]
set_property PACKAGE_PIN AP18 [get_ports {DRAM0_DQ[4]}]
set_property PACKAGE_PIN AP19 [get_ports {DRAM0_DQ[5]}]
set_property PACKAGE_PIN AP20 [get_ports {DRAM0_DQ[6]}]
set_property PACKAGE_PIN AR21 [get_ports {DRAM0_DQ[7]}]
set_property PACKAGE_PIN AM12 [get_ports {DRAM0_DM_n[1]}]
set_property PACKAGE_PIN AM13 [get_ports {DRAM0_DQS_p[1]}]
set_property PACKAGE_PIN AN13 [get_ports {DRAM0_DQS_n[1]}]
set_property PACKAGE_PIN AM10 [get_ports {DRAM0_DQ[8]}]
set_property PACKAGE_PIN AP10 [get_ports {DRAM0_DQ[9]}]
set_property PACKAGE_PIN AN10 [get_ports {DRAM0_DQ[10]}]
set_property PACKAGE_PIN AR11 [get_ports {DRAM0_DQ[11]}]
set_property PACKAGE_PIN AL10 [get_ports {DRAM0_DQ[12]}]
set_property PACKAGE_PIN AP11 [get_ports {DRAM0_DQ[13]}]
set_property PACKAGE_PIN AN11 [get_ports {DRAM0_DQ[14]}]
set_property PACKAGE_PIN AR12 [get_ports {DRAM0_DQ[15]}]
set_property PACKAGE_PIN AW19 [get_ports {DRAM0_DM_n[2]}]
set_property PACKAGE_PIN AV21 [get_ports {DRAM0_DQS_p[2]}]
set_property PACKAGE_PIN AW21 [get_ports {DRAM0_DQS_n[2]}]
set_property PACKAGE_PIN AV17 [get_ports {DRAM0_DQ[16]}]
set_property PACKAGE_PIN AV18 [get_ports {DRAM0_DQ[17]}]
set_property PACKAGE_PIN AU19 [get_ports {DRAM0_DQ[18]}]
set_property PACKAGE_PIN AU18 [get_ports {DRAM0_DQ[19]}]
set_property PACKAGE_PIN AU17 [get_ports {DRAM0_DQ[20]}]
set_property PACKAGE_PIN AW20 [get_ports {DRAM0_DQ[21]}]
set_property PACKAGE_PIN AU20 [get_ports {DRAM0_DQ[22]}]
set_property PACKAGE_PIN AV20 [get_ports {DRAM0_DQ[23]}]
set_property PACKAGE_PIN AV10 [get_ports {DRAM0_DM_n[3]}]
set_property PACKAGE_PIN AT12 [get_ports {DRAM0_DQS_p[3]}]
set_property PACKAGE_PIN AT11 [get_ports {DRAM0_DQS_n[3]}]
set_property PACKAGE_PIN AW9  [get_ports {DRAM0_DQ[24]}]
set_property PACKAGE_PIN AU12 [get_ports {DRAM0_DQ[25]}]
set_property PACKAGE_PIN AU10 [get_ports {DRAM0_DQ[26]}]
set_property PACKAGE_PIN AV12 [get_ports {DRAM0_DQ[27]}]
set_property PACKAGE_PIN AW8  [get_ports {DRAM0_DQ[28]}]
set_property PACKAGE_PIN AT10 [get_ports {DRAM0_DQ[29]}]
set_property PACKAGE_PIN AV11 [get_ports {DRAM0_DQ[30]}]
set_property PACKAGE_PIN AW11 [get_ports {DRAM0_DQ[31]}]
set_property PACKAGE_PIN AW14 [get_ports {DRAM0_DM_n[4]}]
set_property PACKAGE_PIN AV16 [get_ports {DRAM0_DQS_p[4]}]
set_property PACKAGE_PIN AW16 [get_ports {DRAM0_DQS_n[4]}]
set_property PACKAGE_PIN AU13 [get_ports {DRAM0_DQ[32]}]
set_property PACKAGE_PIN AV15 [get_ports {DRAM0_DQ[33]}]
set_property PACKAGE_PIN AV13 [get_ports {DRAM0_DQ[34]}]
set_property PACKAGE_PIN AU14 [get_ports {DRAM0_DQ[35]}]
set_property PACKAGE_PIN AU15 [get_ports {DRAM0_DQ[36]}]
set_property PACKAGE_PIN AW15 [get_ports {DRAM0_DQ[37]}]
set_property PACKAGE_PIN AT15 [get_ports {DRAM0_DQ[38]}]
set_property PACKAGE_PIN AT16 [get_ports {DRAM0_DQ[39]}]
set_property PACKAGE_PIN AP8  [get_ports {DRAM0_DM_n[5]}]
set_property PACKAGE_PIN AN8  [get_ports {DRAM0_DQS_p[5]}]
set_property PACKAGE_PIN AN7  [get_ports {DRAM0_DQS_n[5]}]
set_property PACKAGE_PIN AM9  [get_ports {DRAM0_DQ[40]}]
set_property PACKAGE_PIN AR9  [get_ports {DRAM0_DQ[41]}]
set_property PACKAGE_PIN AL7  [get_ports {DRAM0_DQ[42]}]
set_property PACKAGE_PIN AM8  [get_ports {DRAM0_DQ[43]}]
set_property PACKAGE_PIN AL9  [get_ports {DRAM0_DQ[44]}]
set_property PACKAGE_PIN AP9  [get_ports {DRAM0_DQ[45]}]
set_property PACKAGE_PIN AL8  [get_ports {DRAM0_DQ[46]}]
set_property PACKAGE_PIN AM7  [get_ports {DRAM0_DQ[47]}]
set_property PACKAGE_PIN AK13 [get_ports {DRAM0_DM_n[6]}]
set_property PACKAGE_PIN AJ14 [get_ports {DRAM0_DQS_p[6]}]
set_property PACKAGE_PIN AK14 [get_ports {DRAM0_DQS_n[6]}]
set_property PACKAGE_PIN AH12 [get_ports {DRAM0_DQ[48]}]
set_property PACKAGE_PIN AJ13 [get_ports {DRAM0_DQ[49]}]
set_property PACKAGE_PIN AJ12 [get_ports {DRAM0_DQ[50]}]
set_property PACKAGE_PIN AK12 [get_ports {DRAM0_DQ[51]}]
set_property PACKAGE_PIN AG12 [get_ports {DRAM0_DQ[52]}]
set_property PACKAGE_PIN AL14 [get_ports {DRAM0_DQ[53]}]
set_property PACKAGE_PIN AH13 [get_ports {DRAM0_DQ[54]}]
set_property PACKAGE_PIN AM14 [get_ports {DRAM0_DQ[55]}]
set_property PACKAGE_PIN AP13 [get_ports {DRAM0_DM_n[7]}]
set_property PACKAGE_PIN AN17 [get_ports {DRAM0_DQS_p[7]}]
set_property PACKAGE_PIN AN16 [get_ports {DRAM0_DQS_n[7]}]
set_property PACKAGE_PIN AM15 [get_ports {DRAM0_DQ[56]}]
set_property PACKAGE_PIN AP14 [get_ports {DRAM0_DQ[57]}]
set_property PACKAGE_PIN AM17 [get_ports {DRAM0_DQ[58]}]
set_property PACKAGE_PIN AP16 [get_ports {DRAM0_DQ[59]}]
set_property PACKAGE_PIN AL17 [get_ports {DRAM0_DQ[60]}]
set_property PACKAGE_PIN AR14 [get_ports {DRAM0_DQ[61]}]
set_property PACKAGE_PIN AN15 [get_ports {DRAM0_DQ[62]}]
set_property PACKAGE_PIN AR16 [get_ports {DRAM0_DQ[63]}]


set_property IOSTANDARD DIFF_POD12_DCI  [get_ports {DRAM0_DQS_*[*]}]
set_property IOSTANDARD DIFF_SSTL12     [get_ports {DRAM0_REFCLK_*}]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports {DRAM0_CLK_*[0]}]
set_property IOSTANDARD LVCMOS12        [get_ports  DRAM0_RESET_n]
set_property IOSTANDARD POD12_DCI       [get_ports {DRAM0_DM_n[*] \
                                                    DRAM0_DQ[*]}]
set_property IOSTANDARD SSTL12_DCI      [get_ports {DRAM0_ACT_n \
                                                    DRAM0_ADDR[*] \
                                                    DRAM0_BA[*] \
                                                    DRAM0_BG[0] \
                                                    DRAM0_CKE[0] \
                                                    DRAM0_CS_n[0] \
                                                    DRAM0_ODT[0]}]

set_property DRIVE      8                [get_ports DRAM0_RESET_n]

set_property SLEW FAST [get_ports {DRAM0_ACT_n \
                                   DRAM0_ADDR[*] \
                                   DRAM0_BA[*] \
                                   DRAM0_BG[0] \
                                   DRAM0_CLK_*[0] \
                                   DRAM0_CKE[0] \
                                   DRAM0_CS_n[0] \
                                   DRAM0_DM_n[*] \
                                   DRAM0_DQ[*] \
                                   DRAM0_DQS_*[*] \
                                   DRAM0_ODT[0]}]

set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {DRAM0_ACT_n \
                                                     DRAM0_ADDR[*] \
                                                     DRAM0_BA[*] \
                                                     DRAM0_BG[0] \
                                                     DRAM0_CLK_*[0] \
                                                     DRAM0_CKE[0] \
                                                     DRAM0_CS_n[0] \
                                                     DRAM0_DM_n[*] \
                                                     DRAM0_DQ[*] \
                                                     DRAM0_DQS_*[*] \
                                                     DRAM0_ODT[0]}]

set_property IBUF_LOW_PWR FALSE [get_ports {DRAM0_DM_n[*] \
                                            DRAM0_DQ[*] \
                                            DRAM0_DQS_*[*]}]



###############################################################################
# Pin constraints for DRAM controller 0
###############################################################################

set_property PACKAGE_PIN E14 [get_ports DRAM1_ACT_n]
set_property PACKAGE_PIN B12 [get_ports {DRAM1_ADDR[0]}]
set_property PACKAGE_PIN G14 [get_ports {DRAM1_ADDR[1]}]
set_property PACKAGE_PIN D13 [get_ports {DRAM1_ADDR[2]}]
set_property PACKAGE_PIN F12 [get_ports {DRAM1_ADDR[3]}]
set_property PACKAGE_PIN C13 [get_ports {DRAM1_ADDR[4]}]
set_property PACKAGE_PIN D14 [get_ports {DRAM1_ADDR[5]}]
set_property PACKAGE_PIN C12 [get_ports {DRAM1_ADDR[6]}]
set_property PACKAGE_PIN C15 [get_ports {DRAM1_ADDR[7]}]
set_property PACKAGE_PIN H12 [get_ports {DRAM1_ADDR[8]}]
set_property PACKAGE_PIN H13 [get_ports {DRAM1_ADDR[9]}]
set_property PACKAGE_PIN A14 [get_ports {DRAM1_ADDR[10]}]
set_property PACKAGE_PIN K12 [get_ports {DRAM1_ADDR[11]}]
set_property PACKAGE_PIN D11 [get_ports {DRAM1_ADDR[12]}]
set_property PACKAGE_PIN J7  [get_ports {DRAM1_ADDR[13]}]
set_property PACKAGE_PIN A15 [get_ports {DRAM1_ADDR[14]}]
set_property PACKAGE_PIN B14 [get_ports {DRAM1_ADDR[15]}]
set_property PACKAGE_PIN E11 [get_ports {DRAM1_ADDR[16]}]
set_property PACKAGE_PIN A12 [get_ports {DRAM1_BA[0]}]
set_property PACKAGE_PIN A11 [get_ports {DRAM1_BA[1]}]
set_property PACKAGE_PIN B13 [get_ports {DRAM1_BG[0]}]
set_property PACKAGE_PIN E13 [get_ports {DRAM1_CLK_P[0]}]
set_property PACKAGE_PIN E12 [get_ports {DRAM1_CLK_N[0]}]
set_property PACKAGE_PIN C11 [get_ports {DRAM1_CKE[0]}]
set_property PACKAGE_PIN F14 [get_ports {DRAM1_CS_n[0]}]
set_property PACKAGE_PIN B15 [get_ports {DRAM1_ODT[0]}]
set_property PACKAGE_PIN G24 [get_ports DRAM1_RESET_n]
set_property PACKAGE_PIN G13 [get_ports DRAM1_REFCLK_P]
set_property PACKAGE_PIN G12 [get_ports DRAM1_REFCLK_N]
set_property PACKAGE_PIN J8  [get_ports {DRAM1_DM_n[0]}]
set_property PACKAGE_PIN H8  [get_ports {DRAM1_DQS_p[0]}]
set_property PACKAGE_PIN G8  [get_ports {DRAM1_DQS_n[0]}]
set_property PACKAGE_PIN G9  [get_ports {DRAM1_DQ[0]}]
set_property PACKAGE_PIN J9  [get_ports {DRAM1_DQ[1]}]
set_property PACKAGE_PIN H7  [get_ports {DRAM1_DQ[2]}]
set_property PACKAGE_PIN H6  [get_ports {DRAM1_DQ[3]}]
set_property PACKAGE_PIN G7  [get_ports {DRAM1_DQ[4]}]
set_property PACKAGE_PIN G6  [get_ports {DRAM1_DQ[5]}]
set_property PACKAGE_PIN F9  [get_ports {DRAM1_DQ[6]}]
set_property PACKAGE_PIN K9  [get_ports {DRAM1_DQ[7]}]
set_property PACKAGE_PIN K13 [get_ports {DRAM1_DM_n[1]}]
set_property PACKAGE_PIN J14 [get_ports {DRAM1_DQS_p[1]}]
set_property PACKAGE_PIN J13 [get_ports {DRAM1_DQS_n[1]}]
set_property PACKAGE_PIN F10 [get_ports {DRAM1_DQ[8]}]
set_property PACKAGE_PIN K10 [get_ports {DRAM1_DQ[9]}]
set_property PACKAGE_PIN F11 [get_ports {DRAM1_DQ[10]}]
set_property PACKAGE_PIN H10 [get_ports {DRAM1_DQ[11]}]
set_property PACKAGE_PIN H11 [get_ports {DRAM1_DQ[12]}]
set_property PACKAGE_PIN J10 [get_ports {DRAM1_DQ[13]}]
set_property PACKAGE_PIN J11 [get_ports {DRAM1_DQ[14]}]
set_property PACKAGE_PIN K11 [get_ports {DRAM1_DQ[15]}]
set_property PACKAGE_PIN D18 [get_ports {DRAM1_DM_n[2]}]
set_property PACKAGE_PIN B18 [get_ports {DRAM1_DQS_p[2]}]
set_property PACKAGE_PIN B17 [get_ports {DRAM1_DQS_n[2]}]
set_property PACKAGE_PIN A17 [get_ports {DRAM1_DQ[16]}]
set_property PACKAGE_PIN D15 [get_ports {DRAM1_DQ[17]}]
set_property PACKAGE_PIN A16 [get_ports {DRAM1_DQ[18]}]
set_property PACKAGE_PIN D16 [get_ports {DRAM1_DQ[19]}]
set_property PACKAGE_PIN C17 [get_ports {DRAM1_DQ[20]}]
set_property PACKAGE_PIN B19 [get_ports {DRAM1_DQ[21]}]
set_property PACKAGE_PIN A19 [get_ports {DRAM1_DQ[22]}]
set_property PACKAGE_PIN C16 [get_ports {DRAM1_DQ[23]}]
set_property PACKAGE_PIN N14 [get_ports {DRAM1_DM_n[3]}]
set_property PACKAGE_PIN L15 [get_ports {DRAM1_DQS_p[3]}]
set_property PACKAGE_PIN L14 [get_ports {DRAM1_DQS_n[3]}]
set_property PACKAGE_PIN N15 [get_ports {DRAM1_DQ[24]}]
set_property PACKAGE_PIN M12 [get_ports {DRAM1_DQ[25]}]
set_property PACKAGE_PIN M15 [get_ports {DRAM1_DQ[26]}]
set_property PACKAGE_PIN M13 [get_ports {DRAM1_DQ[27]}]
set_property PACKAGE_PIN N17 [get_ports {DRAM1_DQ[28]}]
set_property PACKAGE_PIN L12 [get_ports {DRAM1_DQ[29]}]
set_property PACKAGE_PIN M17 [get_ports {DRAM1_DQ[30]}]
set_property PACKAGE_PIN N13 [get_ports {DRAM1_DQ[31]}]
set_property PACKAGE_PIN C23 [get_ports {DRAM1_DM_n[4]}]
set_property PACKAGE_PIN B22 [get_ports {DRAM1_DQS_p[4]}]
set_property PACKAGE_PIN A22 [get_ports {DRAM1_DQS_n[4]}]
set_property PACKAGE_PIN B24 [get_ports {DRAM1_DQ[32]}]
set_property PACKAGE_PIN C21 [get_ports {DRAM1_DQ[33]}]
set_property PACKAGE_PIN C22 [get_ports {DRAM1_DQ[34]}]
set_property PACKAGE_PIN A21 [get_ports {DRAM1_DQ[35]}]
set_property PACKAGE_PIN A24 [get_ports {DRAM1_DQ[36]}]
set_property PACKAGE_PIN B20 [get_ports {DRAM1_DQ[37]}]
set_property PACKAGE_PIN C20 [get_ports {DRAM1_DQ[38]}]
set_property PACKAGE_PIN A20 [get_ports {DRAM1_DQ[39]}]
set_property PACKAGE_PIN F21 [get_ports {DRAM1_DM_n[5]}]
set_property PACKAGE_PIN D23 [get_ports {DRAM1_DQS_p[5]}]
set_property PACKAGE_PIN D24 [get_ports {DRAM1_DQS_n[5]}]
set_property PACKAGE_PIN E24 [get_ports {DRAM1_DQ[40]}]
set_property PACKAGE_PIN E22 [get_ports {DRAM1_DQ[41]}]
set_property PACKAGE_PIN F24 [get_ports {DRAM1_DQ[42]}]
set_property PACKAGE_PIN E23 [get_ports {DRAM1_DQ[43]}]
set_property PACKAGE_PIN E21 [get_ports {DRAM1_DQ[44]}]
set_property PACKAGE_PIN D21 [get_ports {DRAM1_DQ[45]}]
set_property PACKAGE_PIN F20 [get_ports {DRAM1_DQ[46]}]
set_property PACKAGE_PIN G20 [get_ports {DRAM1_DQ[47]}]
set_property PACKAGE_PIN J23 [get_ports {DRAM1_DM_n[6]}]
set_property PACKAGE_PIN J20 [get_ports {DRAM1_DQS_p[6]}]
set_property PACKAGE_PIN H20 [get_ports {DRAM1_DQS_n[6]}]
set_property PACKAGE_PIN L24 [get_ports {DRAM1_DQ[48]}]
set_property PACKAGE_PIN H23 [get_ports {DRAM1_DQ[49]}]
set_property PACKAGE_PIN J21 [get_ports {DRAM1_DQ[50]}]
set_property PACKAGE_PIN H22 [get_ports {DRAM1_DQ[51]}]
set_property PACKAGE_PIN K24 [get_ports {DRAM1_DQ[52]}]
set_property PACKAGE_PIN G23 [get_ports {DRAM1_DQ[53]}]
set_property PACKAGE_PIN H21 [get_ports {DRAM1_DQ[54]}]
set_property PACKAGE_PIN G22 [get_ports {DRAM1_DQ[55]}]
set_property PACKAGE_PIN N20 [get_ports {DRAM1_DM_n[7]}]
set_property PACKAGE_PIN K21 [get_ports {DRAM1_DQS_p[7]}]
set_property PACKAGE_PIN K22 [get_ports {DRAM1_DQS_n[7]}]
set_property PACKAGE_PIN M19 [get_ports {DRAM1_DQ[56]}]
set_property PACKAGE_PIN L21 [get_ports {DRAM1_DQ[57]}]
set_property PACKAGE_PIN M20 [get_ports {DRAM1_DQ[58]}]
set_property PACKAGE_PIN L23 [get_ports {DRAM1_DQ[59]}]
set_property PACKAGE_PIN N19 [get_ports {DRAM1_DQ[60]}]
set_property PACKAGE_PIN L22 [get_ports {DRAM1_DQ[61]}]
set_property PACKAGE_PIN L20 [get_ports {DRAM1_DQ[62]}]
set_property PACKAGE_PIN L19 [get_ports {DRAM1_DQ[63]}]


set_property IOSTANDARD DIFF_POD12_DCI  [get_ports {DRAM1_DQS_*[*]}]
set_property IOSTANDARD DIFF_SSTL12     [get_ports {DRAM1_REFCLK_*}]
set_property IOSTANDARD DIFF_SSTL12_DCI [get_ports {DRAM1_CLK_*[0]}]
set_property IOSTANDARD LVCMOS12        [get_ports  DRAM1_RESET_n]
set_property IOSTANDARD POD12_DCI       [get_ports {DRAM1_DM_n[*] \
                                                    DRAM1_DQ[*]}]
set_property IOSTANDARD SSTL12_DCI      [get_ports {DRAM1_ACT_n \
                                                    DRAM1_ADDR[*] \
                                                    DRAM1_BA[*] \
                                                    DRAM1_BG[0] \
                                                    DRAM1_CKE[0] \
                                                    DRAM1_CS_n[0] \
                                                    DRAM1_ODT[0]}]

set_property DRIVE      8                [get_ports DRAM1_RESET_n]

set_property SLEW FAST [get_ports {DRAM1_ACT_n \
                                   DRAM1_ADDR[*] \
                                   DRAM1_BA[*] \
                                   DRAM1_BG[0] \
                                   DRAM1_CLK_*[0] \
                                   DRAM1_CKE[0] \
                                   DRAM1_CS_n[0] \
                                   DRAM1_DM_n[*] \
                                   DRAM1_DQ[*] \
                                   DRAM1_DQS_*[*] \
                                   DRAM1_ODT[0]}]

set_property OUTPUT_IMPEDANCE RDRV_40_40 [get_ports {DRAM1_ACT_n \
                                                     DRAM1_ADDR[*] \
                                                     DRAM1_BA[*] \
                                                     DRAM1_BG[0] \
                                                     DRAM1_CLK_*[0] \
                                                     DRAM1_CKE[0] \
                                                     DRAM1_CS_n[0] \
                                                     DRAM1_DM_n[*] \
                                                     DRAM1_DQ[*] \
                                                     DRAM1_DQS_*[*] \
                                                     DRAM1_ODT[0]}]

set_property IBUF_LOW_PWR FALSE [get_ports {DRAM1_DM_n[*] \
                                            DRAM1_DQ[*] \
                                            DRAM1_DQS_*[*]}]
