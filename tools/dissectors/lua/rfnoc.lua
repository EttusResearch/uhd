-- Copyright 2022 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: GPL-3.0-or-later

-- This file defines a LUA dissector for RFNoC. All chapter references refer to
-- https://files.ettus.com/app_notes/RFNoC_Specification.pdf.

-- to install the dissector choose a global or personal LUA plugin folder
-- these folder can be found under Help -> About Wireshark -> Folders

-- RFNoC UDP port
local RFNOC_PORT = 49153

-- general purpose lookup for boolean fields
local yesno = {
  [0] = "no",
  [1] = "yes"
}

-- lookup table for CHDR with preferences
local chdr_widths = {
  { 0, "64", 64 },
  { 1, "128", 128 },
  { 2, "256", 256 },
  { 3, "512", 512 }
}

-- Packet type definitions for RFNoC (2.2.1)
local packet_types = {
  [0x0] = "Management",
  [0x1] = "Stream Status",
  [0x2] = "Stream Command",
  [0x3] = "<Reserved>",
  [0x4] = "Control Transaction",
  [0x5] = "<Reserved>",
  [0x6] = "Data Packet without a Timestamp",
  [0x7] = "Data Packet with a Timestamp"
}

-- same packet type definitions as short names
local packet_types_short = {
  [0x0] = "Mgmt",
  [0x1] = "StrS",
  [0x2] = "StrC",
  [0x3] = "<Re>",
  [0x4] = "Ctrl",
  [0x5] = "<Re>",
  [0x6] = "Data",
  [0x7] = "DaTS"
}

-- CHDR widths reported by management packet (table 15)
local mgmt_chdr_widths = {
  [0x0] = "64 bits",
  [0x1] = "128 bits",
  [0x2] = "256 bits",
  [0x3] = "512 bits",
  [0x4] = "<Reserved>",
  [0x5] = "<Reserved>",
  [0x6] = "<Reserved>",
  [0x7] = "<Reserved>"
}

-- op codes for control transaction packet (table 9)
local ctrl_op_codes = {
  [0] = "sleep",
  [1] = "write",
  [2] = "read",
  [3] = "read then write",
  [4] = "block write",
  [5] = "block read",
  [6] = "poll",
  [7] = "<reserved>",
  [8] = "<reserved>",
  [9] = "<reserved>",
  [10] = "user 1",
  [11] = "user 2",
  [12] = "user 3",
  [13] = "user 4",
  [14] = "user 5",
  [15] = "user 6"
}

-- op codes for stream command packet (table 13)
local strc_op_codes = {
  [0] = "Initialize stream",
  [1] = "Ping",
  [2] = "Resynchronize flow control",
  [3] = "<reserved>",
  [4] = "<reserved>",
  [5] = "<reserved>",
  [6] = "<reserved>",
  [7] = "<reserved>"
}

-- op codes for management packet (table 15)
local mgmt_op_codes = {
  [0] = "no-op",
  [1] = "advertise",
  [2] = "select destination",
  [3] = "return to sender",
  [4] = "node info request",
  [5] = "node info response",
  [6] = "config write",
  [7] = "config read request",
  [8] = "config read response"
}

-- node types from management node info response (no ref)
local mgmt_op_nir_types = {
  [0] = "<invalid>",
  [1] = "Crossbar",
  [2] = "Stream Endpoint",
  [3] = "Transport Adapter"
}

-- fill remaining types
for i = 4, 255 do
  mgmt_op_nir_types[i] = "<invalid>"
end

-- states of control status field (table 8)
local ctrl_states = {
  [0] = "OK",
  [1] = "CMDERR",
  [2] = "TSERR",
  [3] = "WARNING",
}

-- states of stream status' status field (table 11)
local strs_states = {
  [0] = "Okay (No Error)",
  [1] = "Command Error (Command execution failed)",
  [2] = "Sequence Error (Sequence number discontinuity)",
  [3] = "Data Error (Data integrity check failed)",
  [4] = "Routing Error (Unexpected destination)",
  [5] = "<reserved>",
  [6] = "<reserved>",
  [7] = "<reserved>"
}

