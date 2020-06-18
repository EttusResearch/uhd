//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_chdr64_adapter
// Description: A generic transport adapter module that can be used in
//   a veriety of transports. It does the following:
//   - Exposes a configuration port for mgmt packets to configure the node
//   - Implements a return-address map for packets with metadata other than
//     the CHDR. Additional metadata can be passed as a tuser to this module
//     which will store it in a map indexed by the SrcEPID in a management
//     packet. For all returning packets, the metadata will be looked up in
//     the map and attached as the outgoing tuser.
//   - Implements a loopback path for node-info discovery
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - MTU: Log2 of the MTU of the packet in 64-bit words
//   - CPU_FIFO_SIZE: Log2 of the FIFO depth (in 64-bit words) for the CPU egress path
//   - RT_TBL_SIZE: Log2 of the depth of the return-address routing table
//   - NODE_INST: The node type to return for a node-info discovery
//   - DROP_UNKNOWN_MAC: Drop packets not addressed to us?
//
// Signals:
//   - device_id : The ID of the device that has instantiated this module
//   - s_mac_*: The input Ethernet stream from the MAC (plus tuser for trailing bytes + err)
//   - m_mac_*: The output Ethernet stream to the MAC (plus tuser for trailing bytes + err)
//   - s_chdr_*: The input CHDR stream from the rfnoc infrastructure
//   - m_chdr_*: The output CHDR stream to the rfnoc infrastructure
//   - s_cpu_*: The input Ethernet stream from the CPU (plus tuser for trailing bytes + err)
//   - m_cpu_*: The output Ethernet stream to the CPU (plus tuser for trailing bytes + err)
//   - my_eth_addr: The Ethernet (MAC) address of this endpoint
//   - my_ipv4_addr: The IPv4 address of this endpoint
//   - my_udp_chdr_port: The UDP port allocated for CHDR traffic on this endpoint
//

