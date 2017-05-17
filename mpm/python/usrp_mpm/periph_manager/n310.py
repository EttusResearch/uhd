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
from ..ethtable import EthDispatcherTable
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
        'CLK-MAINREF-SEL1',
        'CLK-MAINREF-SEL0',
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
    #########################################################################
    # Overridables
    #
    # See PeriphManagerBase for documentation on these fields
    #########################################################################
    pids = [0x4242,]
    mboard_eeprom_addr = "e0005000.i2c"
    mboard_eeprom_max_len = 256
    mboard_info = {"type": "n3xx"}
    dboard_eeprom_addr = "e0004000.i2c"
    dboard_eeprom_max_len = 64
    dboard_spimaster_addrs = ["e0006000.spi",]
    chdr_interfaces = ['eth1', 'eth2']
    # N310-specific settings
    eth_tables = {'eth1': 'misc-enet-regs0', 'eth2': 'misc-enet-regs1'}

    def __init__(self, args):
        # First initialize parent class - will populate self._eeprom_head and self._eeprom_rawdata
        super(n310, self).__init__(args)

        self.log.trace("Initializing TCA6424 port expander controls...")
        self._gpios = TCA6424()

        # Initialize reference clock
        self._gpios.set("PWREN-CLK-MAINREF")
        self._ext_clock_freq = float(
            args.default_args.get('ext_clock_freq', N3XX_DEFAULT_EXT_CLOCK_FREQ)
        )
        self._clock_source = None # Gets set in set_clock_source()
        self.set_clock_source(
            args.default_args.get('clock_source', N3XX_DEFAULT_CLOCK_SOURCE)
        )

        with open("/sys/class/rfnoc_crossbar/crossbar0/local_addr", "w") as xbar:
            xbar.write("0x2")
        # if header.get("dataversion", 0) == 1:
        self.log.info("mboard info: {}".format(self.mboard_info))

    def init(self, args):
        """
        Calls init() on the parent class, and then programs the Ethernet
        dispatchers accordingly.
        """
        super(n310, self).init(args)
        self._eth_dispatchers = {
            x: EthDispatcherTable(self.eth_tables.get(x))
            for x in self._chdr_interfaces.keys()
        }
        for ifname, table in iteritems(self._eth_dispatchers):
            table.set_ipv4_addr(self._chdr_interfaces[ifname]['ip_addr'])

    def _allocate_sid(self, sender_addr, port, sid, xbar_src_addr, xbar_src_port):
        """
        Get the MAC address of the sender and store it in the FPGA ARP table
        """
        mac_addr = get_mac_addr(sender_addr)
        new_ep = self._available_endpoints.pop(0)
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
            eth_dispatcher = self._eth_dispatchers['eth1'] # TODO
            eth_dispatcher.set_route(sid.reversed(), sender_addr, port)
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
        for slot, dboard in enumerate(self.dboards):
            if hasattr(dboard, 'update_ref_clock_freq'):
                self.log.trace(
                    "Updating reference clock on dboard `{}' to {} MHz...".format(slot, ref_clk_freq/1e6)
                )
                dboard.update_ref_clock_freq(ref_clk_freq)