-- Protocol field definitions
-- Definitions are made for all fields of 32 bit or smaller. Larger fields
-- are handled directly, because the ProtoField does not support bitmasks
-- bigger than 32 bit. Therefore CHDR header fields are split into
-- 32 bit chunks (to make use of the bit-wise display of the ProtoField
-- which eases orientation in the detail pane). Also all CHDR header
-- fields are uint32 even if they would fit into a smaller field.
-- This eases orientation in the bit-wise display of the detail pane.
-- Because most stream packets fields are bigger than 32 bit there are no
-- ProtoField definitions with bit mask for the stream command/status
-- packets.
-- The naming convention for the ProtoFields follows the specification
-- names. All fields starts with rfnoc followed by a dot. CHDR header
-- fields have the format rfnoc.<field_name>. Packet fields follow the
-- convention rfnoc.<packet_short_name>.<field_name>.
-- The display name is a human readable form of the field name without
-- references to the packet name they are associated with.
-- The protocol field LUA names follow the naming scheme
-- pf_<packet_short_name>_<field_name>. CHDR fields use chdr instead of
-- the packet short name. The field name uses underscores as word separator.

-- CHDR header fields
pf_chdr_dst_epid        = ProtoField.uint32("rfnoc.DstEPID",   "dest EPID",       base.DEC, nil,            0x0000FFFF)
pf_chdr_length          = ProtoField.uint32("rfnoc.Length",    "length",          base.DEC, nil,            0xFFFF0000)
pf_chdr_seq_num         = ProtoField.uint32("rfnoc.SeqNum",    "seq num",         base.DEC, nil,            0x0000FFFF)
pf_chdr_num_mdata       = ProtoField.uint32("rfnoc.NumMData",  "num metadata",    base.DEC, nil,            0x001F0000)
pf_chdr_pkt_type        = ProtoField.uint32("rfnoc.PktType",   "packet type",     base.DEC, packet_types,   0x00E00000)
pf_chdr_end_of_vector   = ProtoField.uint32("rfnoc.EOV",       "end of vector",   base.DEC, yesno,          0x01000000)
pf_chdr_end_of_burst    = ProtoField.uint32("rfnoc.EOB",       "end of burst",    base.DEC, yesno,          0x02000000)
pf_chdr_virtual_channel = ProtoField.uint32("rfnoc.VC",        "virtual channel", base.DEC, nil,            0xFC000000)

pf_chdr_timestamp       = ProtoField.uint64("rfnoc.Timestamp", "timestamp",       base.HEX)
pf_chdr_metadata        = ProtoField.bytes("rfnoc.Metadata",   "metadata")
pf_chdr_payload         = ProtoField.bytes("rfnoc.Payload",    "payload")

-- Management packet fields
pf_mgmt_src_epid      = ProtoField.uint32("rfnoc.mgmt.SrcEPID",   "src EPID",         base.DEC, nil,              0x0000FFFF)
pf_mgmt_num_hops      = ProtoField.uint32("rfnoc.mgmt.NumHops",   "num hops",         base.DEC, nil,              0x03FF0000)
-- reserved                                                                                                       0xFC000000
-- reserved                                                                                                       0x00001FFF
pf_mgmt_chdr_width    = ProtoField.uint32("rfnoc.mgmt.CHDRWidth", "CHDR width",       base.DEC, mgmt_chdr_widths, 0x0000E000)
pf_mgmt_proto_ver     = ProtoField.uint32("rfnoc.mgmt.ProtoVer",  "protocol version", base.DEC, nil,              0xFFFF0000)
pf_mgmt_proto_major   = ProtoField.uint32("rfnoc.mgmt.MajorVer",  "major version",    base.DEC, nil,              0xFF000000)
pf_mgmt_proto_minor   = ProtoField.uint32("rfnoc.mgmt.MinorVer",  "minor version",    base.DEC, nil,              0x00FF0000)

