#!/usr/bin/env python

import struct
import sys

def do_8_words(ofile, which_ram, row, words):
    ofile.write("defparam bootram.RAM%d.INIT_%02X=256'h" % (which_ram, row))
    ofile.write("%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x;\n" % (
        words[7], words[6], words[5], words[4], words[3], words[2], words[1], words[0]))

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

    if len(words) > 2048:
        sys.stderr.write("bin_to_macro_init: error: input file %s is > 8KiB\n" % (bin_input_file,))
        sys.exit(1)

    # first 2KB
    for i in range(0, min(512, len(words)), 8):
        do_8_words(ofile, 0, i/8, words[i:i+8])

    # second 2KB
    for i in range(512, min(1024, len(words)), 8):
        do_8_words(ofile, 1, (i/8) % 64, words[i:i+8])

		# third 2KB
    for i in range(1024, min(1536, len(words)), 8):
        do_8_words(ofile, 2, (i/8) % 64, words[i:i+8])

		# last 2KB
    for i in range(1536, len(words), 8):
        do_8_words(ofile, 3, (i/8) % 64, words[i:i+8])

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("usage: bin_to_ram_macro_init bin_input_file ram_init_output_file\n")
        sys.exit(1)
    
    bin_to_ram_macro_init(sys.argv[1], sys.argv[2])
