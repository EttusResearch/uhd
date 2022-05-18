#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Ethernet dispatcher table control
"""

import netaddr
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import UIO

class EthDispatcherCtrl:
    """
    Controls an Ethernet dispatcher.
    """
    DEFAULT_VITA_PORT = (49153, 49154)
    # Address offsets:
    # pylint: disable=bad-whitespace
    ETH_IP_OFFSET                 = 0x0000
    ETH_PORT_OFFSET               = 0x0004
    FORWARD_ETH_BCAST_OFFSET      = 0x0008
    BRIDGE_INTERNAL_MAC_LO_OFFSET = 0x0010
    BRIDGE_INTERNAL_MAC_HI_OFFSET = 0x0014
    BRIDGE_INTERNAL_IP_OFFSET     = 0x0018
    BRIDGE_INTERNAL_PORT_OFFSET   = 0x001c
    BRIDGE_INTERNAL_ENABLE_OFFSET = 0x0020
    # pylint: enable=bad-whitespace


    def __init__(self, label):
        self.log = get_logger(label)
        self._regs = UIO(label=label, read_only=False)
        self.poke32 = self._regs.poke32
        self.peek32 = self._regs.peek32

    def set_bridge_mode(self, bridge_mode):
        " Enable/Disable Bridge Mode "
        self.log.trace("Bridge Mode {}".format(
            "Enabled" if bridge_mode else "Disabled"
        ))
        self.poke32(self.BRIDGE_INTERNAL_ENABLE_OFFSET, int(bridge_mode))

    def set_bridge_mac_addr(self, mac_addr):
        """
        Set the bridge MAC address for this Ethernet dispatcher.
        Outgoing packets will have this MAC address.
        """
        self.log.debug("Setting bridge MAC address to `{}'".format(mac_addr))
        mac_addr_int = int(netaddr.EUI(mac_addr))
        self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
            self.BRIDGE_INTERNAL_MAC_LO_OFFSET, mac_addr_int & 0xFFFFFFFF
        ))
        self.poke32(self.BRIDGE_INTERNAL_MAC_LO_OFFSET, mac_addr_int & 0xFFFFFFFF)
        self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
            self.BRIDGE_INTERNAL_MAC_HI_OFFSET, mac_addr_int >> 32
        ))
        self.poke32(self.BRIDGE_INTERNAL_MAC_HI_OFFSET, mac_addr_int >> 32)

    def set_ipv4_addr(self, ip_addr, bridge_en=False):
        """
        Set the own IPv4 address for this Ethernet dispatcher.
        Outgoing packets will have this IP address.
        """
        if bridge_en:
            own_ip_offset = self.BRIDGE_INTERNAL_IP_OFFSET
        else:
            own_ip_offset = self.ETH_IP_OFFSET
        self.log.debug("Setting my own IP address to `{}'".format(ip_addr))
        ip_addr_int = int(netaddr.IPAddress(ip_addr))
        with self._regs:
            self.poke32(own_ip_offset, ip_addr_int)

    def set_vita_port(self, port_value=None, port_idx=None, bridge_en=False):
        """
        Set the port that is used for incoming VITA traffic. This is used to
        distinguish traffic that goes to the FPGA from that going to the ARM.
        """
        port_idx = port_idx or 0
        port_value = port_value or self.DEFAULT_VITA_PORT[port_idx]
        assert port_idx in (0,) #FIXME: Fix port_idx = 1
        if bridge_en:
            port_reg_addr = self.BRIDGE_INTERNAL_PORT_OFFSET
        else:
            port_reg_addr = self.ETH_PORT_OFFSET
        with self._regs:
            self.poke32(port_reg_addr, port_value)
        self.log.debug("Setting RFNOC UDP port to `{}'".format(port_value))

    def set_forward_policy(self, forward_eth, forward_bcast):
        """
        Forward Ethernet packet not matching OWN_IP to CROSSOVER
        Forward broadcast packet to CPU and CROSSOVER
        """
        reg_value = int(bool(forward_eth) << 1) | int(bool(forward_bcast))
        self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
            self.FORWARD_ETH_BCAST_OFFSET, reg_value
        ))
        with self._regs:
            self.poke32(self.FORWARD_ETH_BCAST_OFFSET, reg_value)

    def setup_internal_interface(self, mac_addr, ip_addr):
        """
        Set up the FPGA side of the internal interface
        """
        with self._regs:
            self.log.debug("Setting internal MAC address to `{}'".format(mac_addr))
            mac_addr_int = int(netaddr.EUI(mac_addr))
            mac_addr_low = mac_addr_int & 0xFFFFFFFF
            mac_addr_hi = mac_addr_int >> 32
            self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
                self.BRIDGE_INTERNAL_MAC_LO_OFFSET, mac_addr_low
            ))
            self.poke32(self.BRIDGE_INTERNAL_MAC_LO_OFFSET, mac_addr_low)
            self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
                self.BRIDGE_INTERNAL_MAC_HI_OFFSET, mac_addr_hi
            ))
            self.poke32(self.BRIDGE_INTERNAL_MAC_HI_OFFSET, mac_addr_hi)
            self.log.debug("Setting internal IP address to `{}'".format(ip_addr))
            ip_addr_int = int(netaddr.IPAddress(ip_addr))
            self.poke32(self.BRIDGE_INTERNAL_IP_OFFSET, ip_addr_int)
            self.log.debug("Setting internal Mode")
            self.poke32(self.BRIDGE_INTERNAL_ENABLE_OFFSET, int(True))