pf_mgmt_op_codes         = ProtoField.uint8("rfnoc.mgmt.OpCode",            "op code",       base.DEC, mgmt_op_codes)
pf_mgmt_op_sd_dest       = ProtoField.uint16("rfnoc.mgmt.sd.dest",          "select dest",   base.DEC)
pf_mgmt_op_nir_device_id = ProtoField.uint16("rfnoc.mgmt.nir.DeviceID",     "device ID",     base.DEC, nil)
pf_mgmt_op_nir_node_type = ProtoField.uint8("rfnoc.mgmt.nir.NodeType",      "node type",     base.DEC, mgmt_op_nir_types)
pf_mgmt_op_nir_node_inst = ProtoField.uint32("rfnoc.mgmt.nir.NodeInst",     "node inst",     base.DEC, nil)
pf_mgmt_op_nir_node_info = ProtoField.uint32("rfnoc.mgmt.nir.ExtendedInfo", "extended info", base.HEX, nil)
pf_mgmt_op_conf_address  = ProtoField.uint16("rfnoc.mgmt.conf.Address",     "address",       base.HEX)
pf_mgmt_op_conf_data     = ProtoField.uint32("rfnoc.mgmt.conf.Data",        "data",          base.HEX)

-- Control packet fields
pf_ctrl_dst_port      = ProtoField.uint32("rfnoc.ctrl.DstPort",      "dst port",     base.DEC, nil,           0x000003FF)
pf_ctrl_src_port      = ProtoField.uint32("rfnoc.ctrl.SrcPort",      "src port",     base.DEC, nil,           0x000FFC00)
pf_ctrl_num_data      = ProtoField.uint32("rfnoc.ctrl.NumData",      "num data",     base.DEC, nil,           0x00F00000)
pf_ctrl_ctrl_seq_num  = ProtoField.uint32("rfnoc.ctrl.SeqNum",       "ctrl seq num", base.DEC, nil,           0x3F000000)
pf_ctrl_has_timestamp = ProtoField.uint32("rfnoc.ctrl.HasTimestamp", "has time",     base.DEC, yesno,         0x40000000)
pf_ctrl_is_ack        = ProtoField.uint32("rfnoc.ctrl.IsACK",        "is ACK",       base.DEC, yesno,         0x80000000)
pf_ctrl_src_epid      = ProtoField.uint32("rfnoc.ctrl.SrcEPID",      "src EPID",     base.DEC, nil,           0x0000FFFF)
-- reserved                                                                                                   0xFFFF0000
pf_ctrl_timestamp     = ProtoField.uint64("rfnoc.ctrl.Timestamp",    "timestamp",    base.HEX)
pf_ctrl_address       = ProtoField.uint32("rfnoc.ctrl.Address",      "address",      base.HEX, nil,           0x000FFFFF)
pf_ctrl_byte_enable   = ProtoField.uint32("rfnoc.ctrl.ByteEnable",   "byte enable",  base.HEX, nil,           0x00F00000)
pf_ctrl_op_code       = ProtoField.uint32("rfnoc.ctrl.OpCode",       "op code",      base.DEC, ctrl_op_codes, 0x0F000000)
-- reserved                                                                                                   0x30000000
pf_ctrl_status        = ProtoField.uint32("rfnoc.ctrl.Status",       "status",       base.DEC, ctrl_states,   0xC0000000)

pf_ctrl_data          = ProtoField.bytes("rfnoc.ctrl.Data",          "data")

-- Stream Status fields
pf_strs_src_epid           = ProtoField.uint16("rfnoc.strs.SrcEPID",           "src EPID")
pf_strs_status             = ProtoField.uint8("rfnoc.strs.Status",             "status", base.DEC, strs_states)
pf_strs_capacity_bytes     = ProtoField.uint64("rfnoc.strs.CapacityBytes",     "capacity bytes")
pf_strs_capacity_packets   = ProtoField.uint32("rfnoc.strs.CapacityPackets",   "capacity packets")
pf_strs_xfer_count_packets = ProtoField.uint64("rfnoc.strs.XfcerCountPackets", "transferred packets")
pf_strs_xfer_count_bytes   = ProtoField.uint64("rfnoc.strs.XferCountBytes",    "transferred bytes")
pf_strs_buff_info          = ProtoField.uint16("rfnoc.strs.BuffInfo",          "buffer info")
pf_strs_status_info        = ProtoField.uint64("rfnoc.strs.StatusInfo",        "status info")

