#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM types
"""

import ctypes
from multiprocessing import Value
from multiprocessing import Array
from multiprocessing import RLock
from builtins import object

MPM_RPC_PORT = 49601
MPM_DISCOVERY_PORT = 49600
MPM_DISCOVERY_MESSAGE = "MPM-DISC"

class SharedState(object):
    """
    Holds information which should be shared between processes.
    """
    def __init__(self):
        self.lock = RLock()
        self.claim_status = Value(ctypes.c_bool, False, lock=self.lock)
        self.system_ready = Value(ctypes.c_bool, False, lock=self.lock)
        # String with max length of 256:
        self.claim_token = Array(ctypes.c_char, 256, lock=self.lock)
        self.dev_type = Array(ctypes.c_char, 16, lock=self.lock)
        self.dev_serial = Array(ctypes.c_char, 8, lock=self.lock)
        self.dev_product = Array(ctypes.c_char, 16, lock=self.lock)

class SID(object):
    """
    Python representation of a 32-bit SID.
    """
    def __init__(self, sid=None):
        sid = sid or 0
        if isinstance(sid, str):
            src, dst = sid.split(">")
            if src.find(':') != -1:
                self.src_addr, self.src_ep = \
                    [int(x, 16) for x in src.split(':', 2)]
            else:
                self.src_addr, self.src_ep = \
                    [int(x, 10) for x in src.split('.', 2)]
            if dst.find(':') != -1:
                self.dst_addr, self.dst_ep = \
                    [int(x, 16) for x in dst.split(':', 2)]
            else:
                self.dst_addr, self.dst_ep = \
                    [int(x, 10) for x in dst.split('.', 2)]
        else:
            self.src_addr = sid >> 24
            self.src_ep = (sid >> 16) & 0xFF
            self.dst_addr = (sid >> 8) & 0xFF
            self.dst_ep = sid & 0xFF

    def set_src_addr(self, new_addr):
        " Set source address (e.g. 02:30>00:01 -> 2) "
        self.src_addr = new_addr & 0xFF

    def set_dst_addr(self, new_addr):
        " Set destination address (e.g. 02:30>00:01 -> 0) "
        self.dst_addr = new_addr & 0xFF

    def set_src_ep(self, new_addr):
        " Set source endpoint (e.g. 02:30>00:01 -> 0x30) "
        self.src_ep = new_addr & 0xFF

    def set_dst_ep(self, new_addr):
        " Set destination endpoint (e.g. 02:30>00:01 -> 0) "
        self.dst_ep = new_addr & 0xFF

    def get_dst_block(self):
        " Get destination  block 2:30 or 2:31 --> 23 "
        return (self.dst_addr<<4)&((self.dst_ep & 0xF0)>>4);

    def get_dst_ep_port(self):
        " Get destination endpoint "
        return (self.dst_ep & 0x0F);

    def reversed(self):
        """
        Return a reversed SID.
        """
        new_sid = SID(self.get())
        new_sid.src_addr, new_sid.dst_addr = new_sid.dst_addr, new_sid.src_addr
        new_sid.src_ep, new_sid.dst_ep = new_sid.dst_ep, new_sid.src_ep
        return new_sid

    def get(self):
        " Return SID as 32-bit number "
        return (self.src_addr << 24) | (self.src_ep << 16) | (self.dst_addr << 8) | self.dst_ep

    def __repr__(self):
        return "{:02X}:{:02X}>{:02X}:{:02X}".format(
            self.src_addr, self.src_ep,
            self.dst_addr, self.dst_ep,
        )


