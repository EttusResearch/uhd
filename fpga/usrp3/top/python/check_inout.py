#!/usr/bin/env python
#
# Copyright 2010 Ettus Research LLC
#


# Description:
# generates a list of inputs and outputs from the top-level Verilog file and cross-references them to the .ucf.
# outputs errors for pins that aren't found in the UCF, checks for capitalization errors and other common mistakes

import sys
import re

if __name__=='__main__':
  if len(sys.argv) == 2:
    print "Usage: %s <top level Verilog file> <pin definition UCF>"
    sys.exit(-1)

  verilog_filename = sys.argv[1]
  ucf_filename = sys.argv[2]

  verilog_file = open(verilog_filename, 'r')
  ucf_file = open(ucf_filename, 'r')

  verilog_iolist = list()
  ucf_iolist = list()

  #read in all input, inout, and output declarations and compile a list
  for line in verilog_file:
    for match in re.findall(r"(?:input|inout|output) (?:reg )*(?:\[.*\] )*(\w+)", line.split("//")[0]):
      verilog_iolist.append(match)

  for line in ucf_file:
      m = re.search(r"""NET "(\w+).*" """, line.split("#")[0])
      if m is not None:
        ucf_iolist.append(m.group(1))

  #now find corresponding matches and error when you don't find one
  #we search for .v defs without matching .ucf defs since the reverse isn't necessarily a problem
  err = False

  for item in verilog_iolist:
    if item not in ucf_iolist:
      print "Error: %s appears in the top-level Verilog file, but is not in the UCF definition file!" % item
      err = True

  if err:
    sys.exit(-1)

  print "No errors found."
  sys.exit(0)