-- Stream Command fields
pf_strc_src_epid    = ProtoField.uint16("rfnoc.strc.SrcEPID",  "src EPID")
pf_strc_op_code     = ProtoField.uint8("rfnoc.strc.OpCode",    "op code", base.DEC, strc_op_codes)
pf_strc_op_data     = ProtoField.uint8("rfnoc.strc.OpData",    "op data")
pf_strc_num_packets = ProtoField.uint64("rfnoc.strc.NumPkts",  "num packets")
pf_strc_num_bytes   = ProtoField.uint64("rfnoc.strc.NumBytes", "num bytes")


-- RFNoC protocol dissector
rfnoc_proto = Proto("RFNoC",  "RFNoC Protocol")
rfnoc_proto.fields = {
  pf_chdr_dst_epid,
  pf_chdr_length,
  pf_chdr_seq_num,
  pf_chdr_num_mdata,
  pf_chdr_pkt_type,
  pf_chdr_end_of_vector,
  pf_chdr_end_of_burst,
  pf_chdr_virtual_channel,

  pf_chdr_timestamp,
  pf_chdr_metadata,
  pf_chdr_payload,

  pf_mgmt_src_epid,
  pf_mgmt_num_hops,
  pf_mgmt_chdr_width,
  pf_mgmt_proto_ver,
  pf_mgmt_proto_major,
  pf_mgmt_proto_minor,
  pf_mgmt_op_codes,
  pf_mgmt_op_sd_dest,
  pf_mgmt_op_nir_device_id,
  pf_mgmt_op_nir_node_type,
  pf_mgmt_op_nir_node_inst,
  pf_mgmt_op_nir_node_info,
  pf_mgmt_op_conf_address,
  pf_mgmt_op_conf_data,

  pf_ctrl_dst_port,
  pf_ctrl_src_port,
  pf_ctrl_num_data,
  pf_ctrl_ctrl_seq_num,
  pf_ctrl_has_timestamp,
  pf_ctrl_is_ack,
  pf_ctrl_src_epid,
  pf_ctrl_timestamp,
  pf_ctrl_address,
  pf_ctrl_byte_enable,
  pf_ctrl_op_code,
  pf_ctrl_status,

  pf_ctrl_data,

  pf_strs_src_epid,
  pf_strs_status,
  pf_strs_capacity_bytes,
  pf_strs_capacity_packets,
  pf_strs_xfer_count_packets,
  pf_strs_xfer_count_bytes,
  pf_strs_buff_info,
  pf_strs_status_info,

  pf_strc_src_epid,
  pf_strc_op_code,
  pf_strc_op_data,
  pf_strc_num_packets,
  pf_strc_num_bytes,
}

-- define preferences
rfnoc_proto.prefs.chdr_width = Pref.enum(
    "CHDR width",                   -- label
    64,                             -- default
    "CHDR width used for decoding", -- description
    chdr_widths,                    -- lookup
    true                            -- use radio buttons in config
)

