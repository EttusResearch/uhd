#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""This module holds the Emulated registers for a Stream Endpoint Node.
One of these objects is instantiated for each Stream Endpoint Node.
"""
from enum import IntEnum

REG_EPID_SELF = 0x00 # RW
REG_RESET_AND_FLUSH = 0x04 # W
REG_OSTRM_CTRL_STATUS = 0x08 # RW
REG_OSTRM_DST_EPID = 0x0C # W
REG_OSTRM_FC_FREQ_BYTES_LO = 0x10 # W
REG_OSTRM_FC_FREQ_BYTES_HI = 0x14 # W
REG_OSTRM_FC_FREQ_PKTS = 0x18 # W
REG_OSTRM_FC_HEADROOM = 0x1C # W
REG_OSTRM_BUFF_CAP_BYTES_LO = 0x20 # R
REG_OSTRM_BUFF_CAP_BYTES_HI = 0x24 # R
REG_OSTRM_BUFF_CAP_PKTS = 0x28 # R
REG_OSTRM_SEQ_ERR_CNT = 0x2C # R
REG_OSTRM_DATA_ERR_CNT = 0x30 # R
REG_OSTRM_ROUTE_ERR_CNT = 0x34 # R
REG_ISTRM_CTRL_STATUS = 0x38 # RW

RESET_AND_FLUSH_OSTRM = (1 << 0)
RESET_AND_FLUSH_ISTRM = (1 << 1)
RESET_AND_FLUSH_CTRL = (1 << 2)
RESET_AND_FLUSH_ALL = 0x7

STRM_STATUS_FC_ENABLED = 0x80000000
STRM_STATUS_SETUP_ERR = 0x40000000
STRM_STATUS_SETUP_PENDING = 0x20000000

class SwBuff(IntEnum):
    """The size of the elements in a buffer"""
    BUFF_U64 = 0
    BUFF_U32 = 1
    BUFF_U16 = 2
    BUFF_U8 = 3

class CtrlStatusWord:
    """Represents a Control Status Word

    See mgmt_portal:BUILD_CTRL_STATUS_WORD()
    """
    def __init__(self, cfg_start, xport_lossy, pyld_buff_fmt, mdata_buff_fmt, byte_swap):
        self.cfg_start = cfg_start
        self.xport_lossy = xport_lossy
        self.pyld_buff_fmt = pyld_buff_fmt
        self.mdata_buff_fmt = mdata_buff_fmt
        self.byte_swap = byte_swap

    @classmethod
    def parse(cls, val):
        cfg_start = (val & 1) != 0
        xport_lossy = ((val >> 1) & 1) != 0
        pyld_buff_fmt = SwBuff((val >> 2) & 3)
        mdata_buff_fmt = SwBuff((val >> 4) & 3)
        byte_swap = ((val >> 6) & 1) != 0
        return cls(cfg_start, xport_lossy, pyld_buff_fmt, mdata_buff_fmt, byte_swap)

    def __str__(self):
        return "CtrlStatusWord{{cfg_start: {}, xport_lossy: {}, " \
               "pyld_buff_fmt: {}, mdata_buff_fmt: {}, byte_swap: {}}}" \
               .format(self.cfg_start, self.xport_lossy,
                       self.pyld_buff_fmt, self.mdata_buff_fmt, self.byte_swap)

class StreamEpRegs:
    """Represents a set of registers associated with a stream endpoint
    which can be accessed through management packets

    See mgmt_portal.cpp
    """
    def __init__(self, get_epid, set_epid, set_dst_epid, update_status_out,
                 update_status_in, cap_pkts, cap_bytes):
        self.get_epid = get_epid
        self.set_epid = set_epid
        self.set_dst_epid = set_dst_epid
        self.log = None
        self.out_ctrl_status = 0
        self.in_ctrl_status = 0
        self.update_status_out = update_status_out
        self.update_status_in = update_status_in
        self.cap_pkts = cap_pkts
        self.cap_bytes = cap_bytes

    def read(self, addr):
        if addr == REG_EPID_SELF:
            return self.get_epid()
        elif addr == REG_OSTRM_CTRL_STATUS:
            return self.out_ctrl_status
        elif addr == REG_ISTRM_CTRL_STATUS:
            return self.in_ctrl_status
        elif addr == REG_OSTRM_BUFF_CAP_BYTES_LO:
            return self.cap_bytes & 0xFFFFFFFF
        elif addr == REG_OSTRM_BUFF_CAP_BYTES_HI:
            return (self.cap_bytes >> 32) & 0xFFFFFFFF
        elif addr == REG_OSTRM_BUFF_CAP_PKTS:
            return self.cap_pkts
        else:
            raise NotImplementedError("Unable to read addr 0x{:08X} from stream ep regs"
                                      .format(addr))

    def write(self, addr, val):
        if addr == REG_EPID_SELF:
            self.log.debug("Setting EPID to {}".format(val))
            self.set_epid(val)
        elif addr == REG_OSTRM_CTRL_STATUS:
            status = CtrlStatusWord.parse(val)
            self.log.debug("Setting EPID Output Stream Ctrl Status: {}".format(status))
            new_status = self.update_status_out(status)
            self.out_ctrl_status = new_status if new_status is not None else val
        elif addr == REG_RESET_AND_FLUSH:
            self.log.trace("Stream EP Regs Reset and Flush")
        elif addr == REG_OSTRM_DST_EPID:
            self.log.debug("Setting Dest EPID to {}".format(val))
            self.set_dst_epid(val)
        elif REG_OSTRM_FC_FREQ_BYTES_LO <= addr <= REG_OSTRM_FC_HEADROOM:
            pass # TODO: implement these Flow Control parameters
        elif addr == REG_ISTRM_CTRL_STATUS:
            status = CtrlStatusWord.parse(val)
            self.log.debug("Setting EPID Input Stream Ctrl Status: {}".format(status))
            new_status = self.update_status_in(status)
            self.in_ctrl_status = new_status if new_status is not None else val
        else:
            raise NotImplementedError("Unable to write addr 0x{:08X} from stream ep regs"
                                      .format(addr))
