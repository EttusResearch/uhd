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
result           0[0:11]        0
mod              0[12]          0
chn              0[13]          0
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
uint16_t get_reg(void){
    uint16_t reg = 0;
    % for reg in filter(lambda r: r.get_addr() == 0, regs):
    reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
    % endfor
    return reg;
}

void set_reg(uint16_t reg){
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
