//
// Copyright 2016 Ettus Research LLC
//


`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 13

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_cvita_lib.svh"
`include "sim_axi4_lib.svh"
`include "sim_set_rb_lib.svh"

module one_gig_eth_loopback_tb();
  `TEST_BENCH_INIT("one_gig_eth_loopback_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(ETH_CLK_P, 1000/125, 50)  //125MHz GT transceiver clock
  `DEFINE_RESET(GSR, 0, 100)              //100ns for GSR to deassert

  wire ETH_CLK_N = ~ETH_CLK_P;
  wire SFP_LN0_P, SFP_LN0_N, SFP_LN1_P, SFP_LN1_N;

  //localparam PACKET_MODE = 0;
  localparam PORTNUM = 8'd0;

  // One_gigE Loopback Topology:
  //
  // TB Simulus ====> |------------|       |----------------|
  //                  |  gigE MAC  | <===> |  gigE PCS/PMA  | <====>||
  // TB Checker <==== |------------|       |----------------|       || Loopback through
  //                                                                ||
  //            ====> |------------|       |----------------|       || perfect serial channel
  // Loopback   |     |  gigE MAC  | <===> |  gigE PCS/PMA  | <====>||
  //            <==== |------------|       |----------------|

  // Initialize DUT
  wire gige_refclk, gige_refclk_bufg;
  wire m_user_clk, s_user_clk;
  wire m_channel_up, s_channel_up;
  wire [7:0]  m_gmii_txd, m_gmii_rxd;
  wire        m_gmii_tx_en, m_gmii_tx_er, m_gmii_rx_dv, m_gmii_rx_er;
  wire        m_gmii_clk;
  wire [7:0]  s_gmii_txd, s_gmii_rxd;
  wire        s_gmii_tx_en, s_gmii_tx_er, s_gmii_rx_dv, s_gmii_rx_er;
  wire        s_gmii_clk;
  wire [15:0] m_phy_status;
  wire [15:0] s_phy_status;
  wire [63:0] loop_tdata;
  wire        loop_tlast, loop_tvalid, loop_tready;


  reg independent_clock;
  assign m_channel_up = m_phy_status[0];
  assign s_channel_up = s_phy_status[0];
  //assign m_user_clk = gmii_clk;
  //assign s_user_clk = gmii_clk;
  assign m_user_clk = independent_clock;
  assign s_user_clk = independent_clock;
  wire gt0_qplloutclk, gt0_qplloutrefclk, pma_reset;

  one_gige_phy_clk_gen gige_clk_gen_i (
     .areset(GSR),
     .refclk_p(ETH_CLK_P),
     .refclk_n(ETH_CLK_N),
     .refclk(gige_refclk),
     .refclk_bufg(gige_refclk_bufg)
  );

  cvita_master m_tx_chdr (.clk(m_user_clk));
  cvita_slave s_rx_chdr (.clk(s_user_clk));
  initial
  begin
    independent_clock <= 1'b0;
    forever
    begin
      independent_clock <= 1'b0;
      #0.5;
      independent_clock <= 1'b1;
      #0.5;
    end
  end

   //-----------------------------------------------------------------
   // MDIO Master
   //-----------------------------------------------------------------
   wire mdc, mdio_m2s, mdio_s2m;

   mdio_master #(
      .MDC_DIVIDER   (8'd200)
   ) mdio_master_i (
      .clk        (m_user_clk),
      .rst        (GSR),
      .mdc        (mdc),
      .mdio_in    (mdio_s2m),
      .mdio_out   (mdio_m2s),
      .mdio_tri   (),
      .reg_wr_req (/*reg_wr_req*/),
      .reg_wr_addr(/*reg_wr_addr*/),
      .reg_wr_data(/*reg_wr_data*/),
      .reg_rd_req (/*reg_rd_req*/),
      .reg_rd_addr(/*reg_rd_addr*/),
      .reg_rd_data(/*reg_rd_data*/),
      .reg_rd_resp(/*reg_rd_resp*/)
   );

   //GT COMMON
   one_gig_eth_pcs_pma_gt_common core_gt_common_i
   (
    .GTREFCLK0_IN                (gige_refclk) ,
    .QPLLLOCK_OUT                (),
    .QPLLLOCKDETCLK_IN           (independent_clock),
    .QPLLOUTCLK_OUT              (gt0_qplloutclk),
    .QPLLOUTREFCLK_OUT           (gt0_qplloutrefclk),
    .QPLLREFCLKLOST_OUT          (),
    .QPLLRESET_IN                (pma_reset)
   );

  one_gige_phy one_gige_phy_master_i
  (
     .reset(GSR),                  // Asynchronous reset for entire core.
     .independent_clock(independent_clock),
     .pma_reset_out(pma_reset),
     .gt0_qplloutclk_in(gt0_qplloutclk),
     .gt0_qplloutrefclk_in(gt0_qplloutrefclk),
     // Tranceiver Interface
     .gtrefclk(gige_refclk),            // Reference clock for MGT: 125MHz, very high quality.
     .gtrefclk_bufg(gige_refclk_bufg),       // Reference clock routed through a BUFG
     .txp(SFP_LN1_P),                       // Differential +ve of serial transmission from PMA to PMD.
     .txn(SFP_LN1_N),                       // Differential -ve of serial transmission from PMA to PMD.
     .rxp(SFP_LN0_P),                       // Differential +ve for serial reception from PMD to PMA.
     .rxn(SFP_LN0_N),                       // Differential -ve for serial reception from PMD to PMA.
     // GMII Interface (client MAC <=> PCS)
     .gmii_clk(m_gmii_clk),            // Clock to client MAC.
     .gmii_txd(m_gmii_txd),            // Transmit data from client MAC.
     .gmii_tx_en(m_gmii_tx_en),        // Transmit control signal from client MAC.
     .gmii_tx_er(m_gmii_tx_er),        // Transmit control signal from client MAC.
     .gmii_rxd(m_gmii_rxd),            // Received Data to client MAC.
     .gmii_rx_dv(m_gmii_rx_dv),        // Received control signal to client MAC.
     .gmii_rx_er(m_gmii_rx_er),        // Received control signal to client MAC.
     // Management: MDIO Interface
     .mdc(mdc),                      // Management Data Clock
     .mdio_i(mdio_m2s),               // Management Data In
     .mdio_o(mdio_s2m),              // Management Data Out
     .mdio_t(),                       // Management Data Tristate
     .configuration_vector(5'd0),     // Alternative to MDIO interface.
     .configuration_valid(1'b1),      // Validation signal for Config vector (MUST be 1 for proper functionality...undocumented)
     // General IO's
     .status_vector(m_phy_status),    // Core status.
     .signal_detect(1'b1 /*Optical module not supported*/) // Input from PMD to indicate presence of optical input.
  );

      simple_gemac_wrapper #(.RX_FLOW_CTRL(0), .PORTNUM(PORTNUM)) simple_gemac_wrapper_master_i
      (
         .clk125(m_gmii_clk),
         .reset(GSR),

         .GMII_GTX_CLK(),
         .GMII_TX_EN(m_gmii_tx_en),
         .GMII_TX_ER(m_gmii_tx_er),
         .GMII_TXD(m_gmii_txd),
         .GMII_RX_CLK(m_gmii_clk),
         .GMII_RX_DV(m_gmii_rx_dv),
         .GMII_RX_ER(m_gmii_rx_er),
         .GMII_RXD(m_gmii_rxd),

         .sys_clk(m_user_clk),
         .rx_tdata(s_rx_chdr.axis.tdata),
         .rx_tuser(/*s_rx_chdr.axis.tuser*/),
         .rx_tlast(s_rx_chdr.axis.tlast),
         .rx_tvalid(s_rx_chdr.axis.tvalid),
         .rx_tready(s_rx_chdr.axis.tready),
         .tx_tdata(m_tx_chdr.axis.tdata),
         .tx_tuser(/*m_tx_chdr.axis_tuser*/),
         .tx_tlast(m_tx_chdr.axis.tlast),
         .tx_tvalid(m_tx_chdr.axis.tvalid),
         .tx_tready(m_tx_chdr.axis.tready),
         // Debug
         .debug_tx(), .debug_rx()
      );

  one_gige_phy one_gige_phy_slave_i
  (
     .reset(GSR),                  // Asynchronous reset for entire core.
     .independent_clock(independent_clock),
     .pma_reset_out(),
     .gt0_qplloutclk_in(gt0_qplloutclk),
     .gt0_qplloutrefclk_in(gt0_qplloutrefclk),
     // Tranceiver Interface
     .gtrefclk(gige_refclk),            // Reference clock for MGT: 125MHz, very high quality.
     .gtrefclk_bufg(gige_refclk_bufg),       // Reference clock routed through a BUFG
     .txp(SFP_LN0_P),                       // Differential +ve of serial transmission from PMA to PMD.
     .txn(SFP_LN0_N),                       // Differential -ve of serial transmission from PMA to PMD.
     .rxp(SFP_LN1_P),                       // Differential +ve for serial reception from PMD to PMA.
     .rxn(SFP_LN1_N),                       // Differential -ve for serial reception from PMD to PMA.
     // GMII Interface (client MAC <=> PCS)
     .gmii_clk(s_gmii_clk),            // Clock to client MAC.
     .gmii_txd(s_gmii_txd),            // Transmit data from client MAC.
     .gmii_tx_en(s_gmii_tx_en),        // Transmit control signal from client MAC.
     .gmii_tx_er(s_gmii_tx_er),        // Transmit control signal from client MAC.
     .gmii_rxd(s_gmii_rxd),            // Received Data to client MAC.
     .gmii_rx_dv(s_gmii_rx_dv),        // Received control signal to client MAC.
     .gmii_rx_er(s_gmii_rx_er),        // Received control signal to client MAC.
     // Management: MDIO Interface
     .mdc(mdc),                      // Management Data Clock
     .mdio_i(mdio_m2s),               // Management Data In
     .mdio_o(mdio_s2m),              // Management Data Out
     .mdio_t(),                       // Management Data Tristate
     .configuration_vector(5'd0),     // Alternative to MDIO interface.
     .configuration_valid(1'b1),      // Validation signal for Config vector (MUST be 1 for proper functionality...undocumented)
     // General IO's
     .status_vector(s_phy_status),    // Core status.
     .signal_detect(1'b1 /*Optical module not supported*/) // Input from PMD to indicate presence of optical input.
  );

      simple_gemac_wrapper #(.RX_FLOW_CTRL(0), .PORTNUM(PORTNUM)) simple_gemac_wrapper_slave_i
      (
         .clk125(s_gmii_clk),
         .reset(GSR),

         .GMII_GTX_CLK(),
         .GMII_TX_EN(s_gmii_tx_en),
         .GMII_TX_ER(s_gmii_tx_er),
         .GMII_TXD(s_gmii_txd),
         .GMII_RX_CLK(s_gmii_clk),
         .GMII_RX_DV(s_gmii_rx_dv),
         .GMII_RX_ER(s_gmii_rx_er),
         .GMII_RXD(s_gmii_rxd),

         .sys_clk(s_user_clk),
         .rx_tdata(loop_tdata),
         .rx_tuser(),
         .rx_tlast(loop_tlast),
         .rx_tvalid(loop_tvalid),
         .rx_tready(loop_tready),
         .tx_tdata(loop_tdata),
         .tx_tuser(),
         .tx_tlast(loop_tlast),
         .tx_tvalid(loop_tvalid),
         .tx_tready(loop_tready),
         // Debug
         .debug_tx(), .debug_rx()
      );

  //Testbench variables
  cvita_hdr_t   header, header_out;
  cvita_stats_t stats;
  logic [63:0]  crc_cache;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------
  initial begin : tb_main
    `TEST_CASE_START("Wait for reset");
    while (GSR) @(posedge ETH_CLK_P);
    `TEST_CASE_DONE((~GSR));

    m_tx_chdr.push_bubble();

    `TEST_CASE_START("Wait for master channel to come up");
    while (m_channel_up !== 1'b1) @(posedge m_user_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Wait for slave channel to come up");
    while (s_channel_up !== 1'b1) @(posedge s_user_clk);
    `TEST_CASE_DONE(1'b1);

   // `TEST_CASE_START("Run PRBS15 BIST");
   // s_bist_loopback <= PACKET_MODE;
   // @(posedge m_user_clk);
   // m_bist_gen <= 1'b1;
   // m_bist_check <= 1'b1;
   // @(posedge m_user_clk);
   // while (m_bist_locked !== 1'b1) @(posedge m_user_clk);
   // repeat (512) @(posedge m_user_clk);
   // `ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
   // `ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
   // @(posedge m_user_clk);
   // m_bist_gen <= 1'b0;
   // repeat (256) @(posedge m_user_clk);
   // m_bist_check <= 1'b0;
   // `TEST_CASE_DONE(1'b1);

   repeat(1000) @(posedge m_user_clk);

   header = '{
      pkt_type:DATA, has_time:0, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Fill up empty FIFO then drain (short packet)");
      s_rx_chdr.axis.tready = 0;
      m_tx_chdr.push_ramp_pkt(16, 64'd0, 64'h100, header);
      s_rx_chdr.axis.tready = 1;
      s_rx_chdr.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==16,            "Bad packet: Length mismatch");
      `ASSERT_ERROR(header.dst_sid==header_out.dst_sid, "Bad packet: Wrong SID");
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Fill up empty FIFO then drain (long packet)");
      s_rx_chdr.axis.tready = 0;
      m_tx_chdr.push_ramp_pkt(256, 64'd0, 64'h100, header);
      s_rx_chdr.axis.tready = 1;
      s_rx_chdr.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==256,           "Bad packet: Length mismatch");
      `ASSERT_ERROR(header.dst_sid==header_out.dst_sid, "Bad packet: Wrong SID");
    `TEST_CASE_DONE(1);

    header = '{
      pkt_type:DATA, has_time:1, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Concurrent read and write (single packet)");
      s_rx_chdr.axis.tready = 1;
      fork
        begin
          m_tx_chdr.push_ramp_pkt(1000, 64'd0, 64'h100, header);
        end
        begin
          s_rx_chdr.wait_for_pkt_get_info(header_out, stats);
        end
      join
    crc_cache = stats.crc;    //Cache CRC for future test cases
    `ASSERT_ERROR(stats.count==1000, "Bad packet: Length mismatch");
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Concurrent read and write (multiple packets)");
      s_rx_chdr.axis.tready = 1;
      fork
        begin
          repeat (20) begin
            m_tx_chdr.push_ramp_pkt(20, 64'd0, 64'h100, header);
            m_tx_chdr.push_bubble();
          end
        end
        begin
          repeat (20) begin
            s_rx_chdr.wait_for_pkt_get_info(header_out, stats);
            `ASSERT_ERROR(stats.count==20,      "Bad packet: Length mismatch");
            `ASSERT_ERROR(crc_cache==stats.crc, "Bad packet: Wrong CRC");
          end
        end
      join
    `TEST_CASE_DONE(1);

    //`TEST_CASE_START("Validate no drops (master)");
    //`TEST_CASE_DONE((m_overruns === 32'd0));

    //`TEST_CASE_START("Validate no drops (slave)");
    //`TEST_CASE_DONE((s_overruns === 32'd0));

    //s_bist_loopback <= 1'b1;

    //`TEST_CASE_START("Run PRBS15 BIST (Loopback Mode)");
    //@(posedge m_user_clk);
    //m_bist_gen <= 1'b1;
    //m_bist_rate <= 5'd4;
    //m_bist_check <= 1'b1;
    //@(posedge m_user_clk);
    //while (m_bist_locked !== 1'b1) @(posedge m_user_clk);
    //repeat (512) @(posedge m_user_clk);
    //`ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
    //`ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
    //@(posedge m_user_clk);
    //m_bist_gen <= 1'b0;
    //repeat (256) @(posedge m_user_clk);
    //m_bist_check <= 1'b0;
    //`TEST_CASE_DONE(1'b1);

    //s_bist_loopback <= 1'b0;

    //`TEST_CASE_START("Validate no drops (master)");
    //`TEST_CASE_DONE((m_overruns === 32'd0));

    //`TEST_CASE_START("Validate no drops (slave)");
    //`TEST_CASE_DONE((s_overruns === 32'd0));

  end

endmodule
