//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_add_udp.sv
//
// Description: Add a UDP header onto an incoming CHDR stream
//
// Parameters:
//
//   PREAMBLE_BYTES   : Number of bytes of preamble expected
//   MAX_PACKET_BYTES : Maximum expected packet size
//   ETH_TYPE         : EthType in Ethernet frame header
//   MISC_IP          : Version, header length, DSCP, and ECN in IPv4 header
//   FLAG_FRAG        : Flags and fragment offset in IPv4 header
//   TTL_PROT         : Time to live and protocol in IPv4 header
//   LENGTH_IN_TUSER  : Use length in i.TUSER when true. Otherwise, assume
//                      there's a CHDR header with the length.
//


module eth_ipv4_add_udp #(
  int          PREAMBLE_BYTES   = 6,
  int          MAX_PACKET_BYTES = 2**16,
  logic [15:0] ETH_TYPE         = 16'h0800,  // IPv4
  logic [15:0] MISC_IP          = { 4'd4 /* IPv4 */, 4'd5 /* HDR Len */, 8'h00 /* DSCP and ECN */},
  logic [15:0] IDENT            = 16'h0,
  logic [15:0] FLAG_FRAG        = { 3'b010 /* don't fragment */, 13'h0 },
  logic [15:0] TTL_PROT         = { 8'h10 /* TTL */, 8'h11 /* UDP */ },
  bit          LENGTH_IN_TUSER  = 0
)(
  // Clock domain: i.clk (o_.clk is unused)

  // Device addresses
  input  logic [47:0] mac_src,
  input  logic [31:0] ip_src,
  input  logic [15:0] udp_src,

  input  logic [47:0] mac_dst,
  input  logic [31:0] ip_dst,
  input  logic [15:0] udp_dst,

  // Ethernet Stream - CHDR ONLY
  AxiStreamIf.slave  i, // tUser = {Incoming packet length if LENGTH_IN_TUSER = 1}
  AxiStreamIf.master o  // tUser = {1'b0, trailing bytes};

);

  localparam ENET_USER_W = $clog2(i.DATA_WIDTH/8)+1;
  //---------------------------------------
  // Include for byte positions
  //---------------------------------------
  `include "eth_constants.vh"
  `include "../../axi4s_sv/axi4s.vh"

  //   tUser = {1'b0,trailing bytes(always full)}
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(ENET_USER_W),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
     s0(i.clk,i.rst);
  //   tUser = {1'b0,trailing bytes(after add)}
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(ENET_USER_W),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
     s1(i.clk,i.rst);
  //   tUser = {HeaderInfo,trailing bytes(after add)}
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(ENET_USER_W+96+48),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
     s2(i.clk,i.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(ENET_USER_W+96+48),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
     s3(i.clk,i.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(ENET_USER_W),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
     s4(i.clk,i.rst);

  //---------------------------------------
  // Ethernet Framer
  //---------------------------------------
  logic        sop; // Start of packet
  logic [15:0] chdr_len_new, chdr_len_old, chdr_len_reg;

  logic [15:0] ip_len, ip_len_reg;
  logic [15:0] udp_len, udp_len_reg;
  //delayed one clock cycle to pipeline
  logic [47:0] mac_dst_reg;
  logic [31:0] ip_dst_reg;
  logic [15:0] udp_dst_reg;
  logic [ENET_USER_W-1:0] trailing_bytes;

  always_comb begin : s0_assign
    `AXI4S_ASSIGN(s0,i)
    s0.tuser = 0; // all full words going in
  end

  localparam BYTES_TO_ADD = UDP_END+1+PREAMBLE_BYTES;
  axi4s_add_bytes #(.ADD_START(0),.ADD_BYTES(BYTES_TO_ADD)
  ) add_header (
   .i(s0), .o(s1)
  );

  always_ff @(posedge i.clk) begin : chdr_len_ff
    if (i.rst) begin
      sop          <= 1;
      chdr_len_old <= '0;
    end else begin
      if (i.tvalid && i.tready) begin
        chdr_len_old <= chdr_len_new;
        sop <= i.tlast;
      end
    end
  end

  always_comb begin : calc_length_fields
    if (LENGTH_IN_TUSER) begin
      if (i.tvalid && sop) begin
        chdr_len_new = i.tuser;
      end else begin
        chdr_len_new = chdr_len_old;
      end
    end else begin
      chdr_len_new  = s0.get_packet_field16(chdr_len_old,CHDR_LENGTH_BYTE);
    end
    ip_len  = (16'd28 + chdr_len_new);  // 20 for IP, 8 for UDP
    udp_len = (16'd8 + chdr_len_new);
  end

  logic [15:0] iphdr_checksum;

  always_comb begin : pack_unpack_header_info
    `AXI4S_ASSIGN(s2,s1)
    s2.tuser = { udp_len, ip_len, chdr_len_new, udp_dst, ip_dst, mac_dst, s1.tuser};
    { udp_len_reg, ip_len_reg, chdr_len_reg, udp_dst_reg, ip_dst_reg,
      mac_dst_reg,trailing_bytes} = s3.tuser;
  end

  axi4s_fifo #(
    .SIZE(1)
  ) s3_pipline_reg_ (
    .clear(1'b0),.space(),.occupied(),
    .i(s2),.o(s3)
  );

  logic chdr_end_early;

  // track when we cut off the outgoing packet
  always_ff @(posedge i.clk) begin : chdr_end_early_ff
    if (i.rst) begin
      chdr_end_early <= 0;
    end else begin
      if (s3.tvalid && s3.tready && s3.tlast)
        chdr_end_early <= 0;
      else if (s4.tvalid && s4.tready &&s4.tlast)
        chdr_end_early <= 1;
    end
  end

  localparam [15:0] udp_checksum = 16'h0;

  ip_hdr_checksum  #(
    .LATENCY(1)
  ) ip_hdr_checksum_i (
    .clk(i.clk),
    .in({MISC_IP, ip_len, IDENT, FLAG_FRAG, TTL_PROT, 16'd0, ip_src, ip_dst}),
    .clken(s2.tready && s2.tvalid),
    .out(iphdr_checksum)
  );

  // calculate what tuser should be on the final word
  logic [15:0] total_len;
  logic [15:0] rem_bytes;

  always_comb begin : calc_rem_bytes
    total_len = (BYTES_TO_ADD + chdr_len_reg);
    rem_bytes = total_len - s4.word_count*i.DATA_WIDTH/8;
  end

  // zero value fields are commented out because the fill goes to zero
  // if those fields become non zero, uncomment them
  always_comb begin : set_header_fields
    // assign bus
    s4.tvalid = s3.tvalid && !chdr_end_early;
    s3.tready = s4.tready || chdr_end_early;
    if (rem_bytes > i.DATA_WIDTH/8 || chdr_end_early) begin
      s4.tlast = s3.tvalid && s3.tlast; // sometimes packets end early
      s4.tuser = 0;
    end else if (rem_bytes == i.DATA_WIDTH/8) begin
      s4.tlast = s3.tvalid;
      s4.tuser = 0;
    end else begin
      s4.tlast = s3.tvalid;
      s4.tuser = rem_bytes;
    end
    s4.tdata = s3.tdata;
    s4.put_packet_field48(mac_dst_reg,      PREAMBLE_BYTES+DST_MAC_BYTE,     .NETWORK_ORDER(1));
    s4.put_packet_field48(mac_src,          PREAMBLE_BYTES+SRC_MAC_BYTE,     .NETWORK_ORDER(1));
    s4.put_packet_field16(ETH_TYPE,         PREAMBLE_BYTES+ETH_TYPE_BYTE,    .NETWORK_ORDER(1));
    s4.put_packet_field16(MISC_IP,          PREAMBLE_BYTES+IP_VERSION_BYTE,  .NETWORK_ORDER(1));
    s4.put_packet_field16(ip_len_reg,       PREAMBLE_BYTES+IP_LENGTH_BYTE,   .NETWORK_ORDER(1));
//  s4.put_packet_field16(IDENT,            PREAMBLE_BYTES+IP_ID_BYTE,       .NETWORK_ORDER(1));
    s4.put_packet_field16(FLAG_FRAG,        PREAMBLE_BYTES+IP_FRAG_BYTE,     .NETWORK_ORDER(1));
    s4.put_packet_field16(TTL_PROT,         PREAMBLE_BYTES+IP_TTL_BYTE,      .NETWORK_ORDER(1));
    s4.put_packet_field16(iphdr_checksum,   PREAMBLE_BYTES+IP_CHECKSUM_BYTE, .NETWORK_ORDER(1));
    s4.put_packet_field32(ip_src,           PREAMBLE_BYTES+SRC_IP_BYTE,      .NETWORK_ORDER(1));
    s4.put_packet_field32(ip_dst_reg,       PREAMBLE_BYTES+DST_IP_BYTE,      .NETWORK_ORDER(1));
    s4.put_packet_field16(udp_src,          PREAMBLE_BYTES+SRC_PORT_BYTE,    .NETWORK_ORDER(1));
    s4.put_packet_field16(udp_dst_reg,      PREAMBLE_BYTES+DST_PORT_BYTE,    .NETWORK_ORDER(1));
    s4.put_packet_field16(udp_len_reg,      PREAMBLE_BYTES+UDP_LENGTH_BYTE,  .NETWORK_ORDER(1));
//  s4.put_packet_field16(udp_checksum,     PREAMBLE_BYTES+UDP_CHECKSUM_BYTE,.NETWORK_ORDER(1));

  end

  always_comb begin : assign_output
    `AXI4S_ASSIGN(o,s4)
  end

endmodule : eth_ipv4_add_udp
