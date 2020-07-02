#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
from . import libpyuhd as lib

ChdrPacket = lib.chdr.ChdrPacket
Endianness = lib.chdr.Endianness
ChdrWidth = lib.chdr.ChdrWidth
ChdrHeader = lib.chdr.ChdrHeader
CtrlPayload = lib.chdr.CtrlPayload
CtrlStatus = lib.chdr.CtrlStatus
PacketType = lib.chdr.PacketType
CtrlOpCode = lib.chdr.CtrlOpCode
MgmtPayload = lib.chdr.MgmtPayload
MgmtHop = lib.chdr.MgmtHop
MgmtOp = lib.chdr.MgmtOp
MgmtOpCode = lib.chdr.MgmtOpCode
MgmtOpSelDest = lib.chdr.MgmtOpSelDest
MgmtOpCfg = lib.chdr.MgmtOpCfg
MgmtOpNodeInfo = lib.chdr.MgmtOpNodeInfo
StrsPayload = lib.chdr.StrsPayload
StrsStatus = lib.chdr.StrsStatus
StrcPayload = lib.chdr.StrcPayload
StrcOpCode = lib.chdr.StrcOpCode

def __get_payload(self):
    pkt_type = self.get_header().pkt_type
    if pkt_type == PacketType.MGMT:
        return self.get_payload_mgmt()
    elif pkt_type == PacketType.CTRL:
        return self.get_payload_ctrl()
    elif pkt_type == PacketType.STRC:
        return self.get_payload_strc()
    elif pkt_type == PacketType.STRS:
        return self.get_payload_strs()
    elif pkt_type in {PacketType.DATA_NO_TS, PacketType.DATA_WITH_TS}:
        raise RuntimeError("Cannot deserialize a Data Payload")
    else:
        raise RuntimeError("Invalid pkt_type in ChdrHeader")

ChdrPacket.get_payload = __get_payload

def __to_string_with_payload(self):
    pkt_type = self.get_header().pkt_type
    if pkt_type == PacketType.MGMT:
        return self.to_string_with_payload_mgmt()
    elif pkt_type == PacketType.CTRL:
        return self.to_string_with_payload_ctrl()
    elif pkt_type == PacketType.STRC:
        return self.to_string_with_payload_strc()
    elif pkt_type == PacketType.STRS:
        return self.to_string_with_payload_strs()
    elif pkt_type in {PacketType.DATA_NO_TS, PacketType.DATA_WITH_TS}:
        return self.__str__()
    else:
        raise RuntimeError("Invalid pkt_type in ChdrHeader")

ChdrPacket.to_string_with_payload = __to_string_with_payload
