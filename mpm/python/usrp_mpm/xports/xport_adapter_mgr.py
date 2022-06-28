#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Transport adapter manager

This allows programming custom routes into the transport manager and checking
its capabilities.
"""

import netaddr
from usrp_mpm.sys_utils import net
from .xport_adapter_ctrl import XportAdapterCtrl

class XportAdapterMgr:
    """
    Transport adapter manager
    """
    def __init__(self, log, iface, uio_label):
        self.log = log.getChild(f'XportAdapterMgr@{iface}')
        self.iface = iface
        self._ta_ctrl = XportAdapterCtrl(uio_label)
        self.log.debug(
            "Transport adapter compat number: %s Capabilities: %s Node instance: %d",
            str(self._ta_ctrl.get_compat_num()),
            ', '.join(self.get_capabilities()) if self.get_capabilities() else 'none',
            self.get_xport_adapter_inst()
        )

    def get_xport_adapter_inst(self):
        """
        Return the instance of the transport adapter that is connected to a
        given interface.
        """
        return self._ta_ctrl.get_xport_adapter_inst()

    def get_capabilities(self):
        """
        Return transport adapter capabilities as a list. For a list of values,
        see XportAdapterCtrl.
        """
        return list(self._ta_ctrl.features)

    def add_remote_ep_route(self, epid, **kwargs):
        """
        Adds a route from this transport adapter to a CHDR streaming endpoint
        with endpoint ID epid.
        When CHDR packets with the given epid reach the transport adapter, they
        will be sent to the dest_addr/dest_port/dest_mac_addr destination.

        :param epid: The 16-bit endpoint ID value.
        :param dest_addr: A string representation of the destination IPv4 address
        :param dest_port: An integer representation of the destination port
        :param dest_mac_addr: A string representation of the destination MAC address.
                         May be left empty, in which case, we will do an ARP
                         lookup.
        :param stream_mode: The stream mode used for outgoing packets. This is
                            a string representation of the available streaming
                            modes in EthDispatcherCtrl.StreamModes (may be lower
                            case).
        :returns: An integer value identifying the transport adapter used
        """
        dest_addr = kwargs.get('dest_addr')
        dest_port = kwargs.get('dest_port')
        dest_mac_addr = kwargs.get('dest_mac_addr')
        stream_mode = kwargs.get('stream_mode', '')
        def map_stream_mode(mode):
            """
            Map a string-based stream mode value to an Enum-based one.
            """
            for available_mode in self._ta_ctrl.StreamModes:
                if mode.upper() == available_mode.name:
                    return available_mode
            assert False
            return None
        try:
            stream_mode = map_stream_mode(stream_mode)
        except:
            raise ValueError(f"Invalid stream mode provided: `{stream_mode}'")
        try:
            assert netaddr.IPAddress(dest_addr).version == 4
        except:
            raise ValueError(f"Invalid IPv4 destination address: {dest_addr}")
        if not dest_mac_addr:
            self.log.debug(f"Looking up MAC address for IP address {dest_addr}...")
            dest_mac_addr = net.get_mac_addr(dest_addr)
        if not dest_mac_addr:
            raise RuntimeError(f"Could not find MAC address for IP address {dest_addr}!")
        try:
            assert str(netaddr.EUI(dest_mac_addr))
        except:
            raise ValueError(f"Invalid MAC address: {dest_mac_addr}")
        # Inputs are good, poke the regs
        self.log.debug(
            f"Adding route for endpoint ID {epid} from interface {self.iface}...")
        self._ta_ctrl.add_remote_ep_route(epid, dest_addr, dest_port, dest_mac_addr, stream_mode)
        return self._ta_ctrl.get_xport_adapter_inst()
