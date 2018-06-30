#!/usr/bin/env python
#
# Copyright 2010 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
########################################################################
## General
########################################################################
sdio_bidir         0[7]              0                  sdio_sdo, sdio
lsb_first          0[6]              0                  msb, lsb
soft_reset         0[5]              0
########################################################################
## Rx Power Down
########################################################################
vref_diff_pd       1[7]              0
vref_pd            1[6]              0
rx_digital_pd      1[5]              0
rx_channel_b_pd    1[4]              0
rx_channel_a_pd    1[3]              0
buffer_b_pd        1[2]              0
buffer_a_pd        1[1]              0
all_rx_pd          1[0]              0
########################################################################
## Rx A and B
########################################################################
% for x, i in (('a', 2), ('b', 3)):
byp_buffer_${x}      ${i}[7]           0
rx_pga_${x}          ${i}[0:4]         0
% endfor
########################################################################
## Rx Misc
########################################################################
hs_duty_cycle      4[2]              0
shared_ref         4[1]              0
clk_duty           4[0]              0
########################################################################
## RX I/F (INTERFACE)
########################################################################
three_state        5[4]              0
rx_retime          5[3]              0       clkout1, clkout2
rx_twos_comp       5[2]              0
inv_rxsync         5[1]              0
mux_out            5[0]              0       rx_mux_mode=1, dual_port_mode=0
########################################################################
## RX Digital
########################################################################
two_channel        6[3]              1       rx_b_dis, both_enb
rx_keep_ve         6[2]              0       pass_pos, pass_neg
rx_hilbert         6[1]              0       dis, enb
decimate           6[0]              0       dis, enb
########################################################################
## TX Power Down
########################################################################
alt_timing_mode    8[5]              0
txoff_enable       8[4]              0
tx_digital_pd      8[3]              0
tx_analog_pd       8[0:2]            0        none=0, txb=4, txa=2, both=7
########################################################################
## Tx Offset and Gain
########################################################################
% for x, i, j, k in (('a', 10, 11, 14), ('b', 12, 13, 15)):
dac_${x}_offset_1_0   ${i}[6:7]           0
dac_${x}_offset_dir   ${i}[0]             0        neg_diff, pos_dif
dac_${x}_offset_9_2   ${j}[0:7]           0
dac_${x}_coarse_gain  ${k}[6:7]           0
dac_${x}_fine_gain    ${k}[0:5]           0
% endfor
tx_pga_gain            16[0:7]            0
########################################################################
## Tx Misc
########################################################################
tx_slave_enable        17[1]              0
tx_pga_mode            17[0]              0        normal, fast
########################################################################
## Tx IF (INTERFACE)
########################################################################
tx_retime              18[6]              1        clkout1=1, clkout2=0
qi_order               18[5]              0        iq, qi
inv_txsync             18[4]              0
tx_twos_comp           18[3]              0
inverse_samp           18[2]              0        rise, fall
edges                  18[1]              0        normal, both
interleaved            18[0]              0        single, interleaved
########################################################################
## TX Digital
########################################################################
two_data_paths         19[4]              0        single, both
tx_keep_ve             19[3]              0        pass_pos, pass_neg
tx_hilbert             19[2]              0        dis, enb
interp                 19[0:1]            0        1, 2, 4
########################################################################
## TX Modulator
########################################################################
neg_fine_tune          20[5]              0        pos_shift, neg_shift
fine_mode              20[4]              0        bypass, nco
real_mix_mode          20[3]              0        complex, real
neg_coarse_tune        20[2]              0        pos_shift, neg_shift
coarse_mod             20[0:1]            0        bypass, fdac_4, fdac_8
########################################################################
## NCO Tuning Word
########################################################################
ftw_7_0                21[0:7]            0
ftw_15_8               22[0:7]            0
ftw_23_16              23[0:7]            0
########################################################################
## DLL
########################################################################
input_clk_ctrl         24[6]              0       internal, external
adc_div2               24[5]              0       normal, div2
dll_mult               24[3:4]            0       1, 2, 4
dll_pd                 24[2]              0
dll_mode               24[0]              0       slow, fast
########################################################################
## Clock Out
########################################################################
clkout2_div_factor     25[6:7]            0       1, 2, 4, 8
inv2                   25[5]              0       normal, inverted
inv1                   25[1]              0       normal, inverted
dis2                   25[4]              0       enb, dis
dis1                   25[0]              0       enb, dis
########################################################################
## Aux ADC
########################################################################
% for x, i in (('a2', 26), ('a1', 28), ('b2', 30), ('b1', 32)):
aux_adc_${x}_1_0       ${i}[6:7]          0
aux_adc_${x}_9_2       ${int(1+i)}[0:7]    0
% endfor
########################################################################
## Aux ADC Control
########################################################################
aux_spi                34[7]              0       dis, enb
sel_bnota              34[6]              0       adc_a, adc_b
% for x, i in (('b', 5), ('a', 2)):
refsel_${x}            34[${i}]           0       external, internal
select_${x}            34[${int(i-1)}]    0       aux_adc2, aux_adc1
start_${x}             34[${int(i-2)}]    0
% endfor
########################################################################
## Aux ADC Clock
########################################################################
clk_4                  35[0]              0       1_2, 1_4
########################################################################
## Aux DAC
########################################################################
% for x, i in (('a', 36), ('b', 37), ('c', 38)):
aux_dac_${x}           ${i}[0:7]          0
% endfor
########################################################################
## Aux DAC Update
########################################################################
aux_dac_slave_enable   39[7]              0
aux_dacupdate_c        39[2]              0
aux_dacupdate_b        39[1]              0
aux_dacupdate_a        39[0]              0
########################################################################
## AUX DAC Power Down
########################################################################
aux_dac_pd_a           40[2]              0
aux_dac_pd_b           40[1]              0
aux_dac_pd_c           40[0]              0
########################################################################
## AUX DAC Control
########################################################################
aux_dac_invert_a       41[2]              0
aux_dac_invert_b       41[1]              0
aux_dac_invert_c       41[0]              0
########################################################################
## Sig Delt
########################################################################
sig_delt_3_0           42[4:7]            0
sig_delt_11_4          43[0:7]            0
########################################################################
## ADC Low Power
########################################################################
rx_low_power_mode_r49 49[0:7]             0
rx_low_power_mode_r50 50[0:7]             0
########################################################################
## Chip ID
########################################################################
chip_id                63[0:7]            0
"""

########################################################################
# Header and Source templates below
########################################################################
BODY_TMPL="""
uint8_t get_reg(uint8_t addr){
    uint8_t reg = 0;
    switch(addr){
    % for addr in range(0, 63+1):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint16_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

void set_reg(uint8_t addr, uint16_t reg){
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        ${reg.get_name()} = ${reg.get_type()}((reg >> ${reg.get_shift()}) & ${reg.get_mask()});
        % endfor
        break;
    % endfor
    }
}

uint16_t get_write_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | get_reg(addr);
}

uint16_t get_read_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | (1 << 15);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad9862_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
