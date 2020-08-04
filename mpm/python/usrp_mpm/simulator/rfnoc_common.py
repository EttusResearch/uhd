#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This file contains common classes that are used by both rfnoc_graph.py
and stream_endpoint_node.py
"""
from enum import IntEnum
from uhd.chdr import MgmtOpCode, MgmtOpNodeInfo, MgmtOp, PacketType

def to_iter(index_func, length):
    """Allows looping over an indexed object in a for-each loop"""
    for i in range(length):
        yield index_func(i)

def swap_src_dst(packet, payload):
    """Swap the src_epid and the dst_epid of a packet"""
    header = packet.get_header()
    our_epid = header.dst_epid
    header.dst_epid = payload.src_epid
    payload.src_epid = our_epid
    packet.set_header(header)

class NodeType(IntEnum):
    """The type of a node in a NoC Core

    RTS is a magic value used to determine when to return a packet to
    the sender
    """
    INVALID = 0
    XBAR = 1
    STRM_EP = 2
    XPORT = 3
    RTS = 0xFF

RETURN_TO_SENDER = (NodeType.RTS, 0)

class Node:
    """Represents a node in a NoC Core

    These objects are usually constructed by the caller of RFNoC Graph.
    Initially, they have references to other nodes as indexes of the
    list of nodes. The graph calls graph_init and from_index so this
    node can initialize their references to node_ids instead.
    """
    def __init__(self, node_inst):
        """node_inst should start at 0 and increase for every Node of
        the same type that is constructed
        """
        self.node_inst = node_inst
        self.log = None
        self.get_device_id = None

    def graph_init(self, log, get_device_id, **kwargs):
        """This method is called to initialize the Node Graph"""
        self.log = log
        self.get_device_id = get_device_id

    def get_id(self):
        """Return the NodeID (device_id, node_type, node_inst)"""
        return (self.get_device_id(), self.get_type(), self.node_inst)

    def get_local_id(self):
        """Return the Local NodeID (node_type, node_inst)"""
        return (self.get_type(), self.node_inst)

    def get_type(self):
        """Returns the NodeType of this node"""
        raise NotImplementedError

    def handle_packet(self, packet, **kwargs):
        """Processes a packet
        The return value should be a node_id or RETURN_TO_SENDER
        """
        packet_type = packet.get_header().pkt_type
        next_node = None
        if packet_type == PacketType.MGMT:
            next_node = self._handle_mgmt_packet(packet, **kwargs)
        elif packet_type == PacketType.CTRL:
            next_node = self._handle_ctrl_packet(packet, **kwargs)
        elif packet_type in (PacketType.DATA_WITH_TS, PacketType.DATA_NO_TS):
            next_node = self._handle_data_packet(packet, **kwargs)
        elif packet_type == PacketType.STRS:
            next_node = self._handle_strs_packet(packet, **kwargs)
        elif packet_type == PacketType.STRC:
            next_node = self._handle_strc_packet(packet, **kwargs)
        else:
            raise RuntimeError("Invalid Enum Value for PacketType: {}".format(packet_type))
        # If the specific method for that packet isn't implemented, try the general method
        if next_node == NotImplemented:
            next_node = self._handle_default_packet(packet, **kwargs)
        return next_node

    # pylint: disable=unused-argument,no-self-use
    def _handle_mgmt_packet(self, packet, **kwargs):
        return NotImplemented

    # pylint: disable=unused-argument,no-self-use
    def _handle_ctrl_packet(self, packet, **kwargs):
        return NotImplemented

    # pylint: disable=unused-argument,no-self-use
    def _handle_data_packet(self, packet, **kwargs):
        return NotImplemented

    # pylint: disable=unused-argument,no-self-use
    def _handle_strc_packet(self, packet, **kwargs):
        return NotImplemented

    # pylint: disable=unused-argument,no-self-use
    def _handle_strs_packet(self, packet, **kwargs):
        return NotImplemented

    def _handle_default_packet(self, packet, **kwargs):
        raise RuntimeError("{} has no operation defined for a {} packet"
                           .format(self.__class__, packet.get_header().pkt_type))

    def from_index(self, nodes):
        """Initialize this node's indexes to block_id references"""
        pass

    def info_response(self, extended_info):
        """Generate a node info response MgmtOp"""
        return MgmtOp(
            op_payload=MgmtOpNodeInfo(self.get_device_id(), self.get_type(),
                                      self.node_inst, extended_info),
            op_code=MgmtOpCode.INFO_RESP
        )

class StreamSpec:
    """This class carries the configuration parameters of a Tx stream.

    total_samples, is_continuous, and packet_samples come from the
    radio registers (noc_block_regs.py)

    sample_rate comes from an rpc to the daughterboard (through the
    set_sample_rate method in chdr_endpoint.py)

    dst_epid comes from the source stream_ep

    addr comes from the xport passed through when routing to dst_epid
    """
    LOW_MASK = 0xFFFFFFFF
    HIGH_MASK = (0xFFFFFFFF) << 32
    def __init__(self):
        self.init_timestamp = 0
        self.is_timed = False
        self.total_samples = 0
        self.is_continuous = True
        self.packet_samples = None
        self.sample_rate = None
        self.dst_epid = None
        self.addr = None
        self.capacity_packets = 0
        self.capacity_bytes = 0

    def set_timestamp_lo(self, low):
        """Set the low 32 bits of the initial timestamp"""
        self.init_timestamp = (self.init_timestamp & (StreamSpec.HIGH_MASK)) \
            | (low & StreamSpec.LOW_MASK)

    def set_timestamp_hi(self, high):
        """Set the high 32 bits of the initial timestamp"""
        self.init_timestamp = (self.init_timestamp & StreamSpec.LOW_MASK) \
            | ((high & StreamSpec.LOW_MASK) << 32)

    def set_num_words_lo(self, low):
        """Set the low 32 bits of the total_samples field"""
        self.total_samples = (self.total_samples & (StreamSpec.HIGH_MASK)) \
            | (low & StreamSpec.LOW_MASK)

    def set_num_words_hi(self, high):
        """Set the high 32 bits of the total_samples field"""
        self.total_samples = (self.total_samples & StreamSpec.LOW_MASK) \
            | ((high & StreamSpec.LOW_MASK) << 32)

    def seconds_per_packet(self):
        """Calculates how many seconds should be between each packet
        transmit
        """
        assert self.packet_samples != 0
        assert self.sample_rate != 0
        return self.packet_samples / self.sample_rate

    def __str__(self):
        return "StreamSpec{{total_samples: {}, is_continuous: {}, packet_samples: {}," \
               "sample_rate: {}, dst_epid: {}, addr: {}}}" \
               .format(self.total_samples, self.is_continuous, self.packet_samples,
                       self.sample_rate, self.dst_epid, self.addr)
