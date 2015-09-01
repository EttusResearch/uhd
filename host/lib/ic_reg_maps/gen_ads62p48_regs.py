#!/usr/bin/env python
#
# Copyright 2013 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
reset                   0[7]          0
serial_readout          0[0]          0
enable_low_speed_mode   0x20[2]       0
ref                     0x3f[6:5]     0       internal=0, external=3
standby                 0x3f[1]       0       normal, standby
power_down              0x40[3:0]     0       pins=0, normal=8, chb=9, cha=10, chab=11, global=12, chb_standby=13, cha_standby=14, mux=15
lvds_cmos               0x41[7]       0       parallel_cmos, ddr_lvds
clk_out_pos_edge        0x44[7:5]     0       normal=0, plus4_26=5, minus4_26=7, minus7_26=6
clk_out_neg_edge        0x44[4:2]     0       normal=0, plus4_26=5, minus4_26=7, minus7_26=6
channel_control         0x50[6]       0                common, independent
data_format                    0x50[2:1]       2       2s_compliment=2, offset_binary=3
custom_pattern_low             0x51[7:0]       0
custom_pattern_high            0x52[5:0]       0
enable_offset_corr_chA         0x53[6]         0
gain_chA                       0x55[7:4]       0
offset_corr_time_const_chA     0x55[3:0]       0
fine_gain_adjust_chA           0x57[6:0]       0
test_patterns_chA              0x62[2:0]       0      normal, zeros, ones, toggle, ramp, custom
offset_pedestal_chA            0x63[5:0]       0
enable_offset_corr_chB         0x66[6]         0
gain_chB                       0x68[7:4]       0
offset_corr_time_const_chB     0x68[3:0]       0
fine_gain_adjust_chB           0x6A[6:0]       0
test_patterns_chB              0x75[2:0]       0      normal, zeros, ones, toggle, ramp, custom
offset_pedestal_chB            0x76[5:0]       0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
boost::uint8_t get_reg(boost::uint8_t addr){
    boost::uint8_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (boost::uint8_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

boost::uint16_t get_write_reg(boost::uint8_t addr){
    return (boost::uint16_t(addr) << 8) | get_reg(addr);
}

boost::uint16_t get_read_reg(boost::uint8_t addr){
    return (boost::uint16_t(addr) << 8) | (1 << 7);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ads62p48_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
