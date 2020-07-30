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
from uhd.chdr import MgmtOpCode, MgmtOpCfg, MgmtOpSelDest
from .noc_block_regs import NocBlockRegs, NocBlock, StreamEndpointPort, NocBlockPort
from .rfnoc_common import Node, NodeType, StreamSpec, to_iter, swap_src_dst, RETURN_TO_SENDER
from .stream_endpoint_node import StreamEndpointNode

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
                swap_src_dst(packet, payload)
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
                swap_src_dst(packet, payload)
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
        else:
            return self._handle_default_packet(packet, **kwargs)

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
            self.ports[i] = node.get_local_id()
            if node.__class__ is XportNode:
                node.downstream = self.get_local_id()
            elif node.__class__ is StreamEndpointNode:
                node.upstream = self.get_local_id()

class RFNoCGraph:
    """This class holds all of the nodes of the NoC core and the Noc
    blocks.

    It serves as an interface between the ChdrEndpoint and the
    individual blocks/nodes.
    """
    def __init__(self, graph_list, log, device_id, send_wrapper, chdr_w, rfnoc_device_id):
        self.log = log.getChild("Graph")
        self.device_id = device_id
        self.stream_spec = StreamSpec()
        self.stream_ep = []
        for node in graph_list:
            if node.__class__ is StreamEndpointNode:
                self.stream_ep.append(node)
            node.graph_init(self.log, self.get_device_id, send_wrapper=send_wrapper,
                            chdr_w=chdr_w, dst_to_addr=self.dst_to_addr)
        # These must be done sequentially so that get_device_id is initialized on all nodes
        # before from_index is called on any node
        for node in graph_list:
            node.from_index(graph_list)
        self.graph_map = {node.get_local_id(): node
                          for node in graph_list}
        # For now, just use one radio block and hardcode it to the first stream endpoint
        radio = NocBlock(1 << 16, 2, 2, 512, 1, 0x12AD1000, 16)
        adj_list = [
            (StreamEndpointPort(0, 0), NocBlockPort(0, 0)),
            (StreamEndpointPort(0, 1), NocBlockPort(0, 1)),
            (NocBlockPort(0, 0), StreamEndpointPort(0, 0)),
            (NocBlockPort(0, 1), StreamEndpointPort(0, 1))
        ]
        self.regs = NocBlockRegs(self.log, 1 << 16, True, 1, [radio], len(self.stream_ep), 1,
                                 rfnoc_device_id, adj_list, 8, 1, self.get_stream_spec,
                                 self.radio_tx_cmd, self.radio_tx_stop)

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
        sep_id = (NodeType.STRM_EP, sep_inst)
        stream_ep = self.graph_map[sep_id]
        self.stream_spec.addr = self.dst_to_addr(stream_ep)
        self.log.info("Streaming with StreamSpec:")
        self.log.info(str(self.stream_spec))
        stream_ep.begin_output(self.stream_spec)

    def radio_tx_stop(self, sep_block_id):
        """Triggers the destuction of a ChdrOutputStream in the ChdrEndpoint

        This method transforms the sep_block_id into an epid useable by
        the transmit code
        """
        sep_blk, sep_port = sep_block_id
        sep_inst = sep_blk - 1
        # The NoC Block index for stream endpoints is the inst + 1
        # See rfnoc_graph.cpp:rfnoc_graph_impl#_init_sep_map()
        sep_id = (NodeType.STRM_EP, sep_inst)
        stream_ep = self.graph_map[sep_id]
        stream_ep.end_output()

    def get_device_id(self):
        return self.device_id

    def set_device_id(self, device_id):
        """Set this graph's rfnoc device id"""
        self.device_id = device_id

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

    def handle_packet(self, packet, xport_input, addr, sender, num_bytes):
        """Given a chdr_packet, the id of an xport node to serve as an
        entry point, and a source address, send the packet through the
        node graph.
        """
        node_id = xport_input
        response_packet = None
        while node_id is not None:
            assert len(node_id) == 2, "Node returned non-local node_id of len {}: {}" \
                .format(len(node_id), node_id)

            if node_id[0] == NodeType.RTS:
                response_packet = packet
                break
            node = self.graph_map[node_id]
            # If the node returns a value, it is the node id of the
            # node the packet should be passed to next
            # or RETURN_TO_SENDER
            node_id = node.handle_packet(packet, regs=self.regs, addr=addr,
                                         sender=sender, num_bytes=num_bytes)
        return response_packet

    def get_stream_spec(self):
        """ Get the current output stream configuration """
        return self.stream_spec
