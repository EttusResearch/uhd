/////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
/////////////////////////////////////////////////////////////////



`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 13

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_cvita_lib.svh"
`include "sim_axi4_lib.svh"
`include "sim_set_rb_lib.svh"

module aurora_loopback_tb();
  `TEST_BENCH_INIT("aurora_loopback_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(XG_CLK_P, 1000/156.25, 50)  //156.25MHz GT transceiver clock
  `DEFINE_CLK(bus_clk, 1000/200, 50)      //define 200 MHz bus clk
  `DEFINE_RESET(GSR, 0, 100)              //100ns for GSR to deassert

  wire XG_CLK_N = ~XG_CLK_P;
  wire SFP_LN0_P, SFP_LN0_N, SFP_LN1_P, SFP_LN1_N;

  localparam PACKET_MODE = 1;

  // Aurora Loopback Topology:
  //
  // TB Simulus ====> |------------|       |----------------|
  //                  | Aurora MAC | <===> | Aurora PCS/PMA | <====>||
  // TB Checker <==== |------------|       |----------------|       || Loopback through
  //                                                                ||
  //
  //                  |-------------------------------------|
  //                  |      n3xx_npio_qsfp_wrapper         |
  //            ====> |------------|       |----------------|       || perfect serial channel
  // Loopback   |     | Aurora MAC | <===> | Aurora PCS/PMA | <====>||
  //            <==== |------------|       |----------------|
  //                  |-------------------------------------|

  // Initialize DUT
  wire aurora_refclk, aurora_init_clk;
  wire aurora_clk156;
  wire m_user_rst, s_user_rst;
  wire m_channel_up, s_channel_up;
  wire m_hard_err, s_hard_err;
  wire m_soft_err, s_soft_err;
  wire s_link_up;

(* dont_touch = "true" *) IBUFDS_GTE2 aurora_refclk_ibuf (
    .ODIV2(),
    .CEB  (1'b0),
    .I (XG_CLK_P),
    .IB(XG_CLK_N),
    .O (aurora_refclk)
  );
  
  aurora_phy_clk_gen aurora_clk_gen_i (
    .refclk_ibuf(aurora_refclk),
    .clk156(aurora_clk156),
    .init_clk(aurora_init_clk)
  );
  wire               qpllreset, qpllreset_slave;
  wire               qplllock, qplllock_slave;
  wire               qplloutclk, qplloutclk_slave;
  wire               qplloutrefclk, qplloutrefclk_slave;
  wire               qpllrefclklost, qpllrefclklost_slave;
  wire    [7:0]      qpll_drpaddr_in_i = 8'h0;
  wire    [15:0]     qpll_drpdi_in_i = 16'h0;
  wire               qpll_drpen_in_i =  1'b0;
  wire               qpll_drpwe_in_i =  1'b0;
  wire    [15:0]     qpll_drpdo_out_i, qpll_drpdo_out_s_i;
  wire               qpll_drprdy_out_i, qpll_drprdy_out_s_i;

  aurora_64b66b_pcs_pma_gt_common_wrapper gt_common_support (
    .gt_qpllclk_quad1_out      (qplloutclk), //to sfp
    .gt_qpllrefclk_quad1_out   (qplloutrefclk), // to sfp
    .GT0_GTREFCLK0_COMMON_IN   (aurora_refclk),
//----------------------- Common Block - QPLL Ports ------------------------
    .GT0_QPLLLOCK_OUT          (qplllock), //from 1st sfp
    .GT0_QPLLRESET_IN          (qpllreset), //from 1st sfp
    .GT0_QPLLLOCKDETCLK_IN     (aurora_init_clk),
    .GT0_QPLLREFCLKLOST_OUT    (qpllrefclklost), //from 1st sfp
//---------------------- Common DRP Ports ---------------------- //not really used???
    .qpll_drpaddr_in           (qpll_drpaddr_in_i),
    .qpll_drpdi_in             (qpll_drpdi_in_i),
    .qpll_drpclk_in            (aurora_init_clk),
    .qpll_drpdo_out            (qpll_drpdo_out_i),
    .qpll_drprdy_out           (qpll_drprdy_out_i),
    .qpll_drpen_in             (qpll_drpen_in_i),
    .qpll_drpwe_in             (qpll_drpwe_in_i)
  );
  
  wire au_master_tx_out_clk, au_slave_tx_out_clk;
  wire au_master_gt_pll_lock, au_slave_gt_pll_lock;
  wire au_user_clk, au_sync_clk, au_mmcm_locked;
  wire au_s_user_clk, au_s_sync_clk, au_s_mmcm_locked;
  wire au_master_phy_reset;

  aurora_phy_mmcm aurora_phy_mmcm_0 (
    .aurora_tx_clk_unbuf(au_master_tx_out_clk),
    .mmcm_reset(!au_master_gt_pll_lock),
    .user_clk(au_user_clk),
    .sync_clk(au_sync_clk),
    .mmcm_locked(au_mmcm_locked)
  );
  
  wire [63:0] m_i_tdata, m_o_tdata;
  wire        m_i_tvalid, m_i_tready, m_o_tvalid;
  wire [63:0] s_i_tdata, s_o_tdata;
  wire        s_i_tvalid, s_i_tready, s_o_tvalid;
  wire [63:0] loop_tdata;
  wire        loop_tlast, loop_tvalid, loop_tready;
  wire [31:0] m_overruns;
  reg  [31:0] s_overruns;
  wire [31:0] m_soft_errors, s_soft_errors;
  reg         m_bist_gen, m_bist_check, s_bist_loopback;
  reg  [5:0]  m_bist_rate;
  wire        m_bist_locked;
  wire [47:0] m_bist_samps, m_bist_errors;

  cvita_master m_tx_chdr (.clk(au_user_clk));
  cvita_slave m_rx_chdr (.clk(au_user_clk));

  aurora_phy_x1 #(.SIMULATION(1)) aurora_phy_master_i (
    // Resets
    .areset(GSR),
    // Clocks
    .refclk(aurora_refclk),
    .qpllclk(qplloutclk),
    .qpllrefclk(qplloutrefclk),
    .user_clk(au_user_clk),
    .sync_clk(au_sync_clk),
    .init_clk(aurora_init_clk),
    .user_rst(m_user_rst),
    // GTX Serial I/O
    .tx_p(SFP_LN0_P), .tx_n(SFP_LN0_N),
    .rx_p(SFP_LN1_P), .rx_n(SFP_LN1_N),
    // AXI4-Stream TX Interface
    .s_axis_tdata(m_i_tdata), .s_axis_tvalid(m_i_tvalid), .s_axis_tready(m_i_tready),
    // AXI4-Stream RX Interface
    .m_axis_tdata(m_o_tdata), .m_axis_tvalid(m_o_tvalid),
    // AXI4-Lite Config Interface
    .s_axi_awaddr(32'h0), .s_axi_araddr(32'h0), .s_axi_awvalid(1'b0), .s_axi_awready(),
    .s_axi_wdata(32'h0), .s_axi_wvalid(1'b0), .s_axi_wstrb(1'b0), .s_axi_wready(),
    .s_axi_bvalid(), .s_axi_bresp(), .s_axi_bready(1'b1),
    .s_axi_arready(), .s_axi_arvalid(1'b0),
    .s_axi_rdata(), .s_axi_rvalid(), .s_axi_rresp(), .s_axi_rready(1'b1),
    // Status and Error Reporting Interface
    .channel_up(m_channel_up), .hard_err(m_hard_err), .soft_err(m_soft_err),
    .qplllock(qplllock),
    .qpllreset(qpllreset),
    .qpllrefclklost(qpllrefclklost),
    .tx_out_clk(au_master_tx_out_clk),
    .gt_pll_lock(au_master_gt_pll_lock),
    .mmcm_locked(au_mmcm_locked)
  );

  aurora_axis_mac #(.PACKET_MODE(PACKET_MODE), .BIST_ENABLED(1)) aurora_mac_master_i (
    // Clocks and resets
    .phy_clk(au_user_clk), .phy_rst(m_user_rst),
    .sys_clk(au_user_clk), .sys_rst(m_user_rst),
    .clear(1'b0),
    // PHY Interface
    .phy_s_axis_tdata(m_o_tdata), .phy_s_axis_tvalid(m_o_tvalid),
    .phy_m_axis_tdata(m_i_tdata), .phy_m_axis_tvalid(m_i_tvalid), .phy_m_axis_tready(m_i_tready),
    // User Interface
    .s_axis_tdata(m_tx_chdr.axis.tdata), .s_axis_tlast(m_tx_chdr.axis.tlast),
    .s_axis_tvalid(m_tx_chdr.axis.tvalid), .s_axis_tready(m_tx_chdr.axis.tready),
    .m_axis_tdata(m_rx_chdr.axis.tdata), .m_axis_tlast(m_rx_chdr.axis.tlast),
    .m_axis_tvalid(m_rx_chdr.axis.tvalid), .m_axis_tready(m_rx_chdr.axis.tready),
    // Misc PHY
    .channel_up(m_channel_up), .hard_err(m_hard_err), .soft_err(m_soft_err),
    .overruns(m_overruns), .soft_errors(m_soft_errors),
    //BIST
    .bist_gen_en(m_bist_gen), .bist_checker_en(m_bist_check), .bist_loopback_en(1'b0), .bist_gen_rate(m_bist_rate),
    .bist_checker_locked(m_bist_locked), .bist_checker_samps(m_bist_samps), .bist_checker_errors(m_bist_errors)
  );

reg         reg_wr_req_s;
reg  [13:0] reg_wr_addr_s;
reg  [31:0] reg_wr_data_s;
reg         reg_rd_req_s;
reg  [13:0] reg_rd_addr_s;
wire        reg_rd_resp_s;
wire [31:0] reg_rd_data_s;

n3xx_npio_qsfp_wrapper #(
   .LANES(1),      // Number of lanes of Aurora to instantiate (Supported = {1,2,3,4})
   .REG_BASE(32'h0),  // Base register address
   .PORTNUM_BASE(4),      // Base port number for discovery
   .REG_DWIDTH(32),     // Width of regport address bus
   .REG_AWIDTH(14)      // Width of regport data bus
) qsfp_wrapper_inst (
  // Clocks and Resets
  .areset(GSR),
  .bus_clk(bus_clk),
  .misc_clk(aurora_init_clk),
  .bus_rst(GSR),
  .gt_refclk(aurora_refclk),
  .gt_clk156(aurora_clk156),
  // Serial lanes
  .txp(SFP_LN1_P),
  .txn(SFP_LN1_N),
  .rxp(SFP_LN0_P),
  .rxn(SFP_LN0_N),
  // AXIS input interface
  .s_axis_tdata(loop_tdata),
  .s_axis_tlast(loop_tlast),
  .s_axis_tvalid(~s_bist_loopback & loop_tvalid),
  .s_axis_tready(loop_tready),
  // AXIS output interface
  .m_axis_tdata(loop_tdata),
  .m_axis_tlast(loop_tlast),
  .m_axis_tvalid(loop_tvalid),
  .m_axis_tready(~s_bist_loopback & loop_tready),
  // Register ports
  .reg_wr_req(reg_wr_req_s),  //input                   reg_wr_req,
  .reg_wr_addr(reg_wr_addr_s), //input  [REG_AWIDTH-1:0] reg_wr_addr,
  .reg_wr_data(reg_wr_data_s), //input  [REG_DWIDTH-1:0] reg_wr_data,
  .reg_rd_req(reg_rd_req_s),  //input                   reg_rd_req,
  .reg_rd_addr(reg_rd_addr_s), //input  [REG_AWIDTH-1:0] reg_rd_addr,
  .reg_rd_resp(reg_rd_resp_s), //output                  reg_rd_resp,
  .reg_rd_data(reg_rd_data_s), //output [REG_DWIDTH-1:0] reg_rd_data,

  .link_up(s_link_up),
  .activity()
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
    while (GSR) @(posedge aurora_refclk);
    `TEST_CASE_DONE((~GSR));

    m_bist_gen <= 1'b0;
    m_bist_rate <= 6'd0;
    m_bist_check <= 1'b0;
    s_bist_loopback <= 1'b0;

    m_tx_chdr.push_bubble();

    `TEST_CASE_START("Wait for master channel to come up");
    while (m_channel_up !== 1'b1) @(posedge au_user_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Wait for slave channel to come up. Uses QSFP Wrapper");
    while (s_link_up !== 1'b1) @(posedge bus_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Run PRBS BIST");
    s_bist_loopback <= PACKET_MODE;
    $display("Need to interact with regport to set s_bist_loopback.");
    reg_wr_req_s <= 1;
    reg_wr_addr_s <= 4;
    reg_wr_data_s <= 4;
    @(posedge bus_clk);
    repeat (3) @(posedge au_user_clk);
    reg_wr_req_s <= 0;
    m_bist_rate <= 6'd60;
    m_bist_gen <= 1'b1;
    m_bist_check <= 1'b1;
    @(posedge au_user_clk);
    while (m_bist_locked !== 1'b1) @(posedge au_user_clk);
    repeat (512) @(posedge au_user_clk);
    `ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
    `ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
    @(posedge au_user_clk);
    m_bist_gen <= 1'b0;
    repeat (256) @(posedge au_user_clk);
    m_bist_check <= 1'b0;
    `TEST_CASE_DONE(1'b1);

    header = '{
      pkt_type:DATA, has_time:0, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Fill up empty FIFO then drain (short packet)");
      m_rx_chdr.axis.tready = 0;
      m_tx_chdr.push_ramp_pkt(16, 64'd0, 64'h100, header);
      m_rx_chdr.axis.tready = 1;
      m_rx_chdr.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==16,            "Bad packet: Length mismatch");
      `ASSERT_ERROR(header.src_sid==header_out.src_sid, "Bad packet: Wrong Src SID");
      `ASSERT_ERROR(header.dst_sid==header_out.dst_sid, "Bad packet: Wrong Dst SID");
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Fill up empty FIFO then drain (long packet)");
      m_rx_chdr.axis.tready = 0;
      m_tx_chdr.push_ramp_pkt(256, 64'd0, 64'h100, header);
      m_rx_chdr.axis.tready = 1;
      m_rx_chdr.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==256,           "Bad packet: Length mismatch");
      `ASSERT_ERROR(header.src_sid==header_out.src_sid, "Bad packet: Wrong Src SID");
      `ASSERT_ERROR(header.dst_sid==header_out.dst_sid, "Bad packet: Wrong Dst SID");
    `TEST_CASE_DONE(1);

    header = '{
      pkt_type:DATA, has_time:1, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Concurrent read and write (single packet)");
      repeat (10) @(posedge au_user_clk); //Wait for clear to go low
      m_rx_chdr.axis.tready = 1;
      fork
        begin
          m_tx_chdr.push_ramp_pkt(200, 64'd0, 64'h100, header);
        end
        begin
          m_rx_chdr.wait_for_pkt_get_info(header_out, stats);
        end
      join
    crc_cache = stats.crc;    //Cache CRC for future test cases
    `ASSERT_ERROR(stats.count==201, "Bad packet: Length mismatch");
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Concurrent read and write (multiple packets)");
      m_rx_chdr.axis.tready = 1;
      fork
        begin
          repeat (20) begin
            m_tx_chdr.push_ramp_pkt(20, 64'd0, 64'h100, header);
            m_tx_chdr.push_bubble();
          end
        end
        begin
          repeat (20) begin
            m_rx_chdr.wait_for_pkt_get_info(header_out, stats);
            `ASSERT_ERROR(stats.count==21,      "Bad packet: Length mismatch");
            `ASSERT_ERROR(crc_cache==stats.crc, "Bad packet: Wrong CRC");
          end
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Validate no drops (master)");
    `TEST_CASE_DONE((m_overruns === 32'd0));

    `TEST_CASE_START("Validate no drops (slave)");
    reg_rd_req_s <= 1;
    reg_rd_addr_s <= 'h20;
    @(posedge reg_rd_resp_s)
    reg_rd_req_s <= 0;  
    s_overruns <= reg_rd_data_s;
    `TEST_CASE_DONE((s_overruns === 32'd0));

    s_bist_loopback <= 1'b1;

    `TEST_CASE_START("Run PRBS BIST (Loopback Mode)");
    @(posedge au_user_clk);
    m_bist_gen <= 1'b1;
    m_bist_rate <= 6'd60;
    m_bist_check <= 1'b1;
    @(posedge au_user_clk);
    while (m_bist_locked !== 1'b1) @(posedge au_user_clk);
    repeat (512) @(posedge au_user_clk);
    `ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
    `ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
    @(posedge au_user_clk);
    m_bist_gen <= 1'b0;
    repeat (256) @(posedge au_user_clk);
    m_bist_check <= 1'b0;
    `TEST_CASE_DONE(1'b1);

    s_bist_loopback <= 1'b0;

    `TEST_CASE_START("Validate no drops (master)");
    `TEST_CASE_DONE((m_overruns === 32'd0));

    `TEST_CASE_START("Validate no drops (slave)");
    reg_rd_req_s <= 1;
    reg_rd_addr_s <= 'h20;
    @(posedge reg_rd_resp_s)
    reg_rd_req_s <= 0;  
    s_overruns <= reg_rd_data_s;
    `TEST_CASE_DONE((s_overruns === 32'd0));

    `TEST_BENCH_DONE;
  end

endmodule
