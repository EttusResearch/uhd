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
from . import lib
from .base import DboardManagerBase
import struct
from logging import getLogger

LOG = getLogger(__name__)


class magnesium(DboardManagerBase):
    """
    Holds all dboard specific information and methods of the magnesium dboard
    """
    hw_pid = 2
    special_eeprom_addrs = {"special0": "something"}
    spi_chipselect = {"0": "lmk", "1": "mykonos", "2": "random"}
    spidevs = {}
    lmk = ""
    mykonos = ""
    random = ""

    def __init__(self, spi_devices, eeprom_data, *args, **kwargs):
        # eeprom_data is a tuple (head_dict, raw_data)
        if len(spi_devices) != len(self.spi_chipselect):
            LOG.error("Expected {0} spi devices, found {1} spi devices".format(len(spi_devices), len(self.spi_chipselect)))
            exit(1)
        for spi in spi_devices:
            device = self.spi_chipselect.get(spi[-1], None)
            if self.chipselect is None:
                LOG.error("Unexpected chipselect {0}".format(spi[-1]))
                exit(1)
            setattr(self, device, spi)
        super(magnesium, self).__init__(*args, **kwargs)

    def init_device(self):
        """
        Execute necessary init dance to bring up dboard
        """
        LOG.debug("initialize hardware")

        self._device = lib.dboards.magnesium(
            self.lmk, self.mykonos, self.random)

    def read_eeprom_v1(self, data):
        """
        read eeprom data version 1
        """
        # magnesium eeprom contains
        # nothing
        return struct.unpack_from("x", data)
