#!/usr/bin/python

import sys
import binascii

if __name__ == '__main__':
    bin_file = sys.argv[1]
    coe_file = sys.argv[2]

    #parse bin file into hex lines
    h = binascii.hexlify(open(bin_file).read()) + '0'*7
    d = [h[i*8:(i+1)*8] for i in range(len(h)/8)]

    #write output coe file
    out = open(coe_file, 'w')
    out.write('memory_initialization_radix=16;\n')
    out.write('memory_initialization_vector=\n')
    out.write(',\n'.join([h[i*8:(i+1)*8] for i in range(len(h)/8)]) + ';')
