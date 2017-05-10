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

from ..mpmlog import get_logger

class DboardManagerBase(object):
    """
    Base class for daughterboard controls
    """
    #########################################################################
    # Overridables
    #
    # These values are meant to be overridden by the according subclasses
    #########################################################################
    # Very important: A list of PIDs that apply to the current device. Must be
    # list, even if there's only one entry.
    pids = []

    def __init__(self, slot_idx, **kwargs):
        self.log = get_logger('dboardManager')
        self.slot_idx = slot_idx

    def get_serial(self):
        return self._eeprom.get("serial", "")

    def update_ref_clock_freq(self, freq):
        """
        Call this function if the frequency of the reference clock changes.
        """
        self.log.warning("update_ref_clock_freq() called but not implemented")
