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
dboard base implementation module
"""
import logging
import struct

LOG = logging.Logger(__name__)


class DboardManagerBase(object):
    """
    Holds shared pointer to wrapped C++ implementation.
    Sanitizes arguments before calling C++ functions.
    Ties various constants to specific daughterboard class
    """
    _eeprom = {}

    def __init__(self, eeprom=None):
        self._eeprom = eeprom or {}

    def get_serial(self):
        return self._eeprom.get("serial", "")
