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
# Template for raw text data describing write registers
# name addr[bit range inclusive] default optional enums
########################################################################
WRITE_REGS_TMPL="""\
########################################################################
## Note: offsets given from perspective of data bits (excludes address)
########################################################################
##
########################################################################
## N-Divider MSB (0) Write
########################################################################
div2                  0[7]          0       div4, div2
n_divider_msb         0[0:6]        3
########################################################################
## N-Divider LSB (1) Write
########################################################################
n_divider_lsb         1[0:7]        0xB6
~n_divider            n_divider_lsb, n_divider_msb
########################################################################
## R, Charge Pump, and VCO (2) Write
########################################################################
<% r_divider_names = ', '.join(map(lambda x: 'div' + str(2**(x+1)), range(0,8))) %>\
r_divider             2[5:7]        1       ${r_divider_names}
<% cp_current_bias = ', '.join(map(lambda x: 'i_cp_%dua'%(50*2**x), range(0,4))) %>\
cp_current            2[3:4]        3       ${cp_current_bias}
osc_band              2[0:2]        5
########################################################################
## I/Q Filter DAC (3) Write
########################################################################
##unused              3[7]          0
## filter tuning dac, depends on m
f_dac                 3[0:6]        0x7F
########################################################################
## LPF Divider DAC (4) Write
########################################################################
adl_vco_adc_latch     4[7]          0       disabled, enabled
ade_vco_ade_read      4[6]          0       disabled, enabled
dl_output_drive       4[5]          0       iq_590m_vpp, iq_1_vpp
## filter tuning counter
m_divider             4[0:4]        2
########################################################################
## GC2 and Diag (5) Write
########################################################################
diag                  5[5:7]        0       normal, cp_i_source, cp_i_sink, cp_high_z, unused, n_and_filt, r_and_gc2, m_div
## Step Size: 0-1: 0dB, 2-22: 1dB, 23-31: 0.5dB
gc2                   5[0:4]        0x1F
"""

########################################################################
# Template for raw text data describing read registers
# name addr[bit range inclusive] default optional enums
########################################################################
READ_REGS_TMPL="""\
########################################################################
## Status (0) Read
########################################################################
pwr                   0[6]          0       not_reset, reset
## VCO tuning voltage, Lock Status
adc                   0[2:4]        0
########################################################################
## I/Q Filter DAC (1) Read
########################################################################
## I/Q Filter tuning DAC, current
filter_dac            1[0:6]        0
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
    return boost::uint8_t(reg);
}

void set_reg(boost::uint8_t addr, boost::uint8_t reg){
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
"""

if __name__ == '__main__':
    import common; common.generate(
        name='max2118_write_regs',
        regs_tmpl=WRITE_REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )

    import common; common.generate(
        name='max2118_read_regs',
        regs_tmpl=READ_REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
        append=True,
    )
