#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
from uhd import chdr
from chdr_resource import rfnoc_packets_data
from chdr_resource import rfnoc_packets_ctrl_mgmt

CHDR_W = chdr.ChdrWidth.W64

def make_control_packet0():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.CTRL
    header.length = 24
    header.dst_epid = 2
    payload = chdr.CtrlPayload()
    payload.src_epid = 1
    payload.set_data([0])
    payload.op_code = chdr.CtrlOpCode.READ
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_control_packet1():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.CTRL
    header.length = 24
    header.dst_epid = 1
    payload = chdr.CtrlPayload()
    payload.src_epid = 2
    payload.is_ack = True
    payload.set_data([0x12C60100])
    payload.op_code = chdr.CtrlOpCode.READ
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_mgmt_packet0():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.MGMT
    header.length = 48
    header.seq_num = 2
    payload = chdr.MgmtPayload()
    payload.set_header(1, (1 << 8) | (0 << 0), chdr.ChdrWidth.W64)
    hop1 = chdr.MgmtHop()
    hop1.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP))
    payload.add_hop(hop1)
    hop2 = chdr.MgmtHop()
    hop2.add_op(chdr.MgmtOp(chdr.MgmtOpCode.INFO_REQ, chdr.MgmtOpNodeInfo(0, 0, 0, 0)))
    hop2.add_op(chdr.MgmtOp(chdr.MgmtOpCode.RETURN))
    payload.add_hop(hop2)
    hop3 = chdr.MgmtHop()
    hop3.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP))
    payload.add_hop(hop3)
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_mgmt_packet1():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.MGMT
    header.length = 48
    header.seq_num = 3
    payload = chdr.MgmtPayload()
    payload.set_header(1, (1 << 8) | (0 << 0), chdr.ChdrWidth.W64)
    hop1 = chdr.MgmtHop()
    hop1.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP))
    payload.add_hop(hop1)
    hop2 = chdr.MgmtHop()
    hop2.add_op(chdr.MgmtOp(chdr.MgmtOpCode.CFG_WR_REQ, chdr.MgmtOpCfg(0x1, 0x2)))
    hop2.add_op(chdr.MgmtOp(chdr.MgmtOpCode.RETURN))
    payload.add_hop(hop2)
    hop3 = chdr.MgmtHop()
    hop3.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP))
    payload.add_hop(hop3)
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_mgmt_packet2():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.MGMT
    header.length = 56
    header.seq_num = 6
    payload = chdr.MgmtPayload()
    payload.set_header(1, (1 << 8) | (0 << 0), chdr.ChdrWidth.W64)
    hop1 = chdr.MgmtHop()
    hop1.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP))
    payload.add_hop(hop1)
    hop2 = chdr.MgmtHop()
    hop2.add_op(chdr.MgmtOp(chdr.MgmtOpCode.SEL_DEST, chdr.MgmtOpSelDest(3)))
    payload.add_hop(hop2)
    hop3 = chdr.MgmtHop()
    hop3.add_op(chdr.MgmtOp(chdr.MgmtOpCode.INFO_REQ, 0))
    hop3.add_op(chdr.MgmtOp(chdr.MgmtOpCode.RETURN, 0))
    payload.add_hop(hop3)
    hop4 = chdr.MgmtHop()
    hop4.add_op(chdr.MgmtOp(chdr.MgmtOpCode.NOP, 0))
    payload.add_hop(hop4)
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_strs_packet0():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.STRS
    header.length = 40
    header.dst_epid = 2
    payload = chdr.StrsPayload()
    payload.capacity_bytes = 163840
    payload.src_epid = 3
    payload.capacity_pkts = 16777215
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_strs_packet1():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.STRS
    header.length = 40
    header.dst_epid = 2
    payload = chdr.StrsPayload()
    payload.capacity_bytes = 163840
    payload.src_epid = 3
    payload.xfer_count_pkts = 1
    payload.capacity_pkts = 16777215
    payload.xfer_count_bytes = 8000
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_strc_packet0():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.STRC
    header.length = 24
    header.dst_epid = 3
    payload = chdr.StrcPayload()
    payload.num_pkts = 16777215
    payload.op_code = chdr.StrcOpCode.INIT
    payload.src_epid = 2
    payload.num_bytes = 5120
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_strc_packet1():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.STRC
    header.length = 24
    header.dst_epid = 3
    payload = chdr.StrcPayload()
    payload.num_pkts = 21
    payload.op_code = chdr.StrcOpCode.RESYNC
    payload.src_epid = 2
    payload.num_bytes = 168000
    return chdr.ChdrPacket(CHDR_W, header, payload)

def make_data_packet0():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.DATA_WITH_TS
    header.length = 8000
    header.dst_epid = 3
    header.seq_num = 1
    timestamp = 0x7C40C83
    data_src = rfnoc_packets_data.peer1_9
    data = bytes(data_src[(2 * 8):])
    return chdr.ChdrPacket(CHDR_W, header, data, timestamp)

def make_data_packet1():
    header = chdr.ChdrHeader()
    header.pkt_type = chdr.PacketType.DATA_WITH_TS
    header.length = 4252
    header.dst_epid = 3
    header.eob = True
    header.seq_num = 1716
    timestamp = 0x21452B97
    data_src = rfnoc_packets_data.eob_packet
    data = bytes(data_src[(2 * 8):])
    return chdr.ChdrPacket(CHDR_W, header, data, timestamp)

packets = [
    (make_control_packet0(), rfnoc_packets_ctrl_mgmt.peer0_19),
    (make_control_packet1(), rfnoc_packets_ctrl_mgmt.peer1_17),
    (make_mgmt_packet0(), rfnoc_packets_ctrl_mgmt.peer0_2),
    (make_mgmt_packet1(), rfnoc_packets_ctrl_mgmt.peer0_3),
    (make_mgmt_packet2(), rfnoc_packets_ctrl_mgmt.peer0_6),
    (make_strs_packet0(), rfnoc_packets_data.peer0_5),
    (make_strs_packet1(), rfnoc_packets_data.peer0_8),
    (make_strc_packet0(), rfnoc_packets_data.peer1_5),
    (make_strc_packet1(), rfnoc_packets_data.peer1_29),
    (make_data_packet0(), rfnoc_packets_data.peer1_9),
    (make_data_packet1(), rfnoc_packets_data.eob_packet)
]

names = [
    "Control_Packet_0",
    "Control_Packet_1",
    "Management_Packet_0",
    "Management_Packet_1",
    "Management_Packet_2",
    "Stream_Status_Packet_0",
    "Stream_Status_Packet_1",
    "Stream_Command_Packet_0",
    "Stream_Command_Packet_1",
    "Data_Packet_0",
    "Data_Packet_1"
]
