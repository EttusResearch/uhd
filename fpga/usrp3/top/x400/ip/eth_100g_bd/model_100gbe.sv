//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: model_100gbe
//
// Description:
//
//    A wrapper of the 100gbe core to axistream interface. this model can be
//    used drive packets into the X400 translated to serial Ethernet.  This is
//    generally pretty slower than just driving things in at the output of the
//    mac.
//

package Pkg100gbMac;
  import PkgAxiLite::*;
  import PkgAxiLiteBfm::*;
  import PkgTestExec::*;

  localparam DATA_WIDTH =32;
  localparam ADDR_WIDTH =15;

  typedef AxiLiteBfm #(DATA_WIDTH, ADDR_WIDTH) MacAxiLiteBfm_t;

  // defined in https://www.xilinx.com/support/documentation/ip_documentation/cmac_usplus/v2_4/pg203-cmac-usplus.pdf
  // pg 187
  localparam CONFIGURATION_TX_REG1 = 32'h000C;
  localparam ctl_tx_ctl_enable       = 0;
  localparam ctl_tx_ctl_tx_send_lfi  = 3;
  localparam ctl_tx_ctl_tx_send_rfi  = 4;
  localparam ctl_tx_ctl_tx_send_idle = 5;
  localparam ctl_tx_ctl_test_pattern = 16;

  localparam CONFIGURATION_RX_REG1 = 32'h0014;
  localparam ctl_rx_ctl_enable           = 0;
  localparam ctl_rx_ctl_rx_force_resync  = 7;
  localparam ctl_rx_ctl_test_pattern     = 8;

  localparam RSFEC_CONFIG_INDICATION_CORRECTION = 32'h1000;
  localparam rs_fec_in_ctl_rx_rsfec_enable_correction       = 0;
  localparam rs_fec_in_ctl_rx_rsfec_enable_indication       = 1;
  localparam rs_fec_in_ctl_rsfec_ieee_error_indication_mode = 2;

  localparam RSFEC_CONFIG_ENABLE = 32'h107C;
  localparam rs_fec_in_ctl_rx_rsfec_enable = 0;
  localparam rs_fec_in_ctl_tx_rsfec_enable = 1;

  localparam STAT_RX_STATUS_REG = 32'h0204;
  localparam stat_rx_status = 0;
  localparam stat_rx_aligned = 1;
  localparam stat_rx_misaligned = 2;
  localparam stat_rx_aligned_err = 3;

  task automatic init_mac (int offset, MacAxiLiteBfm_t axi);
    automatic logic [31:0] data;
    automatic resp_t resp;

    // start transmitting alignment pattern
    data = 0;
    data[ctl_tx_ctl_enable] = 0;
    data[ctl_tx_ctl_tx_send_idle] = 0;
    data[ctl_tx_ctl_tx_send_lfi] = 0;
    data[ctl_tx_ctl_tx_send_rfi] = 1;
    data[ctl_tx_ctl_test_pattern] = 0;
    axi.wr(CONFIGURATION_TX_REG1+offset,data);

    // configure fec
    data = 0;
    data[rs_fec_in_ctl_rx_rsfec_enable_correction] = 1;
    data[rs_fec_in_ctl_rx_rsfec_enable_indication] = 1;
    data[rs_fec_in_ctl_rsfec_ieee_error_indication_mode] = 1;
    axi.wr(RSFEC_CONFIG_INDICATION_CORRECTION+offset,data);

    data = 0;
    data[rs_fec_in_ctl_rx_rsfec_enable] = 1;
    data[rs_fec_in_ctl_tx_rsfec_enable] = 1;
    axi.wr(RSFEC_CONFIG_ENABLE+offset,data);

    // turn on RX interface
    data = 0;
    data[ctl_rx_ctl_enable] = 1;
    data[ctl_rx_ctl_rx_force_resync] = 0;
    data[ctl_rx_ctl_test_pattern] = 0;
    axi.wr(CONFIGURATION_RX_REG1+offset,data);

    do begin
      axi.rd_block(STAT_RX_STATUS_REG+offset,data,resp);
      assert (resp==OKAY);
    end while (data[stat_rx_aligned] !== 1);

    // stop transmitting alignment pattern
    // and start transmitting data
    data = 0;
    data[ctl_tx_ctl_enable] = 1;
    data[ctl_tx_ctl_tx_send_idle] = 0;
    data[ctl_tx_ctl_tx_send_lfi] = 0;
    data[ctl_tx_ctl_tx_send_rfi] = 0;
    data[ctl_tx_ctl_test_pattern] = 0;
    axi.wr(CONFIGURATION_TX_REG1+offset,data);

  endtask : init_mac

