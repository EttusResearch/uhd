#!/usr/bin/env python
#
# Copyright 2010-2012 Ettus Research LLC
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
## serial control port config
########################################################################
long_instruction               0[4]          1       8bits, 16bits
soft_reset                     0[5]          0
lsb_first                      0[6]          0       msb, lsb
sdo_inactive                   0[7]          0       active, inactive
########################################################################
## pll settings
########################################################################
acounter                       4[0:5]        0
bcounter_msb                   5[0:4]        0
bcounter_lsb                   6[0:7]        0
lor_enable                     7[2]          0      enb, dis
lor_ildd                       7[5:6]        0      3cyc, 6cyc, 12cyc, 24cyc
charge_pump_mode               8[0:1]        0      3state, pump_up, pump_down, normal
pll_mux_control                8[2:5]        0      off, dld_high, ndiv, dld_low, rdiv, ald_nchan, acounter, prescaler, pfd_up, pfd_down, lor_high, 3state, ald_pchan, lor_lol_high, lor_lol_low, lor_low
pfd_polarity                   8[6]          0      neg, pos
reset_all_counters             9[0]          0
ncounter_reset                 9[1]          0
rcounter_reset                 9[2]          0
cp_current_setting             9[4:6]        0      0_60ma, 1_2ma, 1_8ma, 2_4ma, 3_0ma, 3_6ma, 4_2ma, 4_8ma
pll_power_down                 0xA[0:1]      0      normal=0, async_pd=1, sync_pd=3
prescaler_value                0xA[2:4]      0      div1, div2, 2_3, 4_5, 8_9, 16_17, 32_33, div3
b_counter_bypass               0xA[6]        0
ref_counter_msb                0xB[0:5]      0
ref_counter_lsb                0xC[0:7]      0
antibacklash_pw                0xD[0:1]      0      1_3ns, 2_9ns, 6_0ns
dld_window                     0xD[5]        0      9_5ns, 3_5ns
lock_detect_disable            0xD[6]        0      enb, dis
########################################################################
## fine delay adjust
########################################################################
% for i, o in ((5, 0), (6, 4)):
delay_control_out${i}            ${hex(0x34+o)}[0]    0
ramp_current_out${i}             ${hex(0x35+o)}[0:2]  0   200ua, 400ua, 600ua, 800ua, 1000ua, 1200ua, 1400ua, 1600ua
ramp_capacitor_out${i}           ${hex(0x35+o)}[3:5]  0   4caps=0, 3caps=1, 2caps=3, 1cap=7
delay_fine_adjust_out${i}        ${hex(0x36+o)}[1:5]  0
% endfor
########################################################################
## outputs
########################################################################
% for i, o in ((0, 0), (1, 1), (2, 2), (3, 3)):
power_down_lvpecl_out${i}        ${hex(0x3C+o)}[0:1]  0   normal, test, safe_pd, total_pd
output_level_lvpecl_out${i}      ${hex(0x3C+o)}[2:3]  2   500mv, 340mv, 810mv, 660mv
% endfor
% for i, o in ((4, 0), (5, 1), (6, 2), (7, 3)):
power_down_lvds_cmos_out${i}     ${hex(0x40+o)}[0]    0
output_level_lvds_out${i}        ${hex(0x40+o)}[1:2]  1   1_75ma, 3_5ma, 5_25ma, 7ma
lvds_cmos_select_out${i}         ${hex(0x40+o)}[3]    1   lvds, cmos
inverted_cmos_driver_out${i}     ${hex(0x40+o)}[4]    0   dis, enb
% endfor
clock_select                 0x45[0]               1   clk2_drives, clk1_drives
clk1_power_down              0x45[1]               0
clk2_power_down              0x45[2]               0
prescaler_clock_pd           0x45[3]               0
refin_power_down             0x45[4]               0
all_clock_inputs_pd          0x45[5]               0
########################################################################
## dividers
########################################################################
% for i, o in ((0, 0), (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14)):
divider_high_cycles_out${i}      ${hex(0x48+o)}[0:3]  0
divider_low_cycles_out${i}       ${hex(0x48+o)}[4:7]  0
phase_offset_out${i}             ${hex(0x49+o)}[0:3]  0
start_out${i}                    ${hex(0x49+o)}[4]    0
force_out${i}                    ${hex(0x49+o)}[5]    0
nosync_out${i}                   ${hex(0x49+o)}[6]    0
bypass_divider_out${i}           ${hex(0x49+o)}[7]    0
% endfor
########################################################################
## function
########################################################################
sync_detect_enable             0x58[0]             0    dis, enb
sync_select                    0x58[1]             0    1_to_0_5, 0_5_to_1
soft_sync                      0x58[2]             0
dist_power_down                0x58[3]             0
sync_power_down                0x58[4]             0
function_pin_select            0x58[5:6]           0    resetb, syncb, test, pdb
update_registers               0x5A[0]             0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint8_t get_reg(uint16_t addr){
    uint8_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

uint32_t get_write_reg(uint16_t addr){
    return (uint32_t(addr) << 8) | get_reg(addr);
}

uint32_t get_read_reg(uint16_t addr){
    return (uint32_t(addr) << 8) | (1 << 23);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad9510_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
