#!/usr/bin/env python

import struct
import sys
import io

BOOTRAM_SIZE = 16384
#BOOTRAM_SIZE = 32768

def do_8_words(ofile, which_ram, row, words):
    ofile.write("%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,\n" % (
        words[0], words[1], words[2], words[3], words[4], words[5], words[6], words[7]))

def bin_to_ram_macro_init(bin_input_file, ram_init_output_file):
    ifile = open(bin_input_file, 'rb')
    ofile = open(ram_init_output_file, 'w')
    idata = ifile.read()
    idata_words = len(idata) / 4
    fmt = ">%dI"%idata_words
    words = struct.unpack(fmt, idata[:idata_words*4])

    # pad to a multiple of 8 words
    r = len(words) % 8
    if r != 0:
        words += (8 - r) * (0,)

    if len(words) > (BOOTRAM_SIZE / 4):
        sys.stderr.write("bin_to_macro_init: error: input file %s is > %dKiB\n" % (bin_input_file,BOOTRAM_SIZE))
        sys.exit(1)

    ofile.write("memory_initialization_radix=16;\n")
    ofile.write("memory_initialization_vector=\n")

    for q in range(0, BOOTRAM_SIZE/4, 512):
        for i in range(q, min(q+512, len(words)), 8):
            do_8_words(ofile, int(q / 1024), (i/8) % 128, words[i:i+8])

    ofile.seek(-2,io.SEEK_END)
    ofile.write(";")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("usage: bin_to_ram_macro_init bin_input_file ram_init_output_file\n")
        sys.exit(1)
    
    bin_to_ram_macro_init(sys.argv[1], sys.argv[2])
