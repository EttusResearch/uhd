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
## Note: offsets given from perspective of data bits (excludes address)
########################################################################
##
########################################################################
## Address byte
########################################################################
adb                   0[1:2]        0
########################################################################
## Divider byte 1
########################################################################
db1                   1[0:6]        0x00
########################################################################
## Divider byte 2
########################################################################
db2                   2[0:7]        0x00
########################################################################
## Control byte 1
########################################################################
atp                   3[3:5]        0       112, 109, 106, 103, 100, 94, 94, disable
refdiv                3[0:2]        0       166.667khz, 142.857khz, 80khz, 62.5khz, 31.25khz, 50khz
########################################################################
## Band switch byte
########################################################################
cpsel                 4[6:7]        0
filterbw              4[4]          0       8mhz, 7mhz
bandsel               4[0:3]        0
########################################################################
## Control byte 2
########################################################################
atc                   5[5]          0       low, high
stby                  5[4]          0       standby, on
xto                   5[0]          0       disable, enable
"""

########################################################################
# Template for methods in the body of the struct
########################################################################
BODY_TMPL="""\
boost::uint8_t get_reg(boost::uint8_t addr){
    boost::uint8_t reg = 0;
    switch(addr){
    #for $addr in sorted(set(map(lambda r: r.get_addr(), $regs)))
    case $addr:
        #for $reg in filter(lambda r: r.get_addr() == addr, $regs)
        reg |= (boost::uint8_t($reg.get_name()) & $reg.get_mask()) << $reg.get_shift();
        #end for
        break;
    #end for
    }
    return boost::uint8_t(reg);
}

"""

if __name__ == '__main__':
    import common; common.generate(
        name='dtt75403_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )
