#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
White Rabbit Control
"""

from builtins import object
from usrp_mpm.sys_utils.uio import UIO


class WhiteRabbitRegsControl(object):
    """
    Control and read the FPGA White Rabbit core registers
    """
    # Memory Map
    #  0x00000000: I/D Memory
    #  0x00020000: Peripheral interconnect
    #      +0x000: Minic
    #      +0x100: Endpoint
    #      +0x200: Softpll
    #      +0x300: PPS gen
    #      +0x400: Syscon
    #      +0x500: UART
    #      +0x600: OneWire
    #      +0x700: Auxillary space (Etherbone config, etc)
    #      +0x800: WRPC diagnostics registers
    PERIPH_INTERCON_BASE = 0x20000

    # PPS_GEN Map
    PPSG_ESCR = 0x31C

    def __init__(self, label, log):
        self.log = log
        self.regs = UIO(
            label=label,
            read_only=False
        )
        self.periph_peek32 = lambda addr: self.regs.peek32(addr + self.PERIPH_INTERCON_BASE)
        self.periph_poke32 = lambda addr, data: self.regs.poke32(addr + self.PERIPH_INTERCON_BASE, data)

    def get_time_lock_status(self):
        """
        Retrieves and decodes the lock status for the PPS out of the WR core.
        """
        with self.regs.open():
            ext_sync_status = self.periph_peek32(self.PPSG_ESCR)
        # bit 2: PPS_VALID
        # bit 3: TM_VALID (timecode)
        # All other bits MUST be ignored since they are not guaranteed to be zero or
        # stable!
        return (ext_sync_status & 0b1100) == 0b1100
