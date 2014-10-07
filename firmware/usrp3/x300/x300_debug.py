#!/usr/bin/env python
#
# Copyright 2010-2011 Ettus Research LLC
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

import optparse
import math
import socket
import struct


########################################################################
# constants
########################################################################
B250_FW_COMMS_UDP_PORT = 49152

B250_FW_COMMS_FLAGS_ACK = 1
B250_FW_COMMS_FLAGS_ERROR = 2
B250_FW_COMMS_FLAGS_POKE32 = 4
B250_FW_COMMS_FLAGS_PEEK32 = 8

#UDP_CTRL_PORT = 49183
UDP_MAX_XFER_BYTES = 1024
UDP_TIMEOUT = 3
#USRP2_FW_PROTO_VERSION = 11 #should be unused after r6

#REG_ARGS_FMT = '!LLLLLB15x'
#REG_IP_FMT = '!LLLL20x'
REG_PEEK_POKE_FMT = '!LLLL'


_seq = -1
def seq():
    global _seq
    _seq = _seq+1
    return _seq


########################################################################
# helper functions
########################################################################

def unpack_reg_peek_poke_fmt(s):
    return struct.unpack(REG_PEEK_POKE_FMT,s) #(flags, seq, addr, data)

def pack_reg_peek_poke_fmt(flags, seq, addr, data):
    return struct.pack(REG_PEEK_POKE_FMT, flags, seq, addr, data);

########################################################################
# Burner class, holds a socket and send/recv routines
########################################################################
class ctrl_socket(object):
    def __init__(self, addr):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.settimeout(UDP_TIMEOUT)
        self._sock.connect((addr, B250_FW_COMMS_UDP_PORT))
        self.set_callbacks(lambda *a: None, lambda *a: None)
        #self.init_update() #check that the device is there

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send_and_recv(self, pkt):
        self._sock.send(pkt)
        return self._sock.recv(UDP_MAX_XFER_BYTES)

    def read_router_stats(self):
        print
        print("            "),
        ports = ['        eth0','        eth1','      radio0','      radio1','    compute0','    compute1','    compute2','        pcie']
        for in_prt in ports:
            print("%s" % in_prt),
        print("   Egress Port")
        print("             "),
        for in_prt in range (0, 8):
            print("____________"),
        print
        for in_prt in range (0, 8):
            print("%s |" % ports[in_prt]),
            for out_prt in range (0, 8):
                out_pkt = pack_reg_peek_poke_fmt(B250_FW_COMMS_FLAGS_PEEK32|B250_FW_COMMS_FLAGS_ACK, seq(), 0xA000+256+((in_prt*8+out_prt)*4), 0)
                in_pkt = self.send_and_recv(out_pkt)
                (flags, rxseq, addr, data) = unpack_reg_peek_poke_fmt(in_pkt)
                if flags & B250_FW_COMMS_FLAGS_ERROR == B250_FW_COMMS_FLAGS_ERROR:
                    raise Exception("B250 peek returns error code")
                print("%10d  " % (data)),
            print
        print
        print("Ingress Port")
        print


    def peek(self,peek_addr):
        out_pkt = pack_reg_peek_poke_fmt(B250_FW_COMMS_FLAGS_PEEK32|B250_FW_COMMS_FLAGS_ACK, seq(), peek_addr, 0)
        in_pkt = self.send_and_recv(out_pkt)
        (flags, rxseq, addr, data) = unpack_reg_peek_poke_fmt(in_pkt)
        if flags & B250_FW_COMMS_FLAGS_ERROR == B250_FW_COMMS_FLAGS_ERROR:
            raise Exception("B250 peek of address %d returns error code" % (addr))
        print("PEEK of address %d(0x%x) reads %d(0x%x)" % (addr,addr,data,data))

    def poke(self,poke_addr,poke_data):
        out_pkt = pack_reg_peek_poke_fmt(B250_FW_COMMS_FLAGS_POKE32|B250_FW_COMMS_FLAGS_ACK, seq(), poke_addr, poke_data)
        in_pkt = self.send_and_recv(out_pkt)
        (flags, rxseq, addr, data) = unpack_reg_peek_poke_fmt(in_pkt)
        if flags & B250_FW_COMMS_FLAGS_ERROR == B250_FW_COMMS_FLAGS_ERROR:
            raise Exception("B250 peek of address %d returns error code" % (addr))
        print("POKE of address %d(0x%x) with %d(0x%x)" % (poke_addr,poke_addr,poke_data,poke_data)  )


########################################################################
# command line options
########################################################################
def get_options():
    parser = optparse.OptionParser()
    parser.add_option("--addr", type="string",                 help="USRP-N2XX device address",       default='')
    parser.add_option("--list", action="store_true",           help="list possible network devices", default=False)
    parser.add_option("--peek", type="int",                 help="Read from memory map",     default=None)
    parser.add_option("--poke", type="int",                 help="Write to memory map",     default=None)
    parser.add_option("--data", type="int",                 help="Data for poke",     default=None)
    parser.add_option("--stats", action="store_true",           help="Display SuperMIMO Network Stats", default=False)
    (options, args) = parser.parse_args()

    return options


########################################################################
# main
########################################################################
if __name__=='__main__':
    options = get_options()


    if options.list:
        print('Possible network devices:')
        print('  ' + '\n  '.join(enumerate_devices()))
        exit()

    if not options.addr: raise Exception('no address specified')

    status = ctrl_socket(addr=options.addr)

    if options.stats:
        status.read_router_stats()


    if options.peek is not None:
        addr = options.peek
        status.peek(addr)

    if options.poke is not None and options.data is not None:
        addr = options.poke
        data = options.data
        status.poke(addr,data)