-- main entry point for dissector
-- * decode the CHDR header
-- * dissect metadata
--   - metadata dissector return new dissector offset
-- * lookup decoder for packet type to decode packet-specific fields
--   - each packet decoder returns a tuple of src EPID (might be nil)
--     and additional info that will be placed along with the info
--     column in the main view
--   - the CHDR header is passed to the packet decoder, because
--     some decoders need references to CHDR fields.
function rfnoc_proto.dissector(buffer, pinfo, tree)
  buf_length = buffer:len()
  if buf_length == 0 then return end

  pinfo.cols.protocol = rfnoc_proto.name
  pinfo.cols.info:clear()

  local subtree = tree:add(rfnoc_proto, buffer(), "RFNoC")
  local chdr = subtree:add(rfnoc_proto, buffer(0, 8), "CHDR")

  chdr:add_le(pf_chdr_dst_epid, buffer(0, 4))
  chdr:add_le(pf_chdr_length, buffer(0, 4))
  chdr:add_le(pf_chdr_seq_num, buffer(4, 4))
  chdr:add_le(pf_chdr_num_mdata, buffer(4, 4))
  chdr:add_le(pf_chdr_pkt_type, buffer(4, 4))
  chdr:add_le(pf_chdr_end_of_vector, buffer(4, 4))
  chdr:add_le(pf_chdr_end_of_burst, buffer(4, 4))
  chdr:add_le(pf_chdr_virtual_channel, buffer(4, 4))

  local packet_decoders = {
    [0] = dissect_mgmt_packet,
    [1] = dissect_stream_status_packet,
    [2] = dissect_stream_command_packet,
    [3] = dissect_reserved_packet,
    [4] = dissect_ctrl_packet,
    [5] = dissect_reserved_packet,
    [6] = dissect_data_packet_without_ts,
    [7] = dissect_data_packet_with_ts
  }

  local packet_len = buffer(2, 2):le_uint()
  local packet_type = buffer(6, 1):bitfield(0, 3)
  local offset = dissect_metadata(buffer, packet_type, pinfo, subtree)
  local src, info = packet_decoders[buffer(6, 1):bitfield(0, 3)](
      buffer(0, offset),
      buffer(offset, packet_len - offset),
      pinfo, subtree)

  local src_str = src == nil and "   " or string.format("%3d", src)
  pinfo.cols.info:append(string.format("[%s] %s -> %3d %s",
      packet_types_short[packet_type], -- packet type
      src_str,                         -- src EPID
      buffer(0, 2):le_uint(),          -- dst EPID
      info                             -- packet specific info
  ))
end

-- see 2.2.1 for details about metadata memory layout depending
-- on CHDR width.
function dissect_metadata(buffer, packet_type, pinfo, subtree)
  local mdata_len = buffer(6,1):bitfield(3, 5)
  local chdr_width_in_bytes = rfnoc_proto.prefs.chdr_width / 8
  local packet_offset = 0
  if rfnoc_proto.prefs.chdr_width == 64 and packet_type == 7 then
    packet_offset = chdr_width_in_bytes * (2 + mdata_len);
  else
    packet_offset = chdr_width_in_bytes * (1 + mdata_len);
  end
  if mdata_len > 0 then
    subtree:add(pf_chdr_metadata, buffer(packet_offset, mdata_len * chdr_width_in_bytes))
  end
  return packet_offset + mdata_len * chdr_width_in_bytes
end

function dissect_mgmt_op(buffer, hop_node, op_index, op_code, op_payload)
  local op_node = hop_node:add(rfnoc_proto, buffer(), "Operation " .. op_index)
  local op_code = buffer(1, 1):le_uint()
  local ext_info = bit.rshift(buffer(5, 3):le_uint(), 6)
  local op_info =  mgmt_op_codes[buffer(1, 1):le_uint()]

  op_node:add_le(pf_mgmt_op_codes, buffer(1, 1))
  if op_code == 2 then     -- select destination
    op_node:add(pf_mgmt_op_sd_dest, bit.band(buffer(2, 2):le_uint(), 0x3FF))
    op_info = op_info .. " " .. bit.band(buffer(2, 2):le_uint(), 0x3FF)
  elseif op_code == 5 then -- node response info
    local node_type = bit.band(buffer(4, 1):le_uint(), 0xF)
    local node_inst = bit.rshift(bit.band(buffer(4, 2):le_uint(), 0x3FF0), 4)
    op_node:add_le(pf_mgmt_op_nir_device_id, buffer(2, 2))
    op_node:add_le(pf_mgmt_op_nir_node_type, node_type)
    op_node:add_le(pf_mgmt_op_nir_node_inst, node_inst)
    local node_ext_info = op_node:add_le(pf_mgmt_op_nir_node_info, ext_info)
    if node_type == 1 then -- crossbar
      node_ext_info:set_text(string.format(
          "NPORTS=%d, NPORTS_MGMT=%d, EXT_RTCFG_PORT=%d (0x%05X)",
          bit.band(ext_info, 0xFF),
          bit.band(bit.rshift(ext_info, 8), 0xFF),
          bit.band(bit.rshift(ext_info, 16), 1),
          ext_info
      ))
    elseif node_type == 2 then -- streaming endpoint
      node_ext_info:set_text(string.format(
          "AXIS_CTRL_EN=%d, AXIS_DATA_EN=%d, NUM_DATA_I=%d NUM_DATA_O=%d, REPORT_STREAM_ERRS=%d (0x%05X)",
          bit.band(ext_info, 1),
          bit.band(bit.rshift(ext_info, 1), 1),
          bit.band(bit.rshift(ext_info, 2), 0x3F),
          bit.band(bit.rshift(ext_info, 8), 0x3F),
          bit.band(bit.rshift(ext_info, 14), 1),
          ext_info
      ))
    elseif node_type == 3 then -- transport adapter
      node_ext_info:set_text(string.format(
          "NODE_SUBTYPE=%d (0x%05X)",
          bit.band(ext_info, 0xFF),
          ext_info
      ))
    else
      node_ext_info:append_text(" <invalid>")
    end
  elseif op_code == 6 then -- config write
    op_node:add_le(pf_mgmt_op_conf_address, buffer(2, 2))
    op_node:add_le(pf_mgmt_op_conf_data, buffer(4, 4))
    op_info = string.format("%s %x->%4.4x", op_info, buffer(4, 4):le_uint(), buffer(2, 2):le_uint())
  elseif op_code == 7 then -- config read request
    op_node:add_le(pf_mgmt_op_conf_address, buffer(2, 2))
  elseif op_code == 8 then -- config read response
    op_node:add_le(pf_mgmt_op_conf_address, buffer(2, 2))
    op_node:add_le(pf_mgmt_op_conf_data, buffer(4, 4))
  end
  return op_info
