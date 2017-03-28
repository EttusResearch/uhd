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
from __future__ import print_function
import struct
from .base import PeriphManagerBase
from .net import get_iface_addrs
from .net import byte_to_mac
from .net import get_mac_addr
from logging import getLogger

LOG = getLogger(__name__)


class n310(PeriphManagerBase):
    """
    Holds N310 specific attributes and methods
    """
    hw_pids = "1"
    mboard_type = "n310"
    mboard_eeprom_addr = "e0007000.spi:ec@0:i2c-tunnel"
    dboard_eeprom_addrs = {"A": "something", "B": "else"}
    dboard_spimaster_addrs = {"A": "something", "B": "else"}
    interfaces = {}

    def __init__(self, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        super(n310, self).__init__(*args, **kwargs)
        data = self._read_eeprom_v1(self._eeprom_rawdata)
        # mac 0: mgmt port, mac1: sfp0, mac2: sfp1
        self.interfaces["mgmt"] = {
            "mac_addr": byte_to_mac(data[0]),
            "addrs": get_iface_addrs(byte_to_mac(data[0]))
        }
        self.interfaces["sfp0"] = {
            "mac_addr": byte_to_mac(data[1]),
            "addrs": get_iface_addrs(byte_to_mac(data[1]))
        }
        self.interfaces["sfp1"] = {
            "mac_addr": byte_to_mac(data[2]),
            "addrs": get_iface_addrs(byte_to_mac(data[2]))
        }
        self.mboard_info["serial"] = data[3]  # some format

        print(data)
        # if header.get("dataversion", 0) == 1:


    def _read_eeprom_v1(self, data):
        """
        read eeprom with data version 1
        """
        # data_version contains
        # 6 bytes mac_addr0
        # 2 bytes pad
        # 6 bytes mac_addr1
        # 2 bytes pad
        # 6 bytes mac_addr2
        # 2 bytes pad
        # 8 bytes serial
        return struct.unpack_from("6s 2x 6s 2x 6s 2x 8s", data)

    def get_interfaces(self):
        """
        returns available transport interfaces
        """
        return [iface for iface in self.interfaces.keys()
                if iface.startswith("sfp")]

    def get_interface_addrs(self, interface):
        """
        returns discovered ipv4 addresses for a given interface
        """
        return self.interfaces.get(interface, {}).get("addrs", [])

    def _probe_interface(self, sender_addr):
        """
        Get the MAC address of the sender and store it in the FPGA ARP table
        """
        mac_addr = get_mac_addr(sender_addr)
        if mac_addr is not None:
            # Do something with mac_address
            return True
        return False