`default_nettype none
module eth_ipv4_chdr64_adapter #(
  parameter [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter        MTU              = 10,
  parameter        CPU_FIFO_SIZE    = MTU,
  parameter        RT_TBL_SIZE      = 6,
  parameter        NODE_INST        = 0,
  parameter [0:0]  DROP_UNKNOWN_MAC = 1,
  parameter [0:0]  IS_CPU_ARM       = 0
)(
  // Clocking and reset interface
  input  wire        clk,
  input  wire        rst,
  // Device info
  input  wire [15:0] device_id,
  // AXI-Stream interface to/from MAC
  input  wire [63:0] s_mac_tdata,
  input  wire [3:0]  s_mac_tuser,
  input  wire        s_mac_tlast,
  input  wire        s_mac_tvalid,
  output wire        s_mac_tready,
  output wire [63:0] m_mac_tdata,
  output wire [3:0]  m_mac_tuser,
  output wire        m_mac_tlast,
  output wire        m_mac_tvalid,
  input  wire        m_mac_tready,
  // AXI-Stream interface to/from CHDR infrastructure
  input  wire [63:0] s_chdr_tdata,
  input  wire        s_chdr_tlast,
  input  wire        s_chdr_tvalid,
  output wire        s_chdr_tready,
  output wire [63:0] m_chdr_tdata,
  output wire        m_chdr_tlast,
  output wire        m_chdr_tvalid,
  input  wire        m_chdr_tready,
  // AXI-Stream interface to/from CPU
  input  wire [63:0] s_cpu_tdata,
  input  wire [3:0]  s_cpu_tuser,
  input  wire        s_cpu_tlast,
  input  wire        s_cpu_tvalid,
  output wire        s_cpu_tready,
  output wire [63:0] m_cpu_tdata,
  output wire [3:0]  m_cpu_tuser,
  output wire        m_cpu_tlast,
  output wire        m_cpu_tvalid,
  input  wire        m_cpu_tready,
  // Device addresses
  input  wire [47:0] my_eth_addr,
  input  wire [31:0] my_ipv4_addr,
  input  wire [15:0] my_udp_chdr_port
);

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"
  `include "rfnoc_xport_types.vh"

  //-----------------------------------------------------------------------
  // Byte-swapping function
  // Ethernet fields we wrote out left-to-right, but AXI-Stream time-orders
  // its data right-to-left.
  //-----------------------------------------------------------------------
  function [63:0] bswap64(
    input  [63:0] din
  );
    begin
      bswap64 = {din[0 +: 8], din[8 +: 8], din[16 +: 8], din[24 +: 8],
                 din[32+: 8], din[40+: 8], din[48 +: 8], din[56 +: 8]};
    end
  endfunction

  //---------------------------------------
  // E2X and E2C DEMUX
  //---------------------------------------
  wire [63:0] e2x_chdr_tdata;
  wire [95:0] e2x_chdr_tuser;
  wire        e2x_chdr_tlast, e2x_chdr_tvalid, e2x_chdr_tready;
  wire [63:0] e2c_chdr_tdata;
  wire [3:0]  e2c_chdr_tuser;
  wire        e2c_chdr_tlast, e2c_chdr_tvalid, e2c_chdr_tready;

  // Ethernet sink. Inspects packet and dispatches
  // to the correct port.
  eth_ipv4_chdr64_dispatch #(
    .DROP_UNKNOWN_MAC(DROP_UNKNOWN_MAC)
  ) eth_dispatch_i (
    .clk              (clk),
    .rst              (rst),
    .s_mac_tdata      (s_mac_tdata),
    .s_mac_tuser      (s_mac_tuser),
    .s_mac_tlast      (s_mac_tlast),
    .s_mac_tvalid     (s_mac_tvalid),
    .s_mac_tready     (s_mac_tready),
    .m_chdr_tdata     (e2x_chdr_tdata),
    .m_chdr_tuser     (e2x_chdr_tuser),
    .m_chdr_tlast     (e2x_chdr_tlast),
    .m_chdr_tvalid    (e2x_chdr_tvalid),
    .m_chdr_tready    (e2x_chdr_tready),
    .m_cpu_tdata      (e2c_chdr_tdata),
    .m_cpu_tuser      (e2c_chdr_tuser),
    .m_cpu_tlast      (e2c_chdr_tlast),
    .m_cpu_tvalid     (e2c_chdr_tvalid),
    .m_cpu_tready     (e2c_chdr_tready),
    .my_eth_addr      (my_eth_addr),
    .my_ipv4_addr     (my_ipv4_addr),
    .my_udp_chdr_port (my_udp_chdr_port)
  );

  //---------------------------------------
  // CHDR Transport Adapter
  //---------------------------------------

  wire [63:0] x2e_chdr_tdata;
  wire [95:0] x2e_chdr_tuser;
  wire        x2e_chdr_tlast, x2e_chdr_tvalid, x2e_chdr_tready;
  wire [63:0] e2x_fifo_tdata;
  wire        e2x_fifo_tlast, e2x_fifo_tvalid, e2x_fifo_tready;
  wire [63:0] e2c_fifo_tdata;
  wire [3:0]  e2c_fifo_tuser;
  wire        e2c_fifo_tlast, e2c_fifo_tvalid, e2c_fifo_tready;

  chdr_xport_adapter_generic #(
    .PROTOVER     (PROTOVER),
    .CHDR_W       (64),
    .USER_W       (96),
    .TBL_SIZE     (RT_TBL_SIZE),
    .NODE_SUBTYPE (NODE_SUBTYPE_XPORT_IPV4_CHDR64),
    .NODE_INST    (NODE_INST),
    .ALLOW_DISC   (1)
  ) xport_adapter_gen_i (
    .clk                 (clk),
    .rst                 (rst),
    .device_id           (device_id),
    .s_axis_xport_tdata  (e2x_chdr_tdata),
    .s_axis_xport_tuser  (e2x_chdr_tuser),
    .s_axis_xport_tlast  (e2x_chdr_tlast),
    .s_axis_xport_tvalid (e2x_chdr_tvalid),
    .s_axis_xport_tready (e2x_chdr_tready),
    .m_axis_xport_tdata  (x2e_chdr_tdata),
    .m_axis_xport_tuser  (x2e_chdr_tuser),
    .m_axis_xport_tlast  (x2e_chdr_tlast),
    .m_axis_xport_tvalid (x2e_chdr_tvalid),
    .m_axis_xport_tready (x2e_chdr_tready),
    .s_axis_rfnoc_tdata  (s_chdr_tdata),
    .s_axis_rfnoc_tlast  (s_chdr_tlast),
    .s_axis_rfnoc_tvalid (s_chdr_tvalid),
    .s_axis_rfnoc_tready (s_chdr_tready),
    .m_axis_rfnoc_tdata  (e2x_fifo_tdata),
    .m_axis_rfnoc_tlast  (e2x_fifo_tlast),
    .m_axis_rfnoc_tvalid (e2x_fifo_tvalid),
    .m_axis_rfnoc_tready (e2x_fifo_tready),
    .ctrlport_req_wr     (/* unused */),
    .ctrlport_req_rd     (/* unused */),
    .ctrlport_req_addr   (/* unused */),
    .ctrlport_req_data   (/* unused */),
    .ctrlport_resp_ack   (/* unused */),
    .ctrlport_resp_data  (/* unused */)
  );

  generate
    if (IS_CPU_ARM == 1'b1) begin
      //---------------------------------------
      // Ethernet framer for ARM
      //---------------------------------------

      // Strip the 6 octet ethernet padding we used internally
      // before sending to ARM.
      // Put SOF into bit[3] of tuser.
      axi64_to_xge64 arm_framer (
        .clk(clk),
        .reset(rst),
        .clear(1'b0),
        .s_axis_tdata(e2c_chdr_tdata),
        .s_axis_tuser(e2c_chdr_tuser),
        .s_axis_tlast(e2c_chdr_tlast),
        .s_axis_tvalid(e2c_chdr_tvalid),
        .s_axis_tready(e2c_chdr_tready),
        .m_axis_tdata(e2c_fifo_tdata),
        .m_axis_tuser(e2c_fifo_tuser),
        .m_axis_tlast(e2c_fifo_tlast),
        .m_axis_tvalid(e2c_fifo_tvalid),
        .m_axis_tready(e2c_fifo_tready)
      );
    end else begin
      assign e2c_fifo_tdata  = e2c_chdr_tdata;
      assign e2c_fifo_tuser  = e2c_chdr_tuser;
      assign e2c_fifo_tlast  = e2c_chdr_tlast;
      assign e2c_fifo_tvalid = e2c_chdr_tvalid;
      assign e2c_chdr_tready = e2c_fifo_tready;
    end
  endgenerate

  //---------------------------------------
  // E2X and E2C Output Buffering
  //---------------------------------------

  // The CPU can be slow to respond (relative to packet wirespeed) so
  // extra buffer for packets destined there so it doesn't back up.
  axi_fifo #(
    .WIDTH(64+4+1),.SIZE(CPU_FIFO_SIZE)
  ) cpu_fifo_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({e2c_fifo_tlast, e2c_fifo_tuser, e2c_fifo_tdata}),
    .i_tvalid(e2c_fifo_tvalid), .i_tready(e2c_fifo_tready),
    .o_tdata({m_cpu_tlast, m_cpu_tuser, m_cpu_tdata}),
    .o_tvalid(m_cpu_tvalid), .o_tready(m_cpu_tready),
    .occupied(), .space()
  );

  // The transport should hook up to a crossbar downstream, which
  // may backpressure this module because it is in the middle of
  // transferring a packet. To ensure that upstream logic is not
  // blocked, we instantiate one packet worth of buffering here.
  axi_fifo #(
    .WIDTH(64+1),.SIZE(MTU)
  ) chdr_fifo_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({e2x_fifo_tlast, e2x_fifo_tdata}),
    .i_tvalid(e2x_fifo_tvalid), .i_tready(e2x_fifo_tready),
    .o_tdata({m_chdr_tlast, m_chdr_tdata}),
    .o_tvalid(m_chdr_tvalid), .o_tready(m_chdr_tready),
    .occupied(), .space()
  );

  //---------------------------------------
  // Ethernet Framer for X2E
  //---------------------------------------
  wire [63:0] x2e_framed_tdata;
  wire [3:0]  x2e_framed_tuser;
  wire        x2e_framed_tlast, x2e_framed_tvalid, x2e_framed_tready;

  localparam [2:0] ST_IDLE           = 3'd0;
  localparam [2:0] ST_ETH_L0         = 3'd1;
  localparam [2:0] ST_ETH_L1         = 3'd2;
  localparam [2:0] ST_ETH_L2_IPV4_L0 = 3'd3;
  localparam [2:0] ST_IPV4_L1        = 3'd4;
  localparam [2:0] ST_IPV4_L2        = 3'd5;
  localparam [2:0] ST_IPV4_UDP_HDR   = 3'd6;
  localparam [2:0] ST_CHDR_PAYLOAD   = 3'd7;

  reg [2:0]  frame_state = ST_IDLE;
  reg [15:0] chdr_len = 16'd0;
  reg [63:0] frame_tdata;

  always @(posedge clk) begin
    if(rst) begin
      frame_state <= ST_IDLE;
      chdr_len <= 16'd0;
    end else begin
      case(frame_state)
        ST_IDLE: begin
          if (x2e_chdr_tvalid) begin
            frame_state <= ST_ETH_L0;
            chdr_len <= chdr_get_length(x2e_chdr_tdata);
          end
        end
        ST_CHDR_PAYLOAD: begin
          if (x2e_chdr_tvalid & x2e_framed_tready)
            if (x2e_chdr_tlast)
              frame_state <= ST_IDLE;
        end
        default: begin
          if(x2e_framed_tready)
            frame_state <= frame_state + 3'd1;
        end
      endcase
    end
  end

  assign x2e_chdr_tready = (frame_state == ST_CHDR_PAYLOAD) ? x2e_framed_tready : 1'b0;
  assign x2e_framed_tvalid = (frame_state == ST_CHDR_PAYLOAD) ? x2e_chdr_tvalid : (frame_state == ST_IDLE) ? 1'b0 : 1'b1;
  assign x2e_framed_tlast = (frame_state == ST_CHDR_PAYLOAD) ? x2e_chdr_tlast : 1'b0;
  assign x2e_framed_tuser = ((frame_state == ST_CHDR_PAYLOAD) & x2e_chdr_tlast) ? {1'b0, chdr_len[2:0]} : 4'b0000;
  assign x2e_framed_tdata = frame_tdata;

  wire [47:0] pad = 48'h0;
  wire [47:0] mac_dst = x2e_chdr_tuser[47:0];   // Extract from router lookup results
  wire [15:0] eth_type = 16'h0800;  // IPv4
  wire [15:0] misc_ip = { 4'd4 /* IPv4 */, 4'd5 /* IP HDR Len */, 8'h00 /* DSCP and ECN */};
  wire [15:0] ip_len = (16'd28 + chdr_len);  // 20 for IP, 8 for UDP
  wire [15:0] ident = 16'h0;
  wire [15:0] flag_frag = { 3'b010 /* don't fragment */, 13'h0 };
  wire [15:0] ttl_prot = { 8'h10 /* TTL */, 8'h11 /* UDP */ };
  wire [15:0] iphdr_checksum;
  wire [31:0] ip_dst = x2e_chdr_tuser[79:48];   // Extract from router lookup results
  wire [15:0] udp_dst = x2e_chdr_tuser[95:80];  // Extract from router lookup results
  wire [15:0] udp_len = (16'd8 + chdr_len);
  wire [15:0] udp_checksum = 16'h0;

  ip_hdr_checksum ip_hdr_checksum (
    .clk(clk), .in({misc_ip, ip_len, ident, flag_frag, ttl_prot, 16'd0,
    my_ipv4_addr, ip_dst}), .clken(1'b1), .out(iphdr_checksum)
  );

  always @(*) begin
    case(frame_state)
      ST_ETH_L0         : frame_tdata <= bswap64({pad[47:0], mac_dst[47:32]});
      ST_ETH_L1         : frame_tdata <= bswap64({mac_dst[31:0], my_eth_addr[47:16]});
      ST_ETH_L2_IPV4_L0 : frame_tdata <= bswap64({my_eth_addr[15:0], eth_type[15:0], misc_ip[15:0], ip_len[15:0]});
      ST_IPV4_L1        : frame_tdata <= bswap64({ident[15:0], flag_frag[15:0], ttl_prot[15:0], iphdr_checksum[15:0]});
      ST_IPV4_L2        : frame_tdata <= bswap64({my_ipv4_addr[31:0], ip_dst[31:0]});
      ST_IPV4_UDP_HDR   : frame_tdata <= bswap64({my_udp_chdr_port[15:0], udp_dst[15:0], udp_len[15:0], udp_checksum[15:0]});
      default           : frame_tdata <= x2e_chdr_tdata;
    endcase
  end

  wire [63:0] c2e_tdata;
  wire [3:0]  c2e_tuser;
  wire        c2e_tlast;
  wire        c2e_tvalid;
  wire        c2e_tready;

  generate
    if (IS_CPU_ARM == 1'b1) begin
      //---------------------------------------
      // Ethernet deframer for ARM
      //---------------------------------------

      // Add pad of 6 empty bytes to the ethernet packet going from the CPU to the
      // SFP. This padding added before MAC addresses aligns the source and
      // destination IP addresses, UDP headers etc.
      // Note that the xge_mac_wrapper strips this padding to recreate the ethernet
      // packet
      arm_deframer inst_arm_deframer
      (
        .clk(clk),
        .reset(rst),
        .clear(1'b0),

        .s_axis_tdata(s_cpu_tdata),
        .s_axis_tuser(s_cpu_tuser),
        .s_axis_tlast(s_cpu_tlast),
        .s_axis_tvalid(s_cpu_tvalid),
        .s_axis_tready(s_cpu_tready),

        .m_axis_tdata(c2e_tdata),
        .m_axis_tuser(c2e_tuser),
        .m_axis_tlast(c2e_tlast),
        .m_axis_tvalid(c2e_tvalid),
        .m_axis_tready(c2e_tready)
      );
    end else begin
      assign c2e_tdata    = s_cpu_tdata;
      assign c2e_tuser    = s_cpu_tuser;
      assign c2e_tlast    = s_cpu_tlast;
      assign c2e_tvalid   = s_cpu_tvalid;
      assign s_cpu_tready = c2e_tready;
    end
  endgenerate

  //---------------------------------------
  // X2E and C2E MUX
  //---------------------------------------
  axi_mux #(
    .SIZE(2), .PRIO(0), .WIDTH(64+4), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) eth_mux_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({c2e_tuser, c2e_tdata, x2e_framed_tuser, x2e_framed_tdata}), .i_tlast({c2e_tlast, x2e_framed_tlast}),
    .i_tvalid({c2e_tvalid, x2e_framed_tvalid}), .i_tready({c2e_tready, x2e_framed_tready}),
    .o_tdata({m_mac_tuser, m_mac_tdata}), .o_tlast(m_mac_tlast),
    .o_tvalid(m_mac_tvalid), .o_tready(m_mac_tready)
  );

endmodule // eth_ipv4_chdr64_adapter
`default_nettype wire
