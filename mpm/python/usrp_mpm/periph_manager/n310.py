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
from .udev import get_uio_node
from ..types import SID
from ..uio import uio
from .. import libpyusrp_periphs as lib
from logging import getLogger
import netaddr
import socket

LOG = getLogger(__name__)


class n310(PeriphManagerBase):
    """
    Holds N310 specific attributes and methods
    """
    hw_pids = "1"
    mboard_type = "n310"
    mboard_eeprom_addr = "e0005000.i2c"
    # dboard_eeprom_addrs = {"A": "something", "B": "else"}
    # dboard_spimaster_addrs = {"A": "something", "B": "else"}
    interfaces = {}

    def __init__(self, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        super(n310, self).__init__(*args, **kwargs)
        data = self._read_eeprom_v1(self._eeprom_rawdata)
        # mac 0: mgmt port, mac1: sfp0, mac2: sfp1
        # self.interfaces["mgmt"] = {
        #     "mac_addr": byte_to_mac(data[0]),
        #     "addrs": get_iface_addrs(byte_to_mac(data[0]))
        # }
        # self.interfaces["sfp0"] = {
        #     "mac_addr": byte_to_mac(data[1]),
        #     "addrs": get_iface_addrs(byte_to_mac(data[1]))
        # }
        # self.interfaces["sfp1"] = {
        #     "mac_addr": byte_to_mac(data[2]),
        #     "addrs": get_iface_addrs(byte_to_mac(data[2]))
        # }
        self.mboard_info["serial"] = data[0]  # some format
        with open("/sys/class/rfnoc_crossbar/crossbar0/local_addr", "w") as xbar:
            xbar.write("0x2")

        # if header.get("dataversion", 0) == 1:


    def _read_eeprom_v1(self, data):
        """
        read eeprom with data version 1
        """
        # data contains
        # 24 bytes header -> ignore them here
        # 8 bytes serial
        # 6 bytes mac_addr0
        # 2 bytes pad
        # 6 bytes mac_addr1
        # 2 bytes pad
        # 6 bytes mac_addr2
        # 2 bytes pad
        # 4 bytes CRC
        return struct.unpack_from("!28x 8s 6s 2x 6s 2x 6s 2x I", data)

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

    def _allocate_sid(self, sender_addr, port, sid, xbar_src_addr, xbar_src_port):
        """
        Get the MAC address of the sender and store it in the FPGA ARP table
        """
        mac_addr = get_mac_addr(sender_addr)
        new_ep = self.available_endpoints.pop(0)
        if mac_addr is not None:
            if sender_addr not in self.sid_endpoints:
                self.sid_endpoints.update({sender_addr: (new_ep,)})
            else:
                current_allocation = self.sid_endpoints.get(sender_addr)
                new_allocation = current_allocation + (new_ep,)
                self.sid_endpoints.update({sender_addr: new_allocation})
            sid = SID(sid)
            sid.set_src_addr(xbar_src_addr)
            sid.set_src_ep(new_ep)
            my_xbar = lib.xbar.xbar.make("/dev/crossbar0") # TODO
            my_xbar.set_route(xbar_src_addr, 0) # TODO
            # uio_path, uio_size = get_uio_node("misc-enet-regs0")
            uio_path = "/dev/uio0"
            uio_size = 0x2000
            LOG.debug("got uio_path and size")
            uio_obj = uio(uio_path, uio_size, read_only=False)
            LOG.info("got my uio")
            LOG.info("ip_addr: %s", sender_addr)
            # LOG.info("mac_addr: %s", mac_addr)
            ip_addr = int(netaddr.IPAddress(sender_addr))
            mac_addr = int(netaddr.EUI(mac_addr))
            uio_obj.poke32(0x1000 + 4*new_ep, ip_addr)
            print("sid: %x" % (sid.get()))
            print("gonna poke: %x %x" % (0x1000+4*new_ep, ip_addr))
            uio_obj.poke32(0x1800 + 4*new_ep, mac_addr & 0xFFFFFFFF)
            print("gonna poke: %x %x" % (0x1800+4*new_ep, mac_addr))
            port = int(port)
            uio_obj.poke32(0x1400 + 4*new_ep, ((int(port) << 16) | (mac_addr >> 32)))
            print("gonna poke: %x %x" % (0x1400+4*new_ep, ((port << 16) | (mac_addr >> 32))))
        return sid.get()
