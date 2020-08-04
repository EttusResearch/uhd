#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
This is a stream endpoint node, which is used in the RFNoCGraph class.
It also houses the logic to create output and input streams.
"""
from uhd.chdr import MgmtOpCode, MgmtOpCfg, MgmtOp, PacketType, CtrlStatus, CtrlOpCode, \
    ChdrHeader, StrcOpCode, StrcPayload, ChdrPacket, StrsStatus
from .rfnoc_common import Node, NodeType, to_iter, swap_src_dst, RETURN_TO_SENDER
from .stream_ep_regs import StreamEpRegs, STRM_STATUS_FC_ENABLED
from .chdr_stream import ChdrOutputStream, ChdrInputStream

class StreamEndpointNode(Node):
    """Represents a Stream endpoint node

    This class contains a StreamEpRegs object. To clarify, management
    packets access these registers, while control packets access the
    registers of the noc_blocks which are held in the RFNoCGraph and
    passed into handle_packet as the regs parameter
    """
    def __init__(self, node_inst, source_gen, sink_gen):
        super().__init__(node_inst)
        self.epid = node_inst
        self.dst_epid = None
        self.upstream = None
        # ---- These 4 aren't configurable right now
        self.has_data = True
        self.has_ctrl = True
        self.input_ports = 1
        self.output_ports = 1
        # ----
        self.input_stream = None
        self.output_stream = None
        self.chdr_w = None
        self.send_wrapper = None
        self.dst_to_addr = None
        self.source_gen = source_gen
        self.sink_gen = sink_gen
        self.downstream_capacity = None
        self.strs_handlers = {}
        self.ep_regs = StreamEpRegs(self.get_epid, self.set_epid, self.set_dst_epid,
                                    self.ctrl_status_callback_out, self.ctrl_status_callback_in,
                                    4, 4 * 8000)

    def ctrl_status_callback_out(self, status):
        """Called by the ep_regs when on a write to the
        REG_OSTRM_CTRL_STATUS register
        """
        if status.cfg_start:
            # This only creates a new ChdrOutputStream if the cfg_start flag is set
            self.log.info("Starting Stream EPID:{} -> EPID:{}".format(self.epid, self.dst_epid))
            addr = self.dst_to_addr(self)
            self.send_strc(addr)
            def handle_initial_strs(strs_packet):
                payload = strs_packet.get_payload_strs()
                assert payload.status == StrsStatus.OKAY, \
                    "Received STRS Packet Err: {}".format(payload.status)
                self.downstream_capacity = (payload.capacity_pkts, payload.capacity_bytes)
            self.strs_handlers[self.epid] = handle_initial_strs
        return STRM_STATUS_FC_ENABLED

    def ctrl_status_callback_in(self, status):
        """Called by the ep_regs on a write to the
        REG_OSTRM_CTRL_STATUS register
        """
        # This always triggers the graph to create a new ChdrInputStream
        self.begin_input()
        return STRM_STATUS_FC_ENABLED

    def graph_init(self, log, set_device_id, send_wrapper, chdr_w, dst_to_addr, **kwargs):
        super().graph_init(log, set_device_id)
        self.ep_regs.log = log
        self.chdr_w = chdr_w
        self.send_wrapper = send_wrapper
        self.dst_to_addr = dst_to_addr

    def get_type(self):
        return NodeType.STRM_EP

    def get_epid(self):
        """Get this endpoint's endpoint id"""
        return self.epid

    def set_epid(self, epid):
        """Set this endpoint's endpoint id"""
        self.epid = epid

    def set_dst_epid(self, dst_epid):
        """Set this endpoint's destination endpoint id"""
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
                swap_src_dst(packet, payload)
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
                raise NotImplementedError("op_code {} is not implemented for StreamEndpointNode"
                                          .format(op.op_code))
        self.log.trace("Stream Endpoint {} processed hop:\n{}"
                       .format(self.node_inst, our_hop))
        packet.set_payload(payload)
        if send_upstream:
            return RETURN_TO_SENDER
        self.log.trace("Stream Endpoint {} received packet:\n{}"
                       .format(self.node_inst, packet))

    def _handle_ctrl_packet(self, packet, regs, **kwargs):
        payload = packet.get_payload_ctrl()
        swap_src_dst(packet, payload)
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

    def _handle_data_packet(self, packet, num_bytes, sender, **kwargs):
        assert self.input_stream is not None
        self.input_stream.queue_packet(packet, num_bytes, sender)

    def _handle_strs_packet(self, packet, **kwargs):
        header = packet.get_header()
        if header.dst_epid in self.strs_handlers:
            self.strs_handlers.pop(header.dst_epid)(packet)
        else:
            self.output_stream.queue_packet(packet)

    def _handle_strc_packet(self, packet, **kwargs):
        self._handle_data_packet(packet, **kwargs)

    def send_strc(self, addr):
        """Send a Stream Command packet from the specified stream_ep
        to the specified address.

        This is not handled in ChdrOutputStream because the STRC must be
        dispatched before UHD configures the samples per packet,
        which is required to complete a StreamSpec
        """
        header = ChdrHeader()
        header.dst_epid = self.dst_epid
        header.pkt_type = PacketType.STRC
        payload = StrcPayload()
        payload.src_epid = self.epid
        payload.op_code = StrcOpCode.INIT
        payload.num_pkts = 10
        payload.num_bytes = 10 * 1024 # 10 KB
        packet = ChdrPacket(self.chdr_w, header, payload)
        self.send_wrapper.send_packet(packet, addr)

    def begin_output(self, stream_spec):
        """Spin up a new ChdrOutputStream thread which transmits from src_epid
        according to stream_spec.

        This is triggered from RFNoC Graph when the radio receives a
        Stream Command
        """
        # As of now, only one stream endpoint port per stream endpoint
        # is supported.
        assert self.output_stream is None, \
            "Output Stream already running on epid: {}".format(self.epid)
        stream_spec.dst_epid = self.dst_epid
        stream_spec.capacity_packets = self.downstream_capacity[0]
        stream_spec.capacity_bytes = self.downstream_capacity[1]
        self.downstream_capacity = None
        self.output_stream = ChdrOutputStream(self.log, self.chdr_w, self.source_gen(),
                                              stream_spec, self.send_wrapper)

    def end_output(self):
        """Stops src_epid's current transmission. This opens up the sep
        to new transmissions in the future.

        This is triggered either by the RFNoC Graph when the radio
        receives a Stop Stream command or when the transmission has no
        more samples to send.
        """
        self.output_stream.finish()
        self.output_stream = None

    def begin_input(self):
        """Spin up a new ChdrInputStream thread which receives all data and strc

        This is triggered by RFNoC Graph when there is a write to the
        REG_ISTRM_CTRL_STATUS register
        """
        # As of now, only one stream endpoint port per stream endpoint
        # is supported.

        # Rx streams aren't explicitly ended by UHD. If we try to start
        # a new one on the same epid, just quietly close the old one.
        if self.input_stream is not None:
            self.input_stream.finish()
        self.input_stream = ChdrInputStream(self.log, self.chdr_w,
                                            self.sink_gen(), self.send_wrapper, self.epid)
