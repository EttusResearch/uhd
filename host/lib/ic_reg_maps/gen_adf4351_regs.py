#!/usr/bin/env python
#
# Copyright 2010 Ettus Research LLC
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
## address 0
########################################################################
frac_12_bit             0[3:14]     0
int_16_bit              0[15:30]    0x23
##reserved              0[31]       0
########################################################################
## address 1
########################################################################
mod_12_bit              1[3:14]     0xfff
phase_12_bit            1[15:26]    0
prescaler               1[27]       0       4_5, 8_9
phase_adjust            1[28]       0
##reserved              1[29:31]    0
########################################################################
## address 2
########################################################################
counter_reset           2[3]        0       disabled, enabled
cp_three_state          2[4]        0       disabled, enabled
power_down              2[5]        0       disabled, enabled
pd_polarity             2[6]        1       negative, positive
ldp                     2[7]        0       10ns, 6ns
ldf                     2[8]        0       frac_n, int_n
#set $current_setting_enums = ', '.join(map(lambda x: '_'.join(("%0.2fma"%(round(x*31.27 + 31.27)/100)).split('.')), range(0,16)))
charge_pump_current     2[9:12]     5       $current_setting_enums
double_buffer           2[13]       0       disabled, enabled
r_counter_10_bit        2[14:23]    0
reference_divide_by_2   2[24]       1       disabled, enabled
reference_doubler       2[25]       0       disabled, enabled
muxout                  2[26:28]    1       3state, dvdd, dgnd, rdiv, ndiv, analog_ld, dld, reserved
low_noise_and_spur      2[29:30]    3       low_noise, reserved0, reserved1, low_spur
##reserved              2[31]       0
########################################################################
## address 3
########################################################################
clock_divider_12_bit    3[3:14]     0
clock_div_mode          3[15:16]    1       clock_divider_off, fast_lock, resync_enable, reserved
##reserved              3[17]       0
cycle_slip_reduction    3[18]       0       disabled, enabled
##reserved              3[19:20]    0
charge_cancel           3[21]       0
anti_backlash_pulse     3[22]       0       6ns, 3ns
band_select_mode        3[23]       0       low, high
##reserved              3[24:31]    0
########################################################################
## address 4
########################################################################
output_power            4[3:4]      3       m4dbm, m1dbm, 2dbm, 5dbm
rf_output_enable        4[5]        1       disabled, enabled
aux_output_power        4[6:7]      0       m4dbm, m1dbm, 2dbm, 5dbm
aux_output_enable       4[8]        0       disabled, enabled
aux_output_select       4[9]        1       divided, fundamental
mute_till_lock_detect   4[10]       0       mute_disabled, mute_enabled
vco_power_down          4[11]       0       vco_powered_up, vco_powered_down
band_select_clock_div   4[12:19]    0
rf_divider_select       4[20:22]    0       div1, div2, div4, div8, div16, div32, div64
feedback_select         4[23]       1       divided, fundamental
##reserved              4[24:31]    0
########################################################################
## address 5
########################################################################
##reserved              5[3:18]     0
##reserved              5[19:20]    0
##reserved              5[21]       0
ld_pin_mode             5[22:23]    1       low0, dld, low, high
##reserved              5[24:31]    0
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
    ADDR_R5 = 5
};

boost::uint32_t get_reg(boost::uint8_t addr){
    boost::uint32_t reg = addr & 0x7;
    switch(addr){
    #for $addr in range(5+1)
    case $addr:
        #for $reg in filter(lambda r: r.get_addr() == addr, $regs)
        reg |= (boost::uint32_t($reg.get_name()) & $reg.get_mask()) << $reg.get_shift();
        #end for
        break;
    #end for
    }
    return reg;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='adf4351_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
