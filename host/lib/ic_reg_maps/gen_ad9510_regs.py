#!/usr/bin/env python
#
# Copyright 2008,2009 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either asversion 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

import re
import os
import sys
from Cheetah.Template import Template
def parse_tmpl(_tmpl_text, **kwargs):
    return str(Template(_tmpl_text, kwargs))
def safe_makedirs(path):
    not os.path.isdir(path) and os.makedirs(path)

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_DATA_TMPL="""\
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
pll_power_down                 A[0:1]        0      normal=0, async_pd=1, sync_pd=3
prescaler_value                A[2:4]        0      div1, div2, 2_3, 4_5, 8_9, 16_17, 32_33, div3
b_counter_bypass               A[6]          0
ref_counter_msb                B[0:5]        0
ref_counter_lsb                C[0:7]        0
antibacklash_pw                D[0:1]        0      1_3ns, 2_9ns, 6_0ns
dld_window                     D[5]          0      9_5ns, 3_5ns
lock_detect_disable            D[6]          0      enb, dis
########################################################################
## fine delay adjust
########################################################################
#for $i, $o in ((5, 0), (6, 4))
delay_control_out$i            $hex(0x34+$o)[0]    0
ramp_current_out$i             $hex(0x35+$o)[0:2]  0   200ua, 400ua, 600ua, 800ua, 1000ua, 1200ua, 1400ua, 1600ua
ramp_capacitor_out$i           $hex(0x35+$o)[3:5]  0   4caps=0, 3caps=1, 2caps=3, 1cap=7
delay_fine_adjust_out$i        $hex(0x36+$o)[1:5]  0
#end for
########################################################################
## outputs
########################################################################
#for $i, $o in ((0, 0), (1, 1), (2, 2), (3, 3))
power_down_lvpecl_out$i        $hex(0x3C+$o)[0:1]  0   normal, test, safe_pd, total_pd
output_level_lvpecl_out$i      $hex(0x3C+$o)[2:3]  2   500mv, 340mv, 810mv, 660mv
#end for
#for $i, $o in ((4, 0), (5, 1), (6, 2), (7, 3))
power_down_lvds_cmos_out$i     $hex(0x40+$o)[0]    0
output_level_lvds_out$i        $hex(0x40+$o)[1:2]  1   1_75ma, 3_5ma, 5_25ma, 7ma
lvds_cmos_select_out$i         $hex(0x40+$o)[3]    1   lvds, cmos
inverted_cmos_driver_out$i     $hex(0x40+$o)[4]    0   dis, enb
#end for
clock_select                   45[0]               1   clk2_drives, clk1_drives
clk1_power_down                45[1]               0
clk2_power_down                45[2]               0
prescaler_clock_pd             45[3]               0
refin_power_down               45[4]               0
all_clock_inputs_pd            45[5]               0
########################################################################
## dividers
########################################################################
#for $i, $o in ((0, 0), (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14))
divider_high_cycles_out$i      $hex(0x48+$o)[0:3]  0
divider_low_cycles_out$i       $hex(0x48+$o)[4:7]  0
phase_offset_out$i             $hex(0x49+$o)[0:3]  0
start_out$i                    $hex(0x49+$o)[4]    0
force_out$i                    $hex(0x49+$o)[5]    0
nosync_out$i                   $hex(0x49+$o)[6]    0
bypass_divider_out$i           $hex(0x49+$o)[7]    0
#end for
########################################################################
## function
########################################################################
sync_detect_enable             58[0]               0    dis, enb
sync_select                    58[1]               0    1_to_0_5, 0_5_to_1
soft_sync                      58[2]               0
dist_power_down                58[3]               0
sync_power_down                58[4]               0
function_pin_select            58[5:6]             0    resetb, syncb, test, pdb
update_registers               5A[0]               0
"""

########################################################################
# Header and Source templates below
########################################################################
HEADER_TEXT="""
#import time

/***********************************************************************
 * This file was generated by $file on $time.strftime("%c")
 **********************************************************************/

\#ifndef INCLUDED_AD9510_REGS_HPP
\#define INCLUDED_AD9510_REGS_HPP

\#include <boost/cstdint.hpp>

struct ad9510_regs_t{
#for $reg in $regs
    #if $reg.get_enums()
    enum $(reg.get_name())_t{
        #for $i, $enum in enumerate($reg.get_enums())
        #set $end_comma = ',' if $i < len($reg.get_enums())-1 else ''
        $(reg.get_name().upper())_$(enum[0].upper()) = $enum[1]$end_comma
        #end for
    } $reg.get_name();
    #else
    boost::$reg.get_stdint_type() $reg.get_name();
    #end if
#end for

    ad9510_regs_t(void){
#for $reg in $regs
        $reg.get_name() = $reg.get_default();
#end for
    }

    boost::uint8_t get_reg(boost::uint16_t addr){
        boost::uint8_t reg = 0;
        switch(addr){
        #for $addr in sorted(set(map(lambda r: r.get_addr(), $regs)))
        case $addr:
            #for $reg in filter(lambda r: r.get_addr() == addr, $regs)
            reg |= (boost::uint32_t($reg.get_name()) & $reg.get_mask()) << $reg.get_shift();
            #end for
            break;
        #end for
        }
        return reg;
    }

    boost::uint32_t get_write_reg(boost::uint16_t addr){
        return (boost::uint32_t(addr) << 8) | get_reg(addr);
    }

    boost::uint32_t get_read_reg(boost::uint16_t addr){
        return (boost::uint32_t(addr) << 8) | (1 << 15);
    }

};

\#endif /* INCLUDED_AD9510_REGS_HPP */
"""

class reg:
    def __init__(self, reg_des):
        x = re.match('^(\w*)\s*(\w*)\[(.*)\]\s*(\w*)\s*(.*)$', reg_des)
        name, addr, bit_range, default, enums = x.groups()

        #store variables
        self._name = name
        self._addr = int(addr, 16)
        if ':' in bit_range: self._addr_spec = map(int, bit_range.split(':'))
        else: self._addr_spec = int(bit_range), int(bit_range)
        self._default = int(default, 16)

        #extract enum
        self._enums = list()
        if enums:
            enum_val = 0
            for enum_str in map(str.strip, enums.split(',')):
                if '=' in enum_str:
                    enum_name, enum_val = enum_str.split('=')
                    enum_val = int(enum_val)
                else: enum_name = enum_str
                self._enums.append((enum_name, enum_val))
                enum_val += 1

    def get_addr(self): return self._addr
    def get_enums(self): return self._enums
    def get_name(self): return self._name
    def get_default(self):
        for key, val in self.get_enums():
            if val == self._default: return str.upper('%s_%s'%(self.get_name(), key))
        return self._default
    def get_stdint_type(self):
        if self.get_bit_width() <=  8: return 'uint8_t'
        if self.get_bit_width() <= 16: return 'uint16_t'
        if self.get_bit_width() <= 32: return 'uint32_t'
        if self.get_bit_width() <= 64: return 'uint64_t'
        raise Exception, 'too damn big'
    def get_shift(self): return self._addr_spec[0]
    def get_mask(self): return hex(int('1'*self.get_bit_width(), 2))
    def get_bit_width(self): return self._addr_spec[1] - self._addr_spec[0] + 1

if __name__ == '__main__':
    regs = map(reg, parse_tmpl(REGS_DATA_TMPL).splitlines())
    safe_makedirs(os.path.dirname(sys.argv[1]))
    open(sys.argv[1], 'w').write(parse_tmpl(HEADER_TEXT, regs=regs, file=__file__))