endpackage : Pkg100gbMac

module model_100gbe (
  input  logic areset,
  // 156.25 Mhz refclk
  input  logic ref_clk,

  // QSFP high-speed IO
  output logic [3:0] tx_p,
  output logic [3:0] tx_n,
  input  logic [3:0] rx_p,
  input  logic [3:0] rx_n,

  // CLK and RESET out
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

  logic refclk_p;
  logic refclk_n;

  logic clk40,clk40_rst;
  logic clk100,clk100_rst;
  logic phy_reset;

  assign refclk_p =  ref_clk;
  assign refclk_n = ~ref_clk;

  //interface
  AxiLiteIf #(Pkg100gbMac::DATA_WIDTH,Pkg100gbMac::ADDR_WIDTH)
   mgt_axil (clk40, clk40_rst);
  //bfm
  Pkg100gbMac::MacAxiLiteBfm_t axi = new(.master(mgt_axil));
  TestExec mac_test = new();

  sim_clock_gen #(.PERIOD(25.0), .AUTOSTART(1))
    clk40_gen    (.clk(clk40), .rst(clk40_rst));
  sim_clock_gen #(.PERIOD(100.0), .AUTOSTART(1))
    clk100_gen    (.clk(clk100), .rst(clk100_rst));

  initial begin : init_model
    clk40_gen.reset();
    axi.run();
    wait(!clk40_rst);
    repeat (10) @(posedge clk40);

    wait(!phy_reset); // both usr_clk's are ok

    mac_test.start_test("model_100gbe::Wait for MAC link_up", 150us);
    //Added autoconnect - uncomment to test connecting over AXI
    //Pkg100gbMac::init_mac(0,axi);
    mac_test.end_test();
  end


  AxiStreamIf #(.DATA_WIDTH(512),.USER_WIDTH(7),.TKEEP(0))
    eth100g_rx(mgt_clk,mgt_rst);

  always_comb begin
    mgt_rx.tdata    = eth100g_rx.tdata;
    mgt_rx.tuser    = eth100g_rx.tuser;
    mgt_rx.tkeep    = eth100g_rx.trailing2keep(eth100g_rx.tuser);
    mgt_rx.tvalid   = eth100g_rx.tvalid;
    mgt_rx.tlast    = eth100g_rx.tlast;
    eth100g_rx.tready = mgt_rx.tready;
    // The MAC ignores hold off.  Data must be consumed every clock it is valid.
    if (!mgt_rst) begin
      if (!mgt_rx.tready && mgt_rx.tvalid) begin
        $error("Model 100Gbe : can't hold off the MAC");
      end
    end
  end

  // model does not pause.  Users could access this heirarchically to test it
  logic mgt_pause_req;
  assign mgt_pause_req = 1'b0;

  // hold off link up untill the stat_auto_config writes are complete
  logic link_up_model;
  logic [31:0] mac_status;
  always_comb begin
    link_up = link_up_model && mac_status[4];
  end

  eth_100g #(.PAUSE_QUANTA(10),.PAUSE_REFRESH(100)) eth_100gx (
    .areset(areset),
    //-- Free running 100 MHz clock used for InitClk and AxiLite to mac
    //-- 3.125 - 161.132812 MHz.
    .clk100(clk100),
    // MGT Reference Clock 100/125/156.25/161.1328125 MHz
    .refclk_p(refclk_p),
    .refclk_n(refclk_n),
    // MGT TX/RX differential signals
    .tx_p(tx_p),
    .tx_n(tx_n),
    .rx_p(rx_p),
    .rx_n(rx_n),
    // 322.26666 Mhz clock generated by 100G Phy from RefClock
    .mgt_clk(mgt_clk),
    .mgt_rst(mgt_rst),
    // pause
    .mgt_pause_req(mgt_pause_req),
    //------------------------ AXI Stream TX Interface ------------------------
    .mgt_tx(mgt_tx),
    //---------------------- AXI Stream RX Interface ------------------------
    // There is no RxTReady signal support by the Ethernet100G IP. Received data has to
    // be read immediately or it is lost.
    // tUser indicates an error on rcvd packet
    .mgt_rx(eth100g_rx),
    .mgt_axil(mgt_axil),
    // LEDs of QSFP28 port
    .phy_status(),
    .mac_ctrl(32'h01000001), // autoconfig / pause mask set to global
    .mac_status(mac_status),
    .phy_reset(phy_reset),
    .link_up(link_up_model)
  );

endmodule