end

function dissect_mgmt_packet(header_buffer, buffer, pinfo, tree)
  local header = tree:add(rfnoc_proto, buffer(), "Management")
  header:add_le(pf_mgmt_src_epid, buffer(0, 4))
  header:add_le(pf_mgmt_num_hops, buffer(0, 4))
  header:add_le(pf_mgmt_chdr_width, buffer(4, 4))
  header:add_le(pf_mgmt_proto_ver, buffer(4, 4))
  header:add_le(pf_mgmt_proto_major, buffer(4, 4))
  header:add_le(pf_mgmt_proto_minor, buffer(4, 4))

  local num_hops = bit.band(buffer(2, 4):le_uint(), 0x3FF)
  local offset = 8
  local hop_info = ""
  for hop = 0, num_hops - 1 do
    local ops_pending = buffer(offset, 1):le_uint()
    local hop_node = header:add(rfnoc_proto, buffer(offset, 8 * (ops_pending + 1)), "Hop " .. hop)
    for op_nr = 0, ops_pending do
      hop_info = hop_info .. dissect_mgmt_op(buffer(offset, 8), hop_node, op_nr, buffer(offset + 1, 1), buffer(offset + 2, 6))
      offset = offset + 8
      if ops_pending > 0 then
        ops_pending = buffer(offset, 1):le_uint()
        hop_info = hop_info .. ", "
      end
    end
    if hop < num_hops - 1 then
      hop_info = hop_info .. " | "
    end
  end
  return buffer(0, 2):le_uint(), hop_info
end

function dissect_stream_status_packet(header_buffer, buffer, pinfo, tree)
  local header = tree:add(rfnoc_proto, buffer(), "Stream Status")
  header:add_le(pf_strs_src_epid, buffer(0, 2))
  header:add_le(pf_strs_status, buffer(2, 1):bitfield(0, 4))
  header:add_le(pf_strs_capacity_bytes, buffer(3, 5))
  header:add_le(pf_strs_capacity_packets, buffer(8, 3))
  header:add_le(pf_strs_xfer_count_packets, buffer(11, 5))
  header:add_le(pf_strs_xfer_count_bytes, buffer(16, 8))
  header:add_le(pf_strs_buff_info, buffer(24, 2))
  header:add_le(pf_strs_status_info, buffer(26, 6))
  return buffer(0, 2):le_uint(), string.format("%s (packets: %s, bytes: %s)",
      strs_states[buffer(2, 1):bitfield(0, 4)],                           -- stream status
      tostring(UInt64(buffer(11, 4):le_uint(), buffer(15, 1):le_uint())), -- transferred packets
      tostring(UInt64(buffer(16, 4):le_uint(), buffer(20, 4):le_uint()))  -- transferred bytes
  )
