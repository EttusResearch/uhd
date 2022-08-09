#!/usr/bin/env python

import struct
import sys

hextab = ('0000', '0001', '0010', '0011',
          '0100', '0101', '0110', '0111',
          '1000', '1001', '1010', '1011',
          '1100', '1101', '1110', '1111')

def w_to_binary_ascii(w):
    return ''.join([hextab[(w >> 4*i) & 0xf] for i in range(7,-1,-1)])

def bin_to_mif(bin_input_file, mif_output_file):
    ifile = open(bin_input_file, 'rb')
    ofile = open(mif_output_file, 'w')
    idata = ifile.read()
    fmt = ">%dI" % ((len(idata) / 4),)
    words = struct.unpack(fmt, idata)
    for w in words:
        ofile.write(w_to_binary_ascii(w))
        ofile.write('\n')

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("usage: bin_to_mif bin_input_file mif_output_file\n")
        raise SystemExit, 1
    
    bin_to_mif(sys.argv[1], sys.argv[2])
