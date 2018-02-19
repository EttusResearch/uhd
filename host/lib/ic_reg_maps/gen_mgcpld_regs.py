#!/usr/bin/env python
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Register map for the Magnesium CPLD. This is controlled via SPI.
"""

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
scratch               0x0040[0:15]      0
cpld_reset            0x0041[0]         0
ch1_idle_tx_sw1       0x0050[0:1]       0 ShutdownTxSw1,FromTxFilterLp1700MHz,FromTxFilterLp3400MHz,FromTxFilterLp0800MHz
ch1_idle_tx_sw2       0x0050[2:5]       1 ToTxFilterLp3400MHz=1,ToTxFilterLp1700MHz=2,ToTxFilterLp0800MHz=4,ToTxFilterLp6400MHz=8
ch1_idle_tx_sw3       0x0050[6]         0 ToTxFilterBanks,BypassPathToTrxSw
ch1_idle_tx_lowband_mixer_path_select 0x0050[7] 0 bypass,enable
ch1_idle_tx_mixer_en  0x0050[8]         0
ch1_idle_tx_amp_en    0x0050[9]         1
ch1_idle_tx_pa_en     0x0050[10]        1
ch1_idle_sw_trx       0x0050[11:12]     0 FromLowerFilterBankTxSw1,FromTxUpperFilterBankLp6400MHz,RxChannelPath,BypassPathToTxSw3
ch1_idle_tx_led       0x0050[13]        0
ch1_idle_tx_myk_en    0x0050[14]        0
ch1_idle_rx_sw1       0x0051[0:1]       3 TxRxInput,RxLoCalInput,TrxSwitchOutput,Rx2Input
ch1_idle_rx_sw2       0x0051[2:3]       0 ShutdownSw2,LowerFilterBankToSwitch3,BypassPathToSwitch6,UpperFilterBankToSwitch4
ch1_idle_rx_sw3       0x0051[4:6]       0 Filter2100x2850MHz=0,Filter0490LpMHz=1,Filter1600x2250MHz=2,Filter0440x0530MHz=4,Filter0650x1000MHz=5,Filter1100x1575MHz=6,ShutdownSw3=7
ch1_idle_rx_sw4       0x0051[7:9]       1 Filter2100x2850MHzFrom=1,Filter1600x2250MHzFrom=2,Filter2700HpMHz=4
ch1_idle_rx_sw5       0x0051[10:13]     1 Filter0440x0530MHzFrom=1,Filter1100x1575MHzFrom=2,Filter0490LpMHzFrom=4,Filter0650x1000MHzFrom=8
ch1_idle_rx_sw6       0x0052[0:2]       1 LowerFilterBankFromSwitch5=1,UpperFilterBankFromSwitch4=2,BypassPathFromSwitch2=4
ch1_idle_rx_loband_mixer_path_sel  0x0052[3] 0 bypass,loband
ch1_idle_rx_mixer_en  0x0052[4]         0
ch1_idle_rx_amp_en    0x0052[5]         0
ch1_idle_rx_lna1_en   0x0052[6]         0
ch1_idle_rx_lna2_en   0x0052[7]         0
ch1_idle_rx2_led      0x0052[8]         0
ch1_idle_rx_led       0x0052[9]         0
ch1_idle_rx_myk_en    0x0052[10]        0
ch1_on_tx_sw1         0x0053[0:1]       0 ShutdownTxSw1,FromTxFilterLp1700MHz,FromTxFilterLp3400MHz,FromTxFilterLp0800MHz
ch1_on_tx_sw2         0x0053[2:5]       1 ToTxFilterLp3400MHz=1,ToTxFilterLp1700MHz=2,ToTxFilterLp0800MHz=4,ToTxFilterLp6400MHz=8
ch1_on_tx_sw3         0x0053[6]         0 ToTxFilterBanks,BypassPathToTrxSw
ch1_on_tx_lowband_mixer_path_select 0x0053[7] 0 bypass,enable
ch1_on_tx_mixer_en    0x0053[8]         0
ch1_on_tx_amp_en      0x0053[9]         1
ch1_on_tx_pa_en       0x0053[10]        1
ch1_on_sw_trx         0x0053[11:12]     0 FromLowerFilterBankTxSw1,FromTxUpperFilterBankLp6400MHz,RxChannelPath,BypassPathToTxSw3
ch1_on_tx_led         0x0053[13]        0
ch1_on_tx_myk_en      0x0053[14]        0
ch1_on_rx_sw1         0x0054[0:1]       3 TxRxInput,RxLoCalInput,TrxSwitchOutput,Rx2Input
ch1_on_rx_sw2         0x0054[2:3]       1 ShutdownSw2,LowerFilterBankToSwitch3,BypassPathToSwitch6,UpperFilterBankToSwitch4
ch1_on_rx_sw3         0x0054[4:6]       0 Filter2100x2850MHz=0,Filter0490LpMHz=1,Filter1600x2250MHz=2,Filter0440x0530MHz=4,Filter0650x1000MHz=5,Filter1100x1575MHz=6,ShutdownSw3=7
ch1_on_rx_sw4         0x0054[7:9]       1 Filter2100x2850MHzFrom=1,Filter1600x2250MHzFrom=2,Filter2700HpMHz=4
ch1_on_rx_sw5         0x0054[10:13]     4 Filter0440x0530MHzFrom=1,Filter1100x1575MHzFrom=2,Filter0490LpMHzFrom=4,Filter0650x1000MHzFrom=8
ch1_on_rx_sw6         0x0055[0:2]       2 LowerFilterBankFromSwitch5=1,UpperFilterBankFromSwitch4=2,BypassPathFromSwitch2=4
ch1_on_rx_loband_mixer_path_sel  0x0055[3] 0 bypass,loband
ch1_on_rx_mixer_en    0x0055[4]         0
ch1_on_rx_amp_en      0x0055[5]         1
ch1_on_rx_lna1_en     0x0055[6]         1
ch1_on_rx_lna2_en     0x0055[7]         1
ch1_on_rx2_led        0x0055[8]         1
ch1_on_rx_led         0x0055[9]         0
ch1_on_rx_myk_en      0x0055[10]        1
ch2_idle_tx_sw1       0x0060[0:1]       0 ShutdownTxSw1,FromTxFilterLp1700MHz,FromTxFilterLp3400MHz,FromTxFilterLp0800MHz
ch2_idle_tx_sw2       0x0060[2:5]       1 ToTxFilterLp3400MHz=1,ToTxFilterLp1700MHz=2,ToTxFilterLp0800MHz=4,ToTxFilterLp6400MHz=8
ch2_idle_tx_sw3       0x0060[6]         0 ToTxFilterBanks,BypassPathToTrxSw
ch2_idle_tx_lowband_mixer_path_select 0x0060[7] 0 bypass,enable
ch2_idle_tx_mixer_en  0x0060[8]         0
ch2_idle_tx_amp_en    0x0060[9]         1
ch2_idle_tx_pa_en     0x0060[10]        1
ch2_idle_sw_trx       0x0060[11:12]     0 FromLowerFilterBankTxSw1,FromTxUpperFilterBankLp6400MHz,RxChannelPath,BypassPathToTxSw3
ch2_idle_tx_led       0x0060[13]        0
ch2_idle_tx_myk_en    0x0060[14]        0
ch2_idle_rx_sw1       0x0061[0:1]       3 TxRxInput,RxLoCalInput,TrxSwitchOutput,Rx2Input
ch2_idle_rx_sw2       0x0061[2:3]       0 ShutdownSw2,LowerFilterBankToSwitch3,BypassPathToSwitch6,UpperFilterBankToSwitch4
ch2_idle_rx_sw3       0x0061[4:6]       0 Filter2100x2850MHz=0,Filter0490LpMHz=1,Filter1600x2250MHz=2,Filter0440x0530MHz=4,Filter0650x1000MHz=5,Filter1100x1575MHz=6,ShutdownSw3=7
ch2_idle_rx_sw4       0x0061[7:9]       1 Filter2100x2850MHzFrom=1,Filter1600x2250MHzFrom=2,Filter2700HpMHz=4
ch2_idle_rx_sw5       0x0061[10:13]     1 Filter0440x0530MHzFrom=1,Filter1100x1575MHzFrom=2,Filter0490LpMHzFrom=4,Filter0650x1000MHzFrom=8
ch2_idle_rx_sw6       0x0062[0:2]       1 LowerFilterBankFromSwitch5=1,UpperFilterBankFromSwitch4=2,BypassPathFromSwitch2=4
ch2_idle_rx_loband_mixer_path_sel  0x0062[3] 0 bypass,loband
ch2_idle_rx_mixer_en  0x0062[4]         0
ch2_idle_rx_amp_en    0x0062[5]         0
ch2_idle_rx_lna1_en   0x0062[6]         0
ch2_idle_rx_lna2_en   0x0062[7]         0
ch2_idle_rx2_led      0x0062[8]         0
ch2_idle_rx_led       0x0062[9]         0
ch2_idle_rx_myk_en    0x0062[10]        0
ch2_on_tx_sw1         0x0063[0:1]       0 ShutdownTxSw1,FromTxFilterLp1700MHz,FromTxFilterLp3400MHz,FromTxFilterLp0800MHz
ch2_on_tx_sw2         0x0063[2:5]       1 ToTxFilterLp3400MHz=1,ToTxFilterLp1700MHz=2,ToTxFilterLp0800MHz=4,ToTxFilterLp6400MHz=8
ch2_on_tx_sw3         0x0063[6]         0 ToTxFilterBanks,BypassPathToTrxSw
ch2_on_tx_lowband_mixer_path_select 0x0063[7] 0 bypass,enable
ch2_on_tx_mixer_en    0x0063[8]         0
ch2_on_tx_amp_en      0x0063[9]         1
ch2_on_tx_pa_en       0x0063[10]        1
ch2_on_sw_trx         0x0063[11:12]     0 FromLowerFilterBankTxSw1,FromTxUpperFilterBankLp6400MHz,RxChannelPath,BypassPathToTxSw3
ch2_on_tx_led         0x0063[13]        0
ch2_on_tx_myk_en      0x0063[14]        0
ch2_on_rx_sw1         0x0064[0:1]       3 TxRxInput,RxLoCalInput,TrxSwitchOutput,Rx2Input
ch2_on_rx_sw2         0x0064[2:3]       1 ShutdownSw2,LowerFilterBankToSwitch3,BypassPathToSwitch6,UpperFilterBankToSwitch4
ch2_on_rx_sw3         0x0064[4:6]       0 Filter2100x2850MHz=0,Filter0490LpMHz=1,Filter1600x2250MHz=2,Filter0440x0530MHz=4,Filter0650x1000MHz=5,Filter1100x1575MHz=6,ShutdownSw3=7
ch2_on_rx_sw4         0x0064[7:9]       1 Filter2100x2850MHzFrom=1,Filter1600x2250MHzFrom=2,Filter2700HpMHz=4
ch2_on_rx_sw5         0x0064[10:13]     4 Filter0440x0530MHzFrom=1,Filter1100x1575MHzFrom=2,Filter0490LpMHzFrom=4,Filter0650x1000MHzFrom=8
ch2_on_rx_sw6         0x0065[0:2]       2 LowerFilterBankFromSwitch5=1,UpperFilterBankFromSwitch4=2,BypassPathFromSwitch2=4
ch2_on_rx_loband_mixer_path_sel  0x0065[3] 0 bypass,loband
ch2_on_rx_mixer_en    0x0065[4]         0
ch2_on_rx_amp_en      0x0065[5]         1
ch2_on_rx_lna1_en     0x0065[6]         1
ch2_on_rx_lna2_en     0x0065[7]         1
ch2_on_rx2_led        0x0065[8]         1
ch2_on_rx_led         0x0065[9]         0
ch2_on_rx_myk_en      0x0065[10]        1
"""


# PS regs for reference:
# signature        0[0:15]           0
# rev              1[0:15]           0
# oldest_compat    2[0:15]           0
# build_code_hh    3[0:7]            0
# build_code_dd    3[8:15]           0
# build_code_mm    4[0:7]            0
# build_code_yy    4[8:15]           0
# scratch          5[0:15]           0
# reset           16[0]              0
# ...

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint32_t get_reg(uint8_t addr){
    uint32_t reg = 0;
    switch(addr){
    % for addr in [0x40, 0x41, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65]:
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

std::set<size_t> get_all_addrs()
{
    std::set<size_t> addrs;
    % for reg in regs:
    // Hopefully, compilers will optimize out this mess...
    addrs.insert(${reg.get_addr()});
    % endfor
    return addrs;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='magnesium_cpld_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )

