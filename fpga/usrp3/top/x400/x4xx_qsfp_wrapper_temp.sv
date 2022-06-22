//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_qsfp_wrapper_temp
//
// Description:
//
//   Translation layer between Verilog and SystemVerilog for x4xx_qsfp_wrapper.
//
// Parameters:
//
//   PROTOCOL       : Indicates the protocol to use for each of the 4 QSFP
//                    lanes. See x4xx_mgt_types.vh for possible values.
//   CPU_W          : Width of CPU interface
//   CHDR_W         : CHDR bus width
//   BYTE_MTU       : Transport MTU in bytes
//   PORTNUM        : Port number to distinguish multiple QSFP ports
//   NODE_INST      : RFNoC transport adapter node instance for the first port
//   RFNOC_PROTOVER : RFNoC protocol version for IPv4 interface
//

`include "./x4xx_mgt_types.vh"


module x4xx_qsfp_wrapper_temp #(
  parameter        PROTOCOL0      = `MGT_Disabled,
  parameter        PROTOCOL1      = `MGT_Disabled,
  parameter        PROTOCOL2      = `MGT_Disabled,
  parameter        PROTOCOL3      = `MGT_Disabled,
  parameter        CPU_W          = 64,
  parameter        CHDR_W         = 64,
  parameter        BYTE_MTU       = $clog2(8*1024),
  parameter [ 7:0] PORTNUM        = 8'd0,
  parameter        NODE_INST      = 0,
  parameter [15:0] RFNOC_PROTOVER = {8'd1, 8'd0}
) (
  // Resets
  input logic areset,
  input logic bus_rst,
  input logic clk40_rst,

  // Clocks
  input logic refclk_p,
  input logic refclk_n,
  input logic clk100,
  input logic clk40,
  input logic bus_clk,

  // AXI-Lite
  input  logic [39:0] s_axi_awaddr,
  input  logic        s_axi_awvalid,
  output logic        s_axi_awready,
  input  logic [31:0] s_axi_wdata,
  input  logic [ 3:0] s_axi_wstrb,
  input  logic        s_axi_wvalid,
  output logic        s_axi_wready,
  output logic [ 1:0] s_axi_bresp,
  output logic        s_axi_bvalid,
  input  logic        s_axi_bready,
  input  logic [39:0] s_axi_araddr,
  input  logic        s_axi_arvalid,
  output logic        s_axi_arready,
  output logic [31:0] s_axi_rdata,
  output logic [ 1:0] s_axi_rresp,
  output logic        s_axi_rvalid,
  input  logic        s_axi_rready,

  // MGT high-speed IO
  output logic [3:0] tx_p,
  output logic [3:0] tx_n,
  input  logic [3:0] rx_p,
  input  logic [3:0] rx_n,

  // CHDR router interface
  output logic [4*CHDR_W-1:0] e2v_tdata,
  output logic [         3:0] e2v_tlast,
  output logic [         3:0] e2v_tvalid,
  input  logic [         3:0] e2v_tready,

  input  logic [4*CHDR_W-1:0] v2e_tdata,
  input  logic [         3:0] v2e_tlast,
  input  logic [         3:0] v2e_tvalid,
  output logic [         3:0] v2e_tready,

  // Ethernet DMA AXI to CPU memory
  output logic [ 48:0] axi_hp_araddr,
  output logic [  1:0] axi_hp_arburst,
  output logic [  3:0] axi_hp_arcache,
  output logic [  7:0] axi_hp_arlen,
  output logic [  0:0] axi_hp_arlock,
  output logic [  2:0] axi_hp_arprot,
  output logic [  3:0] axi_hp_arqos,
  input  logic         axi_hp_arready,
  output logic [  2:0] axi_hp_arsize,
  output logic         axi_hp_arvalid,
  output logic [ 48:0] axi_hp_awaddr,
  output logic [  1:0] axi_hp_awburst,
  output logic [  3:0] axi_hp_awcache,
  output logic [  7:0] axi_hp_awlen,
  output logic [  0:0] axi_hp_awlock,
  output logic [  2:0] axi_hp_awprot,
  output logic [  3:0] axi_hp_awqos,
  input  logic         axi_hp_awready,
  output logic [  2:0] axi_hp_awsize,
  output logic         axi_hp_awvalid,
  output logic         axi_hp_bready,
  input  logic [  1:0] axi_hp_bresp,
  input  logic         axi_hp_bvalid,
  input  logic [127:0] axi_hp_rdata,
  input  logic         axi_hp_rlast,
  output logic         axi_hp_rready,
  input  logic [  1:0] axi_hp_rresp,
  input  logic         axi_hp_rvalid,
  output logic [127:0] axi_hp_wdata,
  output logic         axi_hp_wlast,
  input  logic         axi_hp_wready,
  output logic [ 15:0] axi_hp_wstrb,
  output logic         axi_hp_wvalid,

  // Ethernet DMA IRQs
  output logic [3:0] eth_rx_irq,
  output logic [3:0] eth_tx_irq,

  // Misc.
  output logic        rx_rec_clk_out,
  input  logic [15:0] device_id,

  output logic [31:0] port_info_0,
  output logic [31:0] port_info_1,
  output logic [31:0] port_info_2,
  output logic [31:0] port_info_3,

  output logic [3:0] link_up,
  output logic [3:0] activity

);

  import PkgAxiLite::*;

  `include "../../lib/axi4lite_sv/axi_lite.vh"
  `include "../../lib/axi4s_sv/axi4s.vh"


  //---------------------------------------------------------------------------
  // AXI Interfaces
  //---------------------------------------------------------------------------

  localparam CHDR_USER_W = $clog2(CHDR_W/8);
  localparam CPU_USER_W  = $clog2(CPU_W/8)+1;

  // AXI-Stream for RFNoC CHDR
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W),
                .TKEEP(0), .TUSER(0))
    v2e[4] (bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .USER_WIDTH(CHDR_USER_W),
                .TKEEP(0), .TUSER(0))
    e2v[4] (bus_clk, bus_rst);

  // AXI-Lite register interface
  AxiLiteIf #(.DATA_WIDTH(32), .ADDR_WIDTH(40))
    s_axi (clk40, clk40_rst);

  // AXI (Full) for DMA back to CPU memory
  AxiIf #(.DATA_WIDTH(128), .ADDR_WIDTH(49))
    axi_hp (clk40, clk40_rst);

  logic [3:0][31:0] port_info;


  //---------------------------------------------------------------------------
  // Translate Signals to Interfaces
  //---------------------------------------------------------------------------

  always_comb begin
    port_info_0 = port_info[0];
    port_info_1 = port_info[1];
    port_info_2 = port_info[2];
    port_info_3 = port_info[3];

    //---------------------------------
    // s_axi
    //---------------------------------

    // Write channel
    s_axi.awaddr[39:18] = 0;
    s_axi.awaddr[17:0]  = s_axi_awaddr[17:0]; // 256 KiB window
    s_axi.awvalid       = s_axi_awvalid;
    s_axi_awready       = s_axi.awready;

    s_axi.wdata         = s_axi_wdata[31:0];
    s_axi.wstrb         = s_axi_wstrb;
    s_axi.wvalid        = s_axi_wvalid;
    s_axi_wready        = s_axi.wready;

    s_axi_bresp         = s_axi.bresp[1:0];
    s_axi_bvalid        = s_axi.bvalid;
    s_axi.bready        = s_axi_bready;

    // Read channel
    s_axi.araddr[39:18] = 0;
    s_axi.araddr[17:0]  = s_axi_araddr[17:0]; // 256 KiB window
    s_axi.arvalid       = s_axi_arvalid;
    s_axi_arready       = s_axi.arready;

    s_axi_rdata[31:0]   = s_axi.rdata;
    s_axi_rresp         = s_axi.rresp[1:0];
    s_axi_rvalid        = s_axi.rvalid;
    s_axi.rready        = s_axi_rready;

    //---------------------------------
    // axi_hp
    //---------------------------------

    // Write channel
    axi_hp_awaddr     = axi_hp.awaddr;
    axi_hp_awburst    = axi_hp.awburst;
    axi_hp_awcache    = axi_hp.awcache;
    axi_hp_awlen      = axi_hp.awlen;
    axi_hp_awsize     = axi_hp.awsize;
    axi_hp_awlock     = axi_hp.awlock;
    axi_hp_awprot     = axi_hp.awprot;
    axi_hp_awqos      = axi_hp.awqos;
    axi_hp_awvalid    = axi_hp.awvalid;
    axi_hp.awready    = axi_hp_awready;

    axi_hp_wdata      = axi_hp.wdata;
    axi_hp_wstrb      = axi_hp.wstrb;
    axi_hp_wlast      = axi_hp.wlast;
    axi_hp_wvalid     = axi_hp.wvalid;
    axi_hp.wready     = axi_hp_wready;

    axi_hp.bresp[1:0] = axi_hp_bresp;
    axi_hp.bvalid     = axi_hp_bvalid;
    axi_hp_bready     = axi_hp.bready;

    // Read channel
    axi_hp_araddr     = axi_hp.araddr;
    axi_hp_arburst    = axi_hp.arburst;
    axi_hp_arcache    = axi_hp.arcache;
    axi_hp_arlen      = axi_hp.arlen;
    axi_hp_arsize     = axi_hp.arsize;
    axi_hp_arlock     = axi_hp.arlock;
    axi_hp_arprot     = axi_hp.arprot;
    axi_hp_arqos      = axi_hp.arqos;
    axi_hp_arvalid    = axi_hp.arvalid;
    axi_hp.arready    = axi_hp_arready;

    axi_hp.rdata      = axi_hp_rdata;
    axi_hp.rresp[1:0] = axi_hp_rresp;
    axi_hp.rlast      = axi_hp_rlast;
    axi_hp.rvalid     = axi_hp_rvalid;
    axi_hp_rready     = axi_hp.rready;

    //---------------------------------
    // CHDR Links
    //---------------------------------

    e2v_tdata[1*CHDR_W-1:0*CHDR_W] = e2v[0].tdata;
    e2v_tlast[0]                   = e2v[0].tlast;
    e2v_tvalid[0]                  = e2v[0].tvalid;
    e2v[0].tready                  = e2v_tready[0];

    e2v_tdata[2*CHDR_W-1:1*CHDR_W] = e2v[1].tdata;
    e2v_tlast[1]                   = e2v[1].tlast;
    e2v_tvalid[1]                  = e2v[1].tvalid;
    e2v[1].tready                  = e2v_tready[1];

    e2v_tdata[3*CHDR_W-1:2*CHDR_W] = e2v[2].tdata;
    e2v_tlast[2]                   = e2v[2].tlast;
    e2v_tvalid[2]                  = e2v[2].tvalid;
    e2v[2].tready                  = e2v_tready[2];

    e2v_tdata[4*CHDR_W-1:3*CHDR_W] = e2v[3].tdata;
    e2v_tlast[3]                   = e2v[3].tlast;
    e2v_tvalid[3]                  = e2v[3].tvalid;
    e2v[3].tready                  = e2v_tready[3];

    v2e[0].tdata  = v2e_tdata[1*CHDR_W-1:0*CHDR_W];
    v2e[0].tlast  = v2e_tlast[0];
    v2e[0].tvalid = v2e_tvalid[0];
    v2e_tready[0] = v2e[0].tready;

    v2e[1].tdata  = v2e_tdata[2*CHDR_W-1:1*CHDR_W];
    v2e[1].tlast  = v2e_tlast[1];
    v2e[1].tvalid = v2e_tvalid[1];
    v2e_tready[1] = v2e[1].tready;

    v2e[2].tdata  = v2e_tdata[3*CHDR_W-1:2*CHDR_W];
    v2e[2].tlast  = v2e_tlast[2];
    v2e[2].tvalid = v2e_tvalid[2];
    v2e_tready[2] = v2e[2].tready;

    v2e[3].tdata  = v2e_tdata[4*CHDR_W-1:3*CHDR_W];
    v2e[3].tlast  = v2e_tlast[3];
    v2e[3].tvalid = v2e_tvalid[3];
    v2e_tready[3] = v2e[3].tready;
  end

  x4xx_qsfp_wrapper #(
    .PROTOCOL       ({ PROTOCOL3, PROTOCOL2, PROTOCOL1, PROTOCOL0 }),
    .CPU_W          (CPU_W),
    .CHDR_W         (CHDR_W),
    .BYTE_MTU       (BYTE_MTU),
    .PORTNUM        (PORTNUM),
    .NODE_INST      (NODE_INST),
    .RFNOC_PROTOVER (RFNOC_PROTOVER)
  ) x4xx_qsfp_wrapper_i (
    .areset         (areset),
    .refclk_p       (refclk_p),
    .refclk_n       (refclk_n),
    .bus_rst        (bus_rst),
    .clk40_rst      (clk40_rst),
    .clk100         (clk100),
    .bus_clk        (bus_clk),
    .s_axi          (s_axi),
    .tx_p           (tx_p),
    .tx_n           (tx_n),
    .rx_p           (rx_p),
    .rx_n           (rx_n),
    .e2v            (e2v),
    .v2e            (v2e),
    .axi_hp         (axi_hp),
    .eth_tx_irq     (eth_tx_irq),
    .eth_rx_irq     (eth_rx_irq),
    .device_id      (device_id),
    .rx_rec_clk_out (rx_rec_clk_out),
    .port_info      (port_info),
    .link_up        (link_up),
    .activity       (activity)
  );

endmodule
