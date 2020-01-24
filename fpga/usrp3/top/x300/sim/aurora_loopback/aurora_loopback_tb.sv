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

module aurora_loopback_tb();
  `TEST_BENCH_INIT("aurora_loopback_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(XG_CLK_P, 1000/156.25, 50)  //156.25MHz GT transceiver clock
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
  //            ====> |------------|       |----------------|       || perfect serial channel
  // Loopback   |     | Aurora MAC | <===> | Aurora PCS/PMA | <====>||
  //            <==== |------------|       |----------------|

  // Initialize DUT
  wire aurora_refclk, aurora_init_clk;
  wire m_user_clk, s_user_clk;
  wire m_user_rst, s_user_rst;
  wire m_channel_up, s_channel_up;
  wire m_hard_err, s_hard_err;
  wire m_soft_err, s_soft_err;

  aurora_phy_clk_gen aurora_clk_gen_i (
    .areset(GSR),
    .refclk_p(XG_CLK_P),
    .refclk_n(XG_CLK_N),
    .refclk(aurora_refclk),
    .clk156(),
    .init_clk(aurora_init_clk)
  );

  wire [63:0] m_i_tdata, m_o_tdata;
  wire        m_i_tvalid, m_i_tready, m_o_tvalid;
  wire [63:0] s_i_tdata, s_o_tdata;
  wire        s_i_tvalid, s_i_tready, s_o_tvalid;
  wire [63:0] loop_tdata;
  wire        loop_tlast, loop_tvalid, loop_tready;
  wire [31:0] m_overruns, s_overruns;
  wire [31:0] m_soft_errors, s_soft_errors;
  reg         m_bist_gen, m_bist_check, s_bist_loopback;
  reg  [5:0]  m_bist_rate;
  wire        m_bist_locked;
  wire [47:0] m_bist_samps, m_bist_errors;


  cvita_master m_tx_chdr (.clk(m_user_clk));
  cvita_slave m_rx_chdr (.clk(s_user_clk));

  aurora_phy_x1 #(.SIMULATION(1)) aurora_phy_master_i (
    // Resets
    .areset(GSR),
    // Clocks
    .refclk(aurora_refclk),
    .user_clk(m_user_clk),
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
    .channel_up(m_channel_up), .hard_err(m_hard_err), .soft_err(m_soft_err)
  );

  aurora_axis_mac #(.PACKET_MODE(PACKET_MODE), .BIST_ENABLED(1)) aurora_mac_master_i (
    // Clocks and resets
    .phy_clk(m_user_clk), .phy_rst(m_user_rst),
    .sys_clk(m_user_clk), .sys_rst(m_user_rst),
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

  aurora_phy_x1 #(.SIMULATION(1)) aurora_phy_slave_i (
    // Resets
    .areset(GSR),
    // Clocks
    .refclk(aurora_refclk),
    .user_clk(s_user_clk),
    .init_clk(aurora_init_clk),
    .user_rst(s_user_rst),
    // GTX Serial I/O
    .tx_p(SFP_LN1_P), .tx_n(SFP_LN1_N),
    .rx_p(SFP_LN0_P), .rx_n(SFP_LN0_N),
    // AXI4-Stream TX Interface
    .s_axis_tdata(s_i_tdata), .s_axis_tvalid(s_i_tvalid), .s_axis_tready(s_i_tready),
    // AXI4-Stream RX Interface
    .m_axis_tdata(s_o_tdata), .m_axis_tvalid(s_o_tvalid),
    // AXI4-Lite Config Interface
    // AXI4-Lite Config Interface
    .s_axi_awaddr(32'h0), .s_axi_araddr(32'h0), .s_axi_awvalid(1'b0), .s_axi_awready(),
    .s_axi_wdata(32'h0), .s_axi_wvalid(1'b0), .s_axi_wstrb(1'b0), .s_axi_wready(),
    .s_axi_bvalid(), .s_axi_bresp(), .s_axi_bready(1'b1),
    .s_axi_arready(), .s_axi_arvalid(1'b0),
    .s_axi_rdata(), .s_axi_rvalid(), .s_axi_rresp(), .s_axi_rready(1'b1),
    // Status and Error Reporting Interface
    .channel_up(s_channel_up), .hard_err(s_hard_err), .soft_err(s_soft_err)
  );

  aurora_axis_mac #(.PACKET_MODE(PACKET_MODE), .BIST_ENABLED(1)) aurora_mac_slave_i (
    // Clocks and resets
    .phy_clk(s_user_clk), .phy_rst(s_user_rst),
    .sys_clk(s_user_clk), .sys_rst(s_user_rst),
    .clear(1'b0),
    // PHY Interface
    .phy_s_axis_tdata(s_o_tdata), .phy_s_axis_tvalid(s_o_tvalid),
    .phy_m_axis_tdata(s_i_tdata), .phy_m_axis_tvalid(s_i_tvalid), .phy_m_axis_tready(s_i_tready),
    // User Interface
    .s_axis_tdata(loop_tdata), .s_axis_tlast(loop_tlast),
    .s_axis_tvalid(~s_bist_loopback & loop_tvalid), .s_axis_tready(loop_tready),
    .m_axis_tdata(loop_tdata), .m_axis_tlast(loop_tlast),
    .m_axis_tvalid(loop_tvalid), .m_axis_tready(~s_bist_loopback & loop_tready),
    // Misc PHY
    .channel_up(s_channel_up), .hard_err(s_hard_err), .soft_err(s_soft_err),
    .overruns(s_overruns), .soft_errors(s_soft_errors),
    //BIST
    .bist_gen_en(1'b0), .bist_checker_en(1'b0), .bist_loopback_en(s_bist_loopback), .bist_gen_rate(6'd0),
    .bist_checker_locked(), .bist_checker_samps(), .bist_checker_errors()
  );

  //Testbench variables
  cvita_hdr_t   header, header_out;
  cvita_stats_t stats;
  logic [63:0]  crc_cache;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------
  initial begin : tb_main
    string s;
    `TEST_CASE_START("Wait for reset");
    while (GSR) @(posedge XG_CLK_P);
    `TEST_CASE_DONE((~GSR));

    m_bist_gen <= 1'b0;
    m_bist_rate <= 6'd0;
    m_bist_check <= 1'b0;
    s_bist_loopback <= 1'b0;

    m_tx_chdr.push_bubble();

    `TEST_CASE_START("Wait for master channel to come up");
    while (m_channel_up !== 1'b1) @(posedge m_user_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Wait for slave channel to come up");
    while (s_channel_up !== 1'b1) @(posedge s_user_clk);
    `TEST_CASE_DONE(1'b1);

    `TEST_CASE_START("Run PRBS BIST");
    s_bist_loopback <= PACKET_MODE;
    @(posedge m_user_clk);
    m_bist_rate <= 6'd60;
    m_bist_gen <= 1'b1;
    m_bist_check <= 1'b1;
    @(posedge m_user_clk);
    while (m_bist_locked !== 1'b1) @(posedge m_user_clk);
    repeat (512) @(posedge m_user_clk);
    `ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
    `ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
    @(posedge m_user_clk);
    m_bist_gen <= 1'b0;
    repeat (256) @(posedge m_user_clk);
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
      $sformat(s, "Bad packet: Wrong SID. Expected: %08x, Actual: %08x",
        {header.src_sid,header.dst_sid},{header_out.src_sid,header_out.dst_sid});
      `ASSERT_ERROR({header.src_sid,header.dst_sid}=={header_out.src_sid,header_out.dst_sid}, s);
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Fill up empty FIFO then drain (long packet)");
      m_rx_chdr.axis.tready = 0;
      m_tx_chdr.push_ramp_pkt(256, 64'd0, 64'h100, header);
      m_rx_chdr.axis.tready = 1;
      m_rx_chdr.wait_for_pkt_get_info(header_out, stats);
      `ASSERT_ERROR(stats.count==256,           "Bad packet: Length mismatch");
      $sformat(s, "Bad packet: Wrong SID. Expected: %08x, Actual: %08x",
        {header.src_sid,header.dst_sid},{header_out.src_sid,header_out.dst_sid});
      `ASSERT_ERROR({header.src_sid,header.dst_sid}=={header_out.src_sid,header_out.dst_sid}, s);
    `TEST_CASE_DONE(1);

    header = '{
      pkt_type:DATA, has_time:1, eob:0, seqnum:12'h666, 
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Concurrent read and write (single packet)");
      repeat (10) @(posedge m_user_clk); //Wait for clear to go low
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
    `ASSERT_ERROR(stats.count==200, "Bad packet: Length mismatch");
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
            `ASSERT_ERROR(stats.count==20,      "Bad packet: Length mismatch");
            `ASSERT_ERROR(crc_cache==stats.crc, "Bad packet: Wrong CRC");
          end
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Validate no drops (master)");
    `TEST_CASE_DONE((m_overruns === 32'd0));

    `TEST_CASE_START("Validate no drops (slave)");
    `TEST_CASE_DONE((s_overruns === 32'd0));

    s_bist_loopback <= 1'b1;

    `TEST_CASE_START("Run PRBS BIST (Loopback Mode)");
    @(posedge m_user_clk);
    m_bist_gen <= 1'b1;
    m_bist_rate <= 6'd60;
    m_bist_check <= 1'b1;
    @(posedge m_user_clk);
    while (m_bist_locked !== 1'b1) @(posedge m_user_clk);
    repeat (512) @(posedge m_user_clk);
    `ASSERT_ERROR(m_bist_samps>256, "BIST: Num samples incorrect");
    `ASSERT_ERROR(m_bist_errors===36'd0, "BIST: Errors!");
    @(posedge m_user_clk);
    m_bist_gen <= 1'b0;
    repeat (256) @(posedge m_user_clk);
    m_bist_check <= 1'b0;
    `TEST_CASE_DONE(1'b1);

    s_bist_loopback <= 1'b0;

    `TEST_CASE_START("Validate no drops (master)");
    `TEST_CASE_DONE((m_overruns === 32'd0));

    `TEST_CASE_START("Validate no drops (slave)");
    `TEST_CASE_DONE((s_overruns === 32'd0));

    `TEST_BENCH_DONE;
  end

endmodule
