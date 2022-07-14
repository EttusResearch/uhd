//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_chdr_adapter_wrapper.
//
// Description:
//
//   Verilog-compatible wrapper for eth_ipv4_chdr_adapter. This converts the
//   interfaces to individual ports. See eth_ipv4_chdr_adapter for additional
//   details.
//

`default_nettype none


module eth_ipv4_chdr_adapter_wrapper #(
  parameter logic [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter int          CPU_FIFO_SIZE    = $clog2(8*1024),
  parameter int          CHDR_FIFO_SIZE   = $clog2(8*1024),
  parameter int          RT_TBL_SIZE      = 6,
  parameter int          NODE_INST        = 0,
  parameter bit          DROP_UNKNOWN_MAC = 0,
  parameter bit          DROP_MIN_PACKET  = 0,
  parameter int          PREAMBLE_BYTES   = 6,
  parameter bit          CPU_PREAMBLE     = 0,
  parameter bit          ADD_SOF          = 1,
  parameter bit          SYNC             = 0,
  parameter int          ENET_W           = 64,
  parameter int          CPU_W            = 64,
  parameter int          CHDR_W           = 64,
  parameter int          NET_CHDR_W       = CHDR_W,
  parameter bit          EN_RX_RAW_PYLD   = 1,

  // Trailing bytes
  localparam int CHDR_USER_W = $clog2(CHDR_W/8),
  // SOF, Trailing bytes
  localparam int CPU_USER_W  = $clog2(CPU_W/8)+1,
  // Error, Trailing bytes
  localparam int ENET_USER_W = $clog2(ENET_W/8)+1
) (
  input wire bus_clk,
  input wire bus_rst,

  // Device info
  input wire  [15:0] device_id,

  //-----------------------------------
  // Device Addresses
  //-----------------------------------

  // Domain: bus_clk
  input  wire  [47:0] my_mac,
  input  wire  [31:0] my_ip,
  input  wire  [15:0] my_udp_chdr_port,
  input  wire  [15:0] my_pause_set,
  input  wire  [15:0] my_pause_clear,

  //-----------------------------------
  // Key-value Map Interface
  //-----------------------------------

  // Domain: bus_clk
  input  wire         kv_stb,
  output wire         kv_busy,
  input  wire  [47:0] kv_mac_addr,
  input  wire  [31:0] kv_ip_addr,
  input  wire  [15:0] kv_udp_port,
  input  wire  [15:0] kv_dst_epid,
  input  wire         kv_raw_udp,

  //-----------------------------------
  // Ethernet Interface
  //-----------------------------------

  input wire eth_clk,
  input wire eth_rst,

  // Pause frame control
  output logic eth_pause_req,

  output logic [     ENET_W-1:0] eth_tx_tdata,
  output logic [ENET_USER_W-1:0] eth_tx_tuser, // {1'b0, trailing bytes}
  output logic [   ENET_W/8-1:0] eth_tx_tkeep, // Trailing bytes as byte enables
  output logic                   eth_tx_tlast,
  output logic                   eth_tx_tvalid,
  input  wire                    eth_tx_tready,

  input  wire  [     ENET_W-1:0] eth_rx_tdata,
  input  wire  [ENET_USER_W-1:0] eth_rx_tuser, // {Error, trailing bytes}
  input  wire                    eth_rx_tlast,
  input  wire                    eth_rx_tvalid,
  output logic                   eth_rx_tready,

  //-----------------------------------
  // RFNoC CHDR Interface
  //-----------------------------------

  // Domain: bus_clk
  output logic [CHDR_W-1:0] e2v_tdata,
  output logic              e2v_tlast,
  output logic              e2v_tvalid,
  input  wire               e2v_tready,

  input  wire  [CHDR_W-1:0] v2e_tdata,
  input  wire               v2e_tlast,
  input  wire               v2e_tvalid,
  output logic              v2e_tready,

  //-----------------------------------
  // CPU DMA Interface
  //-----------------------------------

  input wire cpu_clk,
  input wire cpu_rst,

  output logic [     CPU_W-1:0] e2c_tdata,
  output logic [CPU_USER_W-1:0] e2c_tuser,  // {SOF, trailing bytes}
  output logic                  e2c_tlast,
  output logic                  e2c_tvalid,
  input  wire                   e2c_tready,

  input  wire  [     CPU_W-1:0] c2e_tdata,
  input  wire  [CPU_USER_W-1:0] c2e_tuser,  // {1'b0, trailing bytes}
  input  wire                   c2e_tlast,
  input  wire                   c2e_tvalid,
  output logic                  c2e_tready
);


  //---------------------------------------------------------------------------
  // Interface
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W), .TKEEP(0))
    eth_tx (eth_clk, eth_rst);
  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W), .TKEEP(0))
    eth_rx (eth_clk, eth_rst);

  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W), .TKEEP(0), .TUSER(0))
    v2e (bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W), .TKEEP(0), .TUSER(0))
    e2v (bus_clk, bus_rst);

  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TKEEP(0))
    c2e (cpu_clk, cpu_rst);
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TKEEP(0))
    e2c (cpu_clk, cpu_rst);


  //---------------------------------------------------------------------------
  // Translate Between Signals and Interfaces
  //---------------------------------------------------------------------------

  always_comb begin
    eth_tx_tdata  = eth_tx.tdata;
    eth_tx_tuser  = eth_tx.tuser;
    eth_tx_tkeep  = eth_tx.trailing2keep(eth_tx_tuser);
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

  eth_ipv4_chdr_adapter #(
    .PROTOVER        (PROTOVER        ),
    .CPU_FIFO_SIZE   (CPU_FIFO_SIZE   ),
    .CHDR_FIFO_SIZE  (CHDR_FIFO_SIZE  ),
    .RT_TBL_SIZE     (RT_TBL_SIZE     ),
    .NODE_INST       (NODE_INST       ),
    .DROP_UNKNOWN_MAC(DROP_UNKNOWN_MAC),
    .DROP_MIN_PACKET (DROP_MIN_PACKET ),
    .PREAMBLE_BYTES  (PREAMBLE_BYTES  ),
    .CPU_PREAMBLE    (CPU_PREAMBLE    ),
    .ADD_SOF         (ADD_SOF         ),
    .SYNC            (SYNC            ),
    .ENET_W          (ENET_W          ),
    .CPU_W           (CPU_W           ),
    .CHDR_W          (CHDR_W          ),
    .NET_CHDR_W      (NET_CHDR_W      ),
    .EN_RX_RAW_PYLD  (EN_RX_RAW_PYLD  )
  ) eth_ipv4_chdr_adapter_i (
    .device_id       (device_id       ),
    .my_mac          (my_mac          ),
    .my_ip           (my_ip           ),
    .my_udp_chdr_port(my_udp_chdr_port),
    .my_pause_set    (my_pause_set    ),
    .my_pause_clear  (my_pause_clear  ),
    .kv_stb          (kv_stb          ),
    .kv_busy         (kv_busy         ),
    .kv_mac_addr     (kv_mac_addr     ),
    .kv_ip_addr      (kv_ip_addr      ),
    .kv_udp_port     (kv_udp_port     ),
    .kv_dst_epid     (kv_dst_epid     ),
    .kv_raw_udp      (kv_raw_udp      ),
    .chdr_dropped    (                ),
    .cpu_dropped     (                ),
    .eth_pause_req   (eth_pause_req   ),
    .eth_tx          (eth_tx          ),
    .eth_rx          (eth_rx          ),
    .e2v             (e2v             ),
    .v2e             (v2e             ),
    .e2c             (e2c             ),
    .c2e             (c2e             )
  );

endmodule : eth_ipv4_chdr_adapter_wrapper


`default_nettype wire
