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
from . import lib # Pulls in everything from C++-land
from .base import DboardManagerBase
from .. import nijesdcore
from ..uio import uio

class magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    hw_pid = 2
    special_eeprom_addrs = {"special0": "something"}
    spi_chipselect = {"0": "lmk", "1": "mykonos",}
    spidevs = {}
    lmk = ""
    mykonos = ""
    random = ""

    def __init__(self, spi_devices, eeprom_data, *args, **kwargs):
        super(magnesium, self).__init__(*args, **kwargs)
        # eeprom_data is a tuple (head_dict, raw_data)
        if len(spi_devices) != len(self.spi_chipselect):
            self.log.error("Expected {0} spi devices, found {1} spi devices".format(
                len(self.spi_chipselect), len(spi_devices),
            ))
            exit(1)
        for spi in spi_devices:
            device = self.spi_chipselect.get(spi[-1], None)
            # if self.chipselect is None:
                # self.log.error("Unexpected chipselect {0}".format(spi[-1]))
                # exit(1)
            setattr(self, device, spi)
            self.log.debug("Setting spi device for {device}: {spidev}".format(
                device=device, spidev=spi
            ))

    def init_device(self):
        """
        Execute necessary init dance to bring up dboard
        """
        self.log.debug("initialize hardware")
        self._device = lib.dboards.magnesium_periph_manager(
            # self.lmk.encode('ascii'), self.mykonos.encode('ascii')
            '/dev/spidev0.0',
            '/dev/spidev0.1',
        )
        self.lmk = self._device.get_clock_ctrl()
        self.mykonos = self._device.get_radio_ctrl()

        # uio_path, uio_size = get_uio_node("misc-enet-regs0")
        self.log.debug("getting Mg A uio")
        uio_path = "/dev/uio2" # TODO use labels
        uio_size = 0x4000
        self.log.debug("got uio_path and size")
        self.uio = uio(uio_path, uio_size, read_only=False)
        self.log.info("got my uio")
        self.init_jesd(self.uio)

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
