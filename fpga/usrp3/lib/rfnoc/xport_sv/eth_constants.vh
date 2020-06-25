//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_constants (Header File)
//
// Description:
//   Constants for ethernet

//---------------------------------------
// Ethernet constants
//---------------------------------------
localparam [47:0] ETH_ADDR_BCAST = {48{1'b1}};
localparam [15:0] ETH_TYPE_IPV4  = 16'h0800;
localparam [7:0]  IPV4_PROTO_UDP = 8'h11;
localparam [7:0]  IPV4_LEN5      = 8'h45;

// tUser conventions
localparam BYTES_MSB = ENET_USER_W-2;
localparam ERROR_BIT = ENET_USER_W-1;
localparam SOF_BIT   = ENET_USER_W-1;

//---------------------------------------
// Ethernet byte positions
//---------------------------------------
// Bytes 7-0-------------------------
// | DstMAC_HI (16) | Preamble (48) |
// ----------------------------------
localparam PREAMBLE_BYTE = 0;
localparam PREAMBLE_END = 5;
localparam ETH_HDR_BYTE = 0;
localparam DST_MAC_BYTE = ETH_HDR_BYTE+0;
// Bytes 15-8-------------------------
// | SrcMAC_HI (32) | DstMAC_LO (32) |
// -----------------------------------
localparam SRC_MAC_BYTE = ETH_HDR_BYTE+6;
// Bytes 23-16---------------------------------------
// | IPv4_Line0 (32)| EthType (16) | SrcMAC_LO (16) |
// --------------------------------------------------
localparam ETH_TYPE_BYTE = ETH_HDR_BYTE+12;
localparam ETH_PAYLOAD_BYTE = ETH_HDR_BYTE+14;
// Bytes 31-24--------------------------
// | IPv4_Line2 (32) | IPv4_Line1 (32) |
// -------------------------------------
localparam IPV4_HDR_BYTE = ETH_PAYLOAD_BYTE;
localparam IP_VERSION_BYTE  = IPV4_HDR_BYTE+0;
localparam IP_DSCP_BYTE     = IPV4_HDR_BYTE+1;
localparam IP_LENGTH_BYTE   = IPV4_HDR_BYTE+2;
localparam IP_ID_BYTE       = IPV4_HDR_BYTE+4;
localparam IP_FRAG_BYTE     = IPV4_HDR_BYTE+6;
localparam IP_TTL_BYTE      = IPV4_HDR_BYTE+8;
localparam PROTOCOL_BYTE    = IPV4_HDR_BYTE+9;
localparam IP_CHECKSUM_BYTE = IPV4_HDR_BYTE+10;
// Bytes 39-32------------------------
// | IPDstAddr (32) | IPSrcAddr (32) |
// -----------------------------------
localparam SRC_IP_BYTE = IPV4_HDR_BYTE+12;
localparam DST_IP_BYTE = IPV4_HDR_BYTE+16;
localparam IPV4_PAYLOAD_BYTE = IPV4_HDR_BYTE+20;
// Bytes 48-40------------------------------------------------
// | Chksum (16) | Length (16) | DstPort (16) | SrcPort (16) |
// -----------------------------------------------------------
localparam UDP_HDR_BYTE = IPV4_PAYLOAD_BYTE;
localparam SRC_PORT_BYTE = UDP_HDR_BYTE+0;
localparam DST_PORT_BYTE = UDP_HDR_BYTE+2;
localparam UDP_LENGTH_BYTE = UDP_HDR_BYTE+4;
localparam UDP_CHECKSUM_BYTE = UDP_HDR_BYTE+6;
localparam UDP_END = UDP_CHECKSUM_BYTE+1; // last byte in the UDP header

localparam MIN_PACKET_SIZE_BYTE = 63;
//---------------------------------------
// CHDR_BYTE_POSITION
//---------------------------------------
localparam CHDR_HDR_BYTE = 0;
localparam CHDR_LENGTH_BYTE = CHDR_HDR_BYTE+2;
