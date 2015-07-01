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
result           0[0:11]        0
mod              0[12]          0
chn              0[13]          0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
boost::uint16_t get_reg(void){
    boost::uint16_t reg = 0;
    % for reg in filter(lambda r: r.get_addr() == 0, regs):
    reg |= (boost::uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
    % endfor
    return reg;
}

void set_reg(boost::uint16_t reg){
    % for reg in filter(lambda r: r.get_addr() == 0, regs):
    ${reg.get_name()} = ${reg.get_type()}((reg >> ${reg.get_shift()}) & ${reg.get_mask()});
    % endfor
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad7922_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
