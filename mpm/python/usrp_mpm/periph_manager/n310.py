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
import os
import copy
import shutil
from six import iteritems, itervalues
from builtins import object
from .base import PeriphManagerBase
from ..net import get_iface_addrs
from ..net import byte_to_mac
from ..net import get_mac_addr
from ..mpmtypes import SID
from usrp_mpm.uio import UIO
from usrp_mpm.rpc_server import no_claim, no_rpc
from usrp_mpm import net
from ..sysfs_gpio import SysFSGPIO
from ..ethtable import EthDispatcherTable
from ..liberiotable import LiberioDispatcherTable
from .. import libpyusrp_periphs as lib

N3XX_DEFAULT_EXT_CLOCK_FREQ = 10e6
N3XX_DEFAULT_CLOCK_SOURCE = 'external'
N3XX_DEFAULT_TIME_SOURCE = 'internal'
N3XX_DEFAULT_ENABLE_GPS = True
N3XX_DEFAULT_ENABLE_FPGPIO = True

###############################################################################
# Additional peripheral controllers specific to Magnesium
###############################################################################
class TCA6424(object):
    """
    Abstraction layer for the port/gpio expander
    pins_list is  an array of different version of TCA6424 pins map.
    First element of this array corresponding to revC, second is revD etc...
    """
    pins_list = [
        (
            'PWREN-CLK-MGT156MHz',
            'NETCLK-CE',         #revC name: 'PWREN-CLK-WB-CDCM',
            'NETCLK-RESETn',     #revC name: 'WB-CDCM-RESETn',
            'NETCLK-PR0',        #revC name: 'WB-CDCM-PR0',
            'NETCLK-PR1',        #revC name: 'WB-CDCM-PR1',
            'NETCLK-OD0',        #revC name: 'WB-CDCM-OD0',
            'NETCLK-OD1',        #revC name: 'WB-CDCM-OD1',
            'NETCLK-OD2',        #revC name: 'WB-CDCM-OD2',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz', #revC name: 'CLK-MAINREF-SEL1',
            'CLK-MAINSEL-EX_B',  #revC name: 'CLK-MAINREF-SEL0',
            '12',
            'CLK-MAINSEL-GPS',   #revC name: '13',
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
        ),
        (
            'NETCLK-PR1',
            'NETCLK-PR0',
            'NETCLK-CE',
            'NETCLK-RESETn',
            'NETCLK-OD2',
            'NETCLK-OD1',
            'NETCLK-OD0',
            'PWREN-CLK-MGT156MHz',
            'PWREN-CLK-MAINREF',
            'CLK-MAINSEL-25MHz',
            'CLK-MAINSEL-EX_B',
            '12',
            'CLK-MAINSEL-GPS',
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
        )]

    def __init__(self, rev):
        # Default state: Turn on GPS power, take GPS out of reset or
        # init-survey, turn on 156.25 MHz clock
        # min Support from revC or rev = 2
        if rev == 2:
            self.pins = self.pins_list[0]
        else:
            self.pins = self.pins_list[1]

        default_val = 0x860101 if rev == 2 else 0x860780
        self._gpios = SysFSGPIO('tca6424', 0xFFF7FF, 0x86F7FF, default_val)

    def set(self, name, value=None):
        """
        Assert a pin by name
        """
        assert name in self.pins
        self._gpios.set(self.pins.index(name), value=value)

    def reset(self, name):
        """
        Deassert a pin by name
        """
        self.set(name, value=0)

    def get(self, name):
        """
        Read back a pin by name
        """
        assert name in self.pins
        return self._gpios.get(self.pins.index(name))


