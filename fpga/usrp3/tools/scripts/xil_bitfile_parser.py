#!/usr/bin/python

import argparse
import os, sys
import struct
import re

# Parse command line options
def get_options():
    parser = argparse.ArgumentParser(description='Parser for the Xilinx FPGA Bitfile')
    parser.add_argument("bitfile", help="Input bitfile path")
    parser.add_argument("--bin_out", help="Output bin file path")
    parser.add_argument('--flip', action='store_true', default=False, help='Flip 32-bit endianess')
    parser.add_argument('--info', action='store_true', default=False, help='Print bitfile info')
    args = parser.parse_args()
    if (not os.path.isfile(args.bitfile)):
        print('ERROR: Bitfile ' + args.bitfile + ' could not be accessed or is not a file.\n')
        parser.print_help()
        sys.exit(1)
    return args

short = struct.Struct('>H')
ulong = struct.Struct('>I')
KEYNAMES = {'a':'design_name', 'b':'part_name', 'c':'date', 'd':'time'}

# Parse bitfile
def parse_bitfile(bitfile_bytes):
    header = dict()
    ptr = 0
    #Field 1
    if short.unpack(bitfile_bytes[ptr:ptr+2])[0] == 9 and ulong.unpack(bitfile_bytes[ptr+2:ptr+6])[0] == 0x0ff00ff0:
        #Headers
        ptr += short.unpack(bitfile_bytes[ptr:ptr+2])[0] + 2
        ptr += short.unpack(bitfile_bytes[ptr:ptr+2])[0] + 1
        #Fields a-d
        for keynum in range(0, 4):
            key = bitfile_bytes[ptr]; ptr += 1
            val_len = short.unpack(bitfile_bytes[ptr:ptr+2])[0]; ptr += 2
            val = bitfile_bytes[ptr:ptr+val_len]; ptr += val_len
            header[KEYNAMES[key]] = str(val).rstrip('\0')
        #Field e
        ptr += 1
        length = ulong.unpack(bitfile_bytes[ptr:ptr+4])[0]; ptr += 4
        header['bitstream_len'] = length
        header['header_len'] = ptr
        data = bitfile_bytes[ptr:ptr+length]
        return (header, data)
    else:
        raise Exception('Bitfile header validation failed!') 

# Flip 32-bit endianess
def flip32(data):
    sl = struct.Struct('<I')
    sb = struct.Struct('>I')
    b = buffer(data)
    d = bytearray(len(data))
    for offset in xrange(0, len(data), 4):
         sb.pack_into(d, offset, sl.unpack_from(b, offset)[0])
    return d

def main():
    args = get_options();
    with open(args.bitfile, 'rb') as bit_file:
        # Parse bytes into a header map and data buffer
        (header, data) = parse_bitfile(bit_file.read())
        # Print bitfile info
        if args.info:
            m = re.search('(.+);UserID=(.+);COMPRESS=(.+);Version=(.+)', header['design_name'])
            if m:
                print 'Design Name: ' + m.group(1)
                print 'User ID: ' + m.group(2)
                print 'Compression: ' + m.group(3)
                print 'Vivado Version: ' + m.group(4)
            else:
                print 'Design Name: ' + header['design_name']
            print 'Part Name: ' + header['part_name']
            print 'Datestamp: ' + header['date'] + ' ' + header['time']
            print 'Bitstream Size: ' + str(header['bitstream_len'])
        # Write a bin file
        if args.bin_out:
            open(args.bin_out, 'wb').write(flip32(data) if args.flip else data)

if __name__ == '__main__':
    main()