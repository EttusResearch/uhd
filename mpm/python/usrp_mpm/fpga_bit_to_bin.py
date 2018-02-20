#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Convert FPGA Bit files to bin files suitable for flashing
"""
import argparse
import struct

def parse_args():
    """Parse arguments when running this as a script"""
    parser_help = 'Convert FPGA bit files to raw bin format suitable for flashing'
    parser = argparse.ArgumentParser(description=parser_help)
    parser.add_argument('-f', '--flip', dest='flip', action='store_true', default=False,
                        help='Flip 32-bit endianess (needed for Zynq)')
    parser.add_argument('-l', '--blen', type=int, default=-1,
                        help="Size of block (in words) to read at one time")
    parser.add_argument("bitfile", help="Input bit file name")
    parser.add_argument("binfile", help="Output bin file name")
    return parser.parse_args()


def bin_to_file(bitfile, binfilename, flip, blocksize):
    """Reads a raw bitstream, byte-swaps (if desired), and writes to a binfile"""
    with open(binfilename, 'wb') as binfile:
        while True:
            # Calculate how many bytes to read the requested blocksize
            # -1 feels a little cleaner than -1*(large number), but isn't necessary
            readlen = -1 if (blocksize <= 0) else blocksize * struct.calcsize('I')
            byteblock = bitfile.read(readlen)
            if flip:
                # Check how many ints we actually read
                actual_blocksize = int(len(byteblock) / struct.calcsize('I'))
                # Create the format string accordingly
                fmt_string = str(actual_blocksize)+"I"
                # Byte swap with an unpack and a pack with opposite endianness
                int_arr = struct.unpack(">"+fmt_string, byteblock)
                binstream = struct.pack("<"+fmt_string, *int_arr)
            else:
                # Don't need to do anything special
                binstream = byteblock
            binfile.write(binstream)
            # Check if we're done. Either we
            # a) read the entire file in one go, or
            # b) read a partial block in the last read
            if (blocksize == -1) or (len(byteblock) < readlen):
                break


def fpga_bit_to_bin(bitfilename, binfilename, flip=False, blocklen=-1):
    """Process the FPGA bit file at bitfilename, and write a bin file to binfilename"""
    # Read the header
    # The header consists of several fields, with keys and lengths to divide the file.
    with open(bitfilename, 'rb') as bitfile:
        # Field 0:
        # 2 byte length
        # Some header
        length = struct.unpack('>H', bitfile.read(2))[0]
        if length != 9:
            raise RuntimeError("Missing <0009> header (0x%i), not a bit file" % length)
        bitfile.read(length)  # Xilinx header
        # Field 1:
        # 2 byte length
        # The letter 'a'
        length = struct.unpack('>H', bitfile.read(2))[0]  # Should be 1
        key = bitfile.read(length)
        if key != b'a':
            raise RuntimeError("Missing <a> header, not a bit file")
        # 2 byte length
        # File name (with trailing 0x00)
        length = struct.unpack('>H', bitfile.read(2))[0]
        bitfile.read(length)  # read the design name
        # Field 2:
        # 1 byte key ('b')
        # 2 byte length
        # Part name (with trailing 0x00)
        # If bitstream is a partial bitstream, get some information from filename and header
        key = bitfile.read(1)
        if key != b'b':
            raise RuntimeError("Missing <b> header, not a bit file")
        length = struct.unpack('>H', bitfile.read(2))[0]
        part_name = bitfile.read(length)
        if b"PARTIAL=TRUE" in part_name:
            # TODO: Handle this when we need partial bitstreams
            raise NotImplementedError("Partial bitstream processing not implemented")
        # Field 3:
        # 1 byte key ('c')
        # 2 byte length
        # Date YYYY/MM/DD (with trailing 0x00)
        key = bitfile.read(1)
        if key != b'c':
            raise RuntimeError("Missing <c> Date key")
        length = struct.unpack('>H', bitfile.read(2))[0]
        bitfile.read(length)  # read the date
        # Field 4:
        # 1 byte key ('d')
        # 2 byte length
        # Time HH:MM:SS (with trailing 0x00)
        key = bitfile.read(1)
        if key != b'd':
            raise RuntimeError("Missing <d> Time key")
        length = struct.unpack('>H', bitfile.read(2))[0]
        bitfile.read(length)  # read the time
        # Field 5:
        # 1 byte key ('e')
        # 4 byte length
        # Raw bitstream
        key = bitfile.read(1)
        if key != b'e':
            raise RuntimeError("Missing <e> bitstream key.")
        length = struct.unpack('>I', bitfile.read(4))[0]  # The raw bitstream's length
        # Now read the raw bitstream and write to file
        bin_to_file(bitfile, binfilename, flip, blocklen)


if __name__ == "__main__":
    args = parse_args()
    fpga_bit_to_bin(args.bitfile, args.binfile, args.flip, args.blen)
