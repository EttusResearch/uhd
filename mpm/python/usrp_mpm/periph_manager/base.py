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

import os
from six import iteritems
from ..mpmlog import get_logger
from ..mpmtypes import EEPROM
from .. import dboard_manager
from .udev import get_eeprom_path
from .udev import get_spidev_nodes

class PeriphManagerBase(object):
    """"
    Base class for all motherboards. Common function and API calls should
    be implemented here. Motherboard specific information can be stored in
    separate motherboard classes derived from this class
    """
    # stores discovered device information in dicts
    claimed = False
    dboards = {}
    mboard_info = {"type": "unknown"}
    mboard_if_addrs = {}
    mboard_overlays = {}
    # this information has to be provided by
    # the specific periph_manager implementation
    mboard_eeprom_addr = ""
    dboard_eeprom_addrs = {}
    dboard_spimaster_addrs = {}
    updateable_components = []
    sid_endpoints = {}
    available_endpoints = range(256)

    def __init__(self):
        self.log = get_logger('PeriphManager')
        # I know my EEPROM address, lets use it
        self.overlays = ""
        # (self._eeprom_head, self._eeprom_rawdata) = EEPROM().read_eeprom(
            # get_eeprom_path(self.mboard_eeprom_addr))
        # print self._eeprom_head
        self._dboard_eeproms = {}
        self.log.debug("Initializing dboards")
        # for dboard_slot, eeprom_addr in self.dboard_eeprom_addrs.iteritems():
            # self.log.debug("Adding dboard for slot {0}".format(dboard_slot))
            # spi_devices = []
            # # I know EEPROM adresses for my dboard slots
            # eeprom_data = EEPROM().read_eeprom(get_eeprom_path(eeprom_addr))
            # # I know spidev masters on the dboard slots
            # hw_pid = eeprom_data[0].get("hw_pid", 0)
            # if hw_pid in dboard_manager.HW_PIDS:
                # spi_devices = get_spidev_nodes(self.dboard_spimaster_addrs.get(dboard_slot))
            # dboard = dboard_manager.HW_PIDS.get(hw_pid, dboard_manager.unknown)
            # self.dboards.update({dboard_slot: dboard(spi_devices, eeprom_data)})
        dboard_slot = 0
        self.log.debug("Adding dboard for slot {0}".format(dboard_slot))
        spi_devices = []
        # I know EEPROM adresses for my dboard slots
        # eeprom_data = EEPROM().read_eeprom(get_eeprom_path(eeprom_addr))
        eeprom_data = None
        # I know spidev masters on the dboard slots
        hw_pid = 3
        if hw_pid in dboard_manager.HW_PIDS:
            spi_devices = sorted(get_spidev_nodes("e0006000.spi"))
            self.log.debug("Found spidev nodes: {0}".format(spi_devices))
        dboard = dboard_manager.HW_PIDS.get(hw_pid, dboard_manager.unknown)
        self.dboards.update({dboard_slot: dboard(0, spi_devices, eeprom_data)})

    def safe_list_updateable_components(self):
        """
        return list of updateable components
        This method does not require a claim_token in the RPC
        """
        return self.updateable_components

    def get_overlays(self):
        """
        get and store the list of available dt overlays
        """
        self.mboard_overlays = []
        for fw_files in os.listdir("/lib/firmware/"):
            if fw_files.endswith(".dtbo"):
                self.mboard_overlays.append(fw_files.strip(".dtbo"))

    def check_overlay(self):
        """
        check which dt overlay is loaded currently
        """
        for overlay_file in os.listdir("/sys/kernel/device-tree/overlays/"):
            self.overlays = overlay_file

    def _get_device_info(self):
        """
        return the mboard_info dict and add a claimed field
        """
        result = {"claimed": str(self.claimed)}
        result.update(self.mboard_info)
        return result

    def get_dboards(self):
        """
        get a dict with slot: hw_pid for each dboard
        """
        result = {}
        for slot, dboard in iteritems(self.dboards):
            result.update({slot:dboard.hw_pid})
        return result

    def load_fpga_image(self, target=None):
        """
        load a new fpga image
        """
        pass

    def init_device(self, *args, **kwargs):
        """
        Do the real init on the mboard and all dboards
        """
        # Load FPGA
        # Init dboards
        pass

    def _allocate_sid(self, sender_addr, sid, xbar_src_addr, xbar_src_port):
        """
        Overload this method in actual device implementation
        """
        return True

    def get_interfaces(self):
        """
        Overload this method in actual device implementation
        """
        return []
