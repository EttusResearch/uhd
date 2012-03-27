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

import os
import sys

ALL_MAP_FILES = """\
./N2x0/build-N210R4/u2plus_map.map N210
./N2x0/build-N200R4/u2plus_map.map N200
./USRP2/build/u2_rev3_map.map USRP2
./E1x0/build-E100/u1e_map.map E100
./E1x0/build-E110/u1e_map.map E110
./B100/build-B100/B100_map.map B100
"""

def extract_map_from_file(path):
    output = ''
    found = False
    for line in open(path).readlines():
        if line.strip() == 'Mapping completed.': found = False
        if line.strip() == 'Logic Utilization:': found = True
        if found: output += line
    return output

def extract_maps():
    output = ''
    for line in ALL_MAP_FILES.splitlines():
        path, name = line.split()
        if not os.path.exists(path):
            print 'DNE ', path, ' skipping...'
        output += """



########################################################################
## %s Usage Summary
########################################################################

%s"""%(name, extract_map_from_file(path).strip())
    return output + '\n\n'

if __name__ == '__main__':
    summary = extract_maps()
    if len(sys.argv) == 1: print summary
    else: open(sys.argv[1], 'w').write(summary)
