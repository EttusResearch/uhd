#!/usr/bin/env python
#
# Copyright 2011-2012 Ettus Research LLC
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

import sys
import re

def print_timing_constraint_summary(twr_file):
    output = ""
    keep = False
    done = False
    try: open(twr_file)
    except IOError:
        print "cannot open or find %s; no timing summary to print!"%twr_file
        exit(-1)
    for line in open(twr_file).readlines():
        if 'Derived Constraint Report' in line: keep = True
        if 'constraint' in line and 'met' in line: done = True
        if not keep and done: keep = True
        if keep: output += line
        if done: break
    print("\n\n"+output)

if __name__=='__main__': map(print_timing_constraint_summary, sys.argv[1:])
