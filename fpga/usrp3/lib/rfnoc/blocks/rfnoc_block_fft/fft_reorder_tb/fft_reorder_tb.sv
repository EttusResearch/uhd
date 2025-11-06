//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_reorder_tb
//
// Description:
//
//   Testbench for fft_reorder.
//

`default_nettype none


module fft_reorder_tb
  import fft_reorder_pkg::*;
#(
  int         IN_FIFO_LOG2     = 1,
  int         OUT_FIFO_LOG2    = 3,
  fft_order_t INPUT_ORDER      = BIT_REVERSE,
  int         MAX_FFT_LEN_LOG2 = 5,
  int         EN_CP_INSERTION  = 1
) ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  localparam real CLK_PERIOD    = 10.0;
  localparam int DATA_W         = 32;
  localparam int CP_LEN_W       = MAX_FFT_LEN_LOG2;
  localparam int FFT_LEN_LOG2_W = $clog2(MAX_FFT_LEN_LOG2+1);
  localparam bit VERBOSE        = 0;
  localparam int STALL_PROB     = 25;

  // Define parameters for packet randomization
  localparam int MIN_FFT_LEN_LOG2 = 3;  // Same as Xilinx FFT core
  localparam int MIN_FFT_LEN = 2**MIN_FFT_LEN_LOG2;
  localparam int MAX_FFT_LEN = 2**MAX_FFT_LEN_LOG2;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Connections to DUT as interfaces:
  AxiStreamIf #(DATA_W, CP_LEN_W) i_axis (clk, rst);
  AxiStreamIf #(DATA_W, CP_LEN_W) o_axis (clk, rst);

  // AXI-Stream BFM
  AxiStreamBfm #(DATA_W, CP_LEN_W) bfm = new(i_axis, o_axis);

  typedef AxiStreamBfm #(DATA_W, CP_LEN_W)::AxisPacket_t AxisPacket_t;
  typedef AxiStreamBfm #(DATA_W, CP_LEN_W)::data_t data_t;
  typedef AxiStreamBfm #(DATA_W, CP_LEN_W)::user_t user_t;
  typedef AxisPacket_t AxisPacketQueue_t [$];


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic                      fft_cfg_wr = 1'b0;
  logic [FFT_LEN_LOG2_W-1:0] fft_len_log2;
  fft_order_t                fft_out_order;

  fft_reorder #(
    .IN_FIFO_LOG2    (IN_FIFO_LOG2),
    .OUT_FIFO_LOG2   (OUT_FIFO_LOG2),
    .INPUT_ORDER     (INPUT_ORDER),
    .MAX_FFT_LEN_LOG2(MAX_FFT_LEN_LOG2),
    .DATA_W          (DATA_W),
    .EN_CP_INSERTION (EN_CP_INSERTION)
  ) fft_reorder_i (
    .clk          (clk),
    .rst          (rst),
    .fft_cfg_wr   (fft_cfg_wr),
    .fft_len_log2 (fft_len_log2),
    .fft_out_order(fft_out_order),
    .i_tdata      (i_axis.tdata),
    .i_tuser      (i_axis.tuser),
    .i_tlast      (i_axis.tlast),
    .i_tvalid     (i_axis.tvalid),
    .i_tready     (i_axis.tready),
    .o_tdata      (o_axis.tdata),
    .o_tlast      (o_axis.tlast),
    .o_tvalid     (o_axis.tvalid),
    .o_tready     (o_axis.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  // Determine if the expected packet is the same to the actual packet
  // received. Return 1 if they are equivalent, 0 if they differ.
  function automatic bit check_pkt_equal(AxisPacket_t exp, AxisPacket_t act);
    if (exp.data.size() != act.data.size()) begin
      $display("Packets differ in size");
      $display("Exp: %0d words, Act: %0d words", exp.data.size(), act.data.size());
      return 0;
    end
    for (int i = 0; i < exp.data.size(); i++) begin
      if (exp.data[i] != act.data[i]) begin
        $display("Index %0d, expected 0x%X but received 0x%X", i, exp.data[i], act.data[i]);
        return 0;
      end
    end

    // If it made it to here, all is well
    return 1;
  endfunction


  // Generate an FFT packet having the indicated length, order, and start
  // value. All data values are sequential.
  function automatic AxisPacket_t gen_in_packet(
    int         len_log2,
    fft_order_t order,
    int         start_value,
    int         cp_len = 0
  );
    AxisPacket_t pkt;
    int data_count = start_value;
    data_t data [] = new [2**len_log2];
    user_t user [] = new [1];

    case (order)
      NATURAL : begin
        foreach (data[i]) data[i] = data_count++;
      end
      BIT_REVERSE : begin
        foreach (data[i]) data[bit_reverse(i, len_log2)] = data_count++;
      end
      default : begin
        `ASSERT_FATAL(0, "Invalid input FFT order");
      end
    endcase

    user[0] = cp_len;

    pkt = new();
    pkt.data = data;
    pkt.user = user;
    return pkt;
  endfunction : gen_in_packet


  // Generate the expected output packet.
  function automatic AxisPacket_t gen_out_packet(
    int         len_log2,
    fft_order_t order,
    int         start_value,
    int         cp_len = 0
  );
    AxisPacket_t pkt;
    int data_count = start_value;
    int length = 2**len_log2;
    data_t cp [] = new [cp_len];
    data_t data [] = new [length];

    case (order)
      NORMAL : begin
        // 8 9 A B C D E F 0 1 2 3 4 5 6 7
        for (int i = length/2; i < length;   i++) data[i] = data_count++;
        for (int i = 0;        i < length/2; i++) data[i] = data_count++;
      end
      REVERSE : begin
        // 7 6 5 4 3 2 1 0 F E D C B A 9 8
        for (int i = length/2-1; i >= 0;        i--) data[i] = data_count++;
        for (int i = length-1;   i >= length/2; i--) data[i] = data_count++;
      end
      NATURAL : begin
        // 0 1 2 3 4 5 6 7 8 9 A B C D E F
        foreach (data[i]) data[i] = data_count++;
      end
      BIT_REVERSE : begin
        // 0 8 4 C 2 A 6 E 1 9 5 D 3 B 7 F
        foreach (data[i]) data[bit_reverse(i, len_log2)] = data_count++;
      end
      default: begin
        `ASSERT_FATAL(0, "Invalid input FFT order");
      end
    endcase

    foreach (cp[i]) cp[i] = data[length-cp_len+i];

    pkt = new();
    pkt.data = {cp, data};
    return pkt;
  endfunction : gen_out_packet


  // Run a test of the specific configuration.
  //
  //   num_pkts : Number of test packets to test for this configuration
  //   len_log2 : FFT size to test
  //   order    : FFT output order to test
  //
  task automatic test_packets(
    int         num_pkts,
    int         len_log2,
    fft_order_t order,
    int         cp_insertions[] = {}
  );
    int data_count;
    int length = 2**len_log2;
    int cp_idx;
    AxisPacketQueue_t exp_q;

    if (VERBOSE) begin
      $display("test_packets(): num_pkts = %0d, len_log2 = %0d, order = %s",
        num_pkts, len_log2, order.name());
    end

    `ASSERT_FATAL(len_log2 >= MIN_FFT_LEN_LOG2 && len_log2 <= MAX_FFT_LEN_LOG2,
      $sformatf("FFT length %0d is out of allowed range", 2**len_log2));

    @(posedge clk);
    fft_len_log2  <= len_log2;
    fft_out_order <= order;
    fft_cfg_wr    <= 1'b1;
    @(posedge clk);
    fft_cfg_wr    <= 1'b0;
    @(posedge clk);

    repeat (num_pkts) begin
      AxisPacket_t pkt, exp;

      // Generate test packet and expected output packet
      pkt = gen_in_packet(len_log2, INPUT_ORDER, data_count, cp_insertions[cp_idx]);
      exp = gen_out_packet(len_log2, order, data_count, cp_insertions[cp_idx]);
      cp_idx++;
      data_count += length;

      // Queue up the test packet to be sent
      bfm.put(pkt);

      // Save the expected result
      exp_q.push_back(exp);
    end

    repeat (num_pkts) begin
      AxisPacket_t act, exp;
      bfm.get(act);
      exp = exp_q.pop_front();

      // Check if the received packet is equivalent to the expected packet
      if (!check_pkt_equal(exp, act)) begin
        $displayh("Expected: %p", exp.data);
        $displayh("Received: %p", act.data);
        `ASSERT_FATAL(0, "Received packet does not match expected packet");
      end
    end
  endtask : test_packets


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  task automatic test_orders();
    test.start_test($sformatf("Test %s to NORMAL", INPUT_ORDER.name()));
    test_packets(2, 4, NORMAL);
    test.end_test();

    test.start_test($sformatf("Test %s to NATURAL", INPUT_ORDER.name()));
    test_packets(2, 4, NATURAL);
    test.end_test();

    test.start_test($sformatf("Test %s to REVERSE", INPUT_ORDER.name()));
    test_packets(2, 4, REVERSE);
    test.end_test();

    test.start_test($sformatf("Test %s to BIT_REVERSE", INPUT_ORDER.name()));
    test_packets(2, 4, BIT_REVERSE);
    test.end_test();
  endtask : test_orders


  task automatic test_backpressure();
    test.start_test("Test full throttle");
    bfm.set_master_stall_prob(0);
    bfm.set_slave_stall_prob(0);
    test_packets(16, 4, NORMAL);
    test.end_test();

    test.start_test("Test overflow");
    bfm.set_master_stall_prob(10);
    bfm.set_slave_stall_prob(90);
    test_packets(16, 4, NORMAL);
    test.end_test();

    test.start_test("Test underflow");
    bfm.set_master_stall_prob(90);
    bfm.set_slave_stall_prob(10);
    test_packets(16, 4, NORMAL);
    test.end_test();

    // Restore default stall probability
    bfm.set_master_stall_prob(STALL_PROB);
    bfm.set_slave_stall_prob(STALL_PROB);
  endtask : test_backpressure


  task automatic test_random(int num_tests);
    fft_order_t order;
    int num_pkts;
    int len_log2;

    test.start_test("Test random");

    repeat (num_tests) begin
      int cp_insertions [];
      // Choose random parameters for this test
      num_pkts = $urandom_range(1, 4);
      len_log2 = $urandom_range($clog2(MIN_FFT_LEN), $clog2(MAX_FFT_LEN));
      cp_insertions = new [num_pkts];

      // Use a CP for half of all packets groups
      if ($urandom_range(1)) begin
        foreach (cp_insertions[i]) cp_insertions[i] = $urandom_range(0, 2**len_log2-1);
      end

      // Choose a random output order
      order = fft_order_t'($urandom_range(order.num()-1));

      test_packets(num_pkts, len_log2, order);
    end

    test.end_test();
  endtask : test_random


  // Test some basic cyclic prefix insertion/removal
  task automatic test_cp_insertion();
    int cp_insertions[] = {0, 1, 2, 0};
    if (!EN_CP_INSERTION) return;
    test.start_test("Test CP insertion");
    test_packets(cp_insertions.size(), 3, NATURAL, cp_insertions);
    test.end_test();
  endtask


  // Test min/max FFT and CP sizes
  task automatic test_min_max();
    int min_insertion [2];
    int max_insertion [2];

    test.start_test("Test min/max");

    if (EN_CP_INSERTION) begin
      min_insertion = {0, 1};
      max_insertion = {MAX_FFT_LEN-1, 0};
    end else begin
      min_insertion = {0, 0};
      max_insertion = {0, 0};
    end

    test_packets(2, $clog2(MIN_FFT_LEN), NATURAL, min_insertion);
    test_packets(2, $clog2(MAX_FFT_LEN), NATURAL, max_insertion);
    test.end_test();
  endtask


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    //string msg;
    string tb_name;
    tb_name = $sformatf( {
      "fft_reorder_tb\n",
      "IN_FIFO_LOG2     = %0d\n",
      "OUT_FIFO_LOG2    = %0d\n",
      "INPUT_ORDER      = %s\n",
      "MAX_FFT_LEN_LOG2 = %0d"},
      IN_FIFO_LOG2, OUT_FIFO_LOG2, INPUT_ORDER.name(), MAX_FFT_LEN_LOG2
    );
    test.start_tb(tb_name, 100ms);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen.start();

    // Start the BFM
    bfm.run();
    bfm.set_master_stall_prob(STALL_PROB);
    bfm.set_slave_stall_prob(STALL_PROB);

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset", 10us);
    clk_gen.reset(2);
    clk_gen.clk_wait_f(3);
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_orders();
    test_cp_insertion();
    test_backpressure();
    test_min_max();

    repeat (5) begin
      clk_gen.reset(2);
      clk_gen.clk_wait_f(3);
      test_random(50);
    end

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);
    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();
  end : tb_main

endmodule : fft_reorder_tb


`default_nettype wire
