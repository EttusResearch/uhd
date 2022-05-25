//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_interface_wrapper.
//
// Description:
//
//   Verilog-compatible wrapper for eth_ipv4_interface. This converts the
//   interfaces to individual ports. See eth_ipv4_interface for additional
//   details.
//

`default_nettype none


module eth_ipv4_interface_wrapper #(
  parameter logic [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter int          CPU_FIFO_SIZE    = $clog2(8*1024),
  parameter int          CHDR_FIFO_SIZE   = $clog2(8*1024),
  parameter int          NODE_INST        = 0,
  parameter int          RT_TBL_SIZE      = 6,
  parameter int          REG_AWIDTH       = 14,
  parameter int          BASE             = 0,
  parameter bit          DROP_UNKNOWN_MAC = 0,
  parameter bit          DROP_MIN_PACKET  = 0,
  parameter int          PREAMBLE_BYTES   = 6,
  parameter bit          ADD_SOF          = 1,
  parameter bit          SYNC             = 0,
  parameter bit          PAUSE_EN         = 0,
  parameter int          ENET_W           = 64,
  parameter int          CPU_W            = 64,
  parameter int          CHDR_W           = 64,
  parameter int          NET_CHDR_W       = CHDR_W,
  parameter int          EN_RX_KV_MAP_CFG = 1,
  parameter int          EN_RX_RAW_PYLD   = 1,

  // Trailing bytes
  localparam int CHDR_USER_W = $clog2(CHDR_W/8),
  // SOF, Trailing bytes
  localparam int CPU_USER_W  = $clog2(CPU_W/8)+1,
  // Error, Trailing bytes
  localparam int ENET_USER_W = $clog2(ENET_W/8)+1
) (
  input wire         bus_clk,
  input wire         bus_rst,
  input wire  [15:0] device_id,

  //-----------------------------------
  // Register Interface
  //-----------------------------------

  // Register port: Write port (domain: bus_clk)
  input wire                   reg_wr_req,
  input wire  [REG_AWIDTH-1:0] reg_wr_addr,
  input wire  [31:0]           reg_wr_data,

  // Register port: Read port (domain: bus_clk)
  input  wire                   reg_rd_req,
  input  wire  [REG_AWIDTH-1:0] reg_rd_addr,
  output logic                  reg_rd_resp,
  output logic     [31:0]       reg_rd_data,

  //-----------------------------------
  // Device Port Info
  //-----------------------------------

  // Status ports (domain: bus_clk)
  output logic [47:0] my_mac,
  output logic [31:0] my_ip,
  output logic [15:0] my_udp_chdr_port,

  //-----------------------------------
  // Ethernet Interface
  //-----------------------------------

  // Ethernet
  input wire eth_clk,
  input wire eth_rst,

  // Pause frame control (domain: eth_clk)
  output logic eth_pause_req,

  // Ethernet MAC Interface (domain: eth_clk)
  output logic [     ENET_W-1:0] eth_tx_tdata,
  output logic [ENET_USER_W-1:0] eth_tx_tuser, // {1'b0, trailing bytes}
  output logic [   ENET_W/8-1:0] eth_tx_tkeep, // Trailing bytes as byte enables
  output logic                   eth_tx_tlast,
  output logic                   eth_tx_tvalid,
  input  wire                    eth_tx_tready,
  //
  input  wire  [     ENET_W-1:0] eth_rx_tdata,
  input  wire  [ENET_USER_W-1:0] eth_rx_tuser, // {Error, trailing bytes}
  input  wire                    eth_rx_tlast,
  input  wire                    eth_rx_tvalid,
  output logic                   eth_rx_tready,

  //-----------------------------------
  // RFNoC CHDR Interface
  //-----------------------------------

  // RFNoC CHDR Interface (domain: bus_clk)
  output logic [CHDR_W-1:0] e2v_tdata,
  output logic              e2v_tlast,
  output logic              e2v_tvalid,
  input  wire               e2v_tready,
  //
  input  wire  [CHDR_W-1:0] v2e_tdata,
  input  wire               v2e_tlast,
  input  wire               v2e_tvalid,
  output logic              v2e_tready,

  //-----------------------------------
  // CPU DMA Interface
  //-----------------------------------

  input wire cpu_clk,
  input wire cpu_rst,

  // CPU DMA Interface (domain: cpu_clk)
  output logic [     CPU_W-1:0] e2c_tdata,
  output logic [CPU_USER_W-1:0] e2c_tuser,  // {SOF, trailing bytes}
  output logic                  e2c_tlast,
  output logic                  e2c_tvalid,
  input  wire                   e2c_tready,
  //
  input  wire  [     CPU_W-1:0] c2e_tdata,
  input  wire  [CPU_USER_W-1:0] c2e_tuser,  // {1'b0, trailing bytes}
  input  wire                   c2e_tlast,
  input  wire                   c2e_tvalid,
  output logic                  c2e_tready
);


  //---------------------------------------------------------------------------
  // Interface
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W))
    eth_tx (eth_clk, eth_rst);
  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W))
    eth_rx (eth_clk, eth_rst);

  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W), .TKEEP(0), .TUSER(0))
    v2e (bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W), .TKEEP(0), .TUSER(0))
    e2v (bus_clk, bus_rst);

  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(0))
    c2e (cpu_clk, cpu_rst);
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(0))
    e2c (cpu_clk, cpu_rst);


  //---------------------------------------------------------------------------
  // Translate Between Signals and Interfaces
  //---------------------------------------------------------------------------

  always_comb begin
    eth_tx_tdata  = eth_tx.tdata;
    eth_tx_tuser  = eth_tx.tuser;
    eth_tx_tkeep  = eth_tx.tkeep;
    eth_tx_tlast  = eth_tx.tlast;
    eth_tx_tvalid = eth_tx.tvalid;
    eth_tx.tready = eth_tx_tready;

    eth_rx.tdata  = eth_rx_tdata;
    eth_rx.tuser  = eth_rx_tuser;
    eth_rx.tlast  = eth_rx_tlast;
    eth_rx.tvalid = eth_rx_tvalid;
    eth_rx_tready = eth_rx.tready;

    e2v_tdata  = e2v.tdata;
    e2v_tlast  = e2v.tlast;
    e2v_tvalid = e2v.tvalid;
    e2v.tready = e2v_tready;

    v2e.tdata  = v2e_tdata;
    v2e.tlast  = v2e_tlast;
    v2e.tvalid = v2e_tvalid;
    v2e_tready = v2e.tready;

    e2c_tdata  = e2c.tdata;
    e2c_tuser  = e2c.tuser;
    e2c_tlast  = e2c.tlast;
    e2c_tvalid = e2c.tvalid;
    e2c.tready = e2c_tready;

    c2e.tdata  = c2e_tdata;
    c2e.tuser  = c2e_tuser;
    c2e.tlast  = c2e_tlast;
    c2e.tvalid = c2e_tvalid;
    c2e_tready = c2e.tready;
  end


  //---------------------------------------------------------------------------
  // Ethernet Transport Adapter
  //---------------------------------------------------------------------------

  eth_ipv4_interface #(
    .PROTOVER        (PROTOVER        ),
    .CPU_FIFO_SIZE   (CPU_FIFO_SIZE   ),
    .CHDR_FIFO_SIZE  (CHDR_FIFO_SIZE  ),
    .NODE_INST       (NODE_INST       ),
    .RT_TBL_SIZE     (RT_TBL_SIZE     ),
    .REG_AWIDTH      (REG_AWIDTH      ),
    .BASE            (BASE            ),
    .DROP_UNKNOWN_MAC(DROP_UNKNOWN_MAC),
    .DROP_MIN_PACKET (DROP_MIN_PACKET ),
    .PREAMBLE_BYTES  (PREAMBLE_BYTES  ),
    .ADD_SOF         (ADD_SOF         ),
    .SYNC            (SYNC            ),
    .PAUSE_EN        (PAUSE_EN        ),
    .ENET_W          (ENET_W          ),
    .CPU_W           (CPU_W           ),
    .CHDR_W          (CHDR_W          ),
    .NET_CHDR_W      (NET_CHDR_W      ),
    .EN_RX_KV_MAP_CFG(EN_RX_KV_MAP_CFG),
    .EN_RX_RAW_PYLD  (EN_RX_RAW_PYLD  )
  ) eth_ipv4_interface_i (
    .bus_clk         (bus_clk         ),
    .bus_rst         (bus_rst         ),
    .device_id       (device_id       ),
    .reg_wr_req      (reg_wr_req      ),
    .reg_wr_addr     (reg_wr_addr     ),
    .reg_wr_data     (reg_wr_data     ),
    .reg_rd_req      (reg_rd_req      ),
    .reg_rd_addr     (reg_rd_addr     ),
    .reg_rd_resp     (reg_rd_resp     ),
    .reg_rd_data     (reg_rd_data     ),
    .my_mac          (my_mac          ),
    .my_ip           (my_ip           ),
    .my_udp_chdr_port(my_udp_chdr_port),
    .eth_pause_req   (eth_pause_req   ),
    .eth_tx          (eth_tx          ),
    .eth_rx          (eth_rx          ),
    .e2v             (e2v             ),
    .v2e             (v2e             ),
    .e2c             (e2c             ),
    .c2e             (c2e             )
  );

endmodule : eth_ipv4_interface_wrapper


`default_nettype wire
