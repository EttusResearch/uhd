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

from six import iteritems
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
    # A dictionary that maps chips or components to chip selects for SPI.
    # If this is given, a dictionary called self._spi_nodes is created which
    # maps these keys to actual spidev paths. Also throws a warning/error if
    # the SPI configuration is invalid.
    spi_chipselect = {}

    def __init__(self, slot_idx, **kwargs):
        self.log = get_logger('dboardManager')
        self.slot_idx = slot_idx
        self.device_info = {}
        self._init_spi_nodes(kwargs.get('spi_nodes', []))


    def _init_spi_nodes(self, spi_devices):
        """
        Populates the self._spi_nodes dictionary.
        Note that this won't instantiate any spidev objects, it'll just map
        keys from self.spi_chipselect to spidev nodes, and do a sanity check
        that enough nodes are available.
        """
        if len(spi_devices) < len(self.spi_chipselect):
            self.log.error("Expected {0} spi devices, found {1} spi devices".format(
                len(self.spi_chipselect), len(spi_devices),
            ))
            raise RuntimeError("Not enough SPI devices found.")
        self._spi_nodes = {}
        for k, v in iteritems(self.spi_chipselect):
            self._spi_nodes[k] = spi_devices[v]
        self.log.debug("spidev device node map: {}".format(self._spi_nodes))

    def init(self, args):
        """
        Run the dboard initialization. This typically happens at the beginning
        of a UHD session.

        Must be overridden.

        args -- A dictionary of arbitrary settings that can be used by the
                dboard code. Similar to device args for UHD.
        """
        raise NotImplementedError("DboardManagerBase::init() not implemented!")

    def deinit(self):
        """
        Power down the dboard. Does not have be implemented. If it does, it
        needs to be safe to call multiple times.
        """
        self.log.info("deinit() called, but not implemented.")

    def get_serial(self):
        """
        Return this daughterboard's serial number as a dictionary.
        """
        return self.device_info.get("serial", "")

    def update_ref_clock_freq(self, freq):
        """
        Call this function if the frequency of the reference clock changes.
        """
        self.log.warning("update_ref_clock_freq() called but not implemented")
