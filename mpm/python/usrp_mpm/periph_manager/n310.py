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
N310 implementation module
"""
from base import periph_manager
import struct


class n310(periph_manager):
    hw_pids = "1"
    mboard_eeprom_addr = "e0007000.spi:ec@0:i2c-tunnel"
    dboard_eeprom_addrs = {"A": "something", "B": "else"}
    dboard_spimaster_addrs = {"A": "something", "B": "else"}

    def __init__(self, eeprom_device, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        super(n310, self).__init__(*args, **kwargs)
        data = self.read_eeprom_v1(self._eeprom_rawdata)
        print(data)
        # if header.get("dataversion", 0) == 1:

    def read_eeprom_v1(self, data):
        # data_version contains
        # 6 bytes mac_addr0
        # 2 bytes pad
        # 6 bytes mac_addr1
        # 2 bytes pad
        # 6 bytes mac_addr2
        # 2 bytes pad
        # 8 bytes serial
        return struct.unpack_from("6s 2x 6s 2x 6s 2x 8s", data)
