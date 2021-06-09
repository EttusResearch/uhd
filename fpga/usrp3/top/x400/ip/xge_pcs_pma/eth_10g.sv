//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_10g
//
// Description: Wrapper for the 10G mac and phy


module eth_10g (

  // Resets
  input  logic          areset,
  // Clock for misc stuff
  input  logic          clk100,
  // Shared Quad signals
  output logic[0:0]     qpll0_reset,
  input  logic[0:0]     qpll0_lock,
  input  logic[0:0]     qpll0_clk,
  input  logic[0:0]     qpll0_refclk,
  output logic[0:0]     qpll1_reset,
  input  logic[0:0]     qpll1_lock,
  input  logic[0:0]     qpll1_clk,
  input  logic[0:0]     qpll1_refclk,
  // RX Clk for output
  output logic          rx_rec_clk_out,
  // MGT high-speed IO
  output logic          tx_p,
  output logic          tx_n,
  input  logic          rx_p,
  input  logic          rx_n,

  // Data port
  output logic          mgt_clk,
  output logic          mgt_rst,
  // Interface clocks for mgt_tx and mgt_rx are NOT used (logic uses using mgt_clk)
  AxiStreamIf.slave     mgt_tx,
  AxiStreamIf.master    mgt_rx,
  // Axi port
  AxiLiteIf.slave       mgt_axil,
  // Misc
  output logic [31:0]   phy_status,
  input  logic [31:0]   mac_ctrl,
  output logic [31:0]   mac_status,
  output logic          phy_reset,
  output logic          link_up
);

  import PkgAxiLite::*;

  assign phy_status[31:8] = 24'h0;
  assign mac_status[31:9] = 23'h0;
  assign link_up = phy_status[0];

  // respond with error if anyone reads this memory region
  always_comb begin
    mgt_axil.awready = 1'b1;
    mgt_axil.wready = 1'b1;
    mgt_axil.bresp= SLVERR;
    mgt_axil.bvalid = 1'b1;
    mgt_axil.arready = 1'b1;
    mgt_axil.rdata = 'b0;
    mgt_axil.rresp = SLVERR;
    mgt_axil.rvalid = 1'b1;
  end

  logic        xgmii_clk;
  logic [63:0] xgmii_txd;
  logic [7:0]  xgmii_txc;
  logic [63:0] xgmii_rxd;
  logic [7:0]  xgmii_rxc;
  logic        xge_phy_resetdone;

  assign phy_reset = !xge_phy_resetdone;
  assign mgt_clk = xgmii_clk;

  // This is a heavily replicated signal, add some pipeline
  // to it to make it easier to spread out
  logic mgt_rst_0;

  always_ff @(posedge mgt_clk,posedge areset) begin : reset_timing_dff
    if (areset) begin
      mgt_rst_0 = 1'b1;
      mgt_rst   = 1'b1;
    end else begin
      mgt_rst_0 = !link_up;
      mgt_rst   = mgt_rst_0;
    end
  end

  // areset pin notes - reset is used asynchronously
  ten_gige_phy ten_gige_phy_i (
    .areset         (areset),
    .dclk           (clk100),
    .xgmii_clk      (xgmii_clk),
    .txp            (tx_p),
    .txn            (tx_n),
    .rxp            (rx_p),
    .rxn            (rx_n),
    .xgmii_txd      (xgmii_txd),
    .xgmii_txc      (xgmii_txc),
    .xgmii_rxd      (xgmii_rxd),
    .xgmii_rxc      (xgmii_rxc),
    .qpll0_refclk   (qpll0_refclk),
    .qpll0_clk      (qpll0_clk),
    .qpll0_lock     (qpll0_lock),
    .qpll0_reset    (qpll0_reset),
    .qpll1_refclk   (qpll1_refclk),
    .qpll1_clk      (qpll1_clk),
    .qpll1_lock     (qpll1_lock),
    .qpll1_reset    (qpll1_reset),
    .rxrecclkout    (rx_rec_clk_out),
    .core_status    (phy_status[7:0]),
    .reset_done     (xge_phy_resetdone)
  );

  xge_mac_wrapper #(
    .PORTNUM(0),
    .WISHBONE(0),
    .ADD_PREAMBLE(0),
    .CROSS_TO_SYSCLK(0),
    .CUT_THROUGH(15)
  ) xge_mac_wrapper_i (
    // XGMII
    .xgmii_clk(xgmii_clk),
    .xgmii_txd(xgmii_txd),
    .xgmii_txc(xgmii_txc),
    .xgmii_rxd(xgmii_rxd),
    .xgmii_rxc(xgmii_rxc),
    // Client FIFO Interfaces
    .sys_clk(1'b0),
    .sys_rst(1'b0),
    .rx_tdata(mgt_rx.tdata),
    .rx_tuser(mgt_rx.tuser),
    .rx_tlast(mgt_rx.tlast),
    .rx_tvalid(mgt_rx.tvalid),
    .rx_tready(mgt_rx.tready),
    .tx_tdata(mgt_tx.tdata),
    .tx_tuser(mgt_tx.tuser),   // Bit[3] (error) is ignored for now.
    .tx_tlast(mgt_tx.tlast),
    .tx_tvalid(mgt_tx.tvalid),
    .tx_tready(mgt_tx.tready),
    // Control and Status
    .phy_ready(xge_phy_resetdone),
    .ctrl_tx_enable(mac_ctrl[0]),
    .status_crc_error(mac_status[0]),
    .status_fragment_error(mac_status[1]),
    .status_txdfifo_ovflow(mac_status[2]),
    .status_txdfifo_udflow(mac_status[3]),
    .status_rxdfifo_ovflow(mac_status[4]),
    .status_rxdfifo_udflow(mac_status[5]),
    .status_pause_frame_rx(mac_status[6]),
    .status_local_fault(mac_status[7]),
    .status_remote_fault(mac_status[8]),
    // MDIO
    .mdc(),
    .mdio_in(),
    .mdio_out(1'b0),
    // Wishbone
    .wb_ack_o(),
    .wb_dat_o(),
    .wb_adr_i(8'b0),
    .wb_clk_i(1'b0),
    .wb_cyc_i(1'b0),
    .wb_dat_i(32'b0),
    .wb_rst_i(1'b0),
    .wb_stb_i(1'b0),
    .wb_we_i (1'b0),
    .wb_int_o()
  );

endmodule
