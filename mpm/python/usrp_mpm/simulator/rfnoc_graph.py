#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""This module includes all of the components necessary to simulate the
configuration of a NoC Core and NoC Blocks.

This module handles and responds to Management and Ctrl Packets. It
also instantiates the registers and acts as an interface between
the chdr packets on the network and the registers.
"""
from enum import IntEnum
from uhd.chdr import PacketType, MgmtOp, MgmtOpCode, MgmtOpNodeInfo, \
    CtrlStatus, CtrlOpCode, MgmtOpCfg, MgmtOpSelDest
from .noc_block_regs import NocBlockRegs, NocBlock, StreamEndpointPort, NocBlockPort
from .stream_ep_regs import StreamEpRegs, STRM_STATUS_FC_ENABLED

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
        self.total_samples = 0
        self.is_continuous = True
        self.packet_samples = None
        self.sample_rate = None
        self.dst_epid = None
        self.addr = None

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
            " sample_rate: {}, dst_epid: {}, addr: {}}}" \
            .format(self.total_samples, self.is_continuous, self.packet_samples,
                    self.sample_rate, self.dst_epid, self.addr)

def to_iter(index_func, length):
    """Allows looping over an indexed object in a for-each loop"""
    for i in range(length):
        yield index_func(i)

def _swap_src_dst(packet, payload):
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

RETURN_TO_SENDER = (0, NodeType.RTS, 0)

class Node:
    """Represents a node in a RFNoCGraph

    These objects are usually constructed by the caller of RFNoC Graph.
    Initially, they have references to other nodes as indexes of the
    list of nodes. The graph calls graph_init and from_index so this
    node can initialize their references to node_ids instead.

    Subclasses can override _handle_<packet_type>_packet methods, and
    _handle_default_packet is provided, which will receive packets whose
    specific methods are not overridden
    """
    def __init__(self, node_inst):
        """node_inst should start at 0 and increase for every Node of
        the same type that is constructed
        """
        self.device_id = None
        self.node_inst = node_inst
        self.log = None

    def graph_init(self, log, device_id, **kwargs):
        """This method is called to initialize the Node Graph"""
        self.device_id = device_id
        self.log = log

    def get_id(self):
        """Return the NodeID (device_id, node_type, node_inst)"""
        return (self.device_id, self.get_type(), self.node_inst)

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
        elif packet_type in (PacketType.DATA_W_TS, PacketType.DATA_NO_TS):
            next_node = self._handle_data_packet(packet, **kwargs)
        elif packet_type == PacketType.STRS:
            next_node = self._handle_strs_packet(packet, **kwargs)
        elif packet_type == PacketType.STRC:
            next_node = self._handle_strc_packet(packet, **kwargs)
        else:
            raise RuntimeError("Invalid Enum Value for PacketType: {}".format(packet_type))
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
            op_payload=MgmtOpNodeInfo(self.device_id, self.get_type(),
                                      self.node_inst, extended_info),
            op_code=MgmtOpCode.INFO_RESP
        )

class XportNode(Node):
    """Represents an Xport node

    When an Advertise Management op is received, the address and
    src_epid of the packet is placed in self.addr_map
    """
    def __init__(self, node_inst):
        super().__init__(node_inst)
        self.downstream = None
        self.addr_map = {}

    def get_type(self):
        return NodeType.XPORT

    def _handle_mgmt_packet(self, packet, addr, **kwargs):
        send_upstream = False
        payload = packet.get_payload_mgmt()
        our_hop = payload.pop_hop()
        packet.set_payload(payload)
        for op in to_iter(our_hop.get_op, our_hop.get_num_ops()):
            if op.op_code == MgmtOpCode.INFO_REQ:
                payload.get_hop(0).add_op(self.info_response(0))
            elif op.op_code == MgmtOpCode.RETURN:
                send_upstream = True
                _swap_src_dst(packet, payload)
            elif op.op_code == MgmtOpCode.NOP:
                pass
            elif op.op_code == MgmtOpCode.ADVERTISE:
                self.log.info("Advertise: {} | EPID:{} -> EPID:{}"
                              .format(self.get_id(), payload.src_epid,
                                      packet.get_header().dst_epid))
                self.log.info("addr_map updated: EPID:{} -> {}".format(payload.src_epid, addr))
                self.addr_map[payload.src_epid] = addr
            else:
                raise NotImplementedError(op.op_code)
        self.log.trace("Xport {} processed hop:\n{}"
                       .format(self.node_inst, our_hop))
        packet.set_payload(payload)
        if send_upstream:
            return RETURN_TO_SENDER
        else:
            return self.downstream

    def _handle_default_packet(self, packet, **kwargs):
        return self.downstream

class XbarNode(Node):
    """Represents a crossbar node

    self.routing_table stores a mapping of dst_epids to xbar ports

    port numbers start from 0. xports are addressed first
    stream ep ports are then addressed where
    the index of the first ep = len(xport_ports)

    NOTE: UHD inteprets the node_inst of a xbar as the port which
    it is connected to. This differs from its handling of node_inst
    for other types of nodes.
    """
    def __init__(self, node_inst, ports, xport_ports):
        super().__init__(node_inst)
        self.nports = len(ports) + len(xport_ports)
        self.nports_xport = len(xport_ports)
        self.ports = xport_ports + ports
        self.routing_table = {}

    def get_type(self):
        return NodeType.XBAR

    def _handle_mgmt_packet(self, packet, **kwargs):
        send_upstream = False
        destination = None
        payload = packet.get_payload_mgmt()
        our_hop = payload.pop_hop()
        for op in to_iter(our_hop.get_op, our_hop.get_num_ops()):
            if op.op_code == MgmtOpCode.INFO_REQ:
                payload.get_hop(0).add_op(self.info_response(
                    (self.nports_xport << 8) | self.nports))
            elif op.op_code == MgmtOpCode.RETURN:
                send_upstream = True
                _swap_src_dst(packet, payload)
            elif op.op_code == MgmtOpCode.NOP:
                pass
            elif op.op_code == MgmtOpCode.ADVERTISE:
                self.log.info("Advertise: {}".format(self.get_id()))
            elif op.op_code == MgmtOpCode.CFG_WR_REQ:
                cfg = op.get_op_payload()
                cfg = MgmtOpCfg.parse(cfg)
                self.routing_table[cfg.addr] = cfg.data
                self.log.debug("Xbar {} routing changed: {}"
                               .format(self.node_inst, self.routing_table))
            elif op.op_code == MgmtOpCode.SEL_DEST:
                cfg = op.get_op_payload()
                cfg = MgmtOpSelDest.parse(cfg)
                dest_port = cfg.dest
                destination = self.ports[dest_port]
            else:
                raise NotImplementedError(op.op_code)
        self.log.trace("Xbar {} processed hop:\n{}"
                       .format(self.node_inst, our_hop))
        packet.set_payload(payload)
        if send_upstream:
            return RETURN_TO_SENDER
        elif destination is not None:
            return destination

    def _handle_default_packet(self, packet, **kwargs):
        dst_epid = packet.get_header().dst_epid
        if dst_epid not in self.routing_table:
            raise RuntimeError("Xbar no destination for packet (dst_epid: {})".format(dst_epid))
        return self.ports[self.routing_table[dst_epid]]

    def from_index(self, nodes):
        """This iterates through the list of nodes and sets the
        reference in self.ports to the node's id
        """
        for i in range(len(self.ports)):
            nodes_index = self.ports[i]
            node = nodes[nodes_index]
            self.ports[i] = node.get_id()
            if node.__class__ is XportNode:
                node.downstream = self.get_id()
            elif node.__class__ is StreamEndpointNode:
                node.upstream = self.get_id()

class StreamEndpointNode(Node):
    """Represents a Stream endpoint node

    This class contains a StreamEpRegs object. To clarify, management
    packets access these registers, while control packets access the
    registers of the noc_blocks which are held in the RFNoCGraph and
    passed into handle_packet as the regs parameter
    """
    def __init__(self, node_inst):
        super().__init__(node_inst)
        self.epid = node_inst
        self.dst_epid = None
        self.upstream = None
        self.sep_config = None
        # These 4 aren't configurable right now
        self.has_data = True
        self.has_ctrl = True
        self.input_ports = 2
        self.output_ports = 2
        self.ep_regs = StreamEpRegs(self.get_epid, self.set_epid, self.set_dst_epid,
                                    self.status_callback_out, self.status_callback_in, 4, 4 * 8000)

    def status_callback_out(self, status):
        """Called by the ep_regs when on a write to the
        REG_OSTRM_CTRL_STATUS register
        """
        if status.cfg_start:
            # This only creates a new ChdrOutputStream if the cfg_start flag is set
            self.log.info("Starting Stream EPID:{} -> EPID:{}".format(self.epid, self.dst_epid))
            self.sep_config(self, True)
        return STRM_STATUS_FC_ENABLED

    def status_callback_in(self, status):
        """Called by the ep_regs on a write to the
        REG_OSTRM_CTRL_STATUS register
        """
        # This always triggers the graph to create a new ChdrInputStream
        self.sep_config(self, False)
        return STRM_STATUS_FC_ENABLED

    def graph_init(self, log, device_id, sep_config, **kwargs):
        super().graph_init(log, device_id)
        self.ep_regs.log = log
        self.sep_config = sep_config

    def get_type(self):
        return NodeType.STRM_EP

    def get_epid(self):
        """ Get the endpoint id of this stream endpoint """
        return self.epid

    def set_epid(self, epid):
        """ Set the endpoint id of this stream endpoint """
        self.epid = epid

    def set_dst_epid(self, dst_epid):
        """ Set the destination endpoint id of this stream endpoint """
        self.dst_epid = dst_epid

    def _handle_mgmt_packet(self, packet, **kwargs):
        send_upstream = False
        payload = packet.get_payload_mgmt()
        our_hop = payload.pop_hop()
        for op in to_iter(our_hop.get_op, our_hop.get_num_ops()):
            if op.op_code == MgmtOpCode.INFO_REQ:
                ext_info = 0
                ext_info |= (1 if self.has_ctrl else 0) << 0
                ext_info |= (1 if self.has_data else 0) << 1
                ext_info |= (self.input_ports & 0x3F) << 2
                ext_info |= (self.output_ports & 0x3F) << 8
                payload.get_hop(0).add_op(self.info_response(ext_info))
            elif op.op_code == MgmtOpCode.RETURN:
                send_upstream = True
                _swap_src_dst(packet, payload)
            elif op.op_code == MgmtOpCode.NOP:
                pass
            elif op.op_code == MgmtOpCode.CFG_RD_REQ:
                request = MgmtOpCfg.parse(op.get_op_payload())
                value = self.ep_regs.read(request.addr)
                payload.get_hop(0).add_op(MgmtOp(MgmtOpCode.CFG_RD_RESP,
                                                 MgmtOpCfg(request.addr, value)))
            elif op.op_code == MgmtOpCode.CFG_WR_REQ:
                request = MgmtOpCfg.parse(op.get_op_payload())
                self.ep_regs.write(request.addr, request.data)
            else:
                raise NotImplementedError("op_code {} is not implemented for "
                                          "StreamEndpointNode".format(op.op_code))
        self.log.trace("Stream Endpoint {} processed hop:\n{}"
                       .format(self.node_inst, our_hop))
        packet.set_payload(payload)
        if send_upstream:
            return RETURN_TO_SENDER

    def _handle_ctrl_packet(self, packet, regs, **kwargs):
        payload = packet.get_payload_ctrl()
        _swap_src_dst(packet, payload)
        if payload.status != CtrlStatus.OKAY:
            raise RuntimeError("Control Status not OK: {}".format(payload.status))
        if payload.op_code == CtrlOpCode.READ:
            payload.is_ack = True
            payload.set_data([regs.read(payload.address)])
        elif payload.op_code == CtrlOpCode.WRITE:
            payload.is_ack = True
            regs.write(payload.address, payload.get_data()[0])
        else:
            raise NotImplementedError("Unknown Control OpCode: {}".format(payload.op_code))
        packet.set_payload(payload)
        return RETURN_TO_SENDER

class RFNoCGraph:
    """This class holds all of the nodes of the NoC core and the Noc
    blocks.

    It serves as an interface between the ChdrEndpoint and the
    individual blocks/nodes.
    """
    def __init__(self, graph_list, log, device_id, create_tx_func,
                 stop_tx_func, send_strc_func, create_rx_func):
        self.log = log.getChild("Graph")
        self.send_strc = send_strc_func
        self.device_id = device_id
        self.stream_spec = StreamSpec()
        self.create_tx = create_tx_func
        self.stop_tx = stop_tx_func
        self.create_rx = create_rx_func
        num_stream_ep = 0
        for node in graph_list:
            if node.__class__ is StreamEndpointNode:
                num_stream_ep += 1
            node.graph_init(self.log, device_id, sep_config=self.prepare_stream_ep)
        # These must be done sequentially so that device_id is initialized on all nodes
        # before from_index is called on any node
        for node in graph_list:
            node.from_index(graph_list)
        self.graph_map = {node.get_id(): node
                          for node in graph_list}
        # For now, just use one radio block and hardcode it to the first stream endpoint
        radio = NocBlock(1 << 16, 2, 2, 512, 1, 0x12AD1000, 16)
        adj_list = [
            (StreamEndpointPort(0, 0), NocBlockPort(0, 0)),
            (StreamEndpointPort(0, 1), NocBlockPort(0, 1)),
            (NocBlockPort(0, 0), StreamEndpointPort(0, 0)),
            (NocBlockPort(0, 1), StreamEndpointPort(0, 1))
        ]
        self.regs = NocBlockRegs(self.log, 1 << 16, True, 1, [radio], num_stream_ep, 1, 0xE320,
                                 adj_list, 8, 1, self.get_stream_spec, self.radio_tx_cmd,
                                 self.radio_tx_stop)

    def radio_tx_cmd(self, sep_block_id):
        """Triggers the creation of a ChdrOutputStream in the ChdrEndpoint using
        the current stream_spec.

        This method transforms the sep_block_id into an epid useable by
        the transmit code
        """
        # TODO: Use the port
        sep_blk, sep_port = sep_block_id
        # The NoC Block index for stream endpoints is the inst + 1
        # See rfnoc_graph.cpp:rfnoc_graph_impl#_init_sep_map()
        sep_inst = sep_blk - 1
        sep_id = (self.get_device_id(), NodeType.STRM_EP, sep_inst)
        stream_ep = self.graph_map[sep_id]
        self.stream_spec.dst_epid = stream_ep.dst_epid
        self.stream_spec.addr = self.dst_to_addr(stream_ep)
        self.log.info("Streaming with StreamSpec:")
        self.log.info(str(self.stream_spec))
        self.create_tx(stream_ep.epid, self.stream_spec)
        self.stream_spec = StreamSpec()

    def radio_tx_stop(self, sep_block_id):
        """Triggers the destuction of a ChdrOutputStream in the ChdrEndpoint

        This method transforms the sep_block_id into an epid useable by
        the transmit code
        """
        sep_blk, sep_port = sep_block_id
        sep_inst = sep_blk - 1
        # The NoC Block index for stream endpoints is the inst + 1
        # See rfnoc_graph.cpp:rfnoc_graph_impl#_init_sep_map()
        sep_id = (self.get_device_id(), NodeType.STRM_EP, sep_inst)
        src_epid = self.graph_map[sep_id].epid
        self.stop_tx(src_epid)

    def get_device_id(self):
        return self.device_id

    def prepare_stream_ep(self, stream_ep, is_tx):
        """This is called by a stream_ep when it receives a status update

        Depending on whether it was a tx or rx status, it will either
        trigger an strc packet or an rx stream
        """
        if is_tx:
            addr = self.dst_to_addr(stream_ep)
            self.send_strc(stream_ep, addr)
        else:
            self.create_rx(stream_ep.epid)

    def change_spp(self, spp):
        """Change the Stream Samples per Packet"""
        self.stream_spec.packet_samples = spp

    def find_ep_by_id(self, epid):
        """Find a Stream Endpoint which identifies with epid"""
        for node in self.graph_map.values():
            if node.__class__ is StreamEndpointNode:
                if node.epid == epid:
                    return node

    # Fixme: This doesn't support intra-device connections
    # i.e. connecting nodes by connecting two internal stream endpoints
    #
    # That would require looking at the adjacency list if we end up at
    # another stream endpoint instead of an xport
    def dst_to_addr(self, src_ep):
        """This function traverses backwards through the node graph,
        starting from src_ep and taking the path traveled by a packet
        heading for src_ep.dst_epid. When it encounters an xport, it
        returns the address associated with the dst_epid in the xport's
        addr_map
        """
        current_node = src_ep
        dst_epid = src_ep.dst_epid
        while current_node.__class__ != XportNode:
            if current_node.__class__ == StreamEndpointNode:
                current_node = self.graph_map[current_node.upstream]
            else:
                # current_node is a xbar
                port = current_node.routing_table[dst_epid]
                upstream_id = current_node.ports[port]
                current_node = self.graph_map[upstream_id]
        return current_node.addr_map[dst_epid]

    def handle_packet(self, packet, xport_input, addr):
        """Given a chdr_packet, the id of an xport node to serve as an
        entry point, and a source address, send the packet through the
        node graph.
        """
        node_id = xport_input
        response_packet = None
        while node_id is not None:
            if node_id[1] == NodeType.RTS:
                response_packet = packet
                break
            node = self.graph_map[node_id]
            # If the node returns a value, it is the node id of the
            # node the packet should be passed to next
            # or RETURN_TO_SENDER
            node_id = node.handle_packet(packet, regs=self.regs, addr=addr)
        return response_packet

    def get_stream_spec(self):
        """ Get the current output stream configuration """
        return self.stream_spec
