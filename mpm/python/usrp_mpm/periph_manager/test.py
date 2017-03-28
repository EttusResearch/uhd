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
test periph_manager implementation module
"""
from base import periph_manager
from . import dboard_manager
import random
import string
import struct


class test(periph_manager):
    hw_pids = "42"
    mboard_eeprom_addr = None
    dboard_eeprom_addrs = {"A": "something", "B": "else"}
    dboard_spimaster_addrs = {"A": "something", "B": "else"}

    def __init__(self, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        # super(n310, self).__init__(*args, **kwargs)
        # if header.get("dataversion", 0) == 1:
        self._eeprom = self.read_eeprom_fake()
        self._serial = "AABBCCDDEEFF"

        # I'm the test periph_manager, I know I have test dboards attached
        self.dboards = {
            "A": dboard_manager.test(self.read_db_eeprom_random()),
            "B": dboard_manager.test(self.read_db_eeprom_random())
        }

    def read_eeprom_fake(self):
        fake_eeprom = {
            "magic": 42,
            "crc": 4242,
            "data_version": 42,
            "hw_pid": 42,
            "hw_rev": 5
        }

        return fake_eeprom

    def read_db_eeprom_random(self):
        fake_eeprom = {
            "serial": ''.join(
                random.choice("ABCDEF" + string.digits)
                for _ in range(16))
        }
        return fake_eeprom
