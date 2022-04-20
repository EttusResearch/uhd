#!/usr/bin/env python3
#
# Copyright 2019 Ettus Research, a National Instruments Brand
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

# This program is derived from uhd/firmware/usrp3/x300/x300_debug.py.

import argparse
import socket
import struct
import sys

########################################################################
# constants
########################################################################
X300_FW_COMMS_UDP_PORT = 49152

X300_FW_COMMS_FLAGS_ACK = 1
X300_FW_COMMS_FLAGS_ERROR = 2
X300_FW_COMMS_FLAGS_POKE32 = 4
X300_FW_COMMS_FLAGS_PEEK32 = 8

X300_FW_RESET_REG = 0x100058
X300_FW_RESET_DATA = 1

#UDP_CTRL_PORT = 49183
UDP_MAX_XFER_BYTES = 1024
UDP_TIMEOUT = 3

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
        self._sock.connect((addr, X300_FW_COMMS_UDP_PORT))
        self.set_callbacks(lambda *a: None, lambda *a: None)
        #self.init_update() #check that the device is there

    def set_callbacks(self, progress_cb, status_cb):
        self._progress_cb = progress_cb
        self._status_cb = status_cb

    def send(self,pkt):
        self._sock.send(pkt)

    def poke(self, poke_addr, poke_data):
        out_pkt = pack_reg_peek_poke_fmt(
            X300_FW_COMMS_FLAGS_POKE32|X300_FW_COMMS_FLAGS_ACK,
            seq(), poke_addr, poke_data)
        self.send(out_pkt)

########################################################################
# command line options
########################################################################
def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--addr", help="X300 device address", default='')
    return parser.parse_args()

########################################################################
# main
########################################################################
if __name__=='__main__':
    args = get_args()

    if not args.addr:
        raise Exception('no address specified')

    status = ctrl_socket(addr=args.addr)

    sys.stdout.write("Sending reset command...")
    sys.stdout.flush()
    status.poke(X300_FW_RESET_REG, X300_FW_RESET_DATA)
    print("Done.")
