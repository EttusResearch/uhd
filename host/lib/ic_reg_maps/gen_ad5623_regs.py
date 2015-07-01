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
data             0[4:15]           0
addr             0[16:18]          0       DAC_A=0, DAC_B=1, ALL=7
cmd              0[19:21]          0       wr_input_n, up_dac_n, wr_input_n_up_all, wr_up_dac_chan_n, power_down, reset, load_ldac
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
boost::uint32_t get_reg(void){
    boost::uint32_t reg = 0;
    % for reg in filter(lambda r: r.get_addr() == 0, regs):
    reg |= (boost::uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
    % endfor
    return reg;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='ad5623_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
