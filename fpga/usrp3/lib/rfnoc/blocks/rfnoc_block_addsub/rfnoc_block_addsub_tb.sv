//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_addsub_tb
//
// Description: Testbench for the addsub RFNoC block.
//
// Parameters:
//
//   USE_IMPL : Specifies the implementation string to pass to
//              rfnoc_block_addsub.
//

`default_nettype none


module rfnoc_block_addsub_tb #(
  parameter USE_IMPL = "Verilog"
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'hADD00000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;    // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS_I     = 2;
  localparam int    NUM_PORTS_O     = 2;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz

  // Port numbers for A, B, SUM and DIFF
  localparam IN_PORT_A     = 0;
  localparam IN_PORT_B     = 1;
  localparam OUT_PORT_SUM  = 0;
  localparam OUT_PORT_DIFF = 1;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(.PERIOD(CHDR_CLK_PER), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CTRL_CLK_PER), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(.PERIOD(CE_CLK_PER), .AUTOSTART(0))
    ce_clk_gen (.clk(ce_clk), .rst());


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS_I] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS_O] (rfnoc_chdr_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_bfm_input_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
    end
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_bfm_output_connections
    initial begin
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS_I-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tready;

  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS_O-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT connections
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_addsub #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .USE_IMPL            (USE_IMPL)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk          (ce_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Logic
  //---------------------------------------------------------------------------

  // Rand#(WIDTH)::rand_logic() returns a WIDTH-bit random number. We avoid
  // std::randomize() due to license requirements and limited tool support.
  class Rand #(WIDTH = 32);
    static function logic [WIDTH-1:0] rand_logic();
      logic [WIDTH-1:0] result;
      int num_rand32 = (WIDTH + 31) / 32;
      for (int i = 0; i < num_rand32; i++) begin
        result = {result, $urandom()};
      end
      return result;
    endfunction : rand_logic
  endclass : Rand

  typedef struct {
    item_t        samples[$];
    chdr_word_t   mdata[$];
    packet_info_t pkt_info;
  } test_packet_t;

  typedef struct packed {
    bit [15:0] i;
    bit [15:0] q;
  } sc16_t;


  //---------------------------------------------------------------------------
  // Test Tasks
  //---------------------------------------------------------------------------

  task automatic test_rand(
    int num_packets,
    int max_spp   = SPP,
    int prob_a    = STALL_PROB,
    int prob_b    = STALL_PROB,
    int prob_sum  = STALL_PROB,
    int prob_diff = STALL_PROB
  );
    mailbox #(test_packet_t) packets_mb_a = new();
    mailbox #(test_packet_t) packets_mb_b = new();

    // Set the BFM TREADY behavior
    blk_ctrl.set_master_stall_prob(IN_PORT_A, prob_a);
    blk_ctrl.set_master_stall_prob(IN_PORT_B, prob_b);
    blk_ctrl.set_slave_stall_prob(OUT_PORT_SUM, prob_sum);
    blk_ctrl.set_slave_stall_prob(OUT_PORT_DIFF, prob_diff);

    fork
      repeat (num_packets) begin : send_process
        test_packet_t packet_a, packet_b;
        int packet_length;

        packet_length = $urandom_range(1, max_spp);

        // Generate random data and header
        packet_a.samples = {};
        packet_b.samples = {};
        for (int i = 0; i < packet_length; i++) begin
          packet_a.samples.push_back($urandom());
          packet_b.samples.push_back($urandom());
        end

        // Generate random metadata
        packet_a.mdata = {};
        packet_b.mdata = {};
        for (int i = 0; i < $urandom_range(0,31); i++)
          packet_a.mdata.push_back(Rand #(CHDR_W)::rand_logic());
        for (int i = 0; i < $urandom_range(0,31); i++)
          packet_b.mdata.push_back(Rand #(CHDR_W)::rand_logic());

        // Generate random header info
        packet_a.pkt_info = Rand #($bits(packet_a.pkt_info))::rand_logic();
        packet_b.pkt_info = Rand #($bits(packet_b.pkt_info))::rand_logic();

        // Enqueue the packets for each port
        blk_ctrl.send_items(IN_PORT_A, packet_a.samples, packet_a.mdata, packet_a.pkt_info);
        blk_ctrl.send_items(IN_PORT_B, packet_b.samples, packet_b.mdata, packet_b.pkt_info);

        // Enqueue what we sent for the receiver to check the output
        packets_mb_a.put(packet_a);
        packets_mb_b.put(packet_b);
      end
      repeat (num_packets) begin : recv_process
        test_packet_t packet_a, packet_b, packet_add, packet_sub;
        string str;

        // Grab the next pair of packets that was input
        packets_mb_a.get(packet_a);
        packets_mb_b.get(packet_b);

        // Receive a packet from each port
        blk_ctrl.recv_items_adv(OUT_PORT_SUM, packet_add.samples,
          packet_add.mdata, packet_add.pkt_info);
        blk_ctrl.recv_items_adv(OUT_PORT_DIFF, packet_sub.samples,
          packet_sub.mdata, packet_sub.pkt_info);

        // Make sure both output packets have the same length
        `ASSERT_ERROR(packet_add.samples.size() == packet_sub.samples.size(),
          "ADD and SUB packets were not the same length");

        // Make sure the output packet length matches the A input
        `ASSERT_ERROR(packet_a.samples.size() == packet_add.samples.size(),
          "Output packet length didn't match A input");

        // Check that the output packet header info matches the A input
        `ASSERT_ERROR(packet_info_equal(packet_a.pkt_info, packet_add.pkt_info),
          "ADD output header info didn't match A input");
        `ASSERT_ERROR(packet_info_equal(packet_a.pkt_info, packet_sub.pkt_info),
          "SUB output header info didn't match A input");

        // Check the metdata
        `ASSERT_ERROR(ChdrData #(CHDR_W)::chdr_equal(packet_a.mdata, packet_add.mdata),
          "ADD metadata info didn't match A input");
        `ASSERT_ERROR(ChdrData #(CHDR_W)::chdr_equal(packet_a.mdata, packet_sub.mdata),
          "SUB metadata info didn't match A input");

        // Verify that the data has the expected values
        for (int i = 0; i < packet_add.samples.size(); i++) begin
          sc16_t a, b, a_plus_b, a_min_b, add, sub;

          // Grab the input and output samples
          a    = packet_a.samples[i];
          b    = packet_b.samples[i];
          add  = packet_add.samples[i];
          sub  = packet_sub.samples[i];

          // Compute expected sum and difference
          a_plus_b.i = a.i + b.i;
          a_plus_b.q = a.q + b.q;
          a_min_b.i  = a.i - b.i;
          a_min_b.q  = a.q - b.q;

          // Check that the results match
          $sformat(str,
            "Incorrect value received on ADD output! Expected: 0x%X, Received: 0x%X",
            a_plus_b, add);
          `ASSERT_ERROR(add == a_plus_b, str);
          $sformat(str,
            "Incorrect value received on SUB output! Expected: 0x%X, Received: 0x%X",
            a_min_b, sub);
          `ASSERT_ERROR(sub == a_min_b, str);
        end
      end
    join
  endtask : test_rand


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_addsub_tb");

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();
    ce_clk_gen.start();

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();

    //--------------------------------
    // Verify Block Info
    //--------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    begin
      const int NUM_PACKETS = 100;

      test.start_test("Test random packets", 1ms);
      test_rand(NUM_PACKETS);
      test.end_test();

      test.start_test("Test without back pressure", 1ms);
      test_rand(NUM_PACKETS, SPP, 0, 0, 0, 0);
      test.end_test();

      test.start_test("Test back pressure", 1ms);
      test_rand(NUM_PACKETS, SPP, 25, 25, 50, 50);
      test.end_test();

      test.start_test("Test underflow", 1ms);
      test_rand(NUM_PACKETS, SPP, 50, 50, 25, 25);
      test.end_test();

      test.start_test("Test min packet size", 1ms);
      test_rand(10, 1);
      test.end_test();
    end

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    ce_clk_gen.kill();

  end : tb_main

endmodule : rfnoc_block_addsub_tb


`default_nettype wire
