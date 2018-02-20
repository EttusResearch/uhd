#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Ethernet dispatcher table control
"""

from builtins import str
from builtins import object
import netaddr
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import UIO
from usrp_mpm.sys_utils.net import get_mac_addr


class EthDispatcherTable(object):
    """
    Controls an Ethernet dispatcher table.
    """
    DEFAULT_VITA_PORT = (49153, 49154)
    # Address offsets:
    OWN_IP_OFFSET = 0x0000
    OWN_PORT_OFFSET = 0x0004
    FORWARD_ETH_BCAST_OFFSET = 0x0008
    SID_IP_OFFSET = 0x1000
    SID_PORT_MAC_HI_OFFSET = 0x1400
    SID_MAC_LO_OFFSET = 0x1800

    def __init__(self, label):
        self.log = get_logger(label)
        self._regs = UIO(label=label, read_only=False)
        self.poke32 = self._regs.poke32
        self.peek32 = self._regs.peek32

    def set_ipv4_addr(self, ip_addr):
        """
        Set the own IPv4 address for this Ethernet dispatcher.
        Outgoing packets will have this IP address.
        """
        self.log.debug("Setting my own IP address to `{}'".format(ip_addr))
        ip_addr_int = int(netaddr.IPAddress(ip_addr))
        with self._regs.open():
            self.poke32(self.OWN_IP_OFFSET, ip_addr_int)

    def set_vita_port(self, port_value=None, port_idx=None):
        """
        Set the port that is used for incoming VITA traffic. This is used to
        distinguish traffic that goes to the FPGA from that going to the ARM.
        """
        port_idx = port_idx or 0
        port_value = port_value or self.DEFAULT_VITA_PORT[port_idx]
        assert port_idx in (0)                      #FIXME: Fix port_idx = 1
        port_reg_addr = self.OWN_PORT_OFFSET
        with self._regs.open():
            self.poke32(port_reg_addr, port_value)

    def set_route(self, sid, ip_addr, udp_port, mac_addr=None):
        """
        Sets up routing in the Ethernet dispatcher. From sid, only the
        destination part is important. After this call, any CHDR packet
        reaching this Ethernet dispatcher will get routed to `ip_addr' and
        `udp_port'.

        It automatically looks up the MAC address of the destination unless a
        MAC address is given.

        sid -- Full SID, but only destination part matters.
        ip_addr -- IPv4 destination address. String format ("1.2.3.4").
        udp_addr -- Destination UDP port.
        mac_addr -- If given, this will replace an ARP lookup for the MAC
                    address. String format, ("aa:bb:cc:dd:ee:ff"), case does
                    not matter.
        """
        udp_port = int(udp_port)
        if mac_addr is None:
            mac_addr = get_mac_addr(ip_addr)
        if mac_addr is None:
            self.log.error(
                "Could not resolve a MAC address for IP address `{}'".format(ip_addr)
            )
        dst_ep = sid.dst_ep
        self.log.debug(
            "Routing SID `{sid}' (endpoint `{ep}') to IP address `{ip}', " \
            "MAC address `{mac}', port `{port}'".format(
                sid=str(sid),
                ep=dst_ep,
                ip=ip_addr,
                mac=mac_addr,
                port=udp_port
            )
        )
        ip_addr_int = int(netaddr.IPAddress(ip_addr))
        mac_addr_int = int(netaddr.EUI(mac_addr))
        sid_offset = 4 * dst_ep

        def poke_and_trace(addr, data):
            " Do a poke32() and log.trace() "
            self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
                addr, data
            ))
            self.poke32(addr, data)

        with self._regs.open():
            poke_and_trace(
                self.SID_IP_OFFSET + sid_offset,
                ip_addr_int
            )
            poke_and_trace(
                self.SID_MAC_LO_OFFSET + sid_offset,
                mac_addr_int & 0xFFFFFFFF,
            )
            poke_and_trace(
                self.SID_PORT_MAC_HI_OFFSET + sid_offset,
                (udp_port << 16) | (mac_addr_int >> 32)
            )

    def set_forward_policy(self, forward_eth, forward_bcast):
        """
        Forward Ethernet packet not matching OWN_IP to CROSSOVER
        Forward broadcast packet to CPU and CROSSOVER
        """
        reg_value = int(bool(forward_eth) << 1) | int(bool(forward_bcast))
        self.log.trace("Writing to address 0x{:04X}: 0x{:04X}".format(
            self.FORWARD_ETH_BCAST_OFFSET, reg_value
        ))
        with self._regs.open():
            self.poke32(self.FORWARD_ETH_BCAST_OFFSET, reg_value)
