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
reset                   0[1]     0      
serial_readout          0[0]     0      
########################################################################
## address 16
########################################################################
clkout_strength         16[6:7]   0      weaker=1, default=0, stronger=3
########################################################################
## address 17
########################################################################
dataout_strength        17[0:1]   0      weaker=1, default=0, stronger=3, maximum=2
lvds_current            17[2:3]   0      3_5ma, 2_5ma, 4_5ma, 1_75ma
lvds_current_double     17[4:5]   0      default, dblclk, dbldataclock
########################################################################
## address 18
########################################################################
lvds_clk_term           18[0:2]   0      none, 300, 180, 110, 150, 100, 81, 60
lvds_data_term          18[3:5]   0      none, 300, 180, 110, 150, 100, 81, 60
########################################################################
## address 19
########################################################################
offset_freeze           19[4]     0
########################################################################
## address 20
########################################################################
power_down              20[0:2]   0      normal, a_dis, b_dis, ab_dis, global_pd, a_sby, b_sby, mux
ref_select              20[3]     0      internal, external
coarse_gain             20[4]     0      0db, 3_5db
output_interface        20[5]     0      cmos, lvds
override                20[7]     0
########################################################################
## address 22
########################################################################
test_patterns           22[0:2]   0      normal, zeros, ones, toggle, ramp, custom
lvds_bytewise           22[3]     0
data_format             22[4]     0      twos_complement, binary
########################################################################
## address 23
########################################################################
fine_gain               23[0:3]   0
########################################################################
## address 24 and 25
########################################################################
custom_low              24[0:7]   0
custom_high             25[0:5]   0
########################################################################
## address 26
########################################################################
gain_correction         26[0:3]   0
offset_tc               26[4:6]   0      1_1s, 0_55s, 0_27s, 0_13s, 2_15s, 4_3s
low_latency             26[7]     0
########################################################################
## address 27
########################################################################
decimation              27[0:2]   0      decimate_2, decimate_4, decimate_1, decimate_8
odd_tap_enable          27[3]     0
filter_enable           27[4]     0
filter_coeff_sel        27[5]     0      predefined, userdefined
offset_enable           27[7]     0
########################################################################
## address 29
########################################################################
decimation_filter_bands 29[0:1]   0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint8_t get_reg(uint8_t addr){
    uint8_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint8_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

uint16_t get_write_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | get_reg(addr);
}

uint16_t get_read_reg(uint8_t addr){
    return (uint16_t(addr) << 8) | (1 << 7);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ads62p44_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
