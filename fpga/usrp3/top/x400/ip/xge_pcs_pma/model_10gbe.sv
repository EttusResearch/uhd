//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: model_10gbe
//
// Description:
//
//   A wrapper of the 10gbe core to axistream interface. this model can be used
//   drive packets into the x400 translated to serial ethernet.  This is far
//   slower than just driving things in at the output of the mac with force's
//

module model_10gbe  #(
  parameter [7:0] PORTNUM = 8'd0
)(
  input  logic areset,
  // 156.25 Mhz refclk
  input  logic ref_clk,

  // QSFP high-speed IO
  output logic tx_p,
  output logic tx_n,
  input  logic rx_p,
  input  logic rx_n,

  // CLK and RESET - 156.25
  output logic mgt_clk,
  output logic mgt_rst,
  output logic link_up,


  // Data port
  AxiStreamIf.slave  mgt_tx,
  AxiStreamIf.master mgt_rx

);

  // Include macros and time declarations for use with PkgTestExec
  `define TEST_EXEC_OBJ test
  `include "test_exec.svh"
  import PkgAxiLiteBfm::*;
  import PkgTestExec::*;

  logic clk40,clk40_rst;
  logic clk100,clk100_rst;
  logic phy_reset;

  //interface
  AxiLiteIf #(32,32)
   mgt_axil (clk40, clk40_rst);
  //bfm
  AxiLiteBfm #(32, 32) axi = new(.master(mgt_axil));
  TestExec mac_test = new();

  sim_clock_gen #(.PERIOD(25.0), .AUTOSTART(1))
    clk40_gen    (.clk(clk40), .rst(clk40_rst));
  sim_clock_gen #(.PERIOD(100.0), .AUTOSTART(1))
    clk100_gen    (.clk(clk100), .rst(clk100_rst));

  // Register Docs for init_model
  //   MAC CTRL REG Bit positions
  //    ctrl_tx_enable        = mac_ctrl[0]
  //   MAC STATUS REG Bit positions
  //    status_crc_error      = mac_status[0]    1
  //    status_fragment_error = mac_status[1]    2
  //    status_txdfifo_ovflow = mac_status[2]    4
  //    status_txdfifo_udflow = mac_status[3]    8
  //    status_rxdfifo_ovflow = mac_status[4]   10
  //    status_rxdfifo_udflow = mac_status[5]   20
  //    status_pause_frame_rx = mac_status[6]   40
  //    status_local_fault    = mac_status[7]   80
  //    status_remote_fault   = mac_status[8]  100

  logic [31:0] phy_status;
  logic [31:0] mac_status;
  logic [31:0] mac_ctrl;

  initial begin : init_model
    mac_ctrl = 0;

    clk40_gen.reset();
    axi.run();
    wait(!clk40_rst);
    repeat (10) @(posedge clk40);

    mac_test.start_test("model_10gbe::Wait for phy reset done", 150us);
    wait(phy_reset===1'b0);
    mac_test.end_test();

    mac_test.start_test("model_10gbe::Wait for MAC link_up", 150us);
    mac_ctrl[0] = 1; // turn on TX
    wait(link_up===1'b1);
    mac_test.end_test();
  end

  logic [0:0] qpll0_reset;
  logic [0:0] qpll0_lock;
  logic [0:0] qpll0_clk;
  logic [0:0] qpll0_refclk;
  logic [0:0] qpll1_reset;
  logic [0:0] qpll1_lock;
  logic [0:0] qpll1_clk;
  logic [0:0] qpll1_refclk;

  xge_pcs_pma_common_wrapper xge_pcs_pma_common_wrapperx (
    .refclk         (ref_clk),
    .qpll0reset     (qpll0_reset),
    .qpll0lock      (qpll0_lock),
    .qpll0outclk    (qpll0_clk),
    .qpll0outrefclk (qpll0_refclk),
    .qpll1reset     (qpll1_reset),
    .qpll1lock      (qpll1_lock),
    .qpll1outclk    (qpll1_clk),
    .qpll1outrefclk (qpll1_refclk)
  );

  AxiStreamIf #(.DATA_WIDTH(64),.USER_WIDTH(4))
    eth10g_rx(mgt_clk,mgt_rst);

  always_comb begin
    mgt_rx.tdata    = eth10g_rx.tdata;
    mgt_rx.tuser    = eth10g_rx.tuser;
    mgt_rx.tkeep    = eth10g_rx.trailing2keep(eth10g_rx.tuser);
    mgt_rx.tvalid   = eth10g_rx.tvalid;
    mgt_rx.tlast    = eth10g_rx.tlast;
    // The MAC ignores hold off.  Data must be consumed every clock it is valid.
    if (!mgt_rst) begin
      if (!mgt_rx.tready && mgt_rx.tvalid) begin
        $error("Model 100Gbe : can't hold off the MAC");
      end
    end
  end

  eth_10g eth_10g_i (
    .areset(areset),
    //-- Free running 100 MHz clock used for InitClk and AxiLite to mac
    .clk100(clk100),
    // Quad Info
    .qpll0_refclk   (qpll0_refclk),
    .qpll0_clk      (qpll0_clk),
    .qpll0_lock     (qpll0_lock),
    .qpll0_reset    (qpll0_reset),
    .qpll1_refclk   (qpll1_refclk),
    .qpll1_clk      (qpll1_clk),
    .qpll1_lock     (qpll1_lock),
    .qpll1_reset    (qpll1_reset),
    // MGT TX/RX differential signals
    .tx_p(tx_p),
    .tx_n(tx_n),
    .rx_p(rx_p),
    .rx_n(rx_n),
    // MAC system_clock
    .mgt_clk(mgt_clk),
    .mgt_rst(mgt_rst),
    //------------------------ AXI Stream TX Interface ------------------------
    .mgt_tx(mgt_tx),
    //---------------------- AXI Stream RX Interface ------------------------
    // There is no RxTReady signal support by the Ethernet100G IP. Received data has to
    // be read immediately or it is lost.
    // tUser indicates an error on rcvd packet
    .mgt_rx(eth10g_rx),
    // Axi-Lite bus for tie off
    .mgt_axil(mgt_axil),
    // LEDs of QSFP28 port
    .phy_status(phy_status),
    .mac_ctrl(mac_ctrl),
    .mac_status(mac_status),
    .phy_reset(phy_reset),
    .link_up(link_up)
  );

endmodule
