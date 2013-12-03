#!/usr/bin/env python
#
# Copyright 2012 Ettus Research LLC
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


import xml.etree.ElementTree as et
import base64
from optparse import OptionParser


def main():
    parser = OptionParser()
    parser.add_option("-l", "--lvbitxfile", dest="lvbitxfile",
                  help="donor labview fpga bitfile", metavar="LVBITXFILE")

    parser.add_option("-b", "--bitfile", dest="bitfile",
                  help="xilinx generated bitfile", metavar="BITFILE")

    parser.add_option("-o", "--output", dest="outfile",
                  help="output labview fpga bitfile", metavar="OUTFILE")

    parser.add_option("-s", "--signature", dest="signature",
                  help="output labview fpga bitfile signature", metavar="SIGNATURE",
                  default="ABCDEFG")


    (options, args) = parser.parse_args()

    tree = et.parse(options.lvbitxfile)
    root = tree.getroot()
    bs = root.find('Bitstream')
    if bs is None: return

    print('Found "%s" tag in "%s"...' % (bs.tag, options.lvbitxfile))

    print('Writing old bitfile content to "%s"...' % (options.bitfile+'.bak'))
    f_old = open(options.bitfile+'.bak', 'w')
    f_old.write(base64.b64decode(bs.text))
    f_old.close()


    print('Reading new bitfile "%s"...' % options.bitfile)
    f = open(options.bitfile, 'r')
    newbs = base64.b64encode(f.read())
    f.close()


    bs.text = newbs
    print('Saving new labview bitfile to  "%s"...' % options.outfile)
    tree.write(options.outfile, xml_declaration=True, encoding='utf-8')

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
