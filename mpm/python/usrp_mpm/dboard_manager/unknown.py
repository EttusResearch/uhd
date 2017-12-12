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
EISCAT rx board implementation module
"""
from usrp_mpm.dboard_manager import DboardManagerBase

class unknown(DboardManagerBase):
    hw_pid = 0
    special_eeprom_addrs = {}

    def __init__(self, spi_devices, eeprom_data):
        self._eeprom = eeprom_data[0] # save eeprom header
        # Do own init
        super(unknown, self).__init__()