class FP_GPIO(object):
    """
    Abstraction layer for the front panel GPIO
    """
    EMIO_BASE = 54
    FP_GPIO_OFFSET = 32 # Bit offset within the ps_gpio_* pins

    def __init__(self, ddr):
        self._gpiosize = 12
        self._offset = self.FP_GPIO_OFFSET + self.EMIO_BASE
        self.usemask = 0xFFF
        self.ddr = ddr
        self._gpios = SysFSGPIO(
            'zynq_gpio',
            self.usemask<<self._offset,
            self.ddr<<self._offset
        )

    def set_all(self, value):
        """
        Assert all pin to the value.
        This method will convert value into binary and take the first 6 bits
        assign to 6pins.
        """
        bin_value = '{0:06b}'.format(value)
        wr_value = bin_value[-(self._gpiosize):]
        for i in range(self._gpiosize):
            if  (1<<i)&self.ddr:
                self._gpios.set(self._offset+i, wr_value[i%6])

    def set(self, index, value=None):
        """
        Assert a pin by index
        """
        assert index in range(self._gpiosize)
        self._gpios.set(self._offset+index, value)

    def reset_all(self):
        """
         Deassert all pins
        """
        for i in range(self._gpiosize):
            self._gpios.reset(self._offset+i)

    def reset(self, index):
        """
         Deassert a pin by index
        """
        assert index in range(self._gpiosize)
        self._gpios.reset(self._offset+index)

    def get_all(self):
        """
        Read back all pins
        """
        result = 0
        for i in range(self._gpiosize):
            if not (1<<i)&self.ddr:
                value = self._gpios.get(self._offset+i)
                result = (result<<1) | value
        return result

    def get(self, index):
        """
        Read back a pin by index
        """
        assert index in range(self._gpiosize)
        return self._gpios.get(self._offset+index)

