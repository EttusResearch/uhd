//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later


`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 7

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_set_rb_lib.svh"
`include "sim_axis_lib.svh"

`define SIM_TIMEOUT_US 1000000 // Default: 1s

module crossbar_tb #(
  parameter TEST_NAME          = "crossbar_tb",
  // Router parameters
  parameter ROUTER_IMPL        = "axi_crossbar", // Router implementation
  parameter ROUTER_PORTS       = 10,             // # Router ports
  parameter ROUTER_DWIDTH      = 64,             // Router datapath width
  parameter MTU_LOG2           = 7,              // log2 of max packet size for router
  parameter NUM_MASTERS        = ROUTER_PORTS,   // Number of data generators in test
  // Test parameters
  parameter TEST_MAX_PACKETS   = 50,             // How many packets to stream per test case?
  parameter TEST_LPP           = 50,             // Lines per packet
  parameter TEST_MIN_INJ_RATE  = 60,             // Minimum injection rate to test
  parameter TEST_MAX_INJ_RATE  = 100,            // Maximum injection rate to test
  parameter TEST_INJ_RATE_INCR = 10,             // Injection rate increment
  parameter TEST_GEN_LL_FILES  = 0               // Generate files to produce load-latency graphs?

)(
  /* no IO */
);
  `TEST_BENCH_INIT(TEST_NAME,`NUM_TEST_CASES,`NS_PER_TICK)

  //----------------------------------------------------
  // General test setup
  //----------------------------------------------------

  // Clocks and reset
  `DEFINE_CLK(clk, 5.000, 50)
  `DEFINE_RESET(rst, 0, 10)

  // Timekeeper (cycle counter)
  logic [63:0] timestamp;
  initial begin : timekeeper_blk
    while (rst) @(posedge clk);
    timestamp = 'd0;
    while (~rst) begin
      @(posedge clk);
      timestamp = timestamp + 'd1;
    end
  end

  //----------------------------------------------------
  // Instantiate traffic generators, checkers, buses
  //----------------------------------------------------
  localparam FILE_PATH = {`WORKING_DIR, "/data/", ROUTER_IMPL};

  // Data buses
  axis_t #(.DWIDTH(ROUTER_DWIDTH), .NUM_STREAMS(ROUTER_PORTS)) src2rtr_axis (.clk(clk));
  axis_t #(.DWIDTH(ROUTER_DWIDTH), .NUM_STREAMS(ROUTER_PORTS)) rtr2snk_axis (.clk(clk));
  
  // Control buses
  settings_bus_master #(.SR_AWIDTH(16), .SR_DWIDTH(32)) rtr_sb (.clk(clk));
  wire rtr_sb_ack;

  // Test vector source and sink instantiation
  logic [7:0]   set_injection_rate;
  logic [15:0]  set_lines_per_pkt;
  logic [7:0]   set_traffic_patt;
  logic [31:0]  set_num_pkts_to_send;
  logic         snk_start_stb = 0;
  logic         src_start_stb = 0;

  wire [63:0]             session_duration   [0:ROUTER_PORTS-1];
  wire [ROUTER_PORTS-1:0] src_active;
  wire [31:0]             src_xfer_count     [0:ROUTER_PORTS-1];
  wire [31:0]             src_pkt_count      [0:ROUTER_PORTS-1];
  wire [ROUTER_PORTS-1:0] snk_active;
  wire [31:0]             snk_xfer_count     [0:ROUTER_PORTS-1];
  wire [31:0]             snk_pkt_count      [0:ROUTER_PORTS-1];
  wire [31:0]             snk_data_err_count [0:ROUTER_PORTS-1];
  wire [31:0]             snk_route_err_count[0:ROUTER_PORTS-1];

  wire  deadlock_detected;
  reg   deadlock_detected_del = 1'b0;
  always @(posedge clk) deadlock_detected_del <= deadlock_detected;
  wire  deadlock_re = (deadlock_detected & ~deadlock_detected_del);
  wire  deadlock_fe = (~deadlock_detected & deadlock_detected_del);

  genvar i;
  generate for (i = 0; i < ROUTER_PORTS; i=i+1) begin: src_snk_blk
    chdr_traffic_source_sim #(
      .WIDTH            (ROUTER_DWIDTH),
      .MTU              (MTU_LOG2),
      .NODE_ID          (i),
      .NUM_NODES        (ROUTER_PORTS)
    ) traffic_src (     
      .clk              (clk),
      .rst              (rst),
      .current_time     (timestamp),
      .start_stb        (src_start_stb & (i < NUM_MASTERS)),
      .injection_rate   (set_injection_rate),
      .lines_per_pkt    (set_lines_per_pkt),
      .traffic_patt     (set_traffic_patt),
      .num_pkts_to_send (set_num_pkts_to_send),
      .m_axis_tdata     (src2rtr_axis.tdata[((i+1)*ROUTER_DWIDTH)-1:i*ROUTER_DWIDTH]),
      .m_axis_tlast     (src2rtr_axis.tlast[i]),
      .m_axis_tvalid    (src2rtr_axis.tvalid[i]),
      .m_axis_tready    (src2rtr_axis.tready[i]),
      .session_active   (src_active[i]),
      .session_duration (session_duration[i]),
      .xfer_count       (src_xfer_count[i]),
      .pkt_count        (src_pkt_count[i])
    );
  
    chdr_traffic_sink_sim #(
      .WIDTH            (ROUTER_DWIDTH),
      .MTU              (MTU_LOG2),
      .NODE_ID          (i),
      .NUM_NODES        (ROUTER_PORTS),
      .FILE_PATH        (TEST_GEN_LL_FILES==1 ? FILE_PATH : "")
    ) traffic_sink (    
      .clk              (clk),
      .rst              (rst),
      .current_time     (timestamp),
      .start_stb        (snk_start_stb),
      .injection_rate   (set_injection_rate),
      .lines_per_pkt    (set_lines_per_pkt),
      .traffic_patt     (set_traffic_patt),
      .s_axis_tdata     (rtr2snk_axis.tdata[((i+1)*ROUTER_DWIDTH)-1:i*ROUTER_DWIDTH]),
      .s_axis_tlast     (rtr2snk_axis.tlast[i]),
      .s_axis_tvalid    (rtr2snk_axis.tvalid[i]),
      .s_axis_tready    (rtr2snk_axis.tready[i]),
      .session_active   (snk_active[i]),
      .xfer_count       (snk_xfer_count[i]),
      .pkt_count        (snk_pkt_count[i]),
      .data_err_count   (snk_data_err_count[i]),
      .route_err_count  (snk_route_err_count[i])
    );
  end endgenerate

  //----------------------------------------------------
  // Instantiate DUT
  //----------------------------------------------------
  generate if (ROUTER_IMPL == "FIFO") begin
    for (i = 0; i < ROUTER_PORTS; i=i+1) begin
      axi_fifo #(
        .WIDTH(ROUTER_DWIDTH+1), .SIZE(0)
      ) fifo_i (
        .clk      (clk), 
        .reset    (rst), 
        .clear    (1'b0),
        .i_tdata  ({src2rtr_axis.tlast[i], src2rtr_axis.tdata[((i+1)*ROUTER_DWIDTH)-1:i*ROUTER_DWIDTH]}),
        .i_tvalid (src2rtr_axis.tvalid[i]),
        .i_tready (src2rtr_axis.tready[i]),
        .o_tdata  ({rtr2snk_axis.tlast[i], rtr2snk_axis.tdata[((i+1)*ROUTER_DWIDTH)-1:i*ROUTER_DWIDTH]}),
        .o_tvalid (rtr2snk_axis.tvalid[i]),
        .o_tready (rtr2snk_axis.tready[i]),
        .space    (),
        .occupied ()
      );
    end
  end else if (ROUTER_IMPL == "axi_crossbar") begin
    axi_crossbar #(
      .BASE         (0),
      .FIFO_WIDTH   (ROUTER_DWIDTH),
      .DST_WIDTH    (16),
      .NUM_INPUTS   (ROUTER_PORTS),
      .NUM_OUTPUTS  (ROUTER_PORTS)
    ) router_dut_i (
       // General
      .clk          (clk),
      .reset        (rst),
      .clear        (1'b0),
      .local_addr   (8'd0),
      // Inputs     
      .i_tdata      (src2rtr_axis.tdata),
      .i_tlast      (src2rtr_axis.tlast),
      .i_tvalid     (src2rtr_axis.tvalid),
      .i_tready     (src2rtr_axis.tready),
      .pkt_present  (src2rtr_axis.tvalid),
      // Output
      .o_tdata      (rtr2snk_axis.tdata),
      .o_tlast      (rtr2snk_axis.tlast),
      .o_tvalid     (rtr2snk_axis.tvalid),
      .o_tready     (rtr2snk_axis.tready),
      // Setting Bus
      .set_stb      (rtr_sb.settings_bus.set_stb),
      .set_addr     (rtr_sb.settings_bus.set_addr),
      .set_data     (rtr_sb.settings_bus.set_data),
      // Readback bus
      .rb_rd_stb    (1'b0),
      .rb_addr      ({(2*$clog2(ROUTER_PORTS)){1'b0}}),
      .rb_data      ()
    );
  end else if (ROUTER_IMPL == "chdr_crossbar_nxn") begin
    chdr_crossbar_nxn #(
      .CHDR_W         (ROUTER_DWIDTH),
      .NPORTS         (ROUTER_PORTS),
      .DEFAULT_PORT   (0),
      .MTU            (MTU_LOG2),
      .ROUTE_TBL_SIZE (6),
      .MUX_ALLOC      ("ROUND-ROBIN"),
      .OPTIMIZE       ("AREA"),
      .NPORTS_MGMT    (0),
      .EXT_RTCFG_PORT (1)
    ) router_dut_i (
       // General         
      .clk            (clk),
      .reset          (rst),
      // Inputs
      .s_axis_tdata   (src2rtr_axis.tdata),
      .s_axis_tlast   (src2rtr_axis.tlast),
      .s_axis_tvalid  (src2rtr_axis.tvalid),
      .s_axis_tready  (src2rtr_axis.tready),
      // Output
      .m_axis_tdata   (rtr2snk_axis.tdata),
      .m_axis_tlast   (rtr2snk_axis.tlast),
      .m_axis_tvalid  (rtr2snk_axis.tvalid),
      .m_axis_tready  (rtr2snk_axis.tready),
      // External router config
      .ext_rtcfg_stb  (rtr_sb.settings_bus.set_stb),
      .ext_rtcfg_addr (rtr_sb.settings_bus.set_addr),
      .ext_rtcfg_data (rtr_sb.settings_bus.set_data),
      .ext_rtcfg_ack  (rtr_sb_ack)
    );
  end else begin
    axis_ctrl_crossbar_nxn #(
      .WIDTH            (ROUTER_DWIDTH),
      .NPORTS           (ROUTER_PORTS),
      .TOPOLOGY         (ROUTER_IMPL == "axis_ctrl_2d_torus" ? "TORUS" : "MESH"),
      .INGRESS_BUFF_SIZE(MTU_LOG2),
      .ROUTER_BUFF_SIZE (MTU_LOG2),
      .ROUTING_ALLOC    ("WORMHOLE"),
      .SWITCH_ALLOC     ("PRIO")
    ) router_dut_i (
       // General
      .clk              (clk),
      .reset            (rst),
      // Inputs         
      .s_axis_tdata     (src2rtr_axis.tdata),
      .s_axis_tlast     (src2rtr_axis.tlast),
      .s_axis_tvalid    (src2rtr_axis.tvalid),
      .s_axis_tready    (src2rtr_axis.tready),
      // Output         
      .m_axis_tdata     (rtr2snk_axis.tdata),
      .m_axis_tlast     (rtr2snk_axis.tlast),
      .m_axis_tvalid    (rtr2snk_axis.tvalid),
      .m_axis_tready    (rtr2snk_axis.tready),
      // Deadlock detection
      .deadlock_detected(deadlock_detected)
    );
  end endgenerate

  //----------------------------------------------------
  // Test routine. Runs tests and writes metrics to file
  //----------------------------------------------------

  // Constants
  localparam [7:0] TRAFFIC_PATT_LOOPBACK       = 8'd76;  //L
  localparam [7:0] TRAFFIC_PATT_NEIGHBOR       = 8'd78;  //N
  localparam [7:0] TRAFFIC_PATT_BIT_COMPLEMENT = 8'd67;  //C
  localparam [7:0] TRAFFIC_PATT_SEQUENTIAL     = 8'd83;  //S
  localparam [7:0] TRAFFIC_PATT_UNIFORM        = 8'd85;  //U
  localparam [7:0] TRAFFIC_PATT_UNIFORM_OTHERS = 8'd79;  //O
  localparam [7:0] TRAFFIC_PATT_RANDOM_PERM    = 8'd82;  //R

  string        filename;
  integer       node;
  integer       session = 0;
  integer       handle = 0;
  logic [63:0]  start_time;
  integer       total_pkts_recvd = 0, total_pkts_sent = 0;

  task sim_dataflow;
    input [7:0]  injection_rate;
    input [7:0]  traffic_patt;
    input [15:0] lines_per_pkt;
    input [31:0] num_pkts_to_send;
  begin
    session = session + 1;
    $display("--------------- New Simulation ---------------");
    $display("- Module = %s", ROUTER_IMPL);
    $display("- Nodes = %00d", ROUTER_PORTS);
    $display("- Injection Rate = %00d%%", injection_rate);
    $display("- Traffic Pattern = %c", traffic_patt);
    $display("- Packet Size = %00d words (%00d bits)", lines_per_pkt, ROUTER_DWIDTH);
    $display("- Max Packets = %00d", num_pkts_to_send);
    // Configure settings
    @(posedge clk);
    set_injection_rate = injection_rate;
    set_lines_per_pkt = lines_per_pkt;
    set_traffic_patt = traffic_patt;
    set_num_pkts_to_send = num_pkts_to_send;
    @(posedge clk);
    // Start the sink then the source
    $display("Data flow starting...");
    snk_start_stb = 1;
    src_start_stb = 1;
    @(posedge clk);
    src_start_stb = 0;
    snk_start_stb = 0;
    @(posedge clk);
    start_time = timestamp;
    // Wait for source blocks to finish generating
    $display("Waiting for packets to transmit... (may take a while)");
    while (|src_active) begin
      @(posedge clk);
      if (deadlock_re) $display("WARNING: Deadlock detected");
      if (deadlock_fe) $display("Recovered from deadlock");
    end 
    // Wait for sink blocks to finish consuming
    $display("All packets transmitted. Waiting to flush...");
    while (|snk_active) @(posedge clk);
    // If router deadlocks then wait for it to recover
    if (deadlock_detected) begin
      $display("Waiting for deadlock recovery to finish...");
      while (deadlock_detected) @(posedge clk);
    end
    repeat(set_lines_per_pkt) @(posedge clk);
    // Record summary to file and print to console
    $sformat(filename, "%s/info_inj%03d_lpp%05d_traffic%c_sess%04d.csv",
      FILE_PATH, injection_rate, lines_per_pkt, traffic_patt, session);
    if (TEST_GEN_LL_FILES == 1) begin
      handle = $fopen(filename, "w");
      if (handle == 0) begin
        $error("Could not open file: %s", filename);
        $finish();
      end
    end
    if (handle != 0) $fdisplay(handle, "Impl,Node,TxPkts,RxPkts,Duration,ErrRoute,ErrData");
    total_pkts_sent = 0;
    total_pkts_recvd = 0;
    for (node = 0; node < ROUTER_PORTS; node=node+1) begin
      $display("- Node #%03d: TX = %5d pkts, RX = %5d pkts, Inj Rate = %3d%%. Errs = %5d route, %5d data",
        node,src_pkt_count[node], snk_pkt_count[node], ((src_xfer_count[node]*100)/session_duration[node]),
        snk_route_err_count[node], snk_data_err_count[node]);
      if (handle != 0) $fdisplay(handle, "%s,%00d,%00d,%00d,%00d,%00d,%00d", ROUTER_IMPL,
        node,src_pkt_count[node], snk_pkt_count[node], session_duration[node],
        snk_route_err_count[node], snk_data_err_count[node]);
      total_pkts_sent = total_pkts_sent + src_pkt_count[node];
      total_pkts_recvd = total_pkts_recvd + snk_pkt_count[node];
      `ASSERT_ERROR(snk_route_err_count[node] == 0, "Routing errors. Received packets destined to other nodes");
      `ASSERT_ERROR(snk_data_err_count[node] == 0, "Integrity errors. Received corrupted packets");
    end
    $display("Finished. Elapsed = %00d cycles, TX = %00d pkts, RX = %00d pkts",
      (timestamp - start_time), total_pkts_sent, total_pkts_recvd);
    `ASSERT_ERROR(total_pkts_recvd == total_pkts_sent, "Total # TX packets did not match the total # RX packets");
    if (handle != 0) $fclose(handle);
    $display("----------------------------------------------");
  end
  endtask

  //----------------------------------------------------
  // Main test loop
  //----------------------------------------------------

  logic [31:0] MAX_PACKETS    = TEST_MAX_PACKETS;
  logic [15:0] LPP            = TEST_LPP;
  integer      MIN_INJ_RATE   = TEST_MIN_INJ_RATE;
  integer      MAX_INJ_RATE   = TEST_MAX_INJ_RATE;
  integer      INJ_RATE_INCR  = TEST_INJ_RATE_INCR;

  integer inj_rate = 0;
  initial begin : tb_main
    src_start_stb = 0;
    snk_start_stb = 0;
    rtr_sb.reset();
    while (rst) @(posedge clk);

    repeat (10) @(posedge clk);

    `TEST_CASE_START("Set up crossbar");
      for (node = 0; node < ROUTER_PORTS; node=node+1) begin
        if (ROUTER_IMPL == "axi_crossbar") begin
          rtr_sb.write(16'd256 + node[15:0], {16'h0, node[15:0]});
        end else if (ROUTER_IMPL == "chdr_crossbar_nxn") begin
          rtr_sb.write(node[15:0], {16'h0, node[15:0]});
          while (~rtr_sb_ack) @(posedge clk);
        end
      end
    `TEST_CASE_DONE(1)

     `TEST_CASE_START("Simulate LOOPBACK Traffic Pattern");
       for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
         sim_dataflow(inj_rate, TRAFFIC_PATT_LOOPBACK, LPP, MAX_PACKETS);
       end
     `TEST_CASE_DONE(1)

     `TEST_CASE_START("Simulate SEQUENTIAL Traffic Pattern");
       for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
         sim_dataflow(inj_rate, TRAFFIC_PATT_SEQUENTIAL, LPP, MAX_PACKETS);
       end
     `TEST_CASE_DONE(1)

     `TEST_CASE_START("Simulate UNIFORM Traffic Pattern");
       for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
         sim_dataflow(inj_rate, TRAFFIC_PATT_UNIFORM, LPP, MAX_PACKETS);
       end
     `TEST_CASE_DONE(1)

     `TEST_CASE_START("Simulate UNIFORM_OTHERS Traffic Pattern");
       for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
         sim_dataflow(inj_rate, TRAFFIC_PATT_UNIFORM_OTHERS, LPP, MAX_PACKETS);
       end
     `TEST_CASE_DONE(1)

     `TEST_CASE_START("Simulate BIT_COMPLEMENT Traffic Pattern");
       for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
         sim_dataflow(inj_rate, TRAFFIC_PATT_BIT_COMPLEMENT, LPP, MAX_PACKETS);
       end
     `TEST_CASE_DONE(1)

    `TEST_CASE_START("Simulate NEIGHBOR Traffic Pattern");
      for (inj_rate = MIN_INJ_RATE; inj_rate <= MAX_INJ_RATE; inj_rate = inj_rate + INJ_RATE_INCR) begin
        sim_dataflow(inj_rate, TRAFFIC_PATT_NEIGHBOR, LPP, MAX_PACKETS);
      end
    `TEST_CASE_DONE(1)

    `TEST_BENCH_DONE
  end // initial begin

endmodule
