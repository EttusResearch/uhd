#!/usr/bin/env python3
#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import sys
import re

if len(sys.argv) < 2:
    print("Please supply an input filename!")
    sys.exit()

input_arg = sys.argv[1]
filename_pat = re.compile(r"(.*)\.c")
m = filename_pat.match(input_arg)
if not m:
    print("Please supply a .c file as input!")
    sys.exit()
filename = m.group(1)

input_file = open(filename + ".c", "r")
cpp_file = open(filename + ".cpp", "w")
python_file = open(filename + ".py", "w")

OUTPUT_HEADER_CPP = """//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <tuple>

// clang-format off
"""

OUTPUT_HEADER_PYTHON = """#
# Copyright 2020 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

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

cpp_file.write(OUTPUT_HEADER_CPP)
cpp_file.write("namespace {} {{\n\n".format(filename))

python_file.write(OUTPUT_HEADER_PYTHON)

var_names_peer_0 = []
var_names_peer_1 = []

while True:
    line = input_file.readline()
    if not line:
        break
    m = define_pat.match(line)
    if m:
        cpp_file.write("uint8_t {}[] = {{ // {}\n".format(m.group(1), m.group(2)))
        python_file.write("{} = bytes([ # {}\n".format(m.group(1), m.group(2).strip()))
        var_sort_match = var_sort_pat.match(m.group(1))
        if var_sort_match:
            var_names_peer_0.append(m.group(1))
        else:
            var_names_peer_1.append(m.group(1))
        continue
    m = other_pat.match(line)
    if m:
        cpp_file.write(line)
        python_file.write(line.replace("};", "])"))
        continue
    print("Encountered unexpected line:\n{}".format(line))
    sys.exit()

for peer_name, var_names in [("peer0", var_names_peer_0), ("peer1", var_names_peer_1)]:
    cpp_file.write("\n")
    cpp_file.write("size_t {}_len = {};\n".format(peer_name, len(var_names)))
    cpp_file.write("std::tuple<uint8_t*, size_t> {}[] = {{\n".format(peer_name))

    python_file.write("\n")
    python_file.write("{} = [\n\t".format(peer_name))

    for var_name in var_names:
        cpp_file.write("    std::make_tuple({0}, sizeof({0})),\n".format(var_name))
    cpp_file.write("};\n")

    for i, var_name in enumerate(var_names):
        python_file.write(var_name)
        if i + 1 < len(var_names):
            python_file.write(",\t")
            if (i + 1) % 10 == 0:
                python_file.write("\n\t")
    python_file.write("\n]\n")

cpp_file.write("\n")
cpp_file.write("}} // namespace {}\n".format(filename))
