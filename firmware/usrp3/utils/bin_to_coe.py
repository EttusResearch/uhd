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
    binary_data = f.read()
    padding_length = (8 - (len(binary_data) * 2) % 8) % 8  # Calculate necessary padding
    h = binascii.hexlify(binary_data) + b'0' * padding_length

# Convert binary to hexadecimal and format for COE file
coe_str = 'memory_initialization_radix=16;\n'
coe_str += 'memory_initialization_vector=\n'
coe_str += ',\n'.join(h[i:i+8].decode('ascii') for i in range(0, len(h), 8))
coe_str += ';'

# Write the COE formatted string to the output file
with open(coe_file, 'w') as f:
    f.write(coe_str)
