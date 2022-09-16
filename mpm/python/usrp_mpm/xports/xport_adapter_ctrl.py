#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
Transport adapter control
"""

from enum import Enum
import netaddr
from usrp_mpm.mpmutils import poll_with_timeout
from usrp_mpm.compat_num import CompatNumber
from usrp_mpm.mpmlog import get_logger
from usrp_mpm.sys_utils.uio import UIO

MIN_COMPAT_NUM_REMOTE_STREAMING = CompatNumber(1, 0)

# pylint: disable=too-many-arguments

class XportAdapterCtrl:
    """
    Controls advanced transport adapter (TA) capabilities, if present.

    Note that this requires the ability to read/write from the transport adapter
    address space, which is not always available (e.g., on older bitfiles).
    Make sure to not instantiate one of these classes if the feature is not
    available, or it will result in a bus error.

    This class will use the compat number of the TA to determine its
    capabilities, which it will store in the 'features' attribute. Valid entries
    for this attribute include:
    - rx_routing: This TA has the capability to route RX data to arbitrary endpoints
    - rx_hdr_removal: The TA can remove CHDR headers when routing data to
                      arbitrary endpoints (StreamModes.RAW_PAYLOAD may be used)
    """
    # Address offsets (these must match xport_sv/eth_regs.vh)
    # pylint: disable=bad-whitespace
    XPORT_ADAPTER_COMPAT_NUM      = 0x0100 # 8 bits major, 8 bits minor
    XPORT_ADAPTER_INFO            = 0x0104
    XPORT_ADAPTER_NODE_INST       = 0x0108 # read-only
    KV_MAC_LO                     = 0x010C
    KV_MAC_HI                     = 0x0110
    KV_IPV4                       = 0x0114
    KV_UDP_PORT                   = 0x0118
    KV_CFG                        = 0x011C
    # pylint: enable=bad-whitespace

    class StreamModes(Enum):
        """
        Valid streaming modes for outgoing packets

        The numerical values must be such that they can be written to KV_CFG
        (see eth_regs.vh).
        """
        FULL_PACKET = 0
        RAW_PAYLOAD = 1


    def __init__(self, label):
        self.log = get_logger(label)
        self._regs = UIO(label=label, read_only=False)
        self.poke32 = self._regs.poke32
        self.peek32 = self._regs.peek32
        self.features = set()
        self._ta_index = -1
        # These three registers are always available, even if the transport
        # adapter supports nothing else.
        with self._regs:
            compat_num = self.peek32(self.XPORT_ADAPTER_COMPAT_NUM)
            self._ta_index = self.peek32(self.XPORT_ADAPTER_NODE_INST)
            adapter_info = self.peek32(self.XPORT_ADAPTER_INFO)
        self._compat_num = CompatNumber((compat_num >> 8) & 0xFF, compat_num & 0xFF)
        # If the FPGA is new enough to know about remote UDP streaming, but
        # doesn't have that feature enabled, it will have a compat number of
        # zero.
        if self._compat_num < MIN_COMPAT_NUM_REMOTE_STREAMING:
            return
        if bool(adapter_info & (1 << 0)):
            self.features.add('rx_routing')
        if bool(adapter_info & (1 << 1)):
            self.features.add('rx_hdr_removal')

    def get_xport_adapter_inst(self):
        """
        Returns an integer value that tells us which transport adapter is
        connected to this interface (the instance, or node_inst) value.
        """
        return self._ta_index

    def get_compat_num(self):
        """
        Return the compat number of this transport adapter
        """
        return self._compat_num

    def add_remote_ep_route(self, epid, ipv4, port, mac_addr, stream_mode):
        """
        Adds a route from this transport adapter to a CHDR streaming endpoint
        with endpoint ID epid.
        When CHDR packets with the given epid reach the transport adapter, they
        will be sent to the ipv4/port/mac_addr destination.

        :param epid: The 16-bit endpoint ID value.
        :param ipv4: A string representation of the destination IPv4 address
        :param port: An integer representation of the destination port
        :param mac_addr: A string representation of the destination MAC address
        :param stream_mode: The stream mode used for outgoing packets
        """
        # Sanitize inputs
        assert isinstance(stream_mode, self.StreamModes)
        assert (epid >> 16) == 0
        # Check capabilities
        if 'rx_routing' not in self.features:
            raise RuntimeError(
                "This transport adapter does not support routing to remote "
                "destinations!")
        if stream_mode == self.StreamModes.RAW_PAYLOAD \
                and 'rx_hdr_removal' not in self.features:
            raise RuntimeError(
                "Requesting to remove CHDR headers, but feature not enabled!")
        # Encode inputs for our registers
        stream_mode_int = stream_mode.value
        mac_addr_int = int(netaddr.EUI(mac_addr))
        mac_addr_lo = mac_addr_int & 0xFFFFFFFF
        mac_addr_hi = (mac_addr_int >> 32) & 0xFFFF
        ipv4_int = int(netaddr.IPAddress(ipv4))
        port = int(port)
        cfg_word = epid | (stream_mode_int << 16)
        self.log.debug(
            f"On transport adapter {self._ta_index}: Adding route from EPID "
            f"{epid} to destination {ipv4}:{port} (MAC Address: {mac_addr}), "
            f"stream mode {stream_mode.name} ({stream_mode_int})")
        # Now write registers
        with self._regs:
            # Check BUSY bit before writing new values
            if not poll_with_timeout(
                    lambda: not bool(self.peek32(self.KV_CFG) & (1 << 31)),
                    0.5, # timeout at 500 ms
                    0.1, # check at 100 ms interval
                ):
                raise RuntimeError(
                    f"Timeout while polling BUSY bit on transport adapter "
                    f"{self._ta_index}!")
            # Now write all the configurations; CFG goes last
            self.poke32(self.KV_MAC_LO, mac_addr_lo)
            self.poke32(self.KV_MAC_HI, mac_addr_hi)
            self.poke32(self.KV_IPV4, ipv4_int)
            self.poke32(self.KV_UDP_PORT, port)
            self.poke32(self.KV_CFG, cfg_word)
