//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Module: eth_ifc_synth_test
//
// Description: Wrapper to test size of 64 bit version
//
// Parameters: -- get descriptions from eth_ipv4_interface
//  PROTOVER     -
//  MTU          - Max Packet Size
//  NODE_INST    -
//  RT_TBL_SIZE  -
//  REG_AWIDTH   -
//  BASE         -
//  EWIDTH       - Ethernet Width
//  CWIDTH       - CPU Width
//  VWIDTH       - CHDR Width

module eth_ifc_synth_test #(
  parameter [15:0] PROTOVER    = {8'd1, 8'd0},
  parameter        MTU         = 10,
  parameter        NODE_INST   = 0,
  parameter        RT_TBL_SIZE = 6,
  parameter        REG_AWIDTH  = 14,
  parameter        BASE        = 0,
  parameter        EWIDTH      = 512,
  parameter        CWIDTH      = 64,
  parameter        VWIDTH      = 128
) (
  input logic        bus_clk,
  input logic        bus_rst,
  input logic [15:0] device_id,

  // Register port: Write port (domain: clk)
  input logic                  reg_wr_req,
  input logic [REG_AWIDTH-1:0] reg_wr_addr,
  input logic [31:0]           reg_wr_data,

  // Register port: Read port (domain: clk)
  input  logic                  reg_rd_req,
  input  logic [REG_AWIDTH-1:0] reg_rd_addr,
  output logic                  reg_rd_resp,
  output logic     [31:0]       reg_rd_data,

  // Status ports (domain: clk)
  output logic [47:0] my_mac,
  output logic [31:0] my_ip,
  output logic [15:0] my_udp_chdr_port,

  // Ethernet ports
  output logic [EWIDTH-1:0] eth_tx_tdata,
  output logic [$clog2(EWIDTH/8):0] eth_tx_tuser,
  output logic       eth_tx_tlast,
  output logic       eth_tx_tvalid,
  input  logic       eth_tx_tready,
  input  logic [EWIDTH-1:0] eth_rx_tdata,
  input  logic [$clog2(EWIDTH/8):0] eth_rx_tuser,
  input  logic       eth_rx_tlast,
  input  logic       eth_rx_tvalid,
  output logic       eth_rx_tready,

  // CHDR router interface
  output logic [VWIDTH-1:0] e2v_tdata,
  output logic       e2v_tlast,
  output logic       e2v_tvalid,
  input  logic       e2v_tready,
  input  logic [VWIDTH-1:0] v2e_tdata,
  input  logic       v2e_tlast,
  input  logic       v2e_tvalid,
  output logic       v2e_tready,

  // CPU
  output logic [CWIDTH-1:0] e2c_tdata,
  output logic [$clog2(CWIDTH/8):0] e2c_tuser,
  output logic       e2c_tlast,
  output logic       e2c_tvalid,
  input  logic       e2c_tready,
  input  logic [CWIDTH-1:0] c2e_tdata,
  input  logic [$clog2(CWIDTH/8):0] c2e_tuser,
  input  logic       c2e_tlast,
  input  logic       c2e_tvalid,
  output logic       c2e_tready

 );

  localparam MAX_PACKET_BYTES = 2**16;
  localparam ENET_W=EWIDTH;
  localparam ENET_USER_W=$clog2(EWIDTH/8)+1;
  localparam CHDR_W=VWIDTH;
  localparam CHDR_USER_W=$clog2(VWIDTH/8)+1;
  localparam CPU_W=CWIDTH;
  localparam CPU_USER_W=$clog2(CWIDTH/8)+1;

  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    eth_tx (bus_clk, bus_reset);
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    eth_rx (bus_clk, bus_reset);

  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(CHDR_USER_W),.TKEEP(0),.TUSER(0))
    v2e    (bus_clk, bus_reset);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(CHDR_USER_W),.TKEEP(0),.TUSER(0))
    e2v    (bus_clk, bus_reset);

  AxiStreamIf #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),.TKEEP(0))
    c2e    (bus_clk, bus_reset);
  AxiStreamIf #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),.TKEEP(0))
    e2c    (bus_clk, bus_reset);

  assign eth_tx_tdata  = eth_tx.tdata;
  assign eth_tx_tuser  = eth_tx.tuser;
  assign eth_tx_tlast  = eth_tx.tlast;
  assign eth_tx_tvalid = eth_tx.tvalid;
  assign eth_tx.tready = eth_tx_tready;

  assign eth_rx.tdata  = eth_rx_tdata;
  assign eth_rx.tuser  = eth_rx_tuser;
  assign eth_rx.tlast  = eth_rx_tlast;
  assign eth_rx.tvalid = eth_rx_tvalid;
  assign eth_rx_tready = eth_rx.tready;

  assign e2v_tdata     = e2v.tdata;
  assign e2v_tlast     = e2v.tlast;
  assign e2v_tvalid    = e2v.tvalid;
  assign e2v.tready    = e2v_tready;

  assign v2e.tdata     = v2e_tdata;
  assign v2e.tlast     = v2e_tlast;
  assign v2e.tvalid    = v2e_tvalid;
  assign v2e_tready    = v2e.tready;

  assign e2c_tdata     = e2c.tdata;
  assign e2c_tuser     = e2c.tuser;
  assign e2c_tlast     = e2c.tlast;
  assign e2c_tvalid    = e2c.tvalid;
  assign e2c.tready    = e2c_tready;

  assign c2e.tdata     = c2e_tdata;
  assign c2e.tuser     = c2e_tuser;
  assign c2e.tlast     = c2e_tlast;
  assign c2e.tvalid    = c2e_tvalid;
  assign c2e_tready    = c2e.tready;

  eth_ipv4_interface #(
   .PROTOVER(PROTOVER), .MTU(MTU), .NODE_INST(NODE_INST),
   .REG_AWIDTH(REG_AWIDTH), .RT_TBL_SIZE(RT_TBL_SIZE),
   .BASE(BASE),
   .DROP_UNKNOWN_MAC(0),
   .DROP_MIN_PACKET(0),
   .PREAMBLE_BYTES(0),
   .ADD_SOF(0),
   .SYNC(0),
   .ENET_W(EWIDTH),.CPU_W(CWIDTH),.CHDR_W(VWIDTH)
  ) eth_interface (
   .* );

endmodule
