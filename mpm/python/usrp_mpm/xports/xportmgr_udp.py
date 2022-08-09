#
# Copyright 2017 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
UDP Transport manager
"""

import importlib
import subprocess
from usrp_mpm import prefs
from usrp_mpm.sys_utils import net

DEFAULT_BRIDGE_MODE = False

class XportMgrUDP:
    """
    Transport manager for UDP connections
    """
    # The interface configuration describes how the Ethernet interfaces are
    # hooked up to the crossbar and the FPGA. It could look like this:
    # iface_config = {
    #     'sfp0': { # Add key for every Ethernet iface connected to the FPGA
    #         'label': 'misc-enet-regs0', # UIO label for the Eth table
    #     },
    # }
    iface_config = {}
    bridges = {}

    def __init__(self, log, args, eth_dispatcher_cls=None):
        self.eth_dispatcher_cls = eth_dispatcher_cls or \
            importlib.import_module('usrp_mpm.ethdispatch').EthDispatcherCtrl
        assert self.iface_config
        assert all((
            all((key in x for key in ('label',)))
            for x in self.iface_config.values()
        ))
        self.log = log.getChild('UDP')
        self.log.trace("Initializing UDP xport manager...")
        self._possible_chdr_ifaces = self.iface_config.keys()
        self.log.trace("Identifying available network interfaces...")
        self.chdr_port = self.eth_dispatcher_cls.DEFAULT_VITA_PORT[0]
        self._chdr_ifaces = self._init_interfaces(self._possible_chdr_ifaces)
        self._bridge_mode = args.get('bridge_mode', DEFAULT_BRIDGE_MODE)
        self._eth_dispatchers = {}

    def _init_interfaces(self, possible_ifaces):
        """
        Enumerate all network interfaces that are currently able to stream CHDR
        Returns a dictionary iface name -> iface info, where iface info is the
        return value of get_iface_info().

        Arguments:
        - possible_ifaces: A list of strings containing iface names, e.g.
                          ["sfp0", "sfp1"]

        Return Value:
        A list of dictionaries. The keys are determined by net.get_iface_info().
        """
        self.log.trace("Testing available interfaces out of `{}'".format(
            list(possible_ifaces)
        ))
        valid_iface_infos = {
            x: net.get_iface_info(x)
            for x in net.get_valid_interfaces(possible_ifaces)
        }
        # Because get_iface_info() and get_valid_interfaces() are not one atomic
        # operation, there are rare scenarios when their return values are
        # inconsistent. To catch these cases, we filter the list again and warn
        # the user. Usually, this is not a problem and the next call to
        # _init_interfaces() will be back to normal.
        valid_iface_infos_filtered = {
            x: valid_iface_infos[x]
            for x in valid_iface_infos
            if valid_iface_infos[x]['ip_addr']
        }
        if len(valid_iface_infos) != len(valid_iface_infos_filtered):
            self.log.warning(
                "Number of detected CHDR devices is inconsistent. Dropped from "
                "{} to {}."
                .format(len(valid_iface_infos), len(valid_iface_infos_filtered))
            )
        if valid_iface_infos_filtered:
            self.log.debug(
                "Found CHDR interfaces: `{}'"
                .format(", ".join(list(valid_iface_infos.keys())))
            )
        else:
            self.log.info("No CHDR interfaces found!")
        return valid_iface_infos_filtered

    def _update_dispatchers(self):
        """
        Updates the self._eth_dispatchers dictionary, makes sure that all IP
        addresses are programmed correctly.

        After calling this, _chdr_ifaces and _eth_dispatchers are in sync.
        """
        if self._bridge_mode:
            bridge_iface = list(self._chdr_ifaces.keys())[0]
            if len(self._chdr_ifaces) != 1 or bridge_iface != list(self.bridges.keys())[0]:
                self.log.error("No Bridge Interfaces found")
                raise RuntimeError("No Bridge Interfaces found")
            self.log.info(
                "Updated dispatchers in bridge mode with bridge interface {}"
                .format(bridge_iface))
            self._eth_dispatchers = {
                x: self.eth_dispatcher_cls(self.iface_config[x]['label'])
                for x in self.bridges[bridge_iface]
            }
            for dispatcher, table in self._eth_dispatchers.items():
                self.log.info("this dispatcher: {}".format(dispatcher))
                table.set_ipv4_addr(
                    self._chdr_ifaces[bridge_iface]['ip_addr'],
                    self._bridge_mode
                )
                table.set_bridge_mode(self._bridge_mode)
                table.set_bridge_mac_addr(
                    self._chdr_ifaces[bridge_iface]['mac_addr']
                )
        else:
            ifaces_to_remove = [
                x for x in self._eth_dispatchers.keys()
                if x not in self._chdr_ifaces
            ]
            for iface in ifaces_to_remove:
                self._eth_dispatchers.pop(iface)
            for iface in self._chdr_ifaces:
                if self.iface_config[iface]['type'] == 'forward':
                    self._setup_forwarding(iface)
                    continue
                if iface not in self._eth_dispatchers:
                    self._eth_dispatchers[iface] = \
                        self.eth_dispatcher_cls(self.iface_config[iface]['label'])
                self._eth_dispatchers[iface].set_ipv4_addr(
                    self._chdr_ifaces[iface]['ip_addr']
                )
                if self.iface_config[iface]['type'] == 'internal':
                    #TODO: Get MAC address from EEPROM
                    internal_ip_addr = self.get_fpga_internal_ip_address(iface)
                    self._eth_dispatchers[iface].setup_internal_interface(
                        self.get_fpga_int_mac_address(iface),
                        internal_ip_addr)

    def init(self, args):
        """
        Call this when the user calls 'init' on the periph manager
        """
        self._chdr_ifaces = self._init_interfaces(self._possible_chdr_ifaces)
        if "bridge_mode" in args:
            self._bridge_mode = args.get("bridge_mode")
        self._update_dispatchers()
        if self._bridge_mode:
            for _, table in self._eth_dispatchers.items():
                table.set_forward_policy(True, False)
        elif 'forward_eth' in args or 'forward_bcast' in args:
            for _, table in self._eth_dispatchers.items():
                table.set_forward_policy(
                    args.get('forward_eth', False),
                    args.get('forward_bcast', False)
                )

    def deinit(self):
        " Clean up after a session terminates "

    def get_xport_info(self):
        """
        Returns a dictionary of useful information, e.g. for appending into the
        device info.

        Note: This can be run by callers not owning a claim, even when the
        device has been claimed by someone else.

        In this case, returns the available IP addresses.
        """
        # This section of code is intended to prioritize
        # the sfp interfaces over the fowarding interface
        chdr_interfaces = [
            iface
            for iface in self._possible_chdr_ifaces
            if(self.iface_config[iface]['type'] != 'internal')
        ]
        forward_interfaces = [
            iface
            for iface in chdr_interfaces
            if(self.iface_config[iface]['type'] == 'forward')
        ]

        # Call _init_interfaces once to get all valid interfaces then
        # split out the forward interfaces from the external interfaces
        # _init_interfaces calls cannot be split because it emits
        # a user facing warning if no CHDR valid interfaces are found
        available_chdr_interfaces = self._init_interfaces(chdr_interfaces)
        external_chdr_interfaces = available_chdr_interfaces
        forward_chdr_interfaces = {}
        for iface in forward_interfaces:
            if iface in external_chdr_interfaces.keys():
                forward_chdr_interfaces[iface] = external_chdr_interfaces.pop(iface)

        # Create two dictionaries
        # One for the external/sfp interfaces and another forwarding interfaces
        # fourth_addr is the lowest priority for mpmd interface selection
        external_ip_dict = dict(zip(
            ("addr", "second_addr", "third_addr"),
            (x['ip_addr'] for x in external_chdr_interfaces.values())
        ))
        forward_ip_dict = dict(zip(
            ("fourth_addr",),
            (x['ip_addr'] for x in forward_chdr_interfaces.values())
        ))
        return {**external_ip_dict, **forward_ip_dict}

    def get_chdr_link_options(self, host_location='all'):
        """
        Returns a list of dictionaries for returning by
        PeriphManagerBase.get_chdr_link_options().

        Note: This requires a claim, which means that init() was called, and
        deinit() was not yet called.
        """
        assert host_location in ('remote', 'local', 'all')

        return [
            {
                'iface': iface_name,
                'ipv4': str(iface_info['ip_addr']) \
                    if (self.iface_config[iface_name]['type'] != 'internal') \
                    else str(self.get_fpga_internal_ip_address(iface_name)),
                'port': str(self.chdr_port),
                'link_rate': str(int(iface_info['link_speed'] * 1e6 / 8)),
                'type': str(self.iface_config[iface_name]['type']),
                'mtu': str(iface_info['mtu'])
            }
            for iface_name, iface_info in self._chdr_ifaces.items()
            if (
                (self.iface_config[iface_name]['type'] == 'internal' and
                 host_location == 'local') or
                (self.iface_config[iface_name]['type'] != 'internal' and
                 host_location == 'remote') or
                host_location == 'all'
            )
        ]

    def _setup_forwarding(self, iface):
        """
        Configures forwarding with iptables from the specified interface
        to an internal interface.
        """
        internal_ifaces = list(
            filter(
                lambda int_iface: self.iface_config[int_iface]['type'] == 'internal',
                self._chdr_ifaces))
        if len(internal_ifaces) == 0:
            self.log.warning(
                f'No internal interface to forward CHDR packets to from {iface}.')
            return

        int_iface = internal_ifaces[0]
        internal_ip_addr = self.get_fpga_internal_ip_address(int_iface)
        prerouting_arguments = [
            'PREROUTING',
            '-t', 'nat',
            '-i', iface,
            '-p', 'udp',
            '--dport', str(self.chdr_port),
            '-j', 'DNAT',
            '--to-destination', internal_ip_addr]
        forward_arguments = [
            'FORWARD',
            '-p', 'udp',
            '-d', internal_ip_addr,
            '--dport', str(self.chdr_port),
            '-j', 'ACCEPT']
        try:
            result = subprocess.run(
                ['iptables', '-C'] + prerouting_arguments,
                timeout=2)
            if result.returncode != 0:
                self.log.debug('Adding iptables prerouting rule')
                subprocess.run(
                    ['iptables', '-A'] + prerouting_arguments,
                    timeout=2,
                    check=True)
            result = subprocess.run(
                ['iptables', '-C'] + forward_arguments,
                timeout=2)
            if result.returncode != 0:
                self.log.debug('Adding iptables forward rule')
                subprocess.run(
                    ['iptables', '-A'] + forward_arguments,
                    timeout=2,
                    check=True)
        except subprocess.SubprocessError:
            self.log.warning('Unable to configure CHDR forwarding')

    @staticmethod
    def get_fpga_internal_ip_address(iface):
        """
        Returns the IP address of the FPGA reachable via the specified internal interface
        """
        return prefs.get_prefs().get(
            iface,
            'fpga_int_ip_address',
            fallback='169.254.0.2')

    @staticmethod
    def get_fpga_int_mac_address(iface):
        """
        Return the MAC address for the internal network connection
        """
        return prefs.get_prefs().get(
            iface,
            'fpga_int_mac_address',
            fallback='00:01:02:03:04:05')
