//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_logpwr_tb
//
// Description: Testbench for the logpwr RFNoC block.
//

`default_nettype none


module rfnoc_block_logpwr_tb #(
  parameter int CHDR_W = 64    // CHDR size in bits
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'h4C500000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS       = 2;
  localparam int    RANDOM_MODE     = 2'b11;
  localparam int    NUM_PORTS_I     = NUM_PORTS;
  localparam int    NUM_PORTS_O     = NUM_PORTS;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER)   ce_clk_gen         (.clk(ce_clk),         .rst());

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
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t  chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t       item_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_queue_t item_queue_t;

  typedef ChdrPacket #(CHDR_W) ChdrPacket_t;

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

  rfnoc_block_logpwr #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS),
    .RANDOM_MODE         (2'b00)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
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
  // Helper Tasks
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


  // Compute the 16-bit unsigned log-power, modeled after the HDL
  function automatic bit unsigned [15:0] log_pwr(bit [31:0] sample);
    real       i, q, logpwr;
    bit [16:0] temp;
    bit [31:0] result;

    // Compute log power
    i = shortint'(sample[31:16]);
    q = shortint'(sample[15: 0]);
    logpwr = $ln(i*i + q*q) / $ln(2);

    // Shift it the same way the IP does
    result = int'(logpwr) * 2048;

    return result;
  endfunction : log_pwr


  // Generate a random CHDR packet with the given number of samples
  function automatic ChdrPacket_t gen_rand_chdr_pkt(
    int max_samps = SPP,
    int max_mdata = 31
  );
    ChdrPacket_t     packet = new();
    chdr_header_t    header;
    chdr_word_t      data[$];
    chdr_word_t      mdata[$];
    chdr_timestamp_t timestamp;
    int              num_samps;

    // Start with a random header (important fields will be overwritten)
    header = Rand#($bits(header))::rand_logic();
    // Randomly choose if we use a timestamp
    header.pkt_type = ($urandom() & 1) ? CHDR_DATA_NO_TS : CHDR_DATA_WITH_TS;
    // Random timestamp
    timestamp = Rand#(64)::rand_logic();
    // Random metadata
    repeat ($urandom_range(0, max_mdata));
      mdata.push_back(Rand#(CHDR_W)::rand_logic());
    // Random payload
    num_samps = $urandom_range(1, max_samps);
    repeat (num_samps * ITEM_W / CHDR_W)
      data.push_back(Rand#(CHDR_W)::rand_logic());
    // Round up to right number of CHDR words
    if (num_samps * ITEM_W % CHDR_W != 0)
      data.push_back(Rand#(CHDR_W)::rand_logic());

    // Build packet
    packet.write_raw(header, data, mdata, timestamp, num_samps * (ITEM_W/8));

    return packet;
  endfunction : gen_rand_chdr_pkt


  task automatic test_random(int port, int num_packets);
    mailbox #(ChdrPacket_t) packets = new();

    // Generate and enqueue packets for transmission
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      ChdrPacket_t packet;

      packet = gen_rand_chdr_pkt(.max_samps(SPP), .max_mdata(3));
      packets.put(packet);
      blk_ctrl.put_chdr(port, packet);
    end

    // Receive and check the results
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      ChdrPacket_t  sent_packet, recv_packet;
      item_t        sent[$];
      logic [15:0]  received[$], expected[$];
      chdr_header_t expected_header;

      // Retrieve the next packet that was sent and unpack the payload
      packets.get(sent_packet);
      sent = ChdrData#(CHDR_W, ITEM_W)::chdr_to_item(
        sent_packet.data,
        sent_packet.data_bytes()
      );

      // Calculate the expected result based on what was sent
      foreach(sent[i]) begin
        // Right-shift by one, since the actual block converts the unsigned
        // value to a signed value.
        expected[i] = log_pwr(sent[i]) >> 1;
      end

      // Retrieve the packet that was received and unpack the payload
      blk_ctrl.get_chdr(port, recv_packet);
      received = ChdrData#(CHDR_W, 16)::chdr_to_item(
        recv_packet.data,
        recv_packet.data_bytes()
      );

      // Check the header, except the length, which should be different
      expected_header = sent_packet.header;
      expected_header.length = recv_packet.header.length;
      `ASSERT_ERROR(expected_header == recv_packet.header,
        "Header mismatch on received packet");

      // Check the timestamp
      if (sent_packet.header.pkt_type == CHDR_DATA_WITH_TS) begin
        `ASSERT_ERROR(sent_packet.timestamp == recv_packet.timestamp,
          "Timestamp mismatch on received packet");
      end

      // Check the metadata
      `ASSERT_ERROR(sent_packet.metadata.size() == recv_packet.metadata.size(),
        "Metadata length mismatch on received packet");
      foreach(sent_packet.metadata[i]) begin
        `ASSERT_ERROR(sent_packet.metadata[i] == recv_packet.metadata[i],
          "Metadata mismatch on received packet");
      end

      // Check that the packet data length matches what was input
      `ASSERT_ERROR(
        expected.size() == received.size(),
        $sformatf("For packet %0d, received length was incorrect", packet_count)
      );

      // Check that the payload is correct. Because of the random number
      // generator within the logpwr block, a wide range is acceptable.
      foreach(received[i]) begin
        int error;
        error = int'(received[i]) - int'(expected[i]);
        if (error < 0) error = -error;
        `ASSERT_ERROR(
          error <= 16'h0200,
          $sformatf("Unexpected result for packet %0d, sample %0d; Expected %X, received %X for input %X",
            packet_count, i, expected[i], received[i], sent[i])
        );
      end
    end
  endtask : test_random


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    int port;
    int num_packets;

    // Initialize the test exec object for this testbench
    test.start_tb($sformatf("rfnoc_block_logpwr_tb (CHDR_W = %0d)", CHDR_W));

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

    num_packets = 200;

    // Test all ports
    for (port = 0; port < NUM_PORTS; port++) begin
      test.start_test($sformatf("Test random packets (port %0d)", port), 1ms);
      test_random(port, num_packets);
      test.end_test();
    end

    // Run remaining tests on a single port
    port = 0;

    // Test with slow BFM slave to make sure back-pressure is working correctly.
    test.start_test("Test back pressure", 1ms);
    blk_ctrl.set_slave_stall_prob(port, 90);
    test_random(port, num_packets);
    blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    test.end_test();

    // Test with slow BFM master to make sure AXI-stream flow control is
    // working correctly.
    test.start_test("Test underflow", 1ms);
    blk_ctrl.set_master_stall_prob(port, 90);
    test_random(port, num_packets);
    blk_ctrl.set_master_stall_prob(port, STALL_PROB);
    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results, but don't call $finish, since we
    // don't want to kill other instances of this testbench that may be
    // running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    ce_clk_gen.kill();
  end : tb_main

endmodule : rfnoc_block_logpwr_tb


`default_nettype wire
