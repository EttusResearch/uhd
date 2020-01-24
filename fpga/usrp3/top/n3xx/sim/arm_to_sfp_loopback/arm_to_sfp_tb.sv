//
// Copyright 2017 Ettus Research
//


`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 13

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_cvita_lib.svh"
`include "sim_axis_lib.svh"
`include "sim_axi4_lib.svh"
`include "sim_set_rb_lib.svh"

module arm_to_sfp_tb();
  `TEST_BENCH_INIT("arm_to_sfp_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(XG_CLK_P, 1000/156.25, 50)  //156.25MHz GT transceiver clock
  `DEFINE_RESET(GSR, 0, 100)              //100ns for GSR to deassert

  wire XG_CLK_N = ~XG_CLK_P;
  wire SFP_LN0_P, SFP_LN0_N, SFP_LN1_P, SFP_LN1_N;

  //localparam PACKET_MODE = 0;
  localparam PORTNUM = 8'd0;

  // ARM to SFP Loopback Topology:
  //
  // TB Simulus ====> |------------|       |----------------|
  //                  | XgigE MAC  | <===> | XgigE PCS/PMA  | <====>||
  // TB Checker <==== |------------|       |----------------|       || Loopback through
  //                                                                ||
  //            ====> |------------|       |----------------|       || perfect serial channel
  // Loopback   |     | XgigE MAC  | <===> | XgigE PCS/PMA  | <====>||
  //            <==== |------------|       |----------------|

  // Initialize DUT
  wire xgige_refclk, xgige_clk156, xgige_dclk;
  wire m_user_clk, s_user_clk;
  wire m_channel_up, s_channel_up;

  wire [63:0] m_xgmii_txd;
  wire [7:0]  m_xgmii_txc;
  wire [63:0] m_xgmii_rxd;
  wire [7:0]  m_xgmii_rxc;
  wire [63:0] s_xgmii_txd;
  wire [7:0]  s_xgmii_txc;
  wire [63:0] s_xgmii_rxd;
  wire [7:0]  s_xgmii_rxc;
  wire [7:0]  m_xgmii_status;
  wire [7:0]  s_xgmii_status;
  wire        m_xge_phy_resetdone;
  wire        s_xge_phy_resetdone;
  wire        m_mdc, m_mdio_in, m_mdio_out;
  wire        s_mdc, s_mdio_in, s_mdio_out;
  wire        sfpp_rxlos,sfpp_tx_fault,sfpp_tx_disable;


  wire [15:0] m_phy_status;
  wire [15:0] s_phy_status;
  wire [63:0] loop_tdata;
  wire [3:0]  loop_tuser;
  wire        loop_tlast, loop_tvalid, loop_tready;

  wire [7:0]      wb_adr_i;
  wire            wb_cyc_i;
  wire [31:0]     wb_dat_i;
  wire            wb_stb_i;
  wire            wb_we_i;
  wire            wb_ack_o;
  wire [31:0]     wb_dat_o;
  wire            wb_int_o;
  wire  [63:0]  c2e_tdata_int;
  wire  [3:0]   c2e_tuser_int;
  wire          c2e_tlast_int;
  wire          c2e_tvalid_int;
  wire          c2e_tready_int;
  wire  [63:0]  c2e_tdata;
  wire  [3:0]   c2e_tuser;
  wire          c2e_tlast;
  wire          c2e_tvalid;
  wire          c2e_tready;
  wire        m_axis_tvalid;
  wire        m_axis_tlast;
  wire [63:0] m_axis_tdata;
  wire        m_axis_tready;
  wire [3:0]  m_axis_tuser;

  reg independent_clock;
  assign m_channel_up = m_phy_status[0];
  assign s_channel_up = s_phy_status[0];
  //assign m_user_clk = xgige_refclk;
  //assign s_user_clk = xgige_refclk;
  assign m_user_clk = independent_clock;
  assign s_user_clk = independent_clock;

   ten_gige_phy_clk_gen xgige_clk_gen_i (
      .areset(GSR),
      .refclk_p(XG_CLK_P),
      .refclk_n(XG_CLK_N),
      .refclk(xgige_refclk),
      .clk156(xgige_clk156),
      .dclk(xgige_dclk)
   );

  axis_master #(.DWIDTH(34)) m_axis (.clk(m_user_clk));
  axis_slave #(.DWIDTH(68)) s_axis (.clk(s_user_clk));

  initial
  begin
    independent_clock <= 1'b0;
    forever
    begin
      independent_clock <= 1'b0;
      #2.5;
      independent_clock <= 1'b1;
      #2.5;
    end
  end

  assign sfpp_rxlos = 1'b0;
  assign sfpp_tx_fault = 1'b0;

  // Instantiate the 10GBASER/KR GT Common block
  ten_gig_eth_pcs_pma_gt_common # (
      .WRAPPER_SIM_GTRESET_SPEEDUP("TRUE") ) //Does not affect hardware
  ten_gig_eth_pcs_pma_gt_common_block
    (
     .refclk(xgige_refclk),
     .qpllreset(qpllreset),
     .qplllock(qplllock),
     .qplloutclk(qplloutclk),
     .qplloutrefclk(qplloutrefclk),
     .qpllrefclksel(3'b001 /*3'b101*GTSOUTHREFCLK0*/)
    );

      ten_gige_phy ten_gige_phy_master_i
      (
         // Clocks and Reset
         .areset(GSR),                 // Asynchronous reset for entire core.
         .refclk(xgige_refclk),              // Transciever reference clock: 156.25MHz
         .clk156(xgige_clk156),              // Globally buffered core clock: 156.25MHz
         .dclk(xgige_dclk),                 // Management/DRP clock: 78.125MHz
         .sim_speedup_control(~GSR),
         // GMII Interface (client MAC <=> PCS)
         .xgmii_txd(m_xgmii_txd),          // Transmit data from client MAC.
         .xgmii_txc(m_xgmii_txc),          // Transmit control signal from client MAC.
         .xgmii_rxd(m_xgmii_rxd),          // Received Data to client MAC.
         .xgmii_rxc(m_xgmii_rxc),          // Received control signal to client MAC.
         // Tranceiver Interface
         .txp(SFP_LN0_P),                  // Differential +ve of serial transmission from PMA to PMD.
         .txn(SFP_LN0_N),                  // Differential -ve of serial transmission from PMA to PMD.
         .rxp(SFP_LN1_P),                  // Differential +ve for serial reception from PMD to PMA.
         .rxn(SFP_LN1_N),                  // Differential -ve for serial reception from PMD to PMA.
         // Management: MDIO Interface
         .mdc(m_mdc),                      // Management Data Clock
         .mdio_in(m_mdio_in),              // Management Data In
         .mdio_out(m_mdio_out),            // Management Data Out
         .mdio_tri(),                     // Management Data Tristate
         .prtad(5'd4),                    // MDIO address is 4
         // General IO's
         .core_status(m_xgmii_status),     // Core status
         .resetdone(m_xge_phy_resetdone),
         .signal_detect(~sfpp_rxlos),     //FIXME // Input from PMD to indicate presence of optical input. (Undocumented, but it seems Xilinx expect this to be inverted.)
         .tx_fault(sfpp_tx_fault),       //FIXME
         .tx_disable(/*sfpp_tx_disable*/),    //FIXME
         .qpllreset(qpllreset1),
         .qplllock(qplllock),
         .qplloutclk(qplloutclk),
         .qplloutrefclk(qplloutrefclk)

      );

      n310_xge_mac_wrapper #(.PORTNUM(PORTNUM)) xge_mac_wrapper_master_i
      (
         // XGMII
         .xgmii_clk(xgige_clk156),
         .xgmii_txd(m_xgmii_txd),
         .xgmii_txc(m_xgmii_txc),
         .xgmii_rxd(m_xgmii_rxd),
         .xgmii_rxc(m_xgmii_rxc),
         // Client FIFO Interfaces
         .sys_clk(m_user_clk),
         .sys_rst(GSR),
         .rx_tdata(s_axis.axis.tdata[63:0]),
         .rx_tuser(s_axis.axis.tdata[67:64]),
         .rx_tlast(s_axis.axis.tlast),
         .rx_tvalid(s_axis.axis.tvalid),
         .rx_tready(/*s_axis.axis.tready*/1'b1),
         .tx_tdata(c2e_tdata),
         .tx_tuser(c2e_tuser),                // Bit[3] (error) is ignored for now.
         .tx_tlast(c2e_tlast),
         .tx_tvalid(c2e_tvalid),
         .tx_tready(c2e_tready),
         // Other
         .phy_ready(m_xge_phy_resetdone),
         .ctrl_tx_enable         (/*mac_ctrl_reg[0]*/1'b1), //FIXME: Remove hardcoded value
         .status_crc_error       (),
         .status_fragment_error  (),
         .status_txdfifo_ovflow  (),
         .status_txdfifo_udflow  (),
         .status_rxdfifo_ovflow  (),
         .status_rxdfifo_udflow  (),
         .status_pause_frame_rx  (),
         .status_local_fault     (),
         .status_remote_fault    ()
      );

      assign m_phy_status  = {8'h00, m_xgmii_status};

     axi_fifo32_to_fifo64 inst_axi_fifo32_to_fifo64
     (
       .clk(m_user_clk),
       .reset(GSR),
       .i_tdata({m_axis.axis.tdata[31:0]}), // endian swap
       .i_tuser(m_axis.axis.tdata[33:32]),
       .i_tlast(m_axis.axis.tlast),
       .i_tvalid(m_axis.axis.tvalid),
       .i_tready(m_axis.axis.tready),
       .o_tdata(m_axis_tdata),
       .o_tuser(m_axis_tuser),
       .o_tlast(m_axis_tlast),
       .o_tvalid(m_axis_tvalid),
       .o_tready(m_axis_tready)
     );

      arm_framer inst_arm_framer (
      .clk               (m_user_clk),
      .reset             (GSR),
      .clear             (clear),
      .s_axis_tdata      (m_axis_tdata),
      .s_axis_tuser      (m_axis_tuser),
      .s_axis_tlast      (m_axis_tlast),
      .s_axis_tvalid     (m_axis_tvalid),
      .s_axis_tready     (m_axis_tready),
      .m_axis_tdata      (c2e_tdata_int),
      .m_axis_tuser      (c2e_tuser_int),
      .m_axis_tlast      (c2e_tlast_int),
      .m_axis_tvalid     (c2e_tvalid_int),
      .m_axis_tready     (c2e_tready_int)
     );

     axi_mux4 #(.PRIO(0), .WIDTH(68)) eth_mux
       (.clk(m_user_clk), .reset(GSR), .clear(clear),
        .i0_tdata({c2e_tuser_int,c2e_tdata_int}), .i0_tlast(c2e_tlast_int), .i0_tvalid(c2e_tvalid_int), .i0_tready(c2e_tready_int),
        .i1_tdata(), .i1_tlast(), .i1_tvalid(), .i1_tready(),
        .i2_tdata(), .i2_tlast(), .i2_tvalid(), .i2_tready(),
        .i3_tdata(), .i3_tlast(), .i3_tvalid(1'b0), .i3_tready(),
        .o_tdata({c2e_tuser,c2e_tdata}), .o_tlast(c2e_tlast), .o_tvalid(c2e_tvalid), .o_tready(c2e_tready));

     ten_gige_phy ten_gige_phy_slave_i
      (
         // Clocks and Reset
         .areset(GSR),                 // Asynchronous reset for entire core.
         .refclk(xgige_refclk),              // Transciever reference clock: 156.25MHz
         .clk156(xgige_clk156),              // Globally buffered core clock: 156.25MHz
         .dclk(xgige_dclk),                 // Management/DRP clock: 78.125MHz
         .sim_speedup_control(~GSR),
         // GMII Interface (client MAC <=> PCS)
         .xgmii_txd(s_xgmii_txd),          // Transmit data from client MAC.
         .xgmii_txc(s_xgmii_txc),          // Transmit control signal from client MAC.
         .xgmii_rxd(s_xgmii_rxd),          // Received Data to client MAC.
         .xgmii_rxc(s_xgmii_rxc),          // Received control signal to client MAC.
         // Tranceiver Interface
         .txp(SFP_LN1_P),                       // Differential +ve of serial transmission from PMA to PMD.
         .txn(SFP_LN1_N),                       // Differential -ve of serial transmission from PMA to PMD.
         .rxp(SFP_LN0_P),                       // Differential +ve for serial reception from PMD to PMA.
         .rxn(SFP_LN0_N),                       // Differential -ve for serial reception from PMD to PMA.
         // Management: MDIO Interface
         .mdc(s_mdc),                      // Management Data Clock
         .mdio_in(s_mdio_in),              // Management Data In
         .mdio_out(s_mdio_out),            // Management Data Out
         .mdio_tri(),                     // Management Data Tristate
         .prtad(5'd4),                    // MDIO address is 4
         // General IO's
         .core_status(s_xgmii_status),     // Core status
         .resetdone(s_xge_phy_resetdone),
         .signal_detect(~sfpp_rxlos),     //FIXME // Input from PMD to indicate presence of optical input. (Undocumented, but it seems Xilinx expect this to be inverted.)
         .tx_fault(sfpp_tx_fault),       //FIXME
         .tx_disable(/*sfpp_tx_disable*/),    //FIXME
         .qpllreset(qpllreset2),
         .qplllock(qplllock),
         .qplloutclk(qplloutclk),
         .qplloutrefclk(qplloutrefclk)
      );

      n310_xge_mac_wrapper #(.PORTNUM(PORTNUM)) xge_mac_wrapper_slave_i
      (
         // XGMII
         .xgmii_clk(xgige_clk156),
         .xgmii_txd(s_xgmii_txd),
         .xgmii_txc(s_xgmii_txc),
         .xgmii_rxd(s_xgmii_rxd),
         .xgmii_rxc(s_xgmii_rxc),
         // Client FIFO Interfaces
         .sys_clk(s_user_clk),
         .sys_rst(GSR),
         .rx_tdata(loop_tdata),
         .rx_tuser(loop_tuser),
         .rx_tlast(loop_tlast),
         .rx_tvalid(loop_tvalid),
         .rx_tready(loop_tready),
         .tx_tdata(loop_tdata),
         .tx_tuser(loop_tuser),                // Bit[3] (error) is ignored for now.
         .tx_tlast(loop_tlast),
         .tx_tvalid(loop_tvalid),
         .tx_tready(loop_tready),
         // Other
         .phy_ready(s_xge_phy_resetdone),
         .ctrl_tx_enable         (/*mac_ctrl_reg[0]*/1'b1), //FIXME: Remove hardcoded value
         .status_crc_error       (),
         .status_fragment_error  (),
         .status_txdfifo_ovflow  (),
         .status_txdfifo_udflow  (),
         .status_rxdfifo_ovflow  (),
         .status_rxdfifo_udflow  (),
         .status_pause_frame_rx  (),
         .status_local_fault     (),
         .status_remote_fault    ()
      );

      assign s_phy_status  = {8'h00, s_xgmii_status};

  //Testbench variables
  cvita_hdr_t   header, header_out;
  cvita_stats_t stats;
  logic [63:0]  crc_cache;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------
  initial begin : tb_main
    `TEST_CASE_START("Wait for reset");
    m_axis.reset;
    while (GSR) @(posedge XG_CLK_P);
    `TEST_CASE_DONE((~GSR));

    m_axis.push_bubble();

    `TEST_CASE_START("Wait for master channel to come up");
    while (m_channel_up !== 1'b1) @(posedge m_user_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Wait for slave channel to come up");
    while (s_channel_up !== 1'b1) @(posedge s_user_clk);
    `TEST_CASE_DONE(1'b1);

   repeat(2000) @(posedge m_user_clk);

    //`TEST_CASE_START("Test Ethernet packet");
    //  s_axis.axis.tready = 0;
    //  m_axis.push_word({4'b0, 64'h0000_0000_0000_ffff}, 1'b0);
    //  s_axis.axis.tready = 1;
    //  m_axis.push_word({4'b0, 64'hffff_ffff_ce20_ad1b}, 1'b0);
    //  m_axis.push_word({4'b0, 64'hc57a_0806_0001_0800}, 1'b0);
    //  m_axis.push_word({4'b0, 64'h0604_0001_ce20_ad1b}, 1'b0);
    //  m_axis.push_word({4'b0, 64'hc57a_c0a8_0a64_0000}, 1'b0);
    //  m_axis.push_word({4'b0, 64'h0000_0000_c0a8_0a0a}, 1'b1);
    //`TEST_CASE_DONE(1'b1);
    //`TEST_CASE_START("Test Ethernet packet");
    //  s_axis.axis.tready = 0;
    //  m_axis.push_word({4'b0, 64'hffff_ffff_ffff_ce20}, 1'b0);
    //  s_axis.axis.tready = 1;
    //  m_axis.push_word({4'b0, 64'had1b_c57a_0806_0001}, 1'b0);
    //  m_axis.push_word({4'b0, 64'h0800_0604_0001_ce20}, 1'b0);
    //  m_axis.push_word({4'b0, 64'had1b_c57a_c0a8_0a64}, 1'b0);
    //  m_axis.push_word({4'b0, 64'h0000_0000_0000_c0a8}, 1'b0);
    //  m_axis.push_word({4'd2, 64'h0a0a_1111_1111_1111}, 1'b1);
    //`TEST_CASE_DONE(1'b1);
    // repeat(2000) @(posedge m_user_clk);
    `TEST_CASE_START("Test Ethernet packet");
      s_axis.axis.tready = 0;
      m_axis.push_word({2'b0, 32'hffff_ffff}, 1'b0);
      s_axis.axis.tready = 1;
      m_axis.push_word({2'b0, 32'hffff_9aa9}, 1'b0);
      m_axis.push_word({2'b0, 32'h6400_e341}, 1'b0);
      m_axis.push_word({2'b0, 32'h0800_4500}, 1'b0);
      m_axis.push_word({2'b0, 32'h0148_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_4011}, 1'b0);
      m_axis.push_word({2'b0, 32'h79a6_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_ffff}, 1'b0);
      m_axis.push_word({2'b0, 32'hffff_0044}, 1'b0);
      m_axis.push_word({2'b0, 32'h0043_0134}, 1'b0);
      m_axis.push_word({2'b0, 32'h90be_0101}, 1'b0);
      m_axis.push_word({2'b0, 32'h0600_d2ab}, 1'b0);
      m_axis.push_word({2'b0, 32'h9f01_0007}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_9aa9}, 1'b0);
      m_axis.push_word({2'b0, 32'h6400_e341}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_6382}, 1'b0);
      m_axis.push_word({2'b0, 32'h5363_3501}, 1'b0);
      m_axis.push_word({2'b0, 32'h013d_0701}, 1'b0);
      m_axis.push_word({2'b0, 32'h9aa9_6400}, 1'b0);
      m_axis.push_word({2'b0, 32'he341_3902}, 1'b0);
      m_axis.push_word({2'b0, 32'h0240_3707}, 1'b0);
      m_axis.push_word({2'b0, 32'h0103_060c}, 1'b0);
      m_axis.push_word({2'b0, 32'h0f1c_2a3c}, 1'b0);
      m_axis.push_word({2'b0, 32'h0c75_6468}, 1'b0);
      m_axis.push_word({2'b0, 32'h6370_2031}, 1'b0);
      m_axis.push_word({2'b0, 32'h2e32_342e}, 1'b0);
      m_axis.push_word({2'b0, 32'h31ff_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'b0, 32'h0000_0000}, 1'b0);
      m_axis.push_word({2'd2, 32'h0000_0000}, 1'b1);

    `TEST_CASE_DONE(1'b1);

    repeat(2000) @(posedge m_user_clk);

    `TEST_CASE_START("Test Ethernet packet");
      s_axis.axis.tready = 0;
      m_axis.push_word({2'b0, 32'h01005e7f }, 1'b0);
      s_axis.axis.tready = 1;
      m_axis.push_word({2'b0, 32'hfffa90e2 }, 1'b0);
      m_axis.push_word({2'b0, 32'hba3aa7e8 }, 1'b0);
      m_axis.push_word({2'b0, 32'h08004500 }, 1'b0);
      m_axis.push_word({2'b0, 32'h00c7515e }, 1'b0);
      m_axis.push_word({2'b0, 32'h40000111 }, 1'b0);
      m_axis.push_word({2'b0, 32'h6d1bc0a8 }, 1'b0);
      m_axis.push_word({2'b0, 32'h0a0aefff }, 1'b0);
      m_axis.push_word({2'b0, 32'hfffad40f }, 1'b0);
      m_axis.push_word({2'b0, 32'h076c00b3 }, 1'b0);
      m_axis.push_word({2'b0, 32'hbb714d2d }, 1'b0);
      m_axis.push_word({2'b0, 32'h53454152 }, 1'b0);
      m_axis.push_word({2'b0, 32'h4348202a }, 1'b0);
      m_axis.push_word({2'b0, 32'h20485454 }, 1'b0);
      m_axis.push_word({2'b0, 32'h502f312e }, 1'b0);
      m_axis.push_word({2'b0, 32'h310d0a48 }, 1'b0);
      m_axis.push_word({2'b0, 32'h4f53543a }, 1'b0);
      m_axis.push_word({2'b0, 32'h20323339 }, 1'b0);
      m_axis.push_word({2'b0, 32'h2e323535 }, 1'b0);
      m_axis.push_word({2'b0, 32'h2e323535 }, 1'b0);
      m_axis.push_word({2'b0, 32'h2e323530 }, 1'b0);
      m_axis.push_word({2'b0, 32'h3a313930 }, 1'b0);
      m_axis.push_word({2'b0, 32'h300d0a4d }, 1'b0);
      m_axis.push_word({2'b0, 32'h414e3a20 }, 1'b0);
      m_axis.push_word({2'b0, 32'h22737364 }, 1'b0);
      m_axis.push_word({2'b0, 32'h703a6469 }, 1'b0);
      m_axis.push_word({2'b0, 32'h73636f76 }, 1'b0);
      m_axis.push_word({2'b0, 32'h6572220d }, 1'b0);
      m_axis.push_word({2'b0, 32'h0a4d583a }, 1'b0);
      m_axis.push_word({2'b0, 32'h20310d0a }, 1'b0);
      m_axis.push_word({2'b0, 32'h53543a20 }, 1'b0);
      m_axis.push_word({2'b0, 32'h75726e3a }, 1'b0);
      m_axis.push_word({2'b0, 32'h6469616c }, 1'b0);
      m_axis.push_word({2'b0, 32'h2d6d756c }, 1'b0);
      m_axis.push_word({2'b0, 32'h74697363 }, 1'b0);
      m_axis.push_word({2'b0, 32'h7265656e }, 1'b0);
      m_axis.push_word({2'b0, 32'h2d6f7267 }, 1'b0);
      m_axis.push_word({2'b0, 32'h3a736572 }, 1'b0);
      m_axis.push_word({2'b0, 32'h76696365 }, 1'b0);
      m_axis.push_word({2'b0, 32'h3a646961 }, 1'b0);
      m_axis.push_word({2'b0, 32'h6c3a310d }, 1'b0);
      m_axis.push_word({2'b0, 32'h0a555345 }, 1'b0);
      m_axis.push_word({2'b0, 32'h522d4147 }, 1'b0);
      m_axis.push_word({2'b0, 32'h454e543a }, 1'b0);
      m_axis.push_word({2'b0, 32'h20476f6f }, 1'b0);
      m_axis.push_word({2'b0, 32'h676c6520 }, 1'b0);
      m_axis.push_word({2'b0, 32'h4368726f }, 1'b0);
      m_axis.push_word({2'b0, 32'h6d652f35 }, 1'b0);
      m_axis.push_word({2'b0, 32'h342e302e }, 1'b0);
      m_axis.push_word({2'b0, 32'h32383430 }, 1'b0);
      m_axis.push_word({2'b0, 32'h2e353920 }, 1'b0);
      m_axis.push_word({2'b0, 32'h4c696e75 }, 1'b0);
      m_axis.push_word({2'b1, 32'h78111111 }, 1'b1);

    `TEST_CASE_DONE(1'b1);
  end

endmodule
