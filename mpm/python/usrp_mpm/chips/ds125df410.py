#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
DS125DF410 driver class

For use with TI's retimer chip
"""

import math
from builtins import object
from usrp_mpm.mpmlog import get_logger

LINE_RATE_PRESETS = {'Ethernet': 0xF6, 'CPRI1': 0x36, 'CPRI2': 0x46}
ALL_CHANS = range(4)


class DS125DF410(object):
    """
    Driver class for DS125DF410 access.

    Inputs:
    regs_iface : regs_iface bus driver to access device
    parent_log : logger of parent
    """

    # (deemphasis, swing)
    DRIVER_PRESETS = {}

    def __init__(self, regs_iface, parent_log=None):
        self.log = \
            parent_log.getChild("DS125DF410") if parent_log is not None \
            else get_logger("DS125DF410")
        self.regs_iface = regs_iface
        # Set channel select register to control set
        self.regs_iface.poke8(0xFF, 0)
        # Probe chip ID
        chip_id = self.regs_iface.peek8(0x01)
        assert chip_id == 0xd1
        self.log.debug("Probed DS125DF410 retimer")
        for chan in ALL_CHANS:
            self._set_chan_select(chan)
            # Reset channel registers
            self.regs_iface.poke8(0x00, 0x04)
            # Enable DFE mode
            self.regs_iface.poke8(0x1E, 0xE1)
            self.regs_iface.poke8(0x31, 0x40)

    def _rmw(self, addr, data, mask):
        """ Read, modify, write """
        base = self.regs_iface.peek8(addr) & ~mask
        data = (data & mask) | base
        self.regs_iface.poke8(addr, data)

    def _set_chan_select(self, chan):
        """
        Channel select
        """
        assert chan in ALL_CHANS
        self.regs_iface.poke8(0xFF, chan + 4)

    def set_rate_preset(self, preset, channels=None):
        """
        Set rate preset
        """
        channels = channels or ALL_CHANS
        assert preset in LINE_RATE_PRESETS.keys()
        for chan in channels:
            self._set_chan_select(chan)
            self.regs_iface.poke8(0x2F, LINE_RATE_PRESETS[preset])

    def set_rate(self, rate, channels=None):
        """
        Set rate
        """
        channels = channels or ALL_CHANS
        self.log.trace("Writing custom line rate {}".format(rate))
        ppm_val = int(math.ceil(rate*1280.0))
        assert ppm_val <= 0x7FFF
        for chan in channels:
            assert chan in ALL_CHANS
            self._set_chan_select(chan)
            # Set VCO divider to 1
            self.regs_iface.poke8(0x2F, 0xC6)
            # Set frequency range detection
            self.regs_iface.poke8(0x60, (ppm_val & 0x00FF))
            self.regs_iface.poke8(0x62, (ppm_val & 0x00FF))
            self.regs_iface.poke8(0x61, 0x80 | ((ppm_val >> 8) & 0x00FF))
            self.regs_iface.poke8(0x63, 0x80 | ((ppm_val >> 8) & 0x00FF))
            # Set VCO tolerance range to max
            self.regs_iface.poke8(0x64, 0xFF)

    def set_driver_preset(self, preset, channels=None):
        """
        Set driver preset
        """
        channels = channels or ALL_CHANS
        assert preset in self.DRIVER_PRESETS.keys()
        self.log.trace("Setting retimer's driver for " + preset + " preset")
        deemphasis, swing = self.DRIVER_PRESETS[preset]
        for chan in channels:
            self._set_chan_select(chan)
            self._rmw(0x15, deemphasis, 0x47)
            self._rmw(0x2D, swing, 0x07)

