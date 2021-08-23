#!/usr/bin/env python3
"""
Parser for Xilinx bitfiles.

Reads metadata from Xilinx bitfile headers.
"""

import argparse
import os
import sys
import struct
import re

# Parse command line options
def get_args():
    """Run argparser"""
    parser = argparse.ArgumentParser(description='Parser for the Xilinx FPGA Bitfile')
    parser.add_argument("bitfile", help="Input bitfile path")
    parser.add_argument("--bin_out", help="Output bin file path")
    parser.add_argument('--flip', action='store_true', default=False, help='Flip 32-bit endianess')
    parser.add_argument('--info', action='store_true', default=False, help='Print bitfile info')
    args = parser.parse_args()
    if not os.path.isfile(args.bitfile):
        print('ERROR: Bitfile ' + args.bitfile + ' could not be accessed or is not a file.\n')
        parser.print_help()
        sys.exit(1)
    return args

def parse_bitfile(bitfile_bytes):
    """
    Parse bitfile
    """
    short = struct.Struct('>H')
    ulong = struct.Struct('>I')
    keynames = {'a': 'design_name', 'b': 'part_name', 'c': 'date', 'd': 'time'}

    header = dict()
    ptr = 0
    #Field 1
    if short.unpack(bitfile_bytes[ptr:ptr+2])[0] == 9 and \
            ulong.unpack(bitfile_bytes[ptr+2:ptr+6])[0] == 0x0ff00ff0:
        #Headers
        ptr += short.unpack(bitfile_bytes[ptr:ptr+2])[0] + 2
        ptr += short.unpack(bitfile_bytes[ptr:ptr+2])[0] + 1
        #Fields a-d
        for _ in range(0, 4):
            key = chr(bitfile_bytes[ptr])
            ptr += 1
            val_len = short.unpack(bitfile_bytes[ptr:ptr+2])[0]
            ptr += 2
            val = bitfile_bytes[ptr:ptr+val_len]
            ptr += val_len
            header[keynames[key]] = val.decode('ascii').rstrip('\0')
        #Field e
        ptr += 1
        length = ulong.unpack(bitfile_bytes[ptr:ptr+4])[0]
        ptr += 4
        header['bitstream_len'] = length
        header['header_len'] = ptr
        data = bitfile_bytes[ptr:ptr+length]
        return (header, data)
    raise Exception('Bitfile header validation failed!')

def flip32(data):
    """
    Flip 32-bit endianness
    """
    le32_struct = struct.Struct('<I')
    be32_struct = struct.Struct('>I')
    data_flipped = bytearray(len(data))
    for offset in range(0, len(data), 4):
        be32_struct.pack_into(
            data_flipped,
            offset,
            le32_struct.unpack_from(memoryview(data), offset)[0])
    return data_flipped

def main():
    """GoGoGo"""
    args = get_args()
    with open(args.bitfile, 'rb') as bit_file:
        # Parse bytes into a header map and data buffer
        (header, data) = parse_bitfile(bit_file.read())
        # Print bitfile info
        if args.info:
            keynames = {
                'COMPRESS': 'Compression: ',
                'UserID': 'User ID: ',
                'Version': 'Vivado Version: '}
            name_fields = header['design_name'].split(';')
            for field in name_fields:
                mobj = re.search('(.+)=(.+)', field)
                if mobj:
                    if mobj.group(1) in keynames:
                        print(keynames[mobj.group(1)] + mobj.group(2))
                    else:
                        print(mobj.group(1) + ': ' + mobj.group(2))
                else:
                    print('Design Name: ' + field)
            print('Part Name: ' + header['part_name'])
            print('Datestamp: ' + header['date'] + ' ' + header['time'])
            print('Bitstream Size: ' + str(header['bitstream_len']))
        # Write a bin file
        if args.bin_out:
            open(args.bin_out, 'wb').write(flip32(data) if args.flip else data)

if __name__ == '__main__':
    main()
