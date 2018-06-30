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
## Note: offsets given from perspective of data bits (excludes address)
########################################################################
## Divider byte 1
########################################################################
db1                   0[0:6]        0x00
########################################################################
## Divider byte 2
########################################################################
db2                   1[0:7]        0x00
########################################################################
## Control byte 1
########################################################################
cb7                   2[7]          0x01
cp                    2[6]          0x00     low,high
os                    2[0]          0x00     on,off
rs                    2[1:2]        0x00     d512=3,d640=0,d1024=1
test                  2[3:5]        0x01     normal=0x01,cpoff=0x02,cpsink=0x06,cpsrc=0x07,cptest1=0x04,cptest2=0x05
########################################################################
## Control byte 2
########################################################################
bandsel               3[4:7]        0x03     uhf=0x03,vhfhi=0x09,vhflo=0x0a
power                 3[3]          0x00     on,off
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
    return uint8_t(reg);
}

"""

if __name__ == '__main__':
    import common; common.generate(
        name='tuner_4937di5_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
