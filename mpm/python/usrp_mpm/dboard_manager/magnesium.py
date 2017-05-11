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
magnesium dboard implementation module
"""

import struct
import time
from six import iteritems
from . import lib # Pulls in everything from C++-land
from .base import DboardManagerBase
from .. import nijesdcore
from ..uio import UIO
from ..mpmlog import get_logger
from .lmk_mg import LMK04828Mg

def create_spidev_iface(dev_node):
    """
    Create a regs iface from a spidev node
    """
    SPI_SPEED_HZ = 1000000
    SPI_MODE = 3
    SPI_ADDR_SHIFT = 8
    SPI_DATA_SHIFT = 0
    SPI_READ_FLAG = 1<<23
    SPI_WRIT_FLAG = 0
    return lib.spi.make_spidev_regs_iface(
        dev_node,
        SPI_SPEED_HZ,
        SPI_MODE,
        SPI_ADDR_SHIFT,
        SPI_DATA_SHIFT,
        SPI_READ_FLAG,
        SPI_WRIT_FLAG
    )

class Magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    #########################################################################
    # Overridables
    #
    # See DboardManagerBase for documentation on these fields
    #########################################################################
    pids = [0x150]

    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"lmk": 0, "mykonos": 1}

    def __init__(self, slot_idx, **kwargs):
        super(Magnesium, self).__init__(*args, **kwargs)
        self.log = get_logger("Magnesium")
        # eeprom_data is a tuple (head_dict, raw_data)

    def init_device(self):
        """
        Execute necessary init dance to bring up dboard
        """
        self.clock_regs = create_spidev_iface(self._spi_nodes['lmk'])
        self.log.debug("Loading C++ drivers...")
        self._device = lib.dboards.magnesium_manager(
            self._spi_nodes['mykonos'],
        )
        self.spi_lock = self._device.get_spi_lock()
        self.mykonos = self._device.get_radio_ctrl()
        self.log.debug("Loaded C++ drivers.")
        self.lmk = LMK04828Mg(self.clock_regs, self.spi_lock)

        self.log.debug("Getting Mg A uio...")
        self.radio_regs = UIO(label="jesd204b-regs", read_only=False)
        self.log.info("Radio-register UIO object successfully generated!")
        self.init_jesd(self.radio_regs)

    def init_jesd(self, uio):
        """
        Bring up the JESD link between Mykonos and the N310
        """
        self.log.trace("Creating jesdcore object")
        self.jesdcore = nijesdcore.NIMgJESDCore(uio)

        self.log.trace("Checking JESD core...")
        self.jesdcore.check_core()
        self.log.trace("Initializing LMK...")
        self.lmk.init()
        self.radio_regs.poke32(0x2078, 0xA000040)
        self.log.trace("Verify LMK Chip ID...")
        self.lmk.verify_chip_id()

        self.jesdcore.unreset_mmcm()

        self.jesdcore.init()
        self.log.trace("Resetting Mykonos...")
        self.jesdcore.reset_mykonos() #not sure who owns the reset

        self.log.trace("Initializing Mykonos...")
        self.mykonos.begin_initialization()
        self.jesdcore.send_sysref_pulse()
        self.jesdcore.send_sysref_pulse()
        self.mykonos.finish_initialization()

        self.log.trace("Starting Mykonos framer...")
        self.mykonos.start_jesd_rx()
        self.jesdcore.send_sysref_pulse()
        self.log.trace("Resetting FPGA deframer...")
        self.jesdcore.init_deframer()
        self.log.trace("Resetting FPGA framer...")
        self.jesdcore.init_framer()
        self.log.trace("Starting Mykonos deframer...")
        self.mykonos.start_jesd_tx()

        self.log.trace("Enable LMFC and send")
        self.jesdcore.enable_lmfc()
        self.jesdcore.send_sysref_pulse()
        time.sleep(0.2)
        if not self.jesdcore.get_framer_status():
            raise Exception('JESD Core Framer is not synced!')
        if not self.jesdcore.get_deframer_status():
            raise Exception('JESD Core Deframer is not synced!')
        if (self.mykonos.get_framer_status() & 0xFF != 0x3E):
            raise Exception('Mykonos Framer is not synced!')
        if (self.mykonos.get_deframer_status() & 0x7F != 0x28):
            raise Exception('Mykonos Deframer is not synced!')
        self.log.trace("JESD fully synced and ready")

    def dump_jesd_core(self):
        for i in range(0x2000, 0x2110, 0x10):
            print("0x%04X " % i),
            for j in range(0, 0x10, 0x4):
                print("%08X" % self.radio_regs.peek32(i + j)),
            print("")

    def read_eeprom_v1(self, data):
        """
        read eeprom data version 1
        """
        # magnesium eeprom contains
        # nothing
        return struct.unpack_from("x", data)

