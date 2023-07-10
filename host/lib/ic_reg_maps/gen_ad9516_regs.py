#!/usr/bin/env python
#
# Copyright 2010-2012 Ettus Research LLC
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
########################################################################
## serial control port config
########################################################################
sdo_inactive_mirr              0[0]          0
lsb_first_mirr                 0[1]          0       msb, lsb
soft_reset_mirr                0[2]          0
long_instruction_mirr          0[3]          1       8bits, 16bits
long_instruction               0[4]          1       8bits, 16bits
soft_reset                     0[5]          0
lsb_first                      0[6]          0       msb, lsb
sdo_inactive                   0[7]          0
########################################################################
## pll settings
########################################################################
acounter                       0x13[0:5]        0
bcounter_msb                   0x15[0:4]        0
bcounter_lsb                   0x14[0:7]        0
lock_detect_disable            0x18[3]          0
dld_counter                    0x18[5:6]        0      5cyc, 16cyc, 64cyc, 255cyc
dld_window                     0x18[4]          0      high_r, low_r
charge_pump_mode               0x10[2:3]        3      3state, pump_up, pump_down, normal
cp_current_setting             0x10[4:6]        3      0_60ma, 1_2ma, 1_8ma, 2_4ma, 3_0ma, 3_6ma, 4_2ma, 4_8ma
status_pin_control             0x17[2:7]        0      off, ndiv, rdiv, adiv, prescaler, pfd_up, pfd_down, dld=45
ld_pin_control                 0x1A[0:5]        0      dld, p_channel, n_channel, 3state, 110uA_dld
refmon_pin_control             0x1B[0:4]        0      off, ref1, ref2, pll_ref, unsel_pll_ref
pfd_polarity                   0x10[7]          0      pos, neg
reset_all_counters             0x16[4]          0
ncounter_reset                 0x16[5]          0
rcounter_reset                 0x16[6]          0
cp_out_set_half_vss            0x16[7]          0
pll_power_down                 0x10[0:1]        1      normal=0, async_pd=1, sync_pd=3
prescaler_value                0x16[0:2]        1      div1, div2, 2_3, 4_5, 8_9, 16_17, 32_33, div3
ref_counter_msb                0x12[0:5]        0
ref_counter_lsb                0x11[0:7]        1
antibacklash_pw                0x17[0:1]        0      2_9ns, 1_3ns, 6_0ns
ref1_power_on                  0x1C[1]          0      off,on
ref2_power_on                  0x1C[2]          0      off,on
########################################################################
## fine delay adjust
########################################################################
% for i, o in ((6, 0), (7, 3),(8, 6),(9, 9)):
delay_control_out${i}            ${hex(0xA0+o)}[0]    0
ramp_current_out${i}             ${hex(0xA1+o)}[0:2]  0   200ua, 400ua, 600ua, 800ua, 1000ua, 1200ua, 1400ua, 1600ua
ramp_capacitor_out${i}           ${hex(0xA1+o)}[3:5]  0   4caps=0, 3caps=1, 2caps=3, 1cap=7
delay_fine_adjust_out${i}        ${hex(0xA2+o)}[0:5]  0
% endfor
########################################################################
## outputs
########################################################################
% for i, o in ((0, 0), (1, 1), (2, 2), (3, 3), (4, 4), (5, 5)):
power_down_lvpecl_out${i}        ${hex(0xF0+o)}[0:1]  0   normal, test, safe_pd, total_pd
output_level_lvpecl_out${i}      ${hex(0xF0+o)}[2:3]  2   400mv, 600mv, 780mv, 960mv
% endfor
% for i, o in ((6, 0), (7, 1), (8, 2), (9, 3)):
power_down_lvds_cmos_out${i}     ${hex(0x140+o)}[0]    0
output_level_lvds_out${i}        ${hex(0x140+o)}[1:2]  1   1_75ma, 3_5ma, 5_25ma, 7ma
lvds_cmos_select_out${i}         ${hex(0x140+o)}[3]    0   lvds, cmos
inverted_cmos_driver_out${i}     ${hex(0x140+o)}[4]    0
% endfor
clock_select                 0x1E1[1]               0   clk, vco
bypass_vco_div               0x1E1[0]               0
vco_power_down               0x1E1[3]               1
clk_vco_power_down           0x1E1[2]               0
all_clock_inputs_pd          0x1E1[4]               0
########################################################################
## dividers
########################################################################
% for i, o in ((0, 0), (1, 3), (2, 6)):
divider${i}_high_cycles      ${hex(0x190+o)}[0:3]  0
divider${i}_low_cycles       ${hex(0x190+o)}[4:7]  0
divider${i}_phase_offset     ${hex(0x191+o)}[0:3]  0
divider${i}_start            ${hex(0x191+o)}[4]    0
divider${i}_force            ${hex(0x191+o)}[5]    0
divider${i}_nosync           ${hex(0x191+o)}[6]    0
divider${i}_bypass           ${hex(0x191+o)}[7]    1
% endfor
% for i, o in ((3, 0), (4, 5)):
divider${i}_high_cycles      ${hex(0x199+o)}[0:3]  0
divider${i}_low_cycles       ${hex(0x199+o)}[4:7]  0
divider${i}_phase_offset     ${hex(0x19A+o)}[0:3]  0
divider${i}_start            ${hex(0x19C+o)}[0]    0
divider${i}_force            ${hex(0x19C+o)}[2]    0
divider${i}_nosync           ${hex(0x19C+o)}[3]    0
divider${i}_bypass           ${hex(0x19C+o)}[4]    1
divider${i}_bypass2          ${hex(0x19C+o)}[5]    1
% endfor
########################################################################
## system
########################################################################
soft_sync                      0x230[0]             0
dist_power_down                0x230[1]             0
sync_power_down                0x230[2]             0
update_registers               0x232[0]             0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
boost::uint8_t get_reg(boost::uint16_t addr){
    boost::uint8_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (boost::uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}

boost::uint32_t get_write_reg(boost::uint16_t addr){
    return (boost::uint32_t(addr) << 8) | get_reg(addr);
}

boost::uint32_t get_read_reg(boost::uint16_t addr){
    return (boost::uint32_t(addr) << 8) | (1 << 23);
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad9516_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
