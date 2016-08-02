#!/usr/bin/env python
#
# Copyright 2004,2006 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
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
# 

import re
import sys
import os, os.path
from optparse import OptionParser

# USB Vendor and Product ID's

USRP1VID = 0xfffe                            # Free Software Folks
USRP1PID = 0x0002
USRP1PVID= 0x2500                            # Ettus Research
USRP1PPID= 0x0002
    
def msb (x):
    return (x >> 8) & 0xff

def lsb (x):
    return x & 0xff

def build_eeprom_image (filename, rev):
    """Build a ``C2 Load'' EEPROM image.

    For details on this format, see section 3.4.3 of
    the EZ-USB FX2 Technical Reference Manual
    """
    # get the code we want to run
    with open(filename, 'rb') as f:
        out_bytes = f.read()

    devid = 4 #for compatibility
    start_addr = 0 #prove me wrong

    if(rev == 1):
      VID = USRP1VID
      PID = USRP1PID
    else:
      VID = USRP1PVID
      PID = USRP1PPID

    rom_header = [
        0xC2,                           # boot from EEPROM
        lsb (VID),
        msb (VID),
        lsb (PID),
        msb (PID),
        lsb (devid),
        msb (devid),
        0                               # configuration byte
        ]
    
    # 4 byte header that indicates where to load
    # the immediately follow code bytes.
    code_header = [
        msb (len (out_bytes)),
        lsb (len (out_bytes)),
        msb (start_addr),
        lsb (start_addr)
        ]

    # writes 0 to CPUCS reg (brings FX2 out of reset)
    trailer = [
        0x80,
        0x01,
        0xe6,
        0x00,
        0x00
        ]

    image = rom_header + code_header + [ord(c) for c in out_bytes] + trailer

    assert (len (image) <= 256)
    return image 

if __name__ == '__main__':
    usage = "usage: %prog -r REV [options] bootfile.bin outfile.bin"
    parser = OptionParser (usage=usage)
    parser.add_option ("-r", "--rev", type="int", default=-1,
                       help="Specify USRP revision, 1 for USRP1, 2 for USRP1P")
    (options, args) = parser.parse_args ()
    if len (args) != 2:
        parser.print_help ()
        sys.exit (1)
    if options.rev < 0:
        sys.stderr.write (
            "You must specify the USRP revision number (2 or 4) with -r REV\n")
        sys.exit (1)

    infile = args[0]
    outfile = args[1]

    image = "".join(chr(c) for c in build_eeprom_image(infile, options.rev))

    # Opening in binary mode -> why converting image to str
    with open(outfile, 'wb') as f:
        f.write( str(image) )