end

function dissect_stream_command_packet(header_buffer, buffer, pinfo, tree)
  local header = tree:add(rfnoc_proto, buffer(), "Stream Command")
  header:add_le(pf_strc_src_epid, buffer(0, 2))
  header:add_le(pf_strc_op_code, buffer(2, 1):bitfield(4, 4))
  header:add_le(pf_strc_op_data, buffer(2, 1):bitfield(0, 4))
  header:add_le(pf_strc_num_packets, buffer(3, 5))
  header:add_le(pf_strc_num_bytes, buffer(8, 8))
  return buffer(0, 2):le_uint(), string.format("%s (packets: %s, bytes: %s)",
      strc_op_codes[buffer(2, 1):bitfield(4, 4)],                       -- op code
      tostring(UInt64(buffer(3, 4):le_uint(), buffer(7, 1):le_uint())), -- packets
      tostring(UInt64(buffer(8, 4):le_uint(), buffer(12, 4):le_uint())) -- bytes
  )
end

function dissect_reserved_packet(header_buffer, buffer, pinfo, tree)
  return nil, ""
end

function dissect_ctrl_packet(header_buffer, buffer, pinfo, tree)
  local header = tree:add(rfnoc_proto, buffer(), "Control")
  header:add_le(pf_ctrl_dst_port, buffer(0,4))
  header:add_le(pf_ctrl_src_port, buffer(0,4))
  header:add_le(pf_ctrl_num_data, buffer(0,4))
  header:add_le(pf_ctrl_ctrl_seq_num, buffer(0,4))
  header:add_le(pf_ctrl_has_timestamp, buffer(0,4))
  header:add_le(pf_ctrl_is_ack, buffer(0,4))
  header:add_le(pf_ctrl_src_epid, buffer(4,4))
  local offset = 8
  local has_ts = buffer(3,1):bitfield(1)
  local ack_info = ""
  local time_info = ""
  if buffer(3,1):bitfield(0) == 1 then
    ack_info = "ACK "
  end
  if has_ts == 1 then
    header:add_le(pf_ctrl_timestamp, buffer(offset, 8))
    time_info = " @ 0x" .. tostring(buffer(offset, 8):le_uint64():tohex())
    offset = offset + 8
  end
  header:add_le(pf_ctrl_address, buffer(offset, 4))
  header:add_le(pf_ctrl_byte_enable, buffer(offset, 4))
  header:add_le(pf_ctrl_op_code, buffer(offset, 4))
  header:add_le(pf_ctrl_status, buffer(offset, 4))
  header:add(pf_ctrl_data, buffer(offset + 4))
  return buffer(4, 2):le_uint(), string.format("%s%s %s -> %s%s",
      ack_info,                                                  -- add ACK when is_ack is 1
      ctrl_op_codes[buffer(offset + 3):bitfield(4,4)],           -- op code
      buffer(offset + 4):bytes():tohex(),                        -- data
      bit.tohex(bit.band(buffer(offset, 4):le_uint(), 0xFFFFF)), -- address
      time_info                                                  -- timestamp if has_ts is 1
  )
end

function dissect_data_packet_without_ts(header_buffer, buffer, pinfo, tree)
  tree:add(pf_chdr_payload, buffer)
  local burst_info = ""

  if header_buffer(7, 1):bitfield(6) == 1 then
    burst_info = burst_info .. "EOB "
  end
  return nil, burst_info
end

function dissect_data_packet_with_ts(header_buffer, buffer, pinfo, tree)
  tree:add_le(pf_chdr_timestamp, header_buffer(8, 8))
  dstEpid, info = dissect_data_packet_without_ts(header_buffer, buffer, pinfo, tree)
  info = info  .. string.format("time: 0x%.8x%.8x",
      header_buffer(12, 4):le_uint(),
      header_buffer(8, 4):le_uint()
  )
  return nil, info
end

-- register RFNoC dissector
local udp_port = DissectorTable.get("udp.port")
udp_port:add(RFNOC_PORT, rfnoc_proto)
