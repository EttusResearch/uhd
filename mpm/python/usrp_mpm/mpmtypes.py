#
# Copyright 2017 Ettus Research (National Instruments)
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
"""
MPM types
"""
import ctypes
from multiprocessing import Value
from multiprocessing import Array
from multiprocessing import RLock
import struct

MPM_RPC_PORT = 49601
MPM_DISCOVERY_PORT = 49600

MPM_DISCOVERY_MESSAGE = "MPM-DISC"


class SharedState(object):
    """
    Holds information which should be shared between processes
    Usage should be kept to a minimum
    """

    def __init__(self):
        self.lock = RLock()
        self.claim_status = Value(
            ctypes.c_bool,
            False, lock=self.lock)  # lock
        self.claim_token = Array(
            ctypes.c_char, 256,
            lock=self.lock)  # String with max length of 256


class SID(object):
    def __init__(self, sid=0):
        self.src_addr = sid >> 24
        self.src_ep = (sid >> 16) & 0xFF
        self.dst_addr = (sid >> 8) & 0xFF
        self.dst_ep = sid & 0xFF

    def set_src_addr(self, new_addr):
        self.src_addr =  new_addr & 0xFF

    def set_dst_addr(self, new_addr):
        self.dst_addr = new_addr & 0xFF

    def set_src_ep(self, new_addr):
        self.src_ep = new_addr & 0xFF

    def set_dst_ep(self, new_addr):
        self.dst_ep = new_addr & 0xFF

    def reversed(self):
        """
        Return a reversed SID.
        """
        new_sid = SID(self.get())
        new_sid.src_addr, new_sid.dst_addr = new_sid.dst_addr, new_sid.src_addr
        new_sid.src_ep, new_sid.dst_ep = new_sid.dst_ep, new_sid.src_ep
        return new_sid

    def get(self):
        return (self.src_addr << 24) | (self.src_ep << 16) | (self.dst_addr << 8) | self.dst_ep

    def __repr__(self):
        return "{:02X}:{:02X}>{:02X}:{:02X}".format(
            self.src_addr, self.src_ep,
            self.dst_addr, self.dst_ep,
        )


