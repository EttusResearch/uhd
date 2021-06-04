#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Common code for all MPM devices
"""

import datetime
from usrp_mpm.sys_utils.uio import UIO

class MboardRegsCommon:
    """
    Parent class for mboard regs that are common between *all* MPM devices
    """
    # pylint: disable=bad-whitespace
    # Global Registers
    MB_COMPAT_NUM          = 0x0000
    MB_DATESTAMP           = 0x0004
    MB_GIT_HASH            = 0x0008
    MB_SCRATCH             = 0x000C
    MB_DEVICE_ID           = 0x0010
    MB_RFNOC_INFO          = 0x0014
    MB_NUM_TIMEKEEPERS     = 0x0048
    # Timekeeper registers
    MB_TIME_NOW_LO         = 0x1000
    MB_TIME_NOW_HI         = 0x1004
    MB_TIME_EVENT_LO       = 0x1008
    MB_TIME_EVENT_HI       = 0x100C
    MB_TIME_CTRL           = 0x1010
    MB_TIME_LAST_PPS_LO    = 0x1014
    MB_TIME_LAST_PPS_HI    = 0x1018
    MB_TIME_BASE_PERIOD_LO = 0x101C
    MB_TIME_BASE_PERIOD_HI = 0x1020
    MB_TIMEKEEPER_OFFSET   = 12
    # Timekeeper control words
    MB_TIME_SET_NOW       = 0x0001
    MB_TIME_SET_NEXT_PPS  = 0x0002
    MB_TIME_SET_NEXT_SYNC = 0x0004
    # Bitfield locations for the MB_RFNOC_INFO register.
    MB_RFNOC_INFO_PROTO_VER  = 0
    MB_RFNOC_INFO_CHDR_WIDTH = 16
    # pylint: enable=bad-whitespace

    def __init__(self, label, log):
        self.log = log.getChild('MBRegs')
        self.regs = UIO(
            label=label,
            read_only=False
        )
        self.poke32 = self.regs.poke32
        self.peek32 = self.regs.peek32

    ###########################################################################
    # Device ID
    ###########################################################################
    def set_device_id(self, device_id):
        """
        Set device ID
        """
        with self.regs:
            self.log.trace("Writing MB_DEVICE_ID with 0x{:08X}".format(device_id))
            return self.poke32(self.MB_DEVICE_ID, device_id)

    def get_device_id(self):
        """
        Get device ID
        """
        with self.regs:
            regs_val = self.peek32(self.MB_DEVICE_ID)
            device_id = regs_val & 0x0000ffff
            self.log.trace("Read MB_DEVICE_ID 0x{:08X}".format(device_id))
            return device_id

    ###########################################################################
    # FPGA Identification
    ###########################################################################
    def get_compat_number(self):
        """get FPGA compat number

        This function reads back FPGA compat number.
        The return is a tuple of 2 numbers:
        (major compat number, minor compat number)
        """
        with self.regs:
            compat_number = self.peek32(self.MB_COMPAT_NUM)
        minor = compat_number & 0xff
        major = (compat_number>>16) & 0xff
        return (major, minor)

    def get_build_timestamp(self):
        """
        Returns the build date/time for the FPGA image.
        The return value is a datetime string in ISO 8601 format
        (YYYY-MM-DD HH:MM:SS.mmmmmm)
        """
        with self.regs:
            datestamp_rb = self.peek32(self.MB_DATESTAMP)
        if datestamp_rb > 0:
            dt_str = datetime.datetime(
                year=((datestamp_rb>>17)&0x3F)+2000,
                month=(datestamp_rb>>23)&0x0F,
                day=(datestamp_rb>>27)&0x1F,
                hour=(datestamp_rb>>12)&0x1F,
                minute=(datestamp_rb>>6)&0x3F,
                second=((datestamp_rb>>0)&0x3F))
            self.log.trace("FPGA build timestamp: {}".format(str(dt_str)))
            return str(dt_str)
        # Compatibility with FPGAs without datestamp capability
        return ''

    def get_git_hash(self):
        """
        Returns the GIT hash for the FPGA build.
        The return is a tuple of 2 numbers:
        (short git hash, string: dirty/clean)
        """
        with self.regs:
            git_hash_rb = self.peek32(self.MB_GIT_HASH)
        git_hash = git_hash_rb & 0x0FFFFFFF
        tree_dirty = ((git_hash_rb & 0xF0000000) > 0)
        dirtiness_qualifier = 'dirty' if tree_dirty else 'clean'
        self.log.trace("FPGA build GIT Hash: {:07x} ({})".format(
            git_hash, dirtiness_qualifier))
        return (git_hash, dirtiness_qualifier)

    def get_proto_ver(self):
        """
        Return RFNoC protocol version
        """
        with self.regs:
            reg_val = self.peek32(self.MB_RFNOC_INFO)
            proto_ver = (reg_val & 0x0000ffff) >> self.MB_RFNOC_INFO_PROTO_VER
            self.log.trace("Read RFNOC_PROTO_VER 0x{:08X}".format(proto_ver))
            return proto_ver

    def get_chdr_width(self):
        """
        Return RFNoC CHDR width
        """
        with self.regs:
            reg_val = self.peek32(self.MB_RFNOC_INFO)
            chdr_width = (reg_val & 0xffff0000) >> self.MB_RFNOC_INFO_CHDR_WIDTH
            self.log.trace("Read RFNOC_CHDR_WIDTH 0x{:08X}".format(chdr_width))
            return chdr_width

    ###########################################################################
    # Timekeeper API
    ###########################################################################
    def get_num_timekeepers(self):
        """
        Return the number of timekeepers
        """
        with self.regs:
            return self.peek32(self.MB_NUM_TIMEKEEPERS)

    def get_timekeeper_time(self, tk_idx, last_pps):
        """
        Get the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        next_pps: If True, get time at last PPS. Otherwise, get time now.
        """
        addr_lo = \
            (self.MB_TIME_LAST_PPS_LO if last_pps else self.MB_TIME_NOW_LO) + \
            tk_idx * self.MB_TIMEKEEPER_OFFSET
        addr_hi = addr_lo + 4
        with self.regs:
            time_lo = self.peek32(addr_lo)
            time_hi = self.peek32(addr_hi)
        return (time_hi << 32) | time_lo

    def set_timekeeper_time(self, tk_idx, ticks, next_pps):
        """
        Set the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        ticks: Time in ticks
        next_pps: If True, set time at next PPS. Otherwise, set time now.
        """
        addr_lo = \
            self.MB_TIME_EVENT_LO + tk_idx * self.MB_TIMEKEEPER_OFFSET
        addr_hi = \
            self.MB_TIME_EVENT_HI + tk_idx * self.MB_TIMEKEEPER_OFFSET
        addr_ctrl = \
            self.MB_TIME_CTRL + tk_idx * self.MB_TIMEKEEPER_OFFSET
        time_lo = ticks & 0xFFFFFFFF
        time_hi = (ticks >> 32) & 0xFFFFFFFF
        time_ctrl = self.MB_TIME_SET_NEXT_PPS if next_pps else self.MB_TIME_SET_NOW
        self.log.trace("Setting time on timekeeper %d to %d %s", tk_idx, ticks,
                       ("on next pps" if next_pps else "now"))
        with self.regs:
            self.poke32(addr_lo, time_lo)
            self.poke32(addr_hi, time_hi)
            self.poke32(addr_ctrl, time_ctrl)

    def set_tick_period(self, tk_idx, period_ns):
        """
        Set the time per tick in nanoseconds (tick period)

        Arguments:
        tk_idx: Index of timekeeper
        period_ns: Period in nanoseconds
        """
        addr_lo = self.MB_TIME_BASE_PERIOD_LO + tk_idx * self.MB_TIMEKEEPER_OFFSET
        addr_hi = addr_lo + 4
        period_lo = period_ns & 0xFFFFFFFF
        period_hi = (period_ns >> 32) & 0xFFFFFFFF
        with self.regs:
            self.poke32(addr_lo, period_lo)
            self.poke32(addr_hi, period_hi)
