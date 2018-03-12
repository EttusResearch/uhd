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
## address 0
########################################################################
int_16_bit              0[4:19]     0
prescaler               0[20]       0       4_5, 8_9
autocal_en              0[21]       1       disabled, enabled
reg0_reserved0          0[22:31]    0x000
########################################################################
## address 1
########################################################################
frac1_24_bit            1[4:27]     0
reg1_reserved0          1[28:31]    0x0
########################################################################
## address 2
########################################################################
mod2_14_bit             2[4:17]     0
frac2_14_bit            2[18:31]    0
########################################################################
## address 3
########################################################################
phase_24_bit            3[4:27]     0
phase_adjust            3[28]       0       disabled, enabled
phase_resync            3[29]       0       disabled, enabled
sd_load_reset           3[30]       0       on_reg0_update, disabled
##reserved              3[31]       0
########################################################################
## address 4
########################################################################
counter_reset           4[4]        0       disabled, enabled
cp_three_state          4[5]        0       disabled, enabled
power_down              4[6]        0       disabled, enabled
pd_polarity             4[7]        1       negative, positive
mux_logic               4[8]        1       1_8V, 3_3V
ref_mode                4[9]        0       single, diff
<% current_setting_enums = ', '.join(map(lambda x: '_'.join(("%0.2fma"%(round(x*31.27 + 31.27)/100)).split('.')), range(0,16))) %>\
charge_pump_current     4[10:13]    2       ${current_setting_enums}
double_buff_div         4[14]       0       disabled, enabled
r_counter_10_bit        4[15:24]    0
reference_divide_by_2   4[25]       0       disabled, enabled
reference_doubler       4[26]       0       disabled, enabled
muxout                  4[27:29]    1       3state, dvdd, dgnd, rdiv, ndiv, analog_ld, dld, reserved
reg4_reserved0          4[30:31]    0
########################################################################
## address 5
########################################################################
reg5_reserved0          5[4:31]     0x0080002
########################################################################
## address 6
########################################################################
output_power            6[4:5]      0       m4dbm, m1dbm, 2dbm, 5dbm
rf_out_a_enabled        6[6]        0       disabled, enabled
reg6_reserved0          6[7:9]      0x0
rf_out_b_enabled        6[10]       1       enabled, disabled
mute_till_lock_detect   6[11]       0       mute_disabled, mute_enabled
reg6_reserved1          6[12]       0
cp_bleed_current        6[13:20]    2
rf_divider_select       6[21:23]    0       div1, div2, div4, div8, div16, div32, div64
feedback_select         6[24]       0       divided, fundamental
reg6_reserved2          6[25:28]    0xA
negative_bleed          6[29]       0       disabled, enabled
gated_bleed             6[30]       0       disabled, enabled
reg6_reserved3          6[31]       0
########################################################################
## address 7
########################################################################
ld_mode                 7[4]        0       frac_n, int_n
frac_n_ld_precision     7[5:6]      0       5ns, 6ns, 8ns, 12ns
loss_of_lock_mode       7[7]        0       disabled, enabled
ld_cyc_count            7[8:9]      0       1024, 2048, 4096, 8192
reg7_reserved0          7[10:24]    0x0
le_sync                 7[25]       1       disabled, le_synced_to_refin
reg7_reserved1          7[26:31]    0x4
########################################################################
## address 8
########################################################################
reg8_reserved0          8[4:31]     0x102D402
########################################################################
## address 9
########################################################################
synth_lock_timeout      9[4:8]      0
auto_level_timeout      9[9:13]     0
timeout                 9[14:23]    0
vco_band_div            9[24:31]    0
########################################################################
## address 10
########################################################################
adc_enable             10[4]        0       disabled, enabled
adc_conversion         10[5]        0       disabled, enabled
adc_clock_divider      10[6:13]     1
reg10_reserved0        10[14:31]    0x300
########################################################################
## address 11
########################################################################
reg11_reserved0        11[4:31]     0x0061300
########################################################################
## address 12
########################################################################
reg12_reserved0        12[4:15]     0x041
phase_resync_clk_div   12[16:31]    0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
enum addr_t{
    ADDR_R0 = 0,
    ADDR_R1 = 1,
    ADDR_R2 = 2,
    ADDR_R3 = 3,
    ADDR_R4 = 4,
    ADDR_R5 = 5,
    ADDR_R6 = 6,
    ADDR_R7 = 7,
    ADDR_R8 = 8,
    ADDR_R9 = 9,
    ADDR_R10 = 10,
    ADDR_R11 = 11,
    ADDR_R12 = 12
};

uint32_t get_reg(uint8_t addr){
    uint32_t reg = addr & 0xF;
    switch(addr){
    % for addr in range(12+1):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='adf5355_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
