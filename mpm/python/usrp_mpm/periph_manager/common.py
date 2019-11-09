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

class MboardRegsCommon(object):
    """
    Parent class for mboard regs that are common between *all* MPM devices
    """
    # pylint: disable=bad-whitespace
    # Global Registers
    MB_COMPAT_NUM          = 0x0000
    MB_DATESTAMP           = 0x0004
    MB_GIT_HASH            = 0x0008
    MB_SCRATCH             = 0x000C
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
