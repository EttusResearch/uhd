#!/usr/bin/env python
#
# Copyright 2011-2012 Ettus Research LLC
#

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
