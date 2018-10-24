#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Gain table control for Rhodium
"""

from __future__ import print_function
from usrp_mpm.dboard_manager.gaintables_rh import RX_LOWBAND_GAIN_TABLE
from usrp_mpm.dboard_manager.gaintables_rh import RX_HIGHBAND_GAIN_TABLE
from usrp_mpm.dboard_manager.gaintables_rh import TX_LOWBAND_GAIN_TABLE
from usrp_mpm.dboard_manager.gaintables_rh import TX_HIGHBAND_GAIN_TABLE

#from usrp_mpm.dboard_manager.rhodium import Rhodium

###############################################################################
# Constants
###############################################################################

GAIN_TABLE_MIN_INDEX = 0
GAIN_TABLE_MAX_INDEX = 60
DSA1_MIN_INDEX = 0
DSA1_MAX_INDEX = 30
DSA2_MIN_INDEX = 0
DSA2_MAX_INDEX = 30

GAIN_TBL_SEL_ADDR = 6
GAIN_TBL_SEL_TX_SHIFT = 8
GAIN_TBL_SEL_RX_SHIFT = 0
GAIN_TBL_SEL_HIGH_BAND = 1
GAIN_TBL_SEL_LOW_BAND = 0

# convenience data values for GAIN_TBL_SEL
GAIN_TBL_SEL_DATA_BOTH_HIGH = \
    (GAIN_TBL_SEL_HIGH_BAND << GAIN_TBL_SEL_TX_SHIFT) | \
    (GAIN_TBL_SEL_HIGH_BAND << GAIN_TBL_SEL_RX_SHIFT)
GAIN_TBL_SEL_DATA_BOTH_LOW = \
    (GAIN_TBL_SEL_LOW_BAND << GAIN_TBL_SEL_TX_SHIFT) | \
    (GAIN_TBL_SEL_LOW_BAND << GAIN_TBL_SEL_RX_SHIFT)

###############################################################################
# Main class
###############################################################################

class GainTableRh():
    """
    CPLD gain table loader for Rhodium daughterboards
    """
    def __init__(self, cpld_regs, gain_tbl_regs, parent_log=None):
        self.log = parent_log.getChild("CPLDGainTbl") if parent_log is not None \
            else get_logger("CPLDGainTbl")
        self.cpld_regs = cpld_regs
        self.gain_tbl_regs = gain_tbl_regs
        assert hasattr(self.cpld_regs, 'poke16')
        assert hasattr(self.gain_tbl_regs, 'poke16')

    def _load_default_table(self, table, gain_table):
        def _create_spi_loader_message(table, index, dsa1, dsa2):
            addr = 0
            data = 0
            if table == "rx":
                tableindex = 1
            elif table == "tx":
                tableindex = 2
            else:
                raise RuntimeError("Invalid table selected in gain loader: " + table)
            addr |= (tableindex << 6)
            addr |= (index << 0)
            data |= (dsa1 << 5)
            data |= (dsa2 << 0)
            return addr, data
        for i in range(GAIN_TABLE_MIN_INDEX, GAIN_TABLE_MAX_INDEX):
            addr, data = _create_spi_loader_message(
                table,
                i,
                gain_table[i][0],
                gain_table[i][1])
            self.gain_tbl_regs.poke16(addr, data)

    def init(self):
        """
        Loads the default gain table values to the CPLD via SPI
        """
        self.log.trace("Loading gain tables to CPLD")
        self.cpld_regs.poke16(GAIN_TBL_SEL_ADDR, GAIN_TBL_SEL_DATA_BOTH_HIGH)
        self._load_default_table("rx", RX_HIGHBAND_GAIN_TABLE)
        self._load_default_table("tx", TX_HIGHBAND_GAIN_TABLE)
        self.cpld_regs.poke16(GAIN_TBL_SEL_ADDR, GAIN_TBL_SEL_DATA_BOTH_LOW)
        self._load_default_table("rx", RX_LOWBAND_GAIN_TABLE)
        self._load_default_table("tx", TX_LOWBAND_GAIN_TABLE)
        self.log.trace("Gain tables loaded")
