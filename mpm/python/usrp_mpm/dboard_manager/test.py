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
from logging import getLogger

LOG = getLogger(__name__)


class fake_spi(object):
    def __init__(self, addr):
        self.addr = addr


class test_device(object):
    def __init__(self, dev1, dev2, dev3):
        self.dev1 = fake_spi(dev1)
        self.dev2 = fake_spi(dev2)
        self.dev3 = fake_spi(dev3)

    def test_method1(self, argument):
        return argument


class test(DboardManagerBase):
    hw_pid = 234
    special_eeprom_addrs = {"special0": "something"}
    spi_chipselect = {"0": "dev1", "1": "dev2", "2": "dev3"}
    spidevs = {}

    def __init__(self, *args, **kwargs):
        # eeprom_data is a tuple (head_dict, raw_data)
        super(test, self).__init__(*args, **kwargs)
        # I'm the test device, I can fake out my EEPROM
        self.dev1 = "0"
        self.dev2 = "1"
        self.dev3 = "2"

    def init_device(self):
        LOG.debug("initialize hardware")
        self._device = test_device(self.dev1, self.dev2, self.dev3)



