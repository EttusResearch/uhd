#
# Copyright 2009 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

_SBF_MAGIC = 'SBF!'
_SBF_DONT_EXECUTE = 0x1
_SBF_MAX_SECTIONS = 14
_SBF_HEADER_LEN = 128

import struct
import sys
from pprint import pprint

def dump_data(f, offset, data):
    L = len(data) // 4
    for i in range(L):
        f.write('%08x:  %08x\n' % (offset + 4 * i, struct.unpack('>I', data[4*i:4*(i+1)])[0]))
    remainder = len(data) - L * 4
    if remainder != 0:
        f.write('%08x:  ' % (offset + L*4,))
        i = 0
        while i < remainder:
            f.write('%02x' % ((ord(data[L*4 + i]),)))
            i += 1
        f.write('\n')



class sec_desc(object):
    def __init__(self, target_addr, data):
        self.target_addr = target_addr
        self.data = data

    def __repr__(self):
        #print >>sys.stderr, "target_addr:", self.target_addr
        #print >>sys.stderr, "data:", self.data
        return "<sec_desc target_addr=0x%x len=%d>" % (
            self.target_addr, len(self.data))


class header(object):
    def __init__(self, entry, sections):
        self.entry = entry
        self.section = sections

    def dump(self, f):
        if self.entry == _SBF_DONT_EXECUTE:
            f.write("Entry: DONT_EXECUTE\n")
        else:
            f.write("Entry: 0x%x\n" % (self.entry,))
        for i in range(len(self.section)):
            s = self.section[i]
            f.write("Section[%d]: target_addr = 0x%x  length = %d\n" % (i,
                                                                        s.target_addr,
                                                                        len(s.data)))
            dump_data(f, s.target_addr, s.data)

    #
    # Returns an iterator.  Each yield returns (target_addr, data)
    #
    def iterator(self, max_piece=512):
        for s in self.section:
            offset = 0
            L = len(s.data)
            while offset < L:
                n = min(max_piece, L - offset)
                yield (s.target_addr + offset,
                       s.data[offset:offset+n])
                offset += n



def read_sbf(input_file):
    """Parse an SBF file"""
    
    f = input_file.read(_SBF_HEADER_LEN)
    #if len(f) < _SBF_HEADER_LEN or not f.startswith(_SBF_MAGIC):
        #raise ValueError, '%s: not an SBF file' % (input_file.name,)
    
    def extract(i):
        start = 16+8*i
        stop = start+8
        return struct.unpack('>2I', f[start:stop])

    def get_data(ss):
        L = ss[1]
        s = input_file.read(L)
        #if len(s) != L:
            #raise ValueError, '%s: file is too short' % (input_file.name(),)
        return s
        
    (magic, entry, nsections, reserved) = struct.unpack('>4s3I', f[0:16])
    assert nsections <= _SBF_MAX_SECTIONS
    descs = [extract(i) for i in range(nsections)]
    #pprint(descs, sys.stderr)
    data = map(get_data, descs)
    secs = map(lambda ss, data: sec_desc(ss[0], data), descs, data)
    return header(entry, secs)


def write_sbf(output_file, sbf_header):
    assert(len(sbf_header.section) <= _SBF_MAX_SECTIONS)
    sbf_header.nsections = len(sbf_header.section)
    f = output_file

    # write the file header
    f.write(struct.pack('>4s3I', _SBF_MAGIC, sbf_header.entry, sbf_header.nsections, 0))

    # write the section headers
    for i in range(sbf_header.nsections):
        f.write(struct.pack('>2I', 
                            sbf_header.section[i].target_addr,
                            len(sbf_header.section[i].data)))
    for i in range(_SBF_MAX_SECTIONS - sbf_header.nsections):
        f.write(struct.pack('>2I', 0, 0))

    # write the section data
    for i in range(sbf_header.nsections):
        f.write(sbf_header.section[i].data)

    return True