###############################################################################
# Transport managers
###############################################################################
class XportMgrUDP(object):
    """
    Transport manager for UDP connections
    """
    # Map Eth devices to UIO labels
    eth_tables = {'eth1': 'misc-enet-regs0', 'eth2': 'misc-enet-regs1'}

    def __init__(self, possible_chdr_ifaces, log):
        self.log = log
        self._possible_chdr_ifaces = possible_chdr_ifaces
        self.log.info("Identifying available network interfaces...")
        self._chdr_ifaces = \
            self._init_interfaces(self._possible_chdr_ifaces)
        self._eth_dispatchers = {
            x: EthDispatcherTable(self.eth_tables.get(x))
            for x in list(self._chdr_ifaces.keys())
        }
        for ifname, table in iteritems(self._eth_dispatchers):
            table.set_ipv4_addr(self._chdr_ifaces[ifname]['ip_addr'])

    def _init_interfaces(self, possible_ifaces):
        """
        Initialize the list of network interfaces
        """
        self.log.trace("Testing available interfaces out of `{}'".format(
            possible_ifaces
        ))
        valid_ifaces = net.get_valid_interfaces(possible_ifaces)
        if len(valid_ifaces):
            self.log.debug("Found CHDR interfaces: `{}'".format(valid_ifaces))
        else:
            self.log.warning("No CHDR interfaces found!")
        return {
            x: net.get_iface_info(x)
            for x in valid_ifaces
        }

    def init(self, args):
        """
        Call this when the user calls 'init' on the periph manager
        """
        # TODO re-run _init_interfaces, IP addresses could have changed since
        # bootup
        for _, table in iteritems(self._eth_dispatchers):
            if 'forward_eth' in args or 'forward_bcast' in args:
                table.set_forward_policy(
                    args.get('forward_eth', False),
                    args.get('forward_bcast', False)
                )
        if 'preload_ethtables' in args:
            self._preload_ethtables(
                self._eth_dispatchers,
                args['preload_ethtables']
            )

    def deinit(self):
        " Clean up after a session terminates "
        pass

    def _preload_ethtables(self, eth_dispatchers, table_file):
        """
        Populates the ethernet tables from a JSON file
        """
        import json
        try:
            eth_table_data = json.load(open(table_file))
        except ValueError as ex:
            self.log.warning(
                "Bad values in preloading table file: %s",
                str(ex)
            )
            return
        self.log.info(
            "Preloading Ethernet dispatch tables from JSON file `%s'.",
            table_file
        )
        for eth_iface, data in iteritems(eth_table_data):
            if eth_iface not in eth_dispatchers:
                self.log.warning(
                    "Request to preload eth dispatcher table for "
                    "iface `{}', but no such interface is "
                    "registered. Known interfaces: {}".format(
                        str(eth_iface),
                        ",".join(eth_dispatchers.keys())
                    )
                )
                continue
            eth_dispatcher = eth_dispatchers[eth_iface]
            self.log.debug("Preloading {} dispatch table".format(eth_iface))
            try:
                for dst_ep, udp_data in iteritems(data):
                    sid = SID()
                    sid.set_dst_ep(int(dst_ep))
                    eth_dispatcher.set_route(
                        sid,
                        udp_data['ip_addr'],
                        udp_data['port'],
                        udp_data.get('mac_addr', None)
                    )
            except ValueError as ex:
                self.log.warning(
                    "Bad values in preloading table file: %s",
                    str(ex)
                )

    def request_xport(
            self,
            sid,
            xport_type,
        ):
        """
        Return UDP xport info
        """
        assert xport_type in ('CTRL', 'ASYNC_MSG', 'TX_DATA', 'RX_DATA')
        xport_info = {
            'type': 'UDP',
            # TODO what about eth2, huh?
            'ipv4': str(self._chdr_ifaces['eth1']['ip_addr']),
            'port': '49153', # FIXME no hardcoding
            'send_sid': str(sid)
        }
        return [xport_info]

    def commit_xport(self, sid, xport_info):
        """
        fuu
        """
        # TODO do error checking on the xport_info
        self.log.trace("Committing UDP transport using xport_info `%s'",
                       str(xport_info))
        sender_addr = xport_info['src_ipv4']
        sender_port = int(xport_info['src_port'])
        self.log.trace("Incoming connection is coming from %s:%d",
                       sender_addr, sender_port)
        mac_addr = get_mac_addr(sender_addr)
        if mac_addr is None:
            raise RuntimeError(
                "Could not find MAC address for IP address {}".format(
                    sender_addr))
        self.log.trace("Incoming connection is coming from %s",
                       mac_addr)
        # Remove hardcodings in the following lines FIXME
        my_xbar = lib.xbar.xbar.make("/dev/crossbar0") # TODO
        my_xbar.set_route(sid.src_addr, 0) # TODO
        eth_dispatcher = self._eth_dispatchers['eth1'] # TODO
        eth_dispatcher.set_route(sid.reversed(), sender_addr, sender_port)
        self.log.trace("UDP transport successfully committed!")
        return True

class XportMgrLiberio(object):
    """
    Transport manager for UDP connections
    """
    # udev label for the UIO device that controls the DMA engine
    liberio_label = 'liberio'

    def __init__(self, log):
        self.log = log
        self._dma_dispatcher = LiberioDispatcherTable(self.liberio_label)
        self._data_chan_ctr = 0
        self._max_chan = 10 # TODO get this number from somewhere

    def init(self, args):
        """
        Call this when the user calls 'init' on the periph manager
        """
        pass

    def deinit(self):
        " Clean up after a session terminates "
        self._data_chan_ctr = 0

    def request_xport(
            self,
            sid,
            xport_type,
        ):
        """
        Return liberio xport info
        """
        assert xport_type in ('CTRL', 'ASYNC_MSG', 'TX_DATA', 'RX_DATA')
        if xport_type == 'CTRL':
            chan = 0
        elif xport_type == 'ASYNC_MSG':
            chan = 1
        else:
            chan = 2 + self._data_chan_ctr
            self._data_chan_ctr += 1
        xport_info = {
            'type': 'liberio',
            'send_sid': str(sid),
            'muxed': str(xport_type in ('CTRL', 'ASYNC_MSG')),
            'dma_chan': str(chan),
            'tx_dev': "/dev/tx-dma{}".format(chan),
            'rx_dev': "/dev/rx-dma{}".format(chan),
        }
        self.log.trace("Liberio: Chan: {} TX Device: {} RX Device: {}".format(
            chan, xport_info['tx_dev'], xport_info['rx_dev']))
        self.log.trace("Liberio channel is muxed: %s",
                       "Yes" if xport_info['muxed'] else "No")
        return [xport_info]

    def commit_xport(self, sid, xport_info):
        " Commit liberio transport "
        chan = int(xport_info['dma_chan'])
        my_xbar = lib.xbar.xbar.make("/dev/crossbar0") # TODO
        my_xbar.set_route(sid.src_addr, 2) # TODO
        self._dma_dispatcher.set_route(sid.reversed(), chan)
        self.log.trace("Liberio transport successfully committed!")
        return True


