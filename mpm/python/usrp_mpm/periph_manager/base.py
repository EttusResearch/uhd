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
Mboard implementation base class
"""

import re
import os
from . import lib
from . import types
from . import dboard_manager


class periph_manager(object):
    # stores discovered device information in dicts
    dboards = {}
    mboard_if_addrs = {}
    mboard_overlays = {}
    # this information has to be provided by
    # the specific periph_manager implementation
    mboard_eeprom_addr = ""
    dboard_eeprom_addrs = {}
    dboard_spimaster_addrs = {}

    def __init__(self):
        # I know my EEPROM address, lets use it
        helper = lib.udev_helper.udev_helper()
        (self._eeprom_head, self._eeprom_rawdata) = types.eeprom().read_eeprom(helper.get_eeprom(self.mboard_eeprom_addr))
        self._dboard_eeproms = {}
        for dboard_slot, eeprom_addr in self.dboard_eeprom_addrs.iteritems():
            spi_devices = []
            # I know EEPROM adresses for my dboard slots
            eeprom_data = types.eeprom().read(helper.get_eeprom(eeprom_addr))
            # I know spidev masters on the dboard slots
            hw_pid = eeprom_data.get("hw_pid", 0)
            if hw_pid in dboards.hw_pids:
                spi_devices = helper.get_spidev_nodes(self.dboard_spimaster_addrs.get(dboard_slot))
            dboard = dboards.hw_pids.get(hw_pid, dboards.unknown)
            self.dboards.update({dboard_slot: dboard(spi_devices, eeprom_data)})

    def get_overlays(self):
        self.mboard_overlays = []
        for f in os.listdir("/lib/firmware/"):
            if f.endswith(".dtbo"):
                self.mboard_overlays.append(f.strip(".dtbo"))

    def check_overlay(self):
        for f in os.listdir("/sys/kernel/device-tree/overlays/"):
            pass #do stuff

    def get_serial(self):
        return self._serial

    def load_fpga_image(self, target=None):
        pass

    def init_device(self, *args, **kwargs):
        # Load FPGA
        # Init dboards
        pass

