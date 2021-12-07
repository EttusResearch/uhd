//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_dram
//
// Description:
//
//   Contains the DRAM logic for X4xx product series. This module provides up
//   to 4 separate memory interface ports (dram_axi_*). These ports are spread
//   across 1 or 2 DRAM banks, with 2 or 4 ports per bank depending on the bus
//   width requested. Only the DRAM banks that are used will be instantiated.
//   DRAM sharing between ports is achieved using AXI Interconnect IP.
//
// Parameters:
//
//   ENABLE_DRAM : Set to 1 to include the DRAM IP. Set to 0 to exclude it and
//                 save resources.
//   AXI_DWIDTH  : Data width of AXI interface to the DRAM. Can be 64 or 128.
//   NUM_PORTS   : Number of AXI ports to support. Can be 0, 2, or 4.
//


module x4xx_dram #(
  parameter ENABLE_DRAM = 1,
  parameter AXI_DWIDTH  = 128,
  parameter NUM_PORTS   = 4
) (

  //---------------------------------------------------------------------------
  // DRAM IP Clocks and Reset
  //---------------------------------------------------------------------------

  // Asynchronous reset for DRAM IP (active-high)
  input wire sys_rst,

  // Base clocks for the DRAM IP
  input wire dram0_sys_clk_p,
  input wire dram0_sys_clk_n,
  input wire dram1_sys_clk_p,
  input wire dram1_sys_clk_n,

  // DRAM interface output clocks (synchronous to the DRAM PHY)
  output wire dram0_ui_clk,
  output wire dram0_ui_clk_sync_rst,
  output wire dram1_ui_clk,
  output wire dram1_ui_clk_sync_rst,

  //-------------------------------------------------------------------------
  // DRAM Bank 0 Chip Interface
  //-------------------------------------------------------------------------

  output wire         dram0_ck_t,
  output wire         dram0_ck_c,
  output wire         dram0_cs_n,
  output wire         dram0_act_n,
  output wire [ 16:0] dram0_adr,
  output wire [  1:0] dram0_ba,
  output wire         dram0_bg,
  output wire         dram0_cke,
  output wire         dram0_odt,
  output wire         dram0_reset_n,
  inout  wire [  7:0] dram0_dm_dbi_n,
  inout  wire [ 63:0] dram0_dq,
  inout  wire [  7:0] dram0_dqs_t,
  inout  wire [  7:0] dram0_dqs_c,

  //-------------------------------------------------------------------------
  // DRAM Bank 1 Chip Interface
  //-------------------------------------------------------------------------

  output wire         dram1_ck_t,
  output wire         dram1_ck_c,
  output wire         dram1_cs_n,
  output wire         dram1_act_n,
  output wire [ 16:0] dram1_adr,
  output wire [  1:0] dram1_ba,
  output wire         dram1_bg,
  output wire         dram1_cke,
  output wire         dram1_odt,
  output wire         dram1_reset_n,
  inout  wire [  7:0] dram1_dm_dbi_n,
  inout  wire [ 63:0] dram1_dq,
  inout  wire [  7:0] dram1_dqs_t,
  inout  wire [  7:0] dram1_dqs_c,

  //-------------------------------------------------------------------------
  // DRAM User Interfaces (Synchronous to dram_clk)
  //-------------------------------------------------------------------------

  input  wire dram_clk,
  input  wire dram_rst,

  output reg dram_init_calib_complete,

  input  wire [          32*NUM_PORTS-1:0] dram_axi_araddr,
  input  wire [           2*NUM_PORTS-1:0] dram_axi_arburst,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_arcache,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_arid,
  input  wire [           8*NUM_PORTS-1:0] dram_axi_arlen,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_arlock,
  input  wire [           3*NUM_PORTS-1:0] dram_axi_arprot,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_arqos,
  output wire [           1*NUM_PORTS-1:0] dram_axi_arready,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_arregion,
  input  wire [           3*NUM_PORTS-1:0] dram_axi_arsize,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_arvalid,
  input  wire [          32*NUM_PORTS-1:0] dram_axi_awaddr,
  input  wire [           2*NUM_PORTS-1:0] dram_axi_awburst,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_awcache,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_awid,
  input  wire [           8*NUM_PORTS-1:0] dram_axi_awlen,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_awlock,
  input  wire [           3*NUM_PORTS-1:0] dram_axi_awprot,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_awqos,
  output wire [           1*NUM_PORTS-1:0] dram_axi_awready,
  input  wire [           4*NUM_PORTS-1:0] dram_axi_awregion,
  input  wire [           3*NUM_PORTS-1:0] dram_axi_awsize,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_awvalid,
  output wire [           1*NUM_PORTS-1:0] dram_axi_bid,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_bready,
  output wire [           2*NUM_PORTS-1:0] dram_axi_bresp,
  output wire [           1*NUM_PORTS-1:0] dram_axi_bvalid,
  output wire [  AXI_DWIDTH*NUM_PORTS-1:0] dram_axi_rdata,
  output wire [           1*NUM_PORTS-1:0] dram_axi_rid,
  output wire [           1*NUM_PORTS-1:0] dram_axi_rlast,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_rready,
  output wire [           2*NUM_PORTS-1:0] dram_axi_rresp,
  output wire [           1*NUM_PORTS-1:0] dram_axi_rvalid,
  input  wire [  AXI_DWIDTH*NUM_PORTS-1:0] dram_axi_wdata,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_wlast,
  output wire [           1*NUM_PORTS-1:0] dram_axi_wready,
  input  wire [AXI_DWIDTH/8*NUM_PORTS-1:0] dram_axi_wstrb,
  input  wire [           1*NUM_PORTS-1:0] dram_axi_wvalid
);
  //---------------------------------------------------------------------------
  // Assertions
  //---------------------------------------------------------------------------

  if (!(
    (NUM_PORTS == 0                     ) ||
    (NUM_PORTS == 2 && AXI_DWIDTH == 128) ||
    (NUM_PORTS == 4 && AXI_DWIDTH ==  64) ||
    (NUM_PORTS == 4 && AXI_DWIDTH == 128)
  )) begin : check_parameters
    ERROR_Unsupported_combination_of_parameters_on_x4xx_dram();
  end

  //---------------------------------------------------------------------------
  // Calibration Complete Signal
  //---------------------------------------------------------------------------

  wire dram0_init_calib_complete_d;
  wire dram1_init_calib_complete_d;

  // Combine the signals from both banks into a single "complete" signal.
  always @(posedge dram_clk) begin
    if (dram_rst) begin
      dram_init_calib_complete <= 1'b0;
    end else begin
      dram_init_calib_complete <= dram0_init_calib_complete_d &
                                  dram1_init_calib_complete_d;
    end
  end

  generate

    //-------------------------------------------------------------------------
    // DRAM Bank 0
    //-------------------------------------------------------------------------

    if (ENABLE_DRAM && NUM_PORTS > 0) begin : gen_dram_bank0
      wire dram0_init_calib_complete;

      // DRAM IP AXI Interface
      wire [  3:0] dram0_axi_awid;
      wire [ 31:0] dram0_axi_awaddr;
      wire [  7:0] dram0_axi_awlen;
      wire [  2:0] dram0_axi_awsize;
      wire [  1:0] dram0_axi_awburst;
      wire [  0:0] dram0_axi_awlock;
      wire [  3:0] dram0_axi_awcache;
      wire [  2:0] dram0_axi_awprot;
      wire [  3:0] dram0_axi_awqos;
      wire         dram0_axi_awvalid;
      wire         dram0_axi_awready;
      wire [511:0] dram0_axi_wdata;
      wire [ 63:0] dram0_axi_wstrb;
      wire         dram0_axi_wlast;
      wire         dram0_axi_wvalid;
      wire         dram0_axi_wready;
      wire         dram0_axi_bready;
      wire [  3:0] dram0_axi_bid;
      wire [  1:0] dram0_axi_bresp;
      wire         dram0_axi_bvalid;
      wire [  3:0] dram0_axi_arid;
      wire [ 31:0] dram0_axi_araddr;
      wire [  7:0] dram0_axi_arlen;
      wire [  2:0] dram0_axi_arsize;
      wire [  1:0] dram0_axi_arburst;
      wire [  0:0] dram0_axi_arlock;
      wire [  3:0] dram0_axi_arcache;
      wire [  2:0] dram0_axi_arprot;
      wire [  3:0] dram0_axi_arqos;
      wire         dram0_axi_arvalid;
      wire         dram0_axi_arready;
      wire         dram0_axi_rready;
      wire [  3:0] dram0_axi_rid;
      wire [511:0] dram0_axi_rdata;
      wire [  1:0] dram0_axi_rresp;
      wire         dram0_axi_rlast;
      wire         dram0_axi_rvalid;

      if (AXI_DWIDTH == 64) begin : gen_axi_64
        axi_inter_4x64_512_bd axi_inter_4x64_512_bd_i (
          .M0_AXI_ACLK    (dram0_ui_clk                                     ),
          .M0_AXI_ARESETN (~dram0_ui_clk_sync_rst                           ),
          .M0_AXI_araddr  (dram0_axi_araddr                                 ),
          .M0_AXI_arburst (dram0_axi_arburst                                ),
          .M0_AXI_arcache (dram0_axi_arcache                                ),
          .M0_AXI_arid    (dram0_axi_arid                                   ),
          .M0_AXI_arlen   (dram0_axi_arlen                                  ),
          .M0_AXI_arlock  (dram0_axi_arlock                                 ),
          .M0_AXI_arprot  (dram0_axi_arprot                                 ),
          .M0_AXI_arqos   (dram0_axi_arqos                                  ),
          .M0_AXI_arready (dram0_axi_arready                                ),
          .M0_AXI_arregion(                                                 ),
          .M0_AXI_arsize  (dram0_axi_arsize                                 ),
          .M0_AXI_arvalid (dram0_axi_arvalid                                ),
          .M0_AXI_awaddr  (dram0_axi_awaddr                                 ),
          .M0_AXI_awburst (dram0_axi_awburst                                ),
          .M0_AXI_awcache (dram0_axi_awcache                                ),
          .M0_AXI_awid    (dram0_axi_awid                                   ),
          .M0_AXI_awlen   (dram0_axi_awlen                                  ),
          .M0_AXI_awlock  (dram0_axi_awlock                                 ),
          .M0_AXI_awprot  (dram0_axi_awprot                                 ),
          .M0_AXI_awqos   (dram0_axi_awqos                                  ),
          .M0_AXI_awready (dram0_axi_awready                                ),
          .M0_AXI_awregion(                                                 ),
          .M0_AXI_awsize  (dram0_axi_awsize                                 ),
          .M0_AXI_awvalid (dram0_axi_awvalid                                ),
          .M0_AXI_bid     (dram0_axi_bid                                    ),
          .M0_AXI_bready  (dram0_axi_bready                                 ),
          .M0_AXI_bresp   (dram0_axi_bresp                                  ),
          .M0_AXI_bvalid  (dram0_axi_bvalid                                 ),
          .M0_AXI_rdata   (dram0_axi_rdata                                  ),
          .M0_AXI_rid     (dram0_axi_rid                                    ),
          .M0_AXI_rlast   (dram0_axi_rlast                                  ),
          .M0_AXI_rready  (dram0_axi_rready                                 ),
          .M0_AXI_rresp   (dram0_axi_rresp                                  ),
          .M0_AXI_rvalid  (dram0_axi_rvalid                                 ),
          .M0_AXI_wdata   (dram0_axi_wdata                                  ),
          .M0_AXI_wlast   (dram0_axi_wlast                                  ),
          .M0_AXI_wready  (dram0_axi_wready                                 ),
          .M0_AXI_wstrb   (dram0_axi_wstrb                                  ),
          .M0_AXI_wvalid  (dram0_axi_wvalid                                 ),
          .S0_AXI_ACLK    (dram_clk                                         ),
          .S0_AXI_ARESETN (~dram_rst                                        ),
          .S0_AXI_araddr  (dram_axi_araddr  [           2*0 +:           32]),
          .S0_AXI_arburst (dram_axi_arburst [           2*0 +:            2]),
          .S0_AXI_arcache (dram_axi_arcache [           4*0 +:            4]),
          .S0_AXI_arid    (dram_axi_arid    [           1*0 +:            1]),
          .S0_AXI_arlen   (dram_axi_arlen   [           8*0 +:            8]),
          .S0_AXI_arlock  (dram_axi_arlock  [           1*0 +:            1]),
          .S0_AXI_arprot  (dram_axi_arprot  [           3*0 +:            3]),
          .S0_AXI_arqos   (dram_axi_arqos   [           4*0 +:            4]),
          .S0_AXI_arready (dram_axi_arready [           1*0 +:            1]),
          .S0_AXI_arregion(dram_axi_arregion[           4*0 +:            4]),
          .S0_AXI_arsize  (dram_axi_arsize  [           3*0 +:            3]),
          .S0_AXI_arvalid (dram_axi_arvalid [           1*0 +:            1]),
          .S0_AXI_awaddr  (dram_axi_awaddr  [           2*0 +:           32]),
          .S0_AXI_awburst (dram_axi_awburst [           2*0 +:            2]),
          .S0_AXI_awcache (dram_axi_awcache [           4*0 +:            4]),
          .S0_AXI_awid    (dram_axi_awid    [           1*0 +:            1]),
          .S0_AXI_awlen   (dram_axi_awlen   [           8*0 +:            8]),
          .S0_AXI_awlock  (dram_axi_awlock  [           1*0 +:            1]),
          .S0_AXI_awprot  (dram_axi_awprot  [           3*0 +:            3]),
          .S0_AXI_awqos   (dram_axi_awqos   [           4*0 +:            4]),
          .S0_AXI_awready (dram_axi_awready [           1*0 +:            1]),
          .S0_AXI_awregion(dram_axi_awregion[           4*0 +:            4]),
          .S0_AXI_awsize  (dram_axi_awsize  [           3*0 +:            3]),
          .S0_AXI_awvalid (dram_axi_awvalid [           1*0 +:            1]),
          .S0_AXI_bid     (dram_axi_bid     [           1*0 +:            1]),
          .S0_AXI_bready  (dram_axi_bready  [           1*0 +:            1]),
          .S0_AXI_bresp   (dram_axi_bresp   [           2*0 +:            2]),
          .S0_AXI_bvalid  (dram_axi_bvalid  [           1*0 +:            1]),
          .S0_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*0 +:   AXI_DWIDTH]),
          .S0_AXI_rid     (dram_axi_rid     [           1*0 +:            1]),
          .S0_AXI_rlast   (dram_axi_rlast   [           1*0 +:            1]),
          .S0_AXI_rready  (dram_axi_rready  [           1*0 +:            1]),
          .S0_AXI_rresp   (dram_axi_rresp   [           2*0 +:            2]),
          .S0_AXI_rvalid  (dram_axi_rvalid  [           1*0 +:            1]),
          .S0_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*0 +:   AXI_DWIDTH]),
          .S0_AXI_wlast   (dram_axi_wlast   [           1*0 +:            1]),
          .S0_AXI_wready  (dram_axi_wready  [           1*0 +:            1]),
          .S0_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*0 +: AXI_DWIDTH/8]),
          .S0_AXI_wvalid  (dram_axi_wvalid  [           1*0 +:            1]),
          .S1_AXI_araddr  (dram_axi_araddr  [          32*1 +:           32]),
          .S1_AXI_arburst (dram_axi_arburst [           2*1 +:            2]),
          .S1_AXI_arcache (dram_axi_arcache [           4*1 +:            4]),
          .S1_AXI_arid    (dram_axi_arid    [           1*1 +:            1]),
          .S1_AXI_arlen   (dram_axi_arlen   [           8*1 +:            8]),
          .S1_AXI_arlock  (dram_axi_arlock  [           1*1 +:            1]),
          .S1_AXI_arprot  (dram_axi_arprot  [           3*1 +:            3]),
          .S1_AXI_arqos   (dram_axi_arqos   [           4*1 +:            4]),
          .S1_AXI_arready (dram_axi_arready [           1*1 +:            1]),
          .S1_AXI_arregion(dram_axi_arregion[           4*1 +:            4]),
          .S1_AXI_arsize  (dram_axi_arsize  [           3*1 +:            3]),
          .S1_AXI_arvalid (dram_axi_arvalid [           1*1 +:            1]),
          .S1_AXI_awaddr  (dram_axi_awaddr  [          32*1 +:           32]),
          .S1_AXI_awburst (dram_axi_awburst [           2*1 +:            2]),
          .S1_AXI_awcache (dram_axi_awcache [           4*1 +:            4]),
          .S1_AXI_awid    (dram_axi_awid    [           1*1 +:            1]),
          .S1_AXI_awlen   (dram_axi_awlen   [           8*1 +:            8]),
          .S1_AXI_awlock  (dram_axi_awlock  [           1*1 +:            1]),
          .S1_AXI_awprot  (dram_axi_awprot  [           3*1 +:            3]),
          .S1_AXI_awqos   (dram_axi_awqos   [           4*1 +:            4]),
          .S1_AXI_awready (dram_axi_awready [           1*1 +:            1]),
          .S1_AXI_awregion(dram_axi_awregion[           4*1 +:            4]),
          .S1_AXI_awsize  (dram_axi_awsize  [           3*1 +:            3]),
          .S1_AXI_awvalid (dram_axi_awvalid [           1*1 +:            1]),
          .S1_AXI_bid     (dram_axi_bid     [           1*1 +:            1]),
          .S1_AXI_bready  (dram_axi_bready  [           1*1 +:            1]),
          .S1_AXI_bresp   (dram_axi_bresp   [           2*1 +:            2]),
          .S1_AXI_bvalid  (dram_axi_bvalid  [           1*1 +:            1]),
          .S1_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*1 +:   AXI_DWIDTH]),
          .S1_AXI_rid     (dram_axi_rid     [           1*1 +:            1]),
          .S1_AXI_rlast   (dram_axi_rlast   [           1*1 +:            1]),
          .S1_AXI_rready  (dram_axi_rready  [           1*1 +:            1]),
          .S1_AXI_rresp   (dram_axi_rresp   [           2*1 +:            2]),
          .S1_AXI_rvalid  (dram_axi_rvalid  [           1*1 +:            1]),
          .S1_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*1 +:   AXI_DWIDTH]),
          .S1_AXI_wlast   (dram_axi_wlast   [           1*1 +:            1]),
          .S1_AXI_wready  (dram_axi_wready  [           1*1 +:            1]),
          .S1_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*1 +: AXI_DWIDTH/8]),
          .S1_AXI_wvalid  (dram_axi_wvalid  [           1*1 +:            1]),
          .S2_AXI_araddr  (dram_axi_araddr  [          32*2 +:           32]),
          .S2_AXI_arburst (dram_axi_arburst [           2*2 +:            2]),
          .S2_AXI_arcache (dram_axi_arcache [           4*2 +:            4]),
          .S2_AXI_arid    (dram_axi_arid    [           1*2 +:            1]),
          .S2_AXI_arlen   (dram_axi_arlen   [           8*2 +:            8]),
          .S2_AXI_arlock  (dram_axi_arlock  [           1*2 +:            1]),
          .S2_AXI_arprot  (dram_axi_arprot  [           3*2 +:            3]),
          .S2_AXI_arqos   (dram_axi_arqos   [           4*2 +:            4]),
          .S2_AXI_arready (dram_axi_arready [           1*2 +:            1]),
          .S2_AXI_arregion(dram_axi_arregion[           4*2 +:            4]),
          .S2_AXI_arsize  (dram_axi_arsize  [           3*2 +:            3]),
          .S2_AXI_arvalid (dram_axi_arvalid [           1*2 +:            1]),
          .S2_AXI_awaddr  (dram_axi_awaddr  [          32*2 +:           32]),
          .S2_AXI_awburst (dram_axi_awburst [           2*2 +:            2]),
          .S2_AXI_awcache (dram_axi_awcache [           4*2 +:            4]),
          .S2_AXI_awid    (dram_axi_awid    [           1*2 +:            1]),
          .S2_AXI_awlen   (dram_axi_awlen   [           8*2 +:            8]),
          .S2_AXI_awlock  (dram_axi_awlock  [           1*2 +:            1]),
          .S2_AXI_awprot  (dram_axi_awprot  [           3*2 +:            3]),
          .S2_AXI_awqos   (dram_axi_awqos   [           4*2 +:            4]),
          .S2_AXI_awready (dram_axi_awready [           1*2 +:            1]),
          .S2_AXI_awregion(dram_axi_awregion[           4*2 +:            4]),
          .S2_AXI_awsize  (dram_axi_awsize  [           3*2 +:            3]),
          .S2_AXI_awvalid (dram_axi_awvalid [           1*2 +:            1]),
          .S2_AXI_bid     (dram_axi_bid     [           1*2 +:            1]),
          .S2_AXI_bready  (dram_axi_bready  [           1*2 +:            1]),
          .S2_AXI_bresp   (dram_axi_bresp   [           2*2 +:            2]),
          .S2_AXI_bvalid  (dram_axi_bvalid  [           1*2 +:            1]),
          .S2_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*2 +:   AXI_DWIDTH]),
          .S2_AXI_rid     (dram_axi_rid     [           1*2 +:            1]),
          .S2_AXI_rlast   (dram_axi_rlast   [           1*2 +:            1]),
          .S2_AXI_rready  (dram_axi_rready  [           1*2 +:            1]),
          .S2_AXI_rresp   (dram_axi_rresp   [           2*2 +:            2]),
          .S2_AXI_rvalid  (dram_axi_rvalid  [           1*2 +:            1]),
          .S2_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*2 +:   AXI_DWIDTH]),
          .S2_AXI_wlast   (dram_axi_wlast   [           1*2 +:            1]),
          .S2_AXI_wready  (dram_axi_wready  [           1*2 +:            1]),
          .S2_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*2 +: AXI_DWIDTH/8]),
          .S2_AXI_wvalid  (dram_axi_wvalid  [           1*2 +:            1]),
          .S3_AXI_araddr  (dram_axi_araddr  [          32*3 +:           32]),
          .S3_AXI_arburst (dram_axi_arburst [           2*3 +:            2]),
          .S3_AXI_arcache (dram_axi_arcache [           4*3 +:            4]),
          .S3_AXI_arid    (dram_axi_arid    [           1*3 +:            1]),
          .S3_AXI_arlen   (dram_axi_arlen   [           8*3 +:            8]),
          .S3_AXI_arlock  (dram_axi_arlock  [           1*3 +:            1]),
          .S3_AXI_arprot  (dram_axi_arprot  [           3*3 +:            3]),
          .S3_AXI_arqos   (dram_axi_arqos   [           4*3 +:            4]),
          .S3_AXI_arready (dram_axi_arready [           1*3 +:            1]),
          .S3_AXI_arregion(dram_axi_arregion[           4*3 +:            4]),
          .S3_AXI_arsize  (dram_axi_arsize  [           3*3 +:            3]),
          .S3_AXI_arvalid (dram_axi_arvalid [           1*3 +:            1]),
          .S3_AXI_awaddr  (dram_axi_awaddr  [          32*3 +:           32]),
          .S3_AXI_awburst (dram_axi_awburst [           2*3 +:            2]),
          .S3_AXI_awcache (dram_axi_awcache [           4*3 +:            4]),
          .S3_AXI_awid    (dram_axi_awid    [           1*3 +:            1]),
          .S3_AXI_awlen   (dram_axi_awlen   [           8*3 +:            8]),
          .S3_AXI_awlock  (dram_axi_awlock  [           1*3 +:            1]),
          .S3_AXI_awprot  (dram_axi_awprot  [           3*3 +:            3]),
          .S3_AXI_awqos   (dram_axi_awqos   [           4*3 +:            4]),
          .S3_AXI_awready (dram_axi_awready [           1*3 +:            1]),
          .S3_AXI_awregion(dram_axi_awregion[           4*3 +:            4]),
          .S3_AXI_awsize  (dram_axi_awsize  [           3*3 +:            3]),
          .S3_AXI_awvalid (dram_axi_awvalid [           1*3 +:            1]),
          .S3_AXI_bid     (dram_axi_bid     [           1*3 +:            1]),
          .S3_AXI_bready  (dram_axi_bready  [           1*3 +:            1]),
          .S3_AXI_bresp   (dram_axi_bresp   [           2*3 +:            2]),
          .S3_AXI_bvalid  (dram_axi_bvalid  [           1*3 +:            1]),
          .S3_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*3 +:   AXI_DWIDTH]),
          .S3_AXI_rid     (dram_axi_rid     [           1*3 +:            1]),
          .S3_AXI_rlast   (dram_axi_rlast   [           1*3 +:            1]),
          .S3_AXI_rready  (dram_axi_rready  [           1*3 +:            1]),
          .S3_AXI_rresp   (dram_axi_rresp   [           2*3 +:            2]),
          .S3_AXI_rvalid  (dram_axi_rvalid  [           1*3 +:            1]),
          .S3_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*3 +:   AXI_DWIDTH]),
          .S3_AXI_wlast   (dram_axi_wlast   [           1*3 +:            1]),
          .S3_AXI_wready  (dram_axi_wready  [           1*3 +:            1]),
          .S3_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*3 +: AXI_DWIDTH/8]),
          .S3_AXI_wvalid  (dram_axi_wvalid  [           1*3 +:            1])
        );
      end else begin : gen_axi_128
        axi_inter_2x128_512_bd axi_inter_2x128_512_bd_i (
          .M0_AXI_ACLK    (dram0_ui_clk                                     ),
          .M0_AXI_ARESETN (~dram0_ui_clk_sync_rst                           ),
          .M0_AXI_araddr  (dram0_axi_araddr                                 ),
          .M0_AXI_arburst (dram0_axi_arburst                                ),
          .M0_AXI_arcache (dram0_axi_arcache                                ),
          .M0_AXI_arid    (dram0_axi_arid                                   ),
          .M0_AXI_arlen   (dram0_axi_arlen                                  ),
          .M0_AXI_arlock  (dram0_axi_arlock                                 ),
          .M0_AXI_arprot  (dram0_axi_arprot                                 ),
          .M0_AXI_arqos   (dram0_axi_arqos                                  ),
          .M0_AXI_arready (dram0_axi_arready                                ),
          .M0_AXI_arregion(                                                 ),
          .M0_AXI_arsize  (dram0_axi_arsize                                 ),
          .M0_AXI_arvalid (dram0_axi_arvalid                                ),
          .M0_AXI_awaddr  (dram0_axi_awaddr                                 ),
          .M0_AXI_awburst (dram0_axi_awburst                                ),
          .M0_AXI_awcache (dram0_axi_awcache                                ),
          .M0_AXI_awid    (dram0_axi_awid                                   ),
          .M0_AXI_awlen   (dram0_axi_awlen                                  ),
          .M0_AXI_awlock  (dram0_axi_awlock                                 ),
          .M0_AXI_awprot  (dram0_axi_awprot                                 ),
          .M0_AXI_awqos   (dram0_axi_awqos                                  ),
          .M0_AXI_awready (dram0_axi_awready                                ),
          .M0_AXI_awregion(                                                 ),
          .M0_AXI_awsize  (dram0_axi_awsize                                 ),
          .M0_AXI_awvalid (dram0_axi_awvalid                                ),
          .M0_AXI_bid     (dram0_axi_bid                                    ),
          .M0_AXI_bready  (dram0_axi_bready                                 ),
          .M0_AXI_bresp   (dram0_axi_bresp                                  ),
          .M0_AXI_bvalid  (dram0_axi_bvalid                                 ),
          .M0_AXI_rdata   (dram0_axi_rdata                                  ),
          .M0_AXI_rid     (dram0_axi_rid                                    ),
          .M0_AXI_rlast   (dram0_axi_rlast                                  ),
          .M0_AXI_rready  (dram0_axi_rready                                 ),
          .M0_AXI_rresp   (dram0_axi_rresp                                  ),
          .M0_AXI_rvalid  (dram0_axi_rvalid                                 ),
          .M0_AXI_wdata   (dram0_axi_wdata                                  ),
          .M0_AXI_wlast   (dram0_axi_wlast                                  ),
          .M0_AXI_wready  (dram0_axi_wready                                 ),
          .M0_AXI_wstrb   (dram0_axi_wstrb                                  ),
          .M0_AXI_wvalid  (dram0_axi_wvalid                                 ),
          .S0_AXI_ACLK    (dram_clk                                         ),
          .S0_AXI_ARESETN (~dram_rst                                        ),
          .S0_AXI_araddr  (dram_axi_araddr  [          32*0 +:           32]),
          .S0_AXI_arburst (dram_axi_arburst [           2*0 +:            2]),
          .S0_AXI_arcache (dram_axi_arcache [           4*0 +:            4]),
          .S0_AXI_arid    (dram_axi_arid    [           1*0 +:            1]),
          .S0_AXI_arlen   (dram_axi_arlen   [           8*0 +:            8]),
          .S0_AXI_arlock  (dram_axi_arlock  [           1*0 +:            1]),
          .S0_AXI_arprot  (dram_axi_arprot  [           3*0 +:            3]),
          .S0_AXI_arqos   (dram_axi_arqos   [           4*0 +:            4]),
          .S0_AXI_arready (dram_axi_arready [           1*0 +:            1]),
          .S0_AXI_arregion(dram_axi_arregion[           4*0 +:            4]),
          .S0_AXI_arsize  (dram_axi_arsize  [           3*0 +:            3]),
          .S0_AXI_arvalid (dram_axi_arvalid [           1*0 +:            1]),
          .S0_AXI_awaddr  (dram_axi_awaddr  [          32*0 +:           32]),
          .S0_AXI_awburst (dram_axi_awburst [           2*0 +:            2]),
          .S0_AXI_awcache (dram_axi_awcache [           4*0 +:            4]),
          .S0_AXI_awid    (dram_axi_awid    [           1*0 +:            1]),
          .S0_AXI_awlen   (dram_axi_awlen   [           8*0 +:            8]),
          .S0_AXI_awlock  (dram_axi_awlock  [           1*0 +:            1]),
          .S0_AXI_awprot  (dram_axi_awprot  [           3*0 +:            3]),
          .S0_AXI_awqos   (dram_axi_awqos   [           4*0 +:            4]),
          .S0_AXI_awready (dram_axi_awready [           1*0 +:            1]),
          .S0_AXI_awregion(dram_axi_awregion[           4*0 +:            4]),
          .S0_AXI_awsize  (dram_axi_awsize  [           3*0 +:            3]),
          .S0_AXI_awvalid (dram_axi_awvalid [           1*0 +:            1]),
          .S0_AXI_bid     (dram_axi_bid     [           1*0 +:            1]),
          .S0_AXI_bready  (dram_axi_bready  [           1*0 +:            1]),
          .S0_AXI_bresp   (dram_axi_bresp   [           2*0 +:            2]),
          .S0_AXI_bvalid  (dram_axi_bvalid  [           1*0 +:            1]),
          .S0_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*0 +:   AXI_DWIDTH]),
          .S0_AXI_rid     (dram_axi_rid     [           1*0 +:            1]),
          .S0_AXI_rlast   (dram_axi_rlast   [           1*0 +:            1]),
          .S0_AXI_rready  (dram_axi_rready  [           1*0 +:            1]),
          .S0_AXI_rresp   (dram_axi_rresp   [           2*0 +:            2]),
          .S0_AXI_rvalid  (dram_axi_rvalid  [           1*0 +:            1]),
          .S0_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*0 +:   AXI_DWIDTH]),
          .S0_AXI_wlast   (dram_axi_wlast   [           1*0 +:            1]),
          .S0_AXI_wready  (dram_axi_wready  [           1*0 +:            1]),
          .S0_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*0 +: AXI_DWIDTH/8]),
          .S0_AXI_wvalid  (dram_axi_wvalid  [           1*0 +:            1]),
          .S1_AXI_araddr  (dram_axi_araddr  [          32*1 +:           32]),
          .S1_AXI_arburst (dram_axi_arburst [           2*1 +:            2]),
          .S1_AXI_arcache (dram_axi_arcache [           4*1 +:            4]),
          .S1_AXI_arid    (dram_axi_arid    [           1*1 +:            1]),
          .S1_AXI_arlen   (dram_axi_arlen   [           8*1 +:            8]),
          .S1_AXI_arlock  (dram_axi_arlock  [           1*1 +:            1]),
          .S1_AXI_arprot  (dram_axi_arprot  [           3*1 +:            3]),
          .S1_AXI_arqos   (dram_axi_arqos   [           4*1 +:            4]),
          .S1_AXI_arready (dram_axi_arready [           1*1 +:            1]),
          .S1_AXI_arregion(dram_axi_arregion[           4*1 +:            4]),
          .S1_AXI_arsize  (dram_axi_arsize  [           3*1 +:            3]),
          .S1_AXI_arvalid (dram_axi_arvalid [           1*1 +:            1]),
          .S1_AXI_awaddr  (dram_axi_awaddr  [          32*1 +:           32]),
          .S1_AXI_awburst (dram_axi_awburst [           2*1 +:            2]),
          .S1_AXI_awcache (dram_axi_awcache [           4*1 +:            4]),
          .S1_AXI_awid    (dram_axi_awid    [           1*1 +:            1]),
          .S1_AXI_awlen   (dram_axi_awlen   [           8*1 +:            8]),
          .S1_AXI_awlock  (dram_axi_awlock  [           1*1 +:            1]),
          .S1_AXI_awprot  (dram_axi_awprot  [           3*1 +:            3]),
          .S1_AXI_awqos   (dram_axi_awqos   [           4*1 +:            4]),
          .S1_AXI_awready (dram_axi_awready [           1*1 +:            1]),
          .S1_AXI_awregion(dram_axi_awregion[           4*1 +:            4]),
          .S1_AXI_awsize  (dram_axi_awsize  [           3*1 +:            3]),
          .S1_AXI_awvalid (dram_axi_awvalid [           1*1 +:            1]),
          .S1_AXI_bid     (dram_axi_bid     [           1*1 +:            1]),
          .S1_AXI_bready  (dram_axi_bready  [           1*1 +:            1]),
          .S1_AXI_bresp   (dram_axi_bresp   [           2*1 +:            2]),
          .S1_AXI_bvalid  (dram_axi_bvalid  [           1*1 +:            1]),
          .S1_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*1 +:   AXI_DWIDTH]),
          .S1_AXI_rid     (dram_axi_rid     [           1*1 +:            1]),
          .S1_AXI_rlast   (dram_axi_rlast   [           1*1 +:            1]),
          .S1_AXI_rready  (dram_axi_rready  [           1*1 +:            1]),
          .S1_AXI_rresp   (dram_axi_rresp   [           2*1 +:            2]),
          .S1_AXI_rvalid  (dram_axi_rvalid  [           1*1 +:            1]),
          .S1_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*1 +:   AXI_DWIDTH]),
          .S1_AXI_wlast   (dram_axi_wlast   [           1*1 +:            1]),
          .S1_AXI_wready  (dram_axi_wready  [           1*1 +:            1]),
          .S1_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*1 +: AXI_DWIDTH/8]),
          .S1_AXI_wvalid  (dram_axi_wvalid  [           1*1 +:            1])
        );
      end

      ddr4_64bits ddr4_64bits_i (
        .sys_rst                (sys_rst                  ),
        .c0_sys_clk_p           (dram0_sys_clk_p          ),
        .c0_sys_clk_n           (dram0_sys_clk_n          ),
        .c0_ddr4_act_n          (dram0_act_n              ),
        .c0_ddr4_adr            (dram0_adr                ),
        .c0_ddr4_ba             (dram0_ba                 ),
        .c0_ddr4_bg             (dram0_bg                 ),
        .c0_ddr4_cke            (dram0_cke                ),
        .c0_ddr4_odt            (dram0_odt                ),
        .c0_ddr4_cs_n           (dram0_cs_n               ),
        .c0_ddr4_ck_t           (dram0_ck_t               ),
        .c0_ddr4_ck_c           (dram0_ck_c               ),
        .c0_ddr4_reset_n        (dram0_reset_n            ),
        .c0_ddr4_dm_dbi_n       (dram0_dm_dbi_n           ),
        .c0_ddr4_dq             (dram0_dq                 ),
        .c0_ddr4_dqs_c          (dram0_dqs_c              ),
        .c0_ddr4_dqs_t          (dram0_dqs_t              ),
        .c0_init_calib_complete (dram0_init_calib_complete),
        .c0_ddr4_ui_clk         (dram0_ui_clk             ),
        .c0_ddr4_ui_clk_sync_rst(dram0_ui_clk_sync_rst    ),
        .dbg_clk                (                         ),
        .c0_ddr4_aresetn        (~dram0_ui_clk_sync_rst   ),
        .c0_ddr4_s_axi_awid     (dram0_axi_awid           ),
        .c0_ddr4_s_axi_awaddr   (dram0_axi_awaddr         ),
        .c0_ddr4_s_axi_awlen    (dram0_axi_awlen          ),
        .c0_ddr4_s_axi_awsize   (dram0_axi_awsize         ),
        .c0_ddr4_s_axi_awburst  (dram0_axi_awburst        ),
        .c0_ddr4_s_axi_awlock   (dram0_axi_awlock         ),
        .c0_ddr4_s_axi_awcache  (dram0_axi_awcache        ),
        .c0_ddr4_s_axi_awprot   (dram0_axi_awprot         ),
        .c0_ddr4_s_axi_awqos    (dram0_axi_awqos          ),
        .c0_ddr4_s_axi_awvalid  (dram0_axi_awvalid        ),
        .c0_ddr4_s_axi_awready  (dram0_axi_awready        ),
        .c0_ddr4_s_axi_wdata    (dram0_axi_wdata          ),
        .c0_ddr4_s_axi_wstrb    (dram0_axi_wstrb          ),
        .c0_ddr4_s_axi_wlast    (dram0_axi_wlast          ),
        .c0_ddr4_s_axi_wvalid   (dram0_axi_wvalid         ),
        .c0_ddr4_s_axi_wready   (dram0_axi_wready         ),
        .c0_ddr4_s_axi_bready   (dram0_axi_bready         ),
        .c0_ddr4_s_axi_bid      (dram0_axi_bid            ),
        .c0_ddr4_s_axi_bresp    (dram0_axi_bresp          ),
        .c0_ddr4_s_axi_bvalid   (dram0_axi_bvalid         ),
        .c0_ddr4_s_axi_arid     (dram0_axi_arid           ),
        .c0_ddr4_s_axi_araddr   (dram0_axi_araddr         ),
        .c0_ddr4_s_axi_arlen    (dram0_axi_arlen          ),
        .c0_ddr4_s_axi_arsize   (dram0_axi_arsize         ),
        .c0_ddr4_s_axi_arburst  (dram0_axi_arburst        ),
        .c0_ddr4_s_axi_arlock   (dram0_axi_arlock         ),
        .c0_ddr4_s_axi_arcache  (dram0_axi_arcache        ),
        .c0_ddr4_s_axi_arprot   (dram0_axi_arprot         ),
        .c0_ddr4_s_axi_arqos    (dram0_axi_arqos          ),
        .c0_ddr4_s_axi_arvalid  (dram0_axi_arvalid        ),
        .c0_ddr4_s_axi_arready  (dram0_axi_arready        ),
        .c0_ddr4_s_axi_rready   (dram0_axi_rready         ),
        .c0_ddr4_s_axi_rid      (dram0_axi_rid            ),
        .c0_ddr4_s_axi_rdata    (dram0_axi_rdata          ),
        .c0_ddr4_s_axi_rresp    (dram0_axi_rresp          ),
        .c0_ddr4_s_axi_rlast    (dram0_axi_rlast          ),
        .c0_ddr4_s_axi_rvalid   (dram0_axi_rvalid         ),
        .dbg_bus                (                         )
      );

      synchronizer synchronizer_dram0 (
        .clk(dram_clk                   ),
        .rst(dram_rst                   ),
        .in (dram0_init_calib_complete  ),
        .out(dram0_init_calib_complete_d)
      );

    end else begin : gen_dram_bank0_disable
      OBUFDS OBUFDS_i (
        .O (dram0_ck_t),
        .OB(dram0_ck_c),
        .I (1'b0      )
      );

      assign dram0_ui_clk  = 1'b0;
      assign dram0_cs_n    = 1'b1;
      assign dram0_act_n   = 1'b1;
      assign dram0_adr     = 16'b0;
      assign dram0_ba      = 2'b0;
      assign dram0_bg      = 1'b0;
      assign dram0_cke     = 1'b0;
      assign dram0_odt     = 1'b0;
      assign dram0_reset_n = 1'b1;

      assign dram0_init_calib_complete_d = 1'b1;
    end

    //-------------------------------------------------------------------------
    // DRAM Bank 1
    //-------------------------------------------------------------------------

    if (ENABLE_DRAM && NUM_PORTS > 2 && AXI_DWIDTH == 128) begin : gen_dram_bank1
      wire dram1_init_calib_complete;

      // DRAM IP AXI Interface
      wire [  3:0] dram1_axi_awid;
      wire [ 31:0] dram1_axi_awaddr;
      wire [  7:0] dram1_axi_awlen;
      wire [  2:0] dram1_axi_awsize;
      wire [  1:0] dram1_axi_awburst;
      wire [  0:0] dram1_axi_awlock;
      wire [  3:0] dram1_axi_awcache;
      wire [  2:0] dram1_axi_awprot;
      wire [  3:0] dram1_axi_awqos;
      wire         dram1_axi_awvalid;
      wire         dram1_axi_awready;
      wire [511:0] dram1_axi_wdata;
      wire [ 63:0] dram1_axi_wstrb;
      wire         dram1_axi_wlast;
      wire         dram1_axi_wvalid;
      wire         dram1_axi_wready;
      wire         dram1_axi_bready;
      wire [  3:0] dram1_axi_bid;
      wire [  1:0] dram1_axi_bresp;
      wire         dram1_axi_bvalid;
      wire [  3:0] dram1_axi_arid;
      wire [ 31:0] dram1_axi_araddr;
      wire [  7:0] dram1_axi_arlen;
      wire [  2:0] dram1_axi_arsize;
      wire [  1:0] dram1_axi_arburst;
      wire [  0:0] dram1_axi_arlock;
      wire [  3:0] dram1_axi_arcache;
      wire [  2:0] dram1_axi_arprot;
      wire [  3:0] dram1_axi_arqos;
      wire         dram1_axi_arvalid;
      wire         dram1_axi_arready;
      wire         dram1_axi_rready;
      wire [  3:0] dram1_axi_rid;
      wire [511:0] dram1_axi_rdata;
      wire [  1:0] dram1_axi_rresp;
      wire         dram1_axi_rlast;
      wire         dram1_axi_rvalid;

      axi_inter_2x128_512_bd axi_inter_2x128_512_bd_i (
        .M0_AXI_ACLK    (dram1_ui_clk                                     ),
        .M0_AXI_ARESETN (~dram1_ui_clk_sync_rst                           ),
        .M0_AXI_araddr  (dram1_axi_araddr                                 ),
        .M0_AXI_arburst (dram1_axi_arburst                                ),
        .M0_AXI_arcache (dram1_axi_arcache                                ),
        .M0_AXI_arid    (dram1_axi_arid                                   ),
        .M0_AXI_arlen   (dram1_axi_arlen                                  ),
        .M0_AXI_arlock  (dram1_axi_arlock                                 ),
        .M0_AXI_arprot  (dram1_axi_arprot                                 ),
        .M0_AXI_arqos   (dram1_axi_arqos                                  ),
        .M0_AXI_arready (dram1_axi_arready                                ),
        .M0_AXI_arregion(                                                 ),
        .M0_AXI_arsize  (dram1_axi_arsize                                 ),
        .M0_AXI_arvalid (dram1_axi_arvalid                                ),
        .M0_AXI_awaddr  (dram1_axi_awaddr                                 ),
        .M0_AXI_awburst (dram1_axi_awburst                                ),
        .M0_AXI_awcache (dram1_axi_awcache                                ),
        .M0_AXI_awid    (dram1_axi_awid                                   ),
        .M0_AXI_awlen   (dram1_axi_awlen                                  ),
        .M0_AXI_awlock  (dram1_axi_awlock                                 ),
        .M0_AXI_awprot  (dram1_axi_awprot                                 ),
        .M0_AXI_awqos   (dram1_axi_awqos                                  ),
        .M0_AXI_awready (dram1_axi_awready                                ),
        .M0_AXI_awregion(                                                 ),
        .M0_AXI_awsize  (dram1_axi_awsize                                 ),
        .M0_AXI_awvalid (dram1_axi_awvalid                                ),
        .M0_AXI_bid     (dram1_axi_bid                                    ),
        .M0_AXI_bready  (dram1_axi_bready                                 ),
        .M0_AXI_bresp   (dram1_axi_bresp                                  ),
        .M0_AXI_bvalid  (dram1_axi_bvalid                                 ),
        .M0_AXI_rdata   (dram1_axi_rdata                                  ),
        .M0_AXI_rid     (dram1_axi_rid                                    ),
        .M0_AXI_rlast   (dram1_axi_rlast                                  ),
        .M0_AXI_rready  (dram1_axi_rready                                 ),
        .M0_AXI_rresp   (dram1_axi_rresp                                  ),
        .M0_AXI_rvalid  (dram1_axi_rvalid                                 ),
        .M0_AXI_wdata   (dram1_axi_wdata                                  ),
        .M0_AXI_wlast   (dram1_axi_wlast                                  ),
        .M0_AXI_wready  (dram1_axi_wready                                 ),
        .M0_AXI_wstrb   (dram1_axi_wstrb                                  ),
        .M0_AXI_wvalid  (dram1_axi_wvalid                                 ),
        .S0_AXI_ACLK    (dram_clk                                         ),
        .S0_AXI_ARESETN (~dram_rst                                        ),
        .S0_AXI_araddr  (dram_axi_araddr  [          32*2 +:          32] ),
        .S0_AXI_arburst (dram_axi_arburst [           2*2 +:            2]),
        .S0_AXI_arcache (dram_axi_arcache [           4*2 +:            4]),
        .S0_AXI_arid    (dram_axi_arid    [           1*2 +:            1]),
        .S0_AXI_arlen   (dram_axi_arlen   [           8*2 +:            8]),
        .S0_AXI_arlock  (dram_axi_arlock  [           1*2 +:            1]),
        .S0_AXI_arprot  (dram_axi_arprot  [           3*2 +:            3]),
        .S0_AXI_arqos   (dram_axi_arqos   [           4*2 +:            4]),
        .S0_AXI_arready (dram_axi_arready [           1*2 +:            1]),
        .S0_AXI_arregion(dram_axi_arregion[           4*2 +:            4]),
        .S0_AXI_arsize  (dram_axi_arsize  [           3*2 +:            3]),
        .S0_AXI_arvalid (dram_axi_arvalid [           1*2 +:            1]),
        .S0_AXI_awaddr  (dram_axi_awaddr  [          32*2 +:           32]),
        .S0_AXI_awburst (dram_axi_awburst [           2*2 +:            2]),
        .S0_AXI_awcache (dram_axi_awcache [           4*2 +:            4]),
        .S0_AXI_awid    (dram_axi_awid    [           1*2 +:            1]),
        .S0_AXI_awlen   (dram_axi_awlen   [           8*2 +:            8]),
        .S0_AXI_awlock  (dram_axi_awlock  [           1*2 +:            1]),
        .S0_AXI_awprot  (dram_axi_awprot  [           3*2 +:            3]),
        .S0_AXI_awqos   (dram_axi_awqos   [           4*2 +:            4]),
        .S0_AXI_awready (dram_axi_awready [           1*2 +:            1]),
        .S0_AXI_awregion(dram_axi_awregion[           4*2 +:            4]),
        .S0_AXI_awsize  (dram_axi_awsize  [           3*2 +:            3]),
        .S0_AXI_awvalid (dram_axi_awvalid [           1*2 +:            1]),
        .S0_AXI_bid     (dram_axi_bid     [           1*2 +:            1]),
        .S0_AXI_bready  (dram_axi_bready  [           1*2 +:            1]),
        .S0_AXI_bresp   (dram_axi_bresp   [           2*2 +:            2]),
        .S0_AXI_bvalid  (dram_axi_bvalid  [           1*2 +:            1]),
        .S0_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*2 +:   AXI_DWIDTH]),
        .S0_AXI_rid     (dram_axi_rid     [           1*2 +:            1]),
        .S0_AXI_rlast   (dram_axi_rlast   [           1*2 +:            1]),
        .S0_AXI_rready  (dram_axi_rready  [           1*2 +:            1]),
        .S0_AXI_rresp   (dram_axi_rresp   [           2*2 +:            2]),
        .S0_AXI_rvalid  (dram_axi_rvalid  [           1*2 +:            1]),
        .S0_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*2 +:   AXI_DWIDTH]),
        .S0_AXI_wlast   (dram_axi_wlast   [           1*2 +:            1]),
        .S0_AXI_wready  (dram_axi_wready  [           1*2 +:            1]),
        .S0_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*2 +: AXI_DWIDTH/8]),
        .S0_AXI_wvalid  (dram_axi_wvalid  [           1*2 +:            1]),
        .S1_AXI_araddr  (dram_axi_araddr  [          32*3 +:           32]),
        .S1_AXI_arburst (dram_axi_arburst [           2*3 +:            2]),
        .S1_AXI_arcache (dram_axi_arcache [           4*3 +:            4]),
        .S1_AXI_arid    (dram_axi_arid    [           1*3 +:            1]),
        .S1_AXI_arlen   (dram_axi_arlen   [           8*3 +:            8]),
        .S1_AXI_arlock  (dram_axi_arlock  [           1*3 +:            1]),
        .S1_AXI_arprot  (dram_axi_arprot  [           3*3 +:            3]),
        .S1_AXI_arqos   (dram_axi_arqos   [           4*3 +:            4]),
        .S1_AXI_arready (dram_axi_arready [           1*3 +:            1]),
        .S1_AXI_arregion(dram_axi_arregion[           4*3 +:            4]),
        .S1_AXI_arsize  (dram_axi_arsize  [           3*3 +:            3]),
        .S1_AXI_arvalid (dram_axi_arvalid [           1*3 +:            1]),
        .S1_AXI_awaddr  (dram_axi_awaddr  [          32*3 +:           32]),
        .S1_AXI_awburst (dram_axi_awburst [           2*3 +:            2]),
        .S1_AXI_awcache (dram_axi_awcache [           4*3 +:            4]),
        .S1_AXI_awid    (dram_axi_awid    [           1*3 +:            1]),
        .S1_AXI_awlen   (dram_axi_awlen   [           8*3 +:            8]),
        .S1_AXI_awlock  (dram_axi_awlock  [           1*3 +:            1]),
        .S1_AXI_awprot  (dram_axi_awprot  [           3*3 +:            3]),
        .S1_AXI_awqos   (dram_axi_awqos   [           4*3 +:            4]),
        .S1_AXI_awready (dram_axi_awready [           1*3 +:            1]),
        .S1_AXI_awregion(dram_axi_awregion[           4*3 +:            4]),
        .S1_AXI_awsize  (dram_axi_awsize  [           3*3 +:            3]),
        .S1_AXI_awvalid (dram_axi_awvalid [           1*3 +:            1]),
        .S1_AXI_bid     (dram_axi_bid     [           1*3 +:            1]),
        .S1_AXI_bready  (dram_axi_bready  [           1*3 +:            1]),
        .S1_AXI_bresp   (dram_axi_bresp   [           2*3 +:            2]),
        .S1_AXI_bvalid  (dram_axi_bvalid  [           1*3 +:            1]),
        .S1_AXI_rdata   (dram_axi_rdata   [  AXI_DWIDTH*3 +:   AXI_DWIDTH]),
        .S1_AXI_rid     (dram_axi_rid     [           1*3 +:            1]),
        .S1_AXI_rlast   (dram_axi_rlast   [           1*3 +:            1]),
        .S1_AXI_rready  (dram_axi_rready  [           1*3 +:            1]),
        .S1_AXI_rresp   (dram_axi_rresp   [           2*3 +:            2]),
        .S1_AXI_rvalid  (dram_axi_rvalid  [           1*3 +:            1]),
        .S1_AXI_wdata   (dram_axi_wdata   [  AXI_DWIDTH*3 +:   AXI_DWIDTH]),
        .S1_AXI_wlast   (dram_axi_wlast   [           1*3 +:            1]),
        .S1_AXI_wready  (dram_axi_wready  [           1*3 +:            1]),
        .S1_AXI_wstrb   (dram_axi_wstrb   [AXI_DWIDTH/8*3 +: AXI_DWIDTH/8]),
        .S1_AXI_wvalid  (dram_axi_wvalid  [           1*3 +:            1])
      );

      ddr4_64bits ddr4_64bits_i (
        .sys_rst                (sys_rst                  ),
        .c0_sys_clk_p           (dram1_sys_clk_p          ),
        .c0_sys_clk_n           (dram1_sys_clk_n          ),
        .c0_ddr4_act_n          (dram1_act_n              ),
        .c0_ddr4_adr            (dram1_adr                ),
        .c0_ddr4_ba             (dram1_ba                 ),
        .c0_ddr4_bg             (dram1_bg                 ),
        .c0_ddr4_cke            (dram1_cke                ),
        .c0_ddr4_odt            (dram1_odt                ),
        .c0_ddr4_cs_n           (dram1_cs_n               ),
        .c0_ddr4_ck_t           (dram1_ck_t               ),
        .c0_ddr4_ck_c           (dram1_ck_c               ),
        .c0_ddr4_reset_n        (dram1_reset_n            ),
        .c0_ddr4_dm_dbi_n       (dram1_dm_dbi_n           ),
        .c0_ddr4_dq             (dram1_dq                 ),
        .c0_ddr4_dqs_c          (dram1_dqs_c              ),
        .c0_ddr4_dqs_t          (dram1_dqs_t              ),
        .c0_init_calib_complete (dram1_init_calib_complete),
        .c0_ddr4_ui_clk         (dram1_ui_clk             ),
        .c0_ddr4_ui_clk_sync_rst(dram1_ui_clk_sync_rst    ),
        .dbg_clk                (                         ),
        .c0_ddr4_aresetn        (~dram1_ui_clk_sync_rst   ),
        .c0_ddr4_s_axi_awid     (dram1_axi_awid           ),
        .c0_ddr4_s_axi_awaddr   (dram1_axi_awaddr         ),
        .c0_ddr4_s_axi_awlen    (dram1_axi_awlen          ),
        .c0_ddr4_s_axi_awsize   (dram1_axi_awsize         ),
        .c0_ddr4_s_axi_awburst  (dram1_axi_awburst        ),
        .c0_ddr4_s_axi_awlock   (dram1_axi_awlock         ),
        .c0_ddr4_s_axi_awcache  (dram1_axi_awcache        ),
        .c0_ddr4_s_axi_awprot   (dram1_axi_awprot         ),
        .c0_ddr4_s_axi_awqos    (dram1_axi_awqos          ),
        .c0_ddr4_s_axi_awvalid  (dram1_axi_awvalid        ),
        .c0_ddr4_s_axi_awready  (dram1_axi_awready        ),
        .c0_ddr4_s_axi_wdata    (dram1_axi_wdata          ),
        .c0_ddr4_s_axi_wstrb    (dram1_axi_wstrb          ),
        .c0_ddr4_s_axi_wlast    (dram1_axi_wlast          ),
        .c0_ddr4_s_axi_wvalid   (dram1_axi_wvalid         ),
        .c0_ddr4_s_axi_wready   (dram1_axi_wready         ),
        .c0_ddr4_s_axi_bready   (dram1_axi_bready         ),
        .c0_ddr4_s_axi_bid      (dram1_axi_bid            ),
        .c0_ddr4_s_axi_bresp    (dram1_axi_bresp          ),
        .c0_ddr4_s_axi_bvalid   (dram1_axi_bvalid         ),
        .c0_ddr4_s_axi_arid     (dram1_axi_arid           ),
        .c0_ddr4_s_axi_araddr   (dram1_axi_araddr         ),
        .c0_ddr4_s_axi_arlen    (dram1_axi_arlen          ),
        .c0_ddr4_s_axi_arsize   (dram1_axi_arsize         ),
        .c0_ddr4_s_axi_arburst  (dram1_axi_arburst        ),
        .c0_ddr4_s_axi_arlock   (dram1_axi_arlock         ),
        .c0_ddr4_s_axi_arcache  (dram1_axi_arcache        ),
        .c0_ddr4_s_axi_arprot   (dram1_axi_arprot         ),
        .c0_ddr4_s_axi_arqos    (dram1_axi_arqos          ),
        .c0_ddr4_s_axi_arvalid  (dram1_axi_arvalid        ),
        .c0_ddr4_s_axi_arready  (dram1_axi_arready        ),
        .c0_ddr4_s_axi_rready   (dram1_axi_rready         ),
        .c0_ddr4_s_axi_rid      (dram1_axi_rid            ),
        .c0_ddr4_s_axi_rdata    (dram1_axi_rdata          ),
        .c0_ddr4_s_axi_rresp    (dram1_axi_rresp          ),
        .c0_ddr4_s_axi_rlast    (dram1_axi_rlast          ),
        .c0_ddr4_s_axi_rvalid   (dram1_axi_rvalid         ),
        .dbg_bus                (                         )
      );

      synchronizer synchronizer_dram1 (
        .clk(dram_clk                   ),
        .rst(dram_rst                   ),
        .in (dram1_init_calib_complete  ),
        .out(dram1_init_calib_complete_d)
      );

    end else begin : gen_dram_bank1_disable
      OBUFDS OBUFDS_i (
        .O (dram1_ck_t),
        .OB(dram1_ck_c),
        .I (1'b0      )
      );

      assign dram1_ui_clk  = 1'b0;
      assign dram1_cs_n    = 1'b1;
      assign dram1_act_n   = 1'b1;
      assign dram1_adr     = 16'b0;
      assign dram1_ba      = 2'b0;
      assign dram1_bg      = 1'b0;
      assign dram1_cke     = 1'b0;
      assign dram1_odt     = 1'b0;
      assign dram1_reset_n = 1'b1;

      assign dram1_init_calib_complete_d = 1'b1;
    end

  endgenerate

endmodule
