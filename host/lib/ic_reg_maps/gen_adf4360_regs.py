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
core_power_level         0[2:3]     0          5ma, 10ma, 15ma, 20ma
counter_operation        0[4]       0          normal, reset
muxout_control           0[5:7]     0          3state, dld, ndiv, dvdd, rdiv, nchan_od_ld, sdo, dgnd
phase_detector_polarity  0[8]       0          neg, pos
charge_pump_output       0[9]       0          normal, 3state
cp_gain_0                0[10]      0          set1, set2
mute_till_ld             0[11]      0          dis, enb
output_power_level       0[12:13]   0          3_5ma, 5_0ma, 7_5ma, 11_0ma
<% current_setting_enums = ', '.join(map(lambda x: x+"ma", "0_31 0_62 0_93 1_25 1_56 1_87 2_18 2_50".split())) %>\
current_setting1         0[14:16]   0          ${current_setting_enums}
current_setting2         0[17:19]   0          ${current_setting_enums}
power_down               0[20:21]   0          normal_op=0, async_pd=1, sync_pd=3
prescaler_value          0[22:23]   0          8_9, 16_17, 32_33
########################################################################
## address 2
########################################################################
a_counter                2[2:6]     0
b_counter                2[8:20]    0
cp_gain_1                2[21]      0          set1, set2
divide_by_2_output       2[22]      0          fund, div2
divide_by_2_prescaler    2[23]      0          fund, div2
########################################################################
## address 1
########################################################################
r_counter                1[2:15]    0
ablpw                    1[16:17]   0           3_0ns, 1_3ns, 6_0ns
lock_detect_precision    1[18]      0           3cycles, 5cycles
test_mode_bit            1[19]      0
band_select_clock_div    1[20:21]   0           1, 2, 4, 8
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
enum addr_t{
    ADDR_CONTROL = 0,
    ADDR_NCOUNTER = 2,
    ADDR_RCOUNTER = 1
};

uint32_t get_reg(addr_t addr){
    uint32_t reg = addr & 0x3;
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
"""

if __name__ == '__main__':
    import common; common.generate(
        name='adf4360_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
