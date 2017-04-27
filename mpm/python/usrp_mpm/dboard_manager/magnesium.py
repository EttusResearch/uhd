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

class Magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    hw_pid = 2
    special_eeprom_addrs = {"special0": "something"}
    # Maps the chipselects to the corresponding devices:
    spi_chipselect = {"lmk": 0, "mykonos": 1}

    def __init__(self, spi_devices, eeprom_data, *args, **kwargs):
        super(Magnesium, self).__init__(*args, **kwargs)
        self.log = get_logger("Magnesium")
        # eeprom_data is a tuple (head_dict, raw_data)
        if len(spi_devices) != len(self.spi_chipselect):
            self.log.error("Expected {0} spi devices, found {1} spi devices".format(
                len(self.spi_chipselect), len(spi_devices),
            ))
            raise RuntimeError("Not enough SPI devices found.")
        self._spi_nodes = {}
        for k, v in iteritems(self.spi_chipselect):
            self._spi_nodes[k] = spi_devices[v]
        self.log.debug("spidev device node map: {}".format(self._spi_nodes))

    def init_device(self):
        """
        Execute necessary init dance to bring up dboard
        """
        self.log.debug("Loading C++ drivers...")
        self._device = lib.dboards.magnesium_manager(
            self._spi_nodes['lmk'],
            self._spi_nodes['mykonos'],
        )
        self.lmk = self._device.get_clock_ctrl()
        self.mykonos = self._device.get_radio_ctrl()
        self.log.debug("Loaded C++ drivers.")
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
        self.lmk.verify_chip_id()
        self.log.trace("Enabling SYSREF pulses...")
        self.lmk.enable_sysref_pulse()

        self.jesdcore.unreset_mmcm()

        self.jesdcore.init()
        self.log.trace("Resetting Mykonos...")
        self.reset_mykonos() #not sure who owns the reset

        self.log.trace("Initializing Mykonos...")
        self.mykonos.begin_initialization()
        self.jesdcore.send_sysref_pulse()
        self.jesdcore.send_sysref_pulse()
        self.mykonos.finish_initialization()

        self.log.trace("Stargin Mykonos JESD framing...")
        self.mykonos.start_jesd_rx()
        self.jesdcore.init_deframer()
        self.jesdcore.init_framer()
        self.mykonos.start_jesd_tx()

        #TODO add function for Enable FPGA LMFC Generator
        self.jesdcore.send_sysref_pulse()
        time.sleep(0.2)
        if not self.jesdcore.get_framer_status():
            raise Exception('JESD Core Framer is not synced!')
        if not self.jesdcore.get_deframer_status():
            raise Exception('JESD Core Deframer is not synced!')
        #if (!self.mykonos.get_framer_status())
        #    raise Exception('Mykonos Framer is not synced!')
        #if (!self.mykonos.get_deframer_status())
        #    raise Exception('Mykonos Deframer is not synced!')

    def reset_mykonos(self):
        " Toggle reset line on Mykonos "
        # SUPER GHETTO FIXME
        import os
        os.system('devmem2 0x4001000C w 2') # Active low reset
        time.sleep(0.001)
        os.system('devmem2 0x4001000C w 10')

    def read_eeprom_v1(self, data):
        """
        read eeprom data version 1
        """
        # magnesium eeprom contains
        # nothing
        return struct.unpack_from("x", data)