###############################################################################
# Main Class
###############################################################################
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
    mboard_max_rev = 3 # 3 == RevD
    mboard_sensor_callback_map = {
        'ref_locked': 'get_ref_lock_sensor',
        'gps_locked': 'get_gps_lock_sensor',
    }
    dboard_eeprom_addr = "e0004000.i2c"
    dboard_eeprom_max_len = 64

    # We're on a Zynq target, so the following two come from the Zynq standard
    # device tree overlay (tree/arch/arm/boot/dts/zynq-7000.dtsi)
    dboard_spimaster_addrs = ["e0006000.spi", "e0007000.spi"]
    chdr_interfaces = ['eth1', 'eth2']
    # N310-specific settings
    # Path to N310 FPGA bin file
    # This file will always contain the current image, regardless of SFP type,
    # dboard, etc. The host is responsible for providing a compatible image
    # for the N310's current setup.
    binfile_path = '/lib/firmware/n310.bin'
    # Override the list of updateable components
    updateable_components = {
        'fpga': {
            'callback': "update_fpga",
        },
    }

    def __init__(self, args):
        super(n310, self).__init__(args)
        self.sid_endpoints = {}
        # Init peripherals
        self.log.trace("Initializing TCA6424 port expander controls...")
        self._gpios = TCA6424(int(self.mboard_info['rev']))
        self.log.trace("Enabling power of MGT156MHZ clk")
        self._gpios.set("PWREN-CLK-MGT156MHz")
        self.enable_1G_ref_clock()
        self.enable_gps(
            enable=bool(
                args.default_args.get('enable_gps', N3XX_DEFAULT_ENABLE_GPS)
            )
        )
        self.enable_fp_gpio(
            enable=bool(
                args.default_args.get(
                    'enable_fp_gpio',
                    N3XX_DEFAULT_ENABLE_FPGPIO
                )
            )
        )
        # Init clocking
        self.enable_ref_clock(enable=True)
        self._ext_clock_freq = None
        self._clock_source = None
        self._time_source = None
        self._init_ref_clock_and_time(args.default_args)
        # Init CHDR transports
        self._xport_mgrs = {
            'udp': XportMgrUDP(self.chdr_interfaces, self.log),
            'liberio': XportMgrLiberio(self.log),
        }
        # Init complete.
        self.log.info("mboard info: {}".format(self.mboard_info))

    def _init_ref_clock_and_time(self, default_args):
        """
        Initialize clock and time sources. After this function returns, the
        reference signals going to the FPGA are valid.
        """
        self._ext_clock_freq = float(
            default_args.get('ext_clock_freq', N3XX_DEFAULT_EXT_CLOCK_FREQ)
        )
        if len(self.dboards) == 0:
            self.log.warning(
                "No dboards found, skipping setting clock and time source " \
                "configuration."
            )
            self._clock_source = N3XX_DEFAULT_CLOCK_SOURCE
            self._time_source = N3XX_DEFAULT_TIME_SOURCE
        else:
            self.set_clock_source(
                default_args.get('clock_source', N3XX_DEFAULT_CLOCK_SOURCE)
            )
            self.set_time_source(
                default_args.get('time_source', N3XX_DEFAULT_TIME_SOURCE)
            )

    def init(self, args):
        """
        Calls init() on the parent class, and then programs the Ethernet
        dispatchers accordingly.
        """
        result = super(n310, self).init(args)
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.init(args)
        return result

    def deinit(self):
        """
        Clean up after a UHD session terminates.
        """
        super(n310, self).deinit()
        for xport_mgr in itervalues(self._xport_mgrs):
            xport_mgr.deinit()

    ###########################################################################
    # Transport API
    ###########################################################################
    def request_xport(
            self,
            dst_address,
            suggested_src_address,
            xport_type
        ):
        """
        See PeriphManagerBase.request_xport() for docs.
        """
        # For now, we always accept the suggestion if available, or fail
        src_address = suggested_src_address
        if src_address not in self._available_endpoints:
            raise RuntimeError("no more sids yo")
        sid = SID(src_address << 16 | dst_address)
        self.log.debug(
            "request_xport(dst=0x%04X, suggested_src_address=0x%04X, xport_type=%s): " \
            "operating on SID: %s",
            dst_address, suggested_src_address, str(xport_type), str(sid))
        # FIXME token!
        assert self.mboard_info['rpc_connection'] in ('remote', 'local')
        if self.mboard_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].request_xport(
                sid,
                xport_type,
            )
        elif self.mboard_info['rpc_connection'] == 'local':
            return self._xport_mgrs['liberio'].request_xport(
                sid,
                xport_type,
            )

    def commit_xport(self, xport_info):
        """
        See PeriphManagerBase.commit_xport() for docs.

        Reminder: All connections are incoming, i.e. "send" or "TX" means
        remote device to local device, and "receive" or "RX" means this local
        device to remote device. "Remote device" can be, for example, a UHD
        session.
        """
        ## Go, go, go
        assert self.mboard_info['rpc_connection'] in ('remote', 'local')
        sid = SID(xport_info['send_sid'])
        self._available_endpoints.remove(sid.src_ep)
        self.log.debug("Committing transport for SID %s, xport info: %s",
                       str(sid), str(xport_info))
        if self.mboard_info['rpc_connection'] == 'remote':
            return self._xport_mgrs['udp'].commit_xport(sid, xport_info)
        elif self.mboard_info['rpc_connection'] == 'local':
            return self._xport_mgrs['liberio'].commit_xport(sid, xport_info)

    def get_clock_sources(self):
        " Lists all available clock sources. "
        self.log.trace("Listing available clock sources...")
        return ('external', 'internal', 'gpsdo')

    def get_clock_source(self):
        " Returns the currently selected clock source "
        return self._clock_source

    def set_clock_source(self, *args):
        """
        Enable given external reference clock.

        Throws if clock_source is not a valid value.
        """
        clock_source = args[0]
        assert clock_source in self.get_clock_sources()
        self.log.trace("Setting clock source to `{}'".format(clock_source))
        if clock_source == 'internal':
            self._gpios.set("CLK-MAINSEL-EX_B")
            self._gpios.set("CLK-MAINSEL-25MHz")
            self._gpios.reset("CLK-MAINSEL-GPS")
        elif clock_source == 'gpsdo':
            self._gpios.set("CLK-MAINSEL-EX_B")
            self._gpios.reset("CLK-MAINSEL-25MHz")
            self._gpios.set("CLK-MAINSEL-GPS")
        else: # external
            self._gpios.reset("CLK-MAINSEL-EX_B")
            self._gpios.reset("CLK-MAINSEL-GPS")
            # SKY13350 needs to be in known state
            self._gpios.set("CLK-MAINSEL-25MHz")
        self._clock_source = clock_source
        ref_clk_freq = self.get_ref_clock_freq()
        self.log.info("Reference clock frequency is: {} MHz".format(
            ref_clk_freq/1e6
        ))
        for slot, dboard in enumerate(self.dboards):
            if hasattr(dboard, 'update_ref_clock_freq'):
                self.log.trace(
                    "Updating reference clock on dboard `{}' to {} MHz...".format(
                        slot, ref_clk_freq/1e6
                    )
                )
                dboard.update_ref_clock_freq(ref_clk_freq)

    def set_ref_clock_freq(self, freq):
        """
        Tell our USRP what the frequency of the external reference clock is.

        Will throw if it's not a valid value.
        """
        assert freq in (10e6, 20e6, 25e6)
        self.log.debug("We've been told the external reference clock " \
                       "frequency is {} MHz.".format(freq/1e6))
        self._ext_clock_freq = freq

    def get_ref_clock_freq(self):
        " Returns the currently active reference clock frequency"
        return {
            'internal': 25e6,
            'external': self._ext_clock_freq,
            'gpsdo': 20e6,
        }[self._clock_source]

    def get_time_sources(self):
        " Returns list of valid time sources "
        return ['internal', 'external']

    def get_time_source(self):
        " Return the currently selected time source "
        return self._time_source

    def set_time_source(self, time_source):
        " Set a time source "
        assert time_source in self.get_time_sources()
        self.log.trace("Setting time source to `{}'".format(time_source))
        # FIXME use uio
        if time_source == 'internal':
            os.system('devmem2 0x4001000C w 0x1')
        elif time_source == 'external':
            os.system('devmem2 0x4001000C w 0x0')
        else:
            assert False

    def enable_gps(self, enable):
        """
        Turn power to the GPS off or on.
        """
        self.log.trace("{} power to GPS".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("PWREN-GPS", int(bool(enable)))

    def enable_fp_gpio(self, enable):
        """
        Turn power to the front panel GPIO off or on.
        """
        self.log.trace("{} power to front-panel GPIO".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("FPGA-GPIO-EN", int(bool(enable)))

    def enable_ref_clock(self, enable):
        """
        Enables the ref clock voltage (+3.3-MAINREF). Without setting this to
        True, *no* ref clock works.
        """
        self.log.trace("{} power to reference clocks".format(
            "Enabling" if enable else "Disabling"
        ))
        self._gpios.set("PWREN-CLK-MAINREF", int(bool(enable)))
    
    def enable_1G_ref_clock(self):
        """
        Enables 125 MHz refclock for 1G interface.
        """
        self.log.trace("Enable 125 MHz Clock for 1G SFP interface.")
        self._gpios.set("NETCLK-CE")
        self._gpios.set("NETCLK-RESETn", 0)
        self._gpios.set("NETCLK-PR0", 1)
        self._gpios.set("NETCLK-PR1", 1)
        self._gpios.set("NETCLK-OD0", 1)
        self._gpios.set("NETCLK-OD1", 1)
        self._gpios.set("NETCLK-OD2", 0)
        self._gpios.set("PWREN-CLK-WB-25MHz", 1)
        self.log.trace("Finished configuring NETCLK CDCM.")
        self._gpios.set("NETCLK-RESETn", 1)

    ###########################################################################
    # Sensors
    ###########################################################################
    def get_ref_lock_sensor(self):
        """
        The N310 has no ref lock sensor, but because the ref lock is
        historically considered a motherboard-level sensor, we will return the
        combined lock status of all daughterboards. If no dboard is connected,
        or none has a ref lock sensor, we simply return True.
        """
        self.log.trace(
            "Querying ref lock status from %d dboards.",
            len(self.dboards)
        )
        lock_status = all([
            not hasattr(db, 'get_ref_lock') or db.get_ref_lock()
            for db in self.dboards
        ])
        return {
            'name': 'ref_locked',
            'type': 'BOOLEAN',
            'unit': 'locked' if lock_status else 'unlocked',
            'value': str(lock_status).lower(),
        }

    def get_gps_lock_sensor(self):
        """
        Get lock status of GPS as a sensor dict
        """
        self.log.trace("Reading status GPS lock pin from port expander")
        gps_locked = bool(self._gpios.get("GPS-LOCKOK"))
        return {
            'name': 'gps_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if gps_locked else 'unlocked',
            'value': str(gps_locked).lower(),
        }
    ###########################################################################
    # EEPROMs
    ###########################################################################
    def get_mb_eeprom(self):
        """
        Return a dictionary with EEPROM contents.

        All key/value pairs are string -> string.

        We don't actually return the EEPROM contents, instead, we return the
        mboard info again. This filters the EEPROM contents to what we think
        the user wants to know/see.
        """
        return self.mboard_info

    def set_mb_eeprom(self, eeprom_vals):
        """
        See PeriphManagerBase.set_mb_eeprom() for docs.
        """
        self.log.warn("Called set_mb_eeprom(), but not implemented!")
        raise NotImplementedError

    def get_db_eeprom(self, dboard_idx):
        """
        See PeriphManagerBase.get_db_eeprom() for docs.
        """
        try:
            dboard = self.dboards[dboard_idx]
        except KeyError:
            error_msg = "Attempted to access invalid dboard index `{}' " \
                        "in get_db_eeprom()!".format(dboard_idx)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        db_eeprom_data = copy.copy(dboard.device_info)
        if hasattr(dboard, 'get_user_eeprom_data') and \
                callable(dboard.get_user_eeprom_data):
            for blob_id, blob in iteritems(dboard.get_user_eeprom_data()):
                if blob_id in db_eeprom_data:
                    self.log.warn("EEPROM user data contains invalid blob ID " \
                                  "%s", blob_id)
                else:
                    db_eeprom_data[blob_id] = blob
        return db_eeprom_data

    def set_db_eeprom(self, dboard_idx, eeprom_data):
        """
        Write new EEPROM contents with eeprom_map.

        Arguments:
        dboard_idx -- Slot index of dboard
        eeprom_data -- Dictionary of EEPROM data to be written. It's up to the
                       specific device implementation on how to handle it.
        """
        try:
            dboard = self.dboards[dboard_idx]
        except KeyError:
            error_msg = "Attempted to access invalid dboard index `{}' " \
                        "in set_db_eeprom()!".format(dboard_idx)
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        if not hasattr(dboard, 'set_user_eeprom_data') or \
                not callable(dboard.set_user_eeprom_data):
            error_msg = "Dboard has no set_user_eeprom_data() method!"
            self.log.error(error_msg)
            raise RuntimeError(error_msg)
        safe_db_eeprom_user_data = {}
        for blob_id, blob in iteritems(eeprom_data):
            if blob_id in dboard.device_info:
                error_msg = "Trying to overwrite read-only EEPROM " \
                            "entry `{}'!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            if not isinstance(blob, str) and not isinstance(blob, bytes):
                error_msg = "Blob data for ID `{}' is not a " \
                            "string!".format(blob_id)
                self.log.error(error_msg)
                raise RuntimeError(error_msg)
            assert isinstance(blob, str)
            safe_db_eeprom_user_data[blob_id] = blob.encode('ascii')
        dboard.set_user_eeprom_data(safe_db_eeprom_user_data)

    @no_rpc
    def update_fpga(self, filepath, metadata):
        """
        Update the FPGA image in the filesystem and reload the overlay
        :param filepath: path to new FPGA image
        :param metadata: Dictionary of strings containing metadata
        """
        self.log.trace("Updating FPGA with image at {}"
                       .format(filepath))
        _, file_extension = os.path.splitext(filepath)
        # Cut off the period from the file extension
        file_extension = file_extension[1:].lower()
        if file_extension == "bit":
            self.log.trace("Converting bit to bin file and writing to {}"
                           .format(self.binfile_path))
            from usrp_mpm.fpga_bit_to_bin import fpga_bit_to_bin
            fpga_bit_to_bin(filepath, self.binfile_path, flip=True)
        elif file_extension == "bin":
            self.log.trace("Copying bin file to {}"
                           .format(self.binfile_path))
            shutil.copy(filepath, self.binfile_path)
        else:
            self.log.error("Invalid FPGA bitfile: {}"
                           .format(filepath))
            raise RuntimeError("Invalid N310 FPGA bitfile")
        # TODO: Implement reload procedure
        return True
