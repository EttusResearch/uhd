//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_ta_x4xx_eth
//
// Description:
//
//   Top-level file for the X4xx Ethernet transport adapter.
//
// Parameters:
//
//   PROTOCOL   : Indicates the port type to use for each of the 4 QSFP lanes.
//                See x4xx_mgt_types.vh for possible values.
//   CHDR_W     : CHDR width used by RFNoC on the FPGA
//   BYTE_MTU   : Transport MTU in bytes
//   QSFP_NUM   : Port number to distinguish multiple QSFP ports
//   NODE_INST  : Node instance, must be unique among transport adapters.
//                Also, for now, must match the crossbar port number it is
//                connected to (for multi-lane transports, it must match the
//                first crossbar port number)
//   PROTOVER   : RFNoC protocol version for IPv4 interface
//
`default_nettype none


`include "./x4xx_mgt_types.vh"


module rfnoc_ta_x4xx_eth #(
  parameter integer PROTOCOL [3:0] = {`MGT_Disabled,
                                      `MGT_Disabled,
                                      `MGT_Disabled,
                                      `MGT_Disabled},
  parameter        CHDR_W     = 64,
  parameter        BYTE_MTU   = $clog2(8*1024),
  parameter [ 7:0] QSFP_NUM   = 8'd0,
  parameter        NODE_INST  = 0,
  parameter [15:0] PROTOVER   = {8'd1, 8'd0}
) (
  // Standard Clocks and Resets
  input  wire  core_arst,
  input  wire  rfnoc_ctrl_clk,
  input  wire  rfnoc_ctrl_rst,
  input  wire  rfnoc_chdr_clk,
  input  wire  rfnoc_chdr_rst,

  // QSFP Clocks
  input  wire  refclk_p,
  input  wire  refclk_n,
  input  wire  dclk,

  // MGT Pins
  output logic [3:0] tx_p,
  output logic [3:0] tx_n,
  input  wire  [3:0] rx_p,
  input  wire  [3:0] rx_n,

  // Transport Adapter Status
  output logic         recovered_clk,
  input  wire  [ 15:0] device_id,
  output logic [  3:0] rx_irq,
  output logic [  3:0] tx_irq,
  output logic [127:0] port_info,
  output logic [  3:0] link_up,
  output logic [  3:0] activity,

  // AXI-Lite Register Interface
  input  wire         axil_rst,
  input  wire         axil_clk,
  input  wire  [39:0] axil_awaddr,
  input  wire         axil_awvalid,
  output logic        axil_awready,
  input  wire  [31:0] axil_wdata,
  input  wire  [ 3:0] axil_wstrb,
  input  wire         axil_wvalid,
  output logic        axil_wready,
  output logic [ 1:0] axil_bresp,
  output logic        axil_bvalid,
  input  wire         axil_bready,
  input  wire  [39:0] axil_araddr,
  input  wire         axil_arvalid,
  output logic        axil_arready,
  output logic [31:0] axil_rdata,
  output logic [ 1:0] axil_rresp,
  output logic        axil_rvalid,
  input  wire         axil_rready,

  // Ethernet DMA AXI to CPU memory
  input  wire          axi_rst,
  input  wire          axi_clk,
  output logic [ 48:0] axi_araddr,
  output logic [  1:0] axi_arburst,
  output logic [  3:0] axi_arcache,
  output logic [  7:0] axi_arlen,
  output logic [  0:0] axi_arlock,
  output logic [  2:0] axi_arprot,
  output logic [  3:0] axi_arqos,
  input  wire          axi_arready,
  output logic [  2:0] axi_arsize,
  output logic         axi_arvalid,
  output logic [ 48:0] axi_awaddr,
  output logic [  1:0] axi_awburst,
  output logic [  3:0] axi_awcache,
  output logic [  7:0] axi_awlen,
  output logic [  0:0] axi_awlock,
  output logic [  2:0] axi_awprot,
  output logic [  3:0] axi_awqos,
  input  wire          axi_awready,
  output logic [  2:0] axi_awsize,
  output logic         axi_awvalid,
  output logic         axi_bready,
  input  wire  [  1:0] axi_bresp,
  input  wire          axi_bvalid,
  input  wire  [127:0] axi_rdata,
  input  wire          axi_rlast,
  output logic         axi_rready,
  input  wire  [  1:0] axi_rresp,
  input  wire          axi_rvalid,
  output logic [127:0] axi_wdata,
  output logic         axi_wlast,
  input  wire          axi_wready,
  output logic [ 15:0] axi_wstrb,
  output logic         axi_wvalid,

  // CHDR Buses
  output logic [4*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  output logic [         3:0] s_rfnoc_chdr_tlast,
  output logic [         3:0] s_rfnoc_chdr_tvalid,
  input  wire  [         3:0] s_rfnoc_chdr_tready,

  input  wire  [4*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  input  wire  [         3:0] m_rfnoc_chdr_tlast,
  input  wire  [         3:0] m_rfnoc_chdr_tvalid,
  output logic [         3:0] m_rfnoc_chdr_tready
);
  import PkgAxiLite::*;

  `include "../../lib/axi4lite_sv/axi_lite.vh"
  `include "../../lib/axi4s_sv/axi4s.vh"


  //---------------------------------------------------------------------------
  // AXI Interfaces
  //---------------------------------------------------------------------------

  localparam CHDR_USER_W = $clog2(CHDR_W/8);

  // AXI-Stream for RFNoC CHDR
  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W     ),
    .USER_WIDTH(CHDR_USER_W),
    .TKEEP     (0          ),
    .TUSER     (0)
  ) v2e[4] (
    .clk(rfnoc_chdr_clk),
    .rst(rfnoc_chdr_rst)
  );

  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W     ),
    .USER_WIDTH(CHDR_USER_W),
    .TKEEP     (0          ),
    .TUSER     (0          )
  ) e2v[4] (
    .clk(rfnoc_chdr_clk),
    .rst(rfnoc_chdr_rst)
  );

  // AXI-Lite register interface
  AxiLiteIf #(
    .DATA_WIDTH(32),
    .ADDR_WIDTH(40)
  ) s_axi (
    .clk(axil_clk),
    .rst(axil_rst)
  );

  // AXI (Full) for DMA back to CPU memory
  AxiIf #(
    .DATA_WIDTH(128),
    .ADDR_WIDTH(49 )
  ) axi_hp (
    .clk(axi_clk),
    .rst(axi_rst)
  );


  //---------------------------------------------------------------------------
  // Translate Signals to Interfaces
  //---------------------------------------------------------------------------

  logic [3:0][31:0] port_info_arr;

  always_comb begin
    port_info = { port_info_arr[3], port_info_arr[2], port_info_arr[1], port_info_arr[0] };

    //---------------------------------
    // AXI-Lite
    //---------------------------------

    // Write channel
    s_axi.awaddr[39:18] = 0;
    s_axi.awaddr[17:0]  = axil_awaddr[17:0]; // 256 KiB window
    s_axi.awvalid       = axil_awvalid;
    axil_awready        = s_axi.awready;

    s_axi.wdata         = axil_wdata[31:0];
    s_axi.wstrb         = axil_wstrb;
    s_axi.wvalid        = axil_wvalid;
    axil_wready         = s_axi.wready;

    axil_bresp          = s_axi.bresp[1:0];
    axil_bvalid         = s_axi.bvalid;
    s_axi.bready        = axil_bready;

    // Read channel
    s_axi.araddr[39:18] = 0;
    s_axi.araddr[17:0]  = axil_araddr[17:0]; // 256 KiB window
    s_axi.arvalid       = axil_arvalid;
    axil_arready        = s_axi.arready;

    axil_rdata[31:0]    = s_axi.rdata;
    axil_rresp          = s_axi.rresp[1:0];
    axil_rvalid         = s_axi.rvalid;
    s_axi.rready        = axil_rready;

    //---------------------------------
    // AXI
    //---------------------------------

    // Write channel
    axi_awaddr        = axi_hp.awaddr;
    axi_awburst       = axi_hp.awburst;
    axi_awcache       = axi_hp.awcache;
    axi_awlen         = axi_hp.awlen;
    axi_awsize        = axi_hp.awsize;
    axi_awlock        = axi_hp.awlock;
    axi_awprot        = axi_hp.awprot;
    axi_awqos         = axi_hp.awqos;
    axi_awvalid       = axi_hp.awvalid;
    axi_hp.awready    = axi_awready;

    axi_wdata         = axi_hp.wdata;
    axi_wstrb         = axi_hp.wstrb;
    axi_wlast         = axi_hp.wlast;
    axi_wvalid        = axi_hp.wvalid;
    axi_hp.wready     = axi_wready;

    axi_hp.bresp[1:0] = axi_bresp;
    axi_hp.bvalid     = axi_bvalid;
    axi_bready        = axi_hp.bready;

    // Read channel
    axi_araddr        = axi_hp.araddr;
    axi_arburst       = axi_hp.arburst;
    axi_arcache       = axi_hp.arcache;
    axi_arlen         = axi_hp.arlen;
    axi_arsize        = axi_hp.arsize;
    axi_arlock        = axi_hp.arlock;
    axi_arprot        = axi_hp.arprot;
    axi_arqos         = axi_hp.arqos;
    axi_arvalid       = axi_hp.arvalid;
    axi_hp.arready    = axi_arready;

    axi_hp.rdata      = axi_rdata;
    axi_hp.rresp[1:0] = axi_rresp;
    axi_hp.rlast      = axi_rlast;
    axi_hp.rvalid     = axi_rvalid;
    axi_rready        = axi_hp.rready;

    //---------------------------------
    // CHDR Links
    //---------------------------------

    s_rfnoc_chdr_tdata[1*CHDR_W-1:0*CHDR_W] = e2v[0].tdata;
    s_rfnoc_chdr_tlast[0]                   = e2v[0].tlast;
    s_rfnoc_chdr_tvalid[0]                  = e2v[0].tvalid;
    e2v[0].tready                           = s_rfnoc_chdr_tready[0];

    s_rfnoc_chdr_tdata[2*CHDR_W-1:1*CHDR_W] = e2v[1].tdata;
    s_rfnoc_chdr_tlast[1]                   = e2v[1].tlast;
    s_rfnoc_chdr_tvalid[1]                  = e2v[1].tvalid;
    e2v[1].tready                           = s_rfnoc_chdr_tready[1];

    s_rfnoc_chdr_tdata[3*CHDR_W-1:2*CHDR_W] = e2v[2].tdata;
    s_rfnoc_chdr_tlast[2]                   = e2v[2].tlast;
    s_rfnoc_chdr_tvalid[2]                  = e2v[2].tvalid;
    e2v[2].tready                           = s_rfnoc_chdr_tready[2];

    s_rfnoc_chdr_tdata[4*CHDR_W-1:3*CHDR_W] = e2v[3].tdata;
    s_rfnoc_chdr_tlast[3]                   = e2v[3].tlast;
    s_rfnoc_chdr_tvalid[3]                  = e2v[3].tvalid;
    e2v[3].tready                           = s_rfnoc_chdr_tready[3];

    v2e[0].tdata           = m_rfnoc_chdr_tdata[1*CHDR_W-1:0*CHDR_W];
    v2e[0].tlast           = m_rfnoc_chdr_tlast[0];
    v2e[0].tvalid          = m_rfnoc_chdr_tvalid[0];
    m_rfnoc_chdr_tready[0] = v2e[0].tready;

    v2e[1].tdata           = m_rfnoc_chdr_tdata[2*CHDR_W-1:1*CHDR_W];
    v2e[1].tlast           = m_rfnoc_chdr_tlast[1];
    v2e[1].tvalid          = m_rfnoc_chdr_tvalid[1];
    m_rfnoc_chdr_tready[1] = v2e[1].tready;

    v2e[2].tdata           = m_rfnoc_chdr_tdata[3*CHDR_W-1:2*CHDR_W];
    v2e[2].tlast           = m_rfnoc_chdr_tlast[2];
    v2e[2].tvalid          = m_rfnoc_chdr_tvalid[2];
    m_rfnoc_chdr_tready[2] = v2e[2].tready;

    v2e[3].tdata           = m_rfnoc_chdr_tdata[4*CHDR_W-1:3*CHDR_W];
    v2e[3].tlast           = m_rfnoc_chdr_tlast[3];
    v2e[3].tvalid          = m_rfnoc_chdr_tvalid[3];
    m_rfnoc_chdr_tready[3] = v2e[3].tready;
  end

  x4xx_qsfp_wrapper #(
    .PROTOCOL       (PROTOCOL),
    .CHDR_W         (CHDR_W),
    .NET_CHDR_W     (CHDR_W),
    .BYTE_MTU       (BYTE_MTU),
    .PORTNUM        (QSFP_NUM),
    .NODE_INST      (NODE_INST),
    .RFNOC_PROTOVER (PROTOVER)
  ) x4xx_qsfp_wrapper_i (
    .areset         (core_arst),
    .refclk_p       (refclk_p),
    .refclk_n       (refclk_n),
    .bus_rst        (rfnoc_chdr_rst),
    .clk100         (dclk),
    .bus_clk        (rfnoc_chdr_clk),
    .s_axi          (s_axi),
    .tx_p           (tx_p),
    .tx_n           (tx_n),
    .rx_p           (rx_p),
    .rx_n           (rx_n),
    .e2v            (e2v),
    .v2e            (v2e),
    .axi_hp         (axi_hp),
    .eth_tx_irq     (tx_irq),
    .eth_rx_irq     (rx_irq),
    .device_id      (device_id),
    .rx_rec_clk_out (recovered_clk),
    .port_info      (port_info_arr),
    .link_up        (link_up),
    .activity       (activity)
  );

endmodule : rfnoc_ta_x4xx_eth

`default_nettype wire
