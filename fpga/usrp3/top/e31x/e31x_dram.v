//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: e31x_dram
//
// Description:
//
//   Includes the AXI interconnect and DRAM IP to allow two AXI ports to share
//   the DRAM on E31x devices.
//
//   Two configurations are currently supported, as shown below.
//
//   AXI_DWIDTH  NUM_PORTS  dram_clk
//   ---------------------------------------
//       128         1      100 MHz (ui_clk)
//       64          2      200 MHz (clk200)
//
// Parameters:
//
//   AXI_DWIDTH : Width of the dram_axi_* data bus. Can be 64 or 128-bit.
//   NUM_PORTS  : Number of AXI ports to make available through the dram_axi_*
//                bus. Can be 1 or 2.
//


module e31x_dram #(
  parameter AXI_DWIDTH = 64,
  parameter NUM_PORTS  = 2
) (
  // Asynchronous reset for DDR3 controller
  input wire sys_rst,

  // Clock for DDR3 controller
  input wire ddr3_sys_clk,

  // 200 MHz clock
  input wire clk200,
  input wire clk200_rst,

  //---------------------------------------------------------------------------
  // DRAM Chip Interface
  //---------------------------------------------------------------------------

  output wire [14:0] ddr3_addr,
  output wire  [2:0] ddr3_ba,
  output wire        ddr3_cas_n,
  output wire  [0:0] ddr3_ck_n,
  output wire  [0:0] ddr3_ck_p,
  output wire  [0:0] ddr3_cke,
  output wire        ddr3_ras_n,
  output wire        ddr3_reset_n,
  output wire        ddr3_we_n,
  inout  wire [15:0] ddr3_dq,
  inout  wire  [1:0] ddr3_dqs_n,
  inout  wire  [1:0] ddr3_dqs_p,
  output wire  [1:0] ddr3_dm,
  output wire  [0:0] ddr3_odt,

  //---------------------------------------------------------------------------
  // DRAM User Interfaces (Synchronous to dram_clk)
  //---------------------------------------------------------------------------

  output wire dram_clk,
  output wire dram_rst,

  input  wire [          29*NUM_PORTS-1:0] dram_axi_araddr,
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
  input  wire [          29*NUM_PORTS-1:0] dram_axi_awaddr,
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
    (NUM_PORTS == 1 && AXI_DWIDTH == 128) ||
    (NUM_PORTS == 2 && AXI_DWIDTH ==  64)
  )) begin : check_parameters
    ERROR_Unsupported_combination_of_parameters_on_e31x_dram();
  end

  //---------------------------------------------------------------------------
  // AXI Interconnect
  //---------------------------------------------------------------------------

  wire ui_clk;
  wire ui_rst;

  // DDR3 Memory Controller AXI Interface
  wire  [11:0] ddr3_axi_awid;
  wire  [28:0] ddr3_axi_awaddr;
  wire  [ 7:0] ddr3_axi_awlen;
  wire  [ 2:0] ddr3_axi_awsize;
  wire  [ 1:0] ddr3_axi_awburst;
  wire  [ 0:0] ddr3_axi_awlock;
  wire  [ 3:0] ddr3_axi_awcache;
  wire  [ 2:0] ddr3_axi_awprot;
  wire  [ 3:0] ddr3_axi_awqos;
  wire         ddr3_axi_awvalid;
  wire         ddr3_axi_awready;
  wire [127:0] ddr3_axi_wdata;
  wire [ 15:0] ddr3_axi_wstrb;
  wire         ddr3_axi_wlast;
  wire         ddr3_axi_wvalid;
  wire         ddr3_axi_wready;
  wire  [11:0] ddr3_axi_bid;
  wire  [ 1:0] ddr3_axi_bresp;
  wire         ddr3_axi_bvalid;
  wire         ddr3_axi_bready;
  wire  [11:0] ddr3_axi_arid;
  wire  [28:0] ddr3_axi_araddr;
  wire  [ 7:0] ddr3_axi_arlen;
  wire  [ 2:0] ddr3_axi_arsize;
  wire  [ 1:0] ddr3_axi_arburst;
  wire  [ 0:0] ddr3_axi_arlock;
  wire  [ 3:0] ddr3_axi_arcache;
  wire  [ 2:0] ddr3_axi_arprot;
  wire  [ 3:0] ddr3_axi_arqos;
  wire         ddr3_axi_arvalid;
  wire         ddr3_axi_arready;
  wire [ 11:0] ddr3_axi_rid;
  wire [127:0] ddr3_axi_rdata;
  wire [  1:0] ddr3_axi_rresp;
  wire         ddr3_axi_rlast;
  wire         ddr3_axi_rvalid;
  wire         ddr3_axi_rready;

  if (NUM_PORTS == 1 && AXI_DWIDTH == 128) begin : gen_single_port
    assign dram_clk = ui_clk;
    assign dram_rst = ui_rst;

    assign ddr3_axi_awid    = dram_axi_awid;
    assign ddr3_axi_awaddr  = dram_axi_awaddr;
    assign ddr3_axi_awlen   = dram_axi_awlen;
    assign ddr3_axi_awsize  = dram_axi_awsize;
    assign ddr3_axi_awburst = dram_axi_awburst;
    assign ddr3_axi_awlock  = dram_axi_awlock;
    assign ddr3_axi_awcache = dram_axi_awcache;
    assign ddr3_axi_awprot  = dram_axi_awprot;
    assign ddr3_axi_awqos   = dram_axi_awqos;
    assign ddr3_axi_awvalid = dram_axi_awvalid;
    assign dram_axi_awready = ddr3_axi_awready;
    assign ddr3_axi_wdata   = dram_axi_wdata;
    assign ddr3_axi_wstrb   = dram_axi_wstrb;
    assign ddr3_axi_wlast   = dram_axi_wlast;
    assign ddr3_axi_wvalid  = dram_axi_wvalid;
    assign dram_axi_wready  = ddr3_axi_wready;
    assign dram_axi_bid     = ddr3_axi_bid;
    assign dram_axi_bresp   = ddr3_axi_bresp;
    assign dram_axi_bvalid  = ddr3_axi_bvalid;
    assign ddr3_axi_bready  = dram_axi_bready;
    assign ddr3_axi_arid    = dram_axi_arid;
    assign ddr3_axi_araddr  = dram_axi_araddr;
    assign ddr3_axi_arlen   = dram_axi_arlen;
    assign ddr3_axi_arsize  = dram_axi_arsize;
    assign ddr3_axi_arburst = dram_axi_arburst;
    assign ddr3_axi_arlock  = dram_axi_arlock;
    assign ddr3_axi_arcache = dram_axi_arcache;
    assign ddr3_axi_arprot  = dram_axi_arprot;
    assign ddr3_axi_arqos   = dram_axi_arqos;
    assign ddr3_axi_arvalid = dram_axi_arvalid;
    assign dram_axi_arready = ddr3_axi_arready;
    assign dram_axi_rid     = ddr3_axi_rid;
    assign dram_axi_rdata   = ddr3_axi_rdata;
    assign dram_axi_rresp   = ddr3_axi_rresp;
    assign dram_axi_rlast   = ddr3_axi_rlast;
    assign dram_axi_rvalid  = ddr3_axi_rvalid;
    assign ddr3_axi_rready  = dram_axi_rready;

  end else if (NUM_PORTS == 2 && AXI_DWIDTH == 64) begin : gen_dual_port
    assign dram_clk = clk200;
    assign dram_rst = clk200_rst;

    axi_inter_2x64_128_bd axi_inter_2x64_128_bd_i (
      .M00_AXI_ACLK    (ui_clk                        ),
      .M00_AXI_ARESETN (~ui_rst                       ),
      .M00_AXI_araddr  (ddr3_axi_araddr               ),
      .M00_AXI_arburst (ddr3_axi_arburst              ),
      .M00_AXI_arcache (ddr3_axi_arcache              ),
      .M00_AXI_arid    (ddr3_axi_arid                 ),
      .M00_AXI_arlen   (ddr3_axi_arlen                ),
      .M00_AXI_arlock  (ddr3_axi_arlock               ),
      .M00_AXI_arprot  (ddr3_axi_arprot               ),
      .M00_AXI_arqos   (ddr3_axi_arqos                ),
      .M00_AXI_arready (ddr3_axi_arready              ),
      .M00_AXI_arregion(                              ),
      .M00_AXI_arsize  (ddr3_axi_arsize               ),
      .M00_AXI_arvalid (ddr3_axi_arvalid              ),
      .M00_AXI_awaddr  (ddr3_axi_awaddr               ),
      .M00_AXI_awburst (ddr3_axi_awburst              ),
      .M00_AXI_awcache (ddr3_axi_awcache              ),
      .M00_AXI_awid    (ddr3_axi_awid                 ),
      .M00_AXI_awlen   (ddr3_axi_awlen                ),
      .M00_AXI_awlock  (ddr3_axi_awlock               ),
      .M00_AXI_awprot  (ddr3_axi_awprot               ),
      .M00_AXI_awqos   (ddr3_axi_awqos                ),
      .M00_AXI_awready (ddr3_axi_awready              ),
      .M00_AXI_awregion(                              ),
      .M00_AXI_awsize  (ddr3_axi_awsize               ),
      .M00_AXI_awvalid (ddr3_axi_awvalid              ),
      .M00_AXI_bid     (ddr3_axi_bid                  ),
      .M00_AXI_bready  (ddr3_axi_bready               ),
      .M00_AXI_bresp   (ddr3_axi_bresp                ),
      .M00_AXI_bvalid  (ddr3_axi_bvalid               ),
      .M00_AXI_rdata   (ddr3_axi_rdata                ),
      .M00_AXI_rid     (ddr3_axi_rid                  ),
      .M00_AXI_rlast   (ddr3_axi_rlast                ),
      .M00_AXI_rready  (ddr3_axi_rready               ),
      .M00_AXI_rresp   (ddr3_axi_rresp                ),
      .M00_AXI_rvalid  (ddr3_axi_rvalid               ),
      .M00_AXI_wdata   (ddr3_axi_wdata                ),
      .M00_AXI_wlast   (ddr3_axi_wlast                ),
      .M00_AXI_wready  (ddr3_axi_wready               ),
      .M00_AXI_wstrb   (ddr3_axi_wstrb                ),
      .M00_AXI_wvalid  (ddr3_axi_wvalid               ),
      .S00_AXI_ACLK    (dram_clk                      ),
      .S00_AXI_ARESETN (~dram_rst                     ),
      .S00_AXI_araddr  (dram_axi_araddr   [29*0 +: 29]),
      .S00_AXI_arburst (dram_axi_arburst  [ 2*0 +:  2]),
      .S00_AXI_arcache (dram_axi_arcache  [ 4*0 +:  4]),
      .S00_AXI_arid    (dram_axi_arid     [ 1*0 +:  1]),
      .S00_AXI_arlen   (dram_axi_arlen    [ 8*0 +:  8]),
      .S00_AXI_arlock  (dram_axi_arlock   [ 1*0 +:  1]),
      .S00_AXI_arprot  (dram_axi_arprot   [ 3*0 +:  3]),
      .S00_AXI_arqos   (dram_axi_arqos    [ 4*0 +:  4]),
      .S00_AXI_arready (dram_axi_arready  [ 1*0 +:  1]),
      .S00_AXI_arregion(dram_axi_arregion [ 4*0 +:  4]),
      .S00_AXI_arsize  (dram_axi_arsize   [ 3*0 +:  3]),
      .S00_AXI_arvalid (dram_axi_arvalid  [ 1*0 +:  1]),
      .S00_AXI_awaddr  (dram_axi_awaddr   [29*0 +: 29]),
      .S00_AXI_awburst (dram_axi_awburst  [ 2*0 +:  2]),
      .S00_AXI_awcache (dram_axi_awcache  [ 4*0 +:  4]),
      .S00_AXI_awid    (dram_axi_awid     [ 1*0 +:  1]),
      .S00_AXI_awlen   (dram_axi_awlen    [ 8*0 +:  8]),
      .S00_AXI_awlock  (dram_axi_awlock   [ 1*0 +:  1]),
      .S00_AXI_awprot  (dram_axi_awprot   [ 3*0 +:  3]),
      .S00_AXI_awqos   (dram_axi_awqos    [ 4*0 +:  4]),
      .S00_AXI_awready (dram_axi_awready  [ 1*0 +:  1]),
      .S00_AXI_awregion(dram_axi_awregion [ 4*0 +:  4]),
      .S00_AXI_awsize  (dram_axi_awsize   [ 3*0 +:  3]),
      .S00_AXI_awvalid (dram_axi_awvalid  [ 1*0 +:  1]),
      .S00_AXI_bid     (dram_axi_bid      [ 1*0 +:  1]),
      .S00_AXI_bready  (dram_axi_bready   [ 1*0 +:  1]),
      .S00_AXI_bresp   (dram_axi_bresp    [ 2*0 +:  2]),
      .S00_AXI_bvalid  (dram_axi_bvalid   [ 1*0 +:  1]),
      .S00_AXI_rdata   (dram_axi_rdata    [64*0 +: 64]),
      .S00_AXI_rid     (dram_axi_rid      [ 1*0 +:  1]),
      .S00_AXI_rlast   (dram_axi_rlast    [ 1*0 +:  1]),
      .S00_AXI_rready  (dram_axi_rready   [ 1*0 +:  1]),
      .S00_AXI_rresp   (dram_axi_rresp    [ 2*0 +:  2]),
      .S00_AXI_rvalid  (dram_axi_rvalid   [ 1*0 +:  1]),
      .S00_AXI_wdata   (dram_axi_wdata    [64*0 +: 64]),
      .S00_AXI_wlast   (dram_axi_wlast    [ 1*0 +:  1]),
      .S00_AXI_wready  (dram_axi_wready   [ 1*0 +:  1]),
      .S00_AXI_wstrb   (dram_axi_wstrb    [ 8*0 +:  8]),
      .S00_AXI_wvalid  (dram_axi_wvalid   [ 1*0 +:  1]),
      .S01_AXI_ACLK    (dram_clk                      ),
      .S01_AXI_ARESETN (~dram_rst                     ),
      .S01_AXI_araddr  (dram_axi_araddr   [29*1 +: 29]),
      .S01_AXI_arburst (dram_axi_arburst  [ 2*1 +:  2]),
      .S01_AXI_arcache (dram_axi_arcache  [ 4*1 +:  4]),
      .S01_AXI_arid    (dram_axi_arid     [ 1*1 +:  1]),
      .S01_AXI_arlen   (dram_axi_arlen    [ 8*1 +:  8]),
      .S01_AXI_arlock  (dram_axi_arlock   [ 1*1 +:  1]),
      .S01_AXI_arprot  (dram_axi_arprot   [ 3*1 +:  3]),
      .S01_AXI_arqos   (dram_axi_arqos    [ 4*1 +:  4]),
      .S01_AXI_arready (dram_axi_arready  [ 1*1 +:  1]),
      .S01_AXI_arregion(dram_axi_arregion [ 4*1 +:  4]),
      .S01_AXI_arsize  (dram_axi_arsize   [ 3*1 +:  3]),
      .S01_AXI_arvalid (dram_axi_arvalid  [ 1*1 +:  1]),
      .S01_AXI_awaddr  (dram_axi_awaddr   [29*1 +: 29]),
      .S01_AXI_awburst (dram_axi_awburst  [ 2*1 +:  2]),
      .S01_AXI_awcache (dram_axi_awcache  [ 4*1 +:  4]),
      .S01_AXI_awid    (dram_axi_awid     [ 1*1 +:  1]),
      .S01_AXI_awlen   (dram_axi_awlen    [ 8*1 +:  8]),
      .S01_AXI_awlock  (dram_axi_awlock   [ 1*1 +:  1]),
      .S01_AXI_awprot  (dram_axi_awprot   [ 3*1 +:  3]),
      .S01_AXI_awqos   (dram_axi_awqos    [ 4*1 +:  4]),
      .S01_AXI_awready (dram_axi_awready  [ 1*1 +:  1]),
      .S01_AXI_awregion(dram_axi_awregion [ 4*1 +:  4]),
      .S01_AXI_awsize  (dram_axi_awsize   [ 3*1 +:  3]),
      .S01_AXI_awvalid (dram_axi_awvalid  [ 1*1 +:  1]),
      .S01_AXI_bid     (dram_axi_bid      [ 1*1 +:  1]),
      .S01_AXI_bready  (dram_axi_bready   [ 1*1 +:  1]),
      .S01_AXI_bresp   (dram_axi_bresp    [ 2*1 +:  2]),
      .S01_AXI_bvalid  (dram_axi_bvalid   [ 1*1 +:  1]),
      .S01_AXI_rdata   (dram_axi_rdata    [64*1 +: 64]),
      .S01_AXI_rid     (dram_axi_rid      [ 1*1 +:  1]),
      .S01_AXI_rlast   (dram_axi_rlast    [ 1*1 +:  1]),
      .S01_AXI_rready  (dram_axi_rready   [ 1*1 +:  1]),
      .S01_AXI_rresp   (dram_axi_rresp    [ 2*1 +:  2]),
      .S01_AXI_rvalid  (dram_axi_rvalid   [ 1*1 +:  1]),
      .S01_AXI_wdata   (dram_axi_wdata    [64*1 +: 64]),
      .S01_AXI_wlast   (dram_axi_wlast    [ 1*1 +:  1]),
      .S01_AXI_wready  (dram_axi_wready   [ 1*1 +:  1]),
      .S01_AXI_wstrb   (dram_axi_wstrb    [ 8*1 +:  8]),
      .S01_AXI_wvalid  (dram_axi_wvalid   [ 1*1 +:  1])
    );
  end

  //---------------------------------------------------------------------------
  // DDR3 Memory Controller
  //---------------------------------------------------------------------------

  ddr3_16bit ddr3_16bit_i (
    // Memory interface ports
    .ddr3_addr          (ddr3_addr       ), // output  [14:0] ddr3_addr
    .ddr3_ba            (ddr3_ba         ), // output   [2:0] ddr3_ba
    .ddr3_cas_n         (ddr3_cas_n      ), // output         ddr3_cas_n
    .ddr3_ck_n          (ddr3_ck_n       ), // output   [0:0] ddr3_ck_n
    .ddr3_ck_p          (ddr3_ck_p       ), // output   [0:0] ddr3_ck_p
    .ddr3_cke           (ddr3_cke        ), // output   [0:0] ddr3_cke
    .ddr3_ras_n         (ddr3_ras_n      ), // output         ddr3_ras_n
    .ddr3_reset_n       (ddr3_reset_n    ), // output         ddr3_reset_n
    .ddr3_we_n          (ddr3_we_n       ), // output         ddr3_we_n
    .ddr3_dq            (ddr3_dq         ), // inout   [15:0] ddr3_dq
    .ddr3_dqs_n         (ddr3_dqs_n      ), // inout    [1:0] ddr3_dqs_n
    .ddr3_dqs_p         (ddr3_dqs_p      ), // inout    [1:0] ddr3_dqs_p
    .ddr3_dm            (ddr3_dm         ), // output   [1:0] ddr3_dm
    .ddr3_odt           (ddr3_odt        ), // output   [0:0] ddr3_odt
    // Application interface ports
    .init_calib_complete(                ), // output         init_calib_complete
    .device_temp        (                ), // output  [11:0] device_temp;
    .ui_clk             (ui_clk          ), // output         ui_clk
    .ui_clk_sync_rst    (ui_rst          ), // output         ui_clk_sync_rst
    .mmcm_locked        (                ), // output         mmcm_locked
    .app_sr_req         (1'b0            ), // input          app_sr_req
    .app_ref_req        (1'b0            ), // input          app_ref_req
    .app_zq_req         (1'b0            ), // input          app_zq_req
    .app_sr_active      (                ), // output         app_sr_active
    .app_ref_ack        (                ), // output         app_ref_ack
    .app_zq_ack         (                ), // output         app_zq_ack
    // AXI Interface Reset
    .aresetn            (~ui_rst         ), // input          aresetn
    // Slave Interface Write Address Ports
    .s_axi_awid         (ddr3_axi_awid   ), // input   [11:0] s_axi_awid
    .s_axi_awaddr       (ddr3_axi_awaddr ), // input   [28:0] s_axi_awaddr
    .s_axi_awlen        (ddr3_axi_awlen  ), // input    [7:0] s_axi_awlen
    .s_axi_awsize       (ddr3_axi_awsize ), // input    [2:0] s_axi_awsize
    .s_axi_awburst      (ddr3_axi_awburst), // input    [1:0] s_axi_awburst
    .s_axi_awlock       (ddr3_axi_awlock ), // input    [0:0] s_axi_awlock
    .s_axi_awcache      (ddr3_axi_awcache), // input    [3:0] s_axi_awcache
    .s_axi_awprot       (ddr3_axi_awprot ), // input    [2:0] s_axi_awprot
    .s_axi_awqos        (ddr3_axi_awqos  ), // input    [3:0] s_axi_awqos
    .s_axi_awvalid      (ddr3_axi_awvalid), // input          s_axi_awvalid
    .s_axi_awready      (ddr3_axi_awready), // output         s_axi_awready
    // Slave Interface Write Data Ports
    .s_axi_wdata        (ddr3_axi_wdata  ), // input  [127:0] s_axi_wdata
    .s_axi_wstrb        (ddr3_axi_wstrb  ), // input   [15:0] s_axi_wstrb
    .s_axi_wlast        (ddr3_axi_wlast  ), // input          s_axi_wlast
    .s_axi_wvalid       (ddr3_axi_wvalid ), // input          s_axi_wvalid
    .s_axi_wready       (ddr3_axi_wready ), // output         s_axi_wready
    // Slave Interface Write Response Ports
    .s_axi_bid          (ddr3_axi_bid    ), // output [11:0]  s_axi_bid
    .s_axi_bresp        (ddr3_axi_bresp  ), // output  [1:0]  s_axi_bresp
    .s_axi_bvalid       (ddr3_axi_bvalid ), // output         s_axi_bvalid
    .s_axi_bready       (ddr3_axi_bready ), // input          s_axi_bready
    // Slave Interface Read Address Ports
    .s_axi_arid         (ddr3_axi_arid   ), // input   [11:0] s_axi_arid
    .s_axi_araddr       (ddr3_axi_araddr ), // input   [28:0] s_axi_araddr
    .s_axi_arlen        (ddr3_axi_arlen  ), // input    [7:0] s_axi_arlen
    .s_axi_arsize       (ddr3_axi_arsize ), // input    [2:0] s_axi_arsize
    .s_axi_arburst      (ddr3_axi_arburst), // input    [1:0] s_axi_arburst
    .s_axi_arlock       (ddr3_axi_arlock ), // input    [0:0] s_axi_arlock
    .s_axi_arcache      (ddr3_axi_arcache), // input    [3:0] s_axi_arcache
    .s_axi_arprot       (ddr3_axi_arprot ), // input    [2:0] s_axi_arprot
    .s_axi_arqos        (ddr3_axi_arqos  ), // input    [3:0] s_axi_arqos
    .s_axi_arvalid      (ddr3_axi_arvalid), // input          s_axi_arvalid
    .s_axi_arready      (ddr3_axi_arready), // output         s_axi_arready
    // Slave Interface Read Data Ports
    .s_axi_rid          (ddr3_axi_rid    ), // output  [11:0] s_axi_rid
    .s_axi_rdata        (ddr3_axi_rdata  ), // output [127:0] s_axi_rdata
    .s_axi_rresp        (ddr3_axi_rresp  ), // output   [1:0] s_axi_rresp
    .s_axi_rlast        (ddr3_axi_rlast  ), // output         s_axi_rlast
    .s_axi_rvalid       (ddr3_axi_rvalid ), // output         s_axi_rvalid
    .s_axi_rready       (ddr3_axi_rready ), // input          s_axi_rready
    // System Clock Ports
    .sys_clk_i          (ddr3_sys_clk    ), // input          sys_clk_i
    // Reference Clock Ports
    .clk_ref_i          (clk200          ),
    .sys_rst            (sys_rst         )  // input          sys_rst
  );

endmodule
