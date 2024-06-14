#!/usr/bin/env python3

import binascii
import sys

if len(sys.argv) < 2:
    print("Usage: bin_to_coe.py <bin file> <coe file>")
    sys.exit(1)

bin_file = sys.argv[1]
coe_file = sys.argv[2]

# Read the binary file in binary mode
with open(bin_file, 'rb') as f:
    h = binascii.hexlify(f.read()) + b'0'*7

# Convert binary to hexadecimal and format for COE file
coe_str = 'memory_initialization_radix=16;\n'
coe_str += 'memory_initialization_vector=\n'
coe_str += ',\n'.join(h[i:i+32].decode('ascii') for i in range(0, len(h), 32))
coe_str += ';\n'

# Write the COE formatted string to the output file
with open(coe_file, 'w') as f:
    f.write(coe_str)
