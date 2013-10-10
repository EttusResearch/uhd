#!/usr/bin/python
import sys
import os
import struct

def flip32(data):
	sl = struct.Struct('<I')
	sb = struct.Struct('>I')
	b = buffer(data)
	d = bytearray(len(data))
	for offset in xrange(0, len(data), 4):
		 sb.pack_into(d, offset, sl.unpack_from(b, offset)[0])
	return d

import argparse
parser = argparse.ArgumentParser(description='Convert FPGA bit files to raw bin format suitable for flashing')
parser.add_argument('-f', '--flip', dest='flip', action='store_true', default=False, help='Flip 32-bit endianess (needed for Zynq)')
parser.add_argument("bitfile", help="Input bit file name")
parser.add_argument("binfile", help="Output bin file name")
args = parser.parse_args()

short = struct.Struct('>H')
ulong = struct.Struct('>I')

bitfile = open(args.bitfile, 'rb')

l = short.unpack(bitfile.read(2))[0]
if l != 9:
	raise Exception, "Missing <0009> header (0x%x), not a bit file" % l
bitfile.read(l)
l = short.unpack(bitfile.read(2))[0]
d = bitfile.read(l)
if d != 'a':
	raise Exception, "Missing <a> header, not a bit file"

l = short.unpack(bitfile.read(2))[0]
d = bitfile.read(l)
print "Design name:", d

KEYNAMES = {'b': "Partname", 'c': "Date", 'd': "Time"}

while 1:
	k = bitfile.read(1)
	if not k:
		raise Exception, "unexpected EOF"
	elif k == 'e':
		l = ulong.unpack(bitfile.read(4))[0]
		print "found binary data:", l
		d = bitfile.read(l)
		if args.flip:
			d = flip32(d)
		open(args.binfile, 'wb').write(d)
		break
	elif k in KEYNAMES:
		l = short.unpack(bitfile.read(2))[0]
		d = bitfile.read(l)
		print KEYNAMES[k], d
	else:
		print "Unexpected key: ", k
		l = short.unpack(bitfile.read(2))[0]
		d = bitfile.read(l)

