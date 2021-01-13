#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
usrp simulation module

This module is used to emulate simulated devices. You can build mpm in this
configuration by using the cmake flag -DMPM_DEVICE=sim
"""

from pyroute2 import IPRoute
from usrp_mpm.xports import XportMgrUDP
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.rpc_server import no_claim
from usrp_mpm.periph_manager import PeriphManagerBase
from usrp_mpm.simulator.sim_dboard import registry as dboards
from usrp_mpm.simulator.chdr_endpoint import ChdrEndpoint
from usrp_mpm.simulator.config import Config

CLOCK_SOURCE_INTERNAL = "internal"

E320_DBOARD_SLOT_IDX = 0

class SimXportMgrUDP(XportMgrUDP):
    """This is an adaptor class for the normal XportMgrUDP
    In radios, the interface names are hardcoded. Since we are on a
    desktop computer, we generate the names at runtime.
    """
    def __init__(self, log, args, eth_dispatcher_cls):
        with IPRoute() as ipr:
            self.iface_config = {
                link.get_attr('IFLA_IFNAME'): {
                    'label': link.get_attr('IFLA_IFNAME'),
                    'type': 'forward'
                } for link in ipr.get_links()
            }
        super().__init__(log, args, eth_dispatcher_cls)

class SimEthDispatcher:
    """This is the hardware specific part of the normal XportMgrUDP
    that we have to simulate. We get the ipv4 addr with IPRoute
    instead of registers
    """
    DEFAULT_VITA_PORT = (49153, 49154)
    LOG = None

    def __init__(self, if_name):
        self.log = get_logger(if_name)
        self.if_name = if_name

    def set_ipv4_addr(self, addr):
        """This doesn't actually change the ipv4 address, it just
        checks to make sure the requested address is already our
        address, and complains otherwise.
        """
        with IPRoute() as ipr:
            valid_iface_idx = ipr.link_lookup(ifname=self.if_name)[0]
            link_info = ipr.get_links(valid_iface_idx)[0]
            real_addr = link_info.get_attr('IFLA_ADDRESS')
            if addr != real_addr:
                self.log.warning("Cannot change ip address on simulator! Requested: {}, Actual: {}"
                                 .format(addr, real_addr))

class sim(PeriphManagerBase):
    """This is a periph manager that is designed to run on a regular
    computer rather than the arm core on an SDR
    """
    #########################################################################
    # Overridables
    #########################################################################
    mboard_sensor_callback_map = {}

    ###########################################################################
    # Ctor and device initialization tasks
    ###########################################################################
    def __init__(self, args):
        # Logger is initialized in super().__init__ but we need config values
        # before we call that
        config_log = get_logger("PeriphConfig")
        if 'config' in args:
            config_path = args['config']
            config_log.info("Loading config from {}".format(config_path))
            self.config = Config.from_path(config_log, config_path)
        else:
            config_log.warn("No config specified, using default")
            self.config = Config.default()

        self.device_id = 1
        self.description = self.config.hardware.description
        self.mboard_info = {"type": self.config.hardware.uhd_device_type,
                            "product": self.config.hardware.product,
                            "simulated": "True"}
        self.pids = {int(self.config.hardware.pid): self.config.hardware.product}
        # This uses the description, mboard_info, and pids
        super().__init__()

        self.chdr_endpoint = ChdrEndpoint(self.log, self.config)

        # Unlike the real hardware drivers, if there is an exception here,
        # we just crash. No use missing an error when testing.
        self._init_peripherals(args)
        self.init_dboards(args)
        if not args.get('skip_boot_init', False):
            self.init(args)

    def _simulator_sample_rate(self, freq):
        self.log.debug("Setting Simulator Sample Rate to {}".format(freq))
        self.chdr_endpoint.set_sample_rate(freq)

    def generate_device_info(self, eeprom_md, mboard_info, dboard_infos):
        """
        Hard-code our product map
        """
        # Add the default PeriphManagerBase information first
        device_info = super().generate_device_info(
            eeprom_md, mboard_info, dboard_infos)
        # Then add device-specific information
        mb_pid = eeprom_md.get('pid')
        device_info['product'] = self.pids.get(mb_pid, 'unknown')
        return device_info

    def _read_mboard_eeprom(self):
        """
        Read out a simulated mboard eeprom and saves it to the appropriate member variable
        """
        self._eeprom_head = self._generate_eeprom_head()

        self.log.trace("Found EEPROM metadata: '{}'"
                       .format(str(self._eeprom_head)))
        return (self._eeprom_head, None)

    def _generate_eeprom_head(self):
        return {'pid': self.config.hardware.pid,
                'rev': 0,
                'serial': self.config.hardware.serial_num}

    def _init_peripherals(self, args):
        """
        Turn on all peripherals. This may throw an error on failure, so make
        sure to catch it.

        Peripherals are initialized in the order of least likely to fail, to most
        likely.
        """
        # Sanity checks
        assert self.mboard_info.get('product') in self.pids.values(), \
            "Device product could not be determined!"
        # Init peripherals

        # Init CHDR transports
        self._xport_mgrs = {
            'udp': SimXportMgrUDP(self.log, args, SimEthDispatcher)
        }

        # Init complete.
        self.log.debug("Device info: {}".format(self.device_info))

    def _init_dboards(self, dboard_infos, override_dboard_pids, default_args):
        # TODO: Support more than one Daughter Board
        # Needs changes here and in config.py
        dboard_name = self.config.hardware.dboard_class
        dboard_class = dboards[dboard_name]
        self.dboards.append(dboard_class(E320_DBOARD_SLOT_IDX, self._simulator_sample_rate))
        self.log.info("Found %d daughterboard(s).", len(self.dboards))

    ###########################################################################
    # Device info
    ###########################################################################
    def get_device_info_dyn(self):
        """
        Append the device info with current IP addresses.
        """
        if not self._device_initialized:
            return {}
        device_info = self._xport_mgrs['udp'].get_xport_info()
        self.log.warn("get_device_info_dyn() - FPGA functionality not implemented yet")
        return device_info

    def set_device_id(self, device_id):
        """
        Sets the device ID for this motherboard.
        The device ID is used to identify the RFNoC components associated with
        this motherboard.
        """
        self.device_id = device_id
        self.chdr_endpoint.set_device_id(device_id)

    def get_device_id(self):
        """
        Gets the device ID for this motherboard.
        The device ID is used to identify the RFNoC components associated with
        this motherboard.
        """
        return self.device_id

    @no_claim
    def get_proto_ver(self):
        """
        Return RFNoC protocol version
        """
        return 0x100

    @no_claim
    def get_chdr_width(self):
        """
        Return RFNoC CHDR width
        """
        return 64

    ###########################################################################
    # Transport API
    ###########################################################################
    def get_chdr_link_types(self):
        """
        This will only ever return a single item (udp).
        """
        return ["udp"]

    def get_chdr_link_options(self, xport_type):
        """
        Returns a list of dictionaries. Every dictionary contains information
        about one way to connect to this device in order to initiate CHDR
        traffic.

        The interpretation of the return value is very highly dependant on the
        transport type (xport_type).
        For UDP, the every entry of the list has the following keys:
        - ipv4 (IP Address)
        - port (UDP port)
        - link_rate (bps of the link, e.g. 10e9 for 10GigE)
        """
        if xport_type not in self._xport_mgrs:
            self.log.warning("Can't get link options for unknown link type: '{}'."
                             .format(xport_type))
            return []
        return self._xport_mgrs[xport_type].get_chdr_link_options()

    #######################################################################
    # Timekeeper API
    #######################################################################
    def get_num_timekeepers(self):
        """
        Return the number of timekeepers
        """
        return 1

    def set_timekeeper_time(self, tk_idx, ticks, next_pps):
        """
        Set the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        ticks: Time in ticks
        next_pps: If True, set time at next PPS. Otherwise, set time now.
        """
        self.log.debug("Setting timekeeper time (tx_idx:{}, ticks: {}, next_pps: {})"
                       .format(tk_idx, ticks, next_pps))

    def get_timekeeper_time(self, tk_idx, last_pps):
        """
        Get the time in ticks

        Arguments:
        tk_idx: Index of timekeeper
        next_pps: If True, get time at last PPS. Otherwise, get time now.
        """
        return 0

    def set_tick_period(self, tk_idx, period_ns):
        """
        Set the time per tick in nanoseconds (tick period)

        Arguments:
        tk_idx: Index of timekeeper
        period_ns: Period in nanoseconds
        """
        self.log.debug("Setting tick period (tk_idx: {}, period_ns: {})"
                       .format(tk_idx, period_ns))

    def get_clocks(self):
        """
        Gets the RFNoC-related clocks present in the FPGA design
        """
        return [
            {
                'name': 'radio_clk',
                'freq': str(122.88e6),
                'mutable': 'true'
            },
            {
                'name': 'bus_clk',
                'freq': str(200e6),
            },
            {
                'name': 'ctrl_clk',
                'freq': str(40e6),
            }
        ]

    def get_time_sources(self):
        " Returns list of valid time sources "
        return (CLOCK_SOURCE_INTERNAL,)

    def get_clock_sources(self):
        " Lists all available clock sources. "
        return (CLOCK_SOURCE_INTERNAL,)

    def get_clock_source(self):
        " Returns the current Clock Source "
        return CLOCK_SOURCE_INTERNAL

    def set_clock_source(self, source):
        " No-op which sets the clock source on a real radio "
        self.log.debug("Setting clock source to {}".format(source))

    def set_channel_mode(self, channel_mode):
        " No-op which sets the channel mode on a real radio "
        self.log.debug("Using channel mode {}".format(channel_mode))
