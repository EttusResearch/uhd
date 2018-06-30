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
data             0[4:15]           0
addr             0[16:18]          0       DAC_A=0, DAC_B=1, ALL=7
cmd              0[19:21]          0       wr_input_n, up_dac_n, wr_input_n_up_all, wr_up_dac_chan_n, power_down, reset, load_ldac
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint32_t get_reg(void){
    uint32_t reg = 0;
    % for reg in filter(lambda r: r.get_addr() == 0, regs):
    reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
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
