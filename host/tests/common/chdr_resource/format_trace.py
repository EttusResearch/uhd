#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import sys
import re

if len(sys.argv) < 2:
    print("Please supply an input filename!")
    exit()

input_arg = sys.argv[1]
filename_pat = re.compile(r"(.*)\.c")
m = filename_pat.match(input_arg)
if not m:
    print("Please supply a .c file as input!")
    exit()
filename = m.group(1)

input_file = open(filename + ".c", "r")
output_file = open(filename + ".cpp", "w")

output_header = """//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <tuple>

"""

# Matches a line defining a new packet, e.g
# char peer1_19[] = { /* Packet 3153 */
# Group 1 is the array name, e.g. "peer1_19"
# Group 2 is the Commented Description, e.g. "Packet 3072"
define_pat = re.compile(r"^char (peer[0|1]_\d+)\[\] = \{ \/\* (Packet \d+) \*\/$")

# Matches a line in the middle of a packet or at the end, e.g.
# 0x01, 0x00, 0x18, 0x00, 0x02, 0x00, 0x80, 0x00, 
# 0x08, 0x00, 0x0f, 0x00 };
# No groups, use full match
other_pat = re.compile(r"^((?:0x[0-9a-f]{2}, )*(?:0x[0-9a-f]{2}))(?: \};|, )$")

# Used to seperate the two parties in the conversation (peer0 and peer1)
var_sort_pat = re.compile(r"^peer0_\d+$")

output_file.write(output_header)
output_file.write("namespace {} {{\n\n".format(filename))

var_names_peer_0 = []
var_names_peer_1 = []

while True:
    line = input_file.readline()
    if not line:
        break
    m = define_pat.match(line)
    if m:
        output_file.write("uint8_t {}[] = {{ // {}\n".format(m.group(1), m.group(2)))
        var_sort_match = var_sort_pat.match(m.group(1))
        if var_sort_match:
            var_names_peer_0.append(m.group(1))
        else:
            var_names_peer_1.append(m.group(1))
        continue
    m = other_pat.match(line)
    if m:
        output_file.write(line)
        continue
    print("Encountered unexpected line:\n{}".format(line))
    exit()

for peer_name, var_names in [("peer0", var_names_peer_0), ("peer1", var_names_peer_1)]:
    output_file.write("\n")
    output_file.write("size_t {}_len = {};\n".format(peer_name, len(var_names)))
    output_file.write("std::tuple<uint8_t*, size_t> {}[] = {{\n".format(peer_name))
    for var_name in var_names:
        output_file.write("\tstd::make_tuple({0}, sizeof({0})),\n".format(var_name))
    output_file.write("};\n")

output_file.write("\n")
output_file.write("}} // namespace {}\n".format(filename))
