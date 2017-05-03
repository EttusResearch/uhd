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
import netaddr
from six import iteritems
from .base import PeriphManagerBase
from .net import get_iface_addrs
from .net import byte_to_mac
from .net import get_mac_addr
from ..mpmtypes import SID
from ..uio import UIO
from ..sysfs_gpio import SysFSGPIO
from .. import libpyusrp_periphs as lib


N3XX_DEFAULT_EXT_CLOCK_FREQ = 10e6
N3XX_DEFAULT_CLOCK_SOURCE = 'external'

class TCA6424(object):
    """
    Abstraction layer for the port/gpio expander
    """
    pins = (
        'PWREN-CLK-MGT156MHz',
        'PWREN-CLK-WB-CDCM',
        'WB-CDCM-RESETn',
        'WB-CDCM-PR0',
        'WB-CDCM-PR1',
        'WB-CDCM-OD0',
        'WB-CDCM-OD1',
        'WB-CDCM-OD2',
        'PWREN-CLK-MAINREF',
        'CLK-MAINREF-SEL0',
        'CLK-MAINREF-SEL1',
        '12',
        '13',
        'FPGA-GPIO-EN',
        'PWREN-CLK-WB-20MHz',
        'PWREN-CLK-WB-25MHz',
        'GPS-PHASELOCK',
        'GPS-nINITSURV',
        'GPS-nRESET',
        'GPS-WARMUP',
        'GPS-SURVEY',
        'GPS-LOCKOK',
        'GPS-ALARM',
        'PWREN-GPS',
    )

    def __init__(self):
        self._gpios = SysFSGPIO('tca6424', 0x700, 0x700)

    def set(self, name):
        """
        Assert a pin by name
        """
        assert name in self.pins
        self._gpios.set(self.pins.index(name))

    def reset(self, name):
        """
        Deassert a pin by name
        """
        assert name in self.pins
        self._gpios.reset(self.pins.index(name))

    def get(self, name):
        """
        Read back a pin by name
        """
        assert name in self.pins
        self._gpios.get(self.pins.index(name))


class n310(PeriphManagerBase):
    """
    Holds N310 specific attributes and methods
    """
    hw_pids = "1"
    mboard_type = "n310"
    mboard_eeprom_addr = "e0005000.i2c"
    # dboard_eeprom_addrs = {"A": "something", "B": "else"}
    # dboard_eeprom_addrs = {"A": "e0004000.i2c",}
    # dboard_spimaster_addrs = {"A": "something", "B": "else"}
    dboard_spimaster_addrs = {"A": "e0006000.spi",}
    interfaces = {}

    def __init__(self, *args, **kwargs):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        super(n310, self).__init__(*args, **kwargs)

        self.log.trace("Initializing TCA6424 port expander controls...")
        self._gpios = TCA6424()

        # Initialize reference clock
        self._gpios.set("PWREN-CLK-MAINREF")
        self._ext_clock_freq = N3XX_DEFAULT_EXT_CLOCK_FREQ
        self._clock_source = None # Gets set in set_clock_source()
        self.set_clock_source(N3XX_DEFAULT_CLOCK_SOURCE)


        # data = self._read_eeprom_v1(self._eeprom_rawdata)
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
        # self.mboard_info["serial"] = data[0]  # some format
        self.mboard_info["serial"] = '123'  # some format
        with open("/sys/class/rfnoc_crossbar/crossbar0/local_addr", "w") as xbar:
            xbar.write("0x2")
        # if header.get("dataversion", 0) == 1:

        # Initialize our daughterboards:
        self.log.debug("Initializing dboards...")
        for k, dboard in iteritems(self.dboards):
            dboard.init_device()

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
            self.log.debug("Getting UIO device for Ethernet configuration...")
            uio_obj = UIO(label="misc-enet-regs0", read_only=False)
            self.log.info("got my uio")
            self.log.info("ip_addr: %s", sender_addr)
            # self.log.info("mac_addr: %s", mac_addr)
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

    def get_clock_sources(self):
        " Lists all available clock sources. "
        self.log.trace("Listing available clock sources...")
        return ('external', 'internal', 'gpsdo')

    def get_clock_source(self):
        " Returns the currently selected clock source "
        return self._clock_source

    def set_ext_clock_freq(self, freq):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        assert freq in (10e6, 20e6, 25e6)
        self._ext_clock_freq = freq

    def get_clock_freq(self):
        " Returns the currently active reference clock frequency"
        return {
            'internal': 25e6,
            'external': self._ext_clock_freq,
            'gpsdo': 20e6,
        }[self._clock_source]

    def set_clock_source(self, clock_source):
        """
        Enable given external reference clock.

        Throws if clock_source is not a valid value.
        """
        assert clock_source in self.get_clock_sources()
        self.log.trace("Setting clock sel to `{}'".format(clock_source))
        if clock_source == 'internal':
            self._gpios.set("CLK-MAINREF-SEL0")
            self._gpios.set("CLK-MAINREF-SEL1")
        elif clock_source == 'gpsdo':
            self._gpios.set("CLK-MAINREF-SEL0")
            self._gpios.reset("CLK-MAINREF-SEL1")
        else: # external
            self._gpios.reset("CLK-MAINREF-SEL0")
            self._gpios.reset("CLK-MAINREF-SEL1")
        self._clock_source = clock_source
        ref_clk_freq = self.get_clock_freq()
        for slot, dboard in iteritems(self.dboards):
            if hasattr(dboard, 'update_ref_clock_freq'):
                self.log.trace(
                    "Updating reference clock on dboard `{}' to {} MHz...".format(slot, ref_clk_freq/1e6)
                )

