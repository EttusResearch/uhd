//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_window_tb
//
// Description: Testbench for the Window RFNoC block.
//

`default_nettype none


module rfnoc_block_window_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  `include "rfnoc_block_window_regs.vh"

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'hD0530000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;    // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS       = 2;
  localparam int    NUM_PORTS_I     = NUM_PORTS;
  localparam int    NUM_PORTS_O     = NUM_PORTS;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 256;   // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz
  localparam int    MAX_WINDOW_SIZE = 128;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER) ce_clk_gen (.clk(ce_clk), .rst());

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

  rfnoc_block_window #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS),
    .MAX_WINDOW_SIZE     (MAX_WINDOW_SIZE)
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
  // Register Access
  //---------------------------------------------------------------------------

  task automatic write_reg(int port, bit [19:0] addr, bit [31:0] value);
    blk_ctrl.reg_write(port * (2**WINDOW_ADDR_W) + addr, value);
  endtask : write_reg

  task automatic read_reg(int port, bit [19:0] addr, output logic [31:0] value);
    blk_ctrl.reg_read(port * (2**WINDOW_ADDR_W) + addr, value);
  endtask : read_reg


  //---------------------------------------------------------------------------
  // Test Procedures
  //---------------------------------------------------------------------------

  // Model the behavior of the mult_rc module. As configured for our DUT, it
  // treats each value as a 16-bit signed fixed point value with 15 fractional
  // bits.
  function bit [31:0] mult_rc(bit signed [15:0] real_data, bit signed [31:0] complex_data);
    bit [47:0] temp;
    bit [31:0] result;
    temp = (real_data * signed'(complex_data[31:16])) >> 15;
    result[31:16] = temp;
    temp = (real_data * signed'(complex_data[15: 0])) >> 15;
    result[15: 0] = temp;
    return result;
  endfunction : mult_rc


  // Test read-only and read/write registers
  task automatic test_registers(int port);
    int unsigned value, expected;

    test.start_test($sformatf("Test registers, port %0d", port), 10us);

    read_reg(port, REG_WINDOW_MAX_SIZE, value);
    `ASSERT_ERROR(value == MAX_WINDOW_SIZE, "REG_WINDOW_MAX_SIZE reports incorrect value");

    expected = 2**$clog2(MAX_WINDOW_SIZE+1)-1;  // Max value (all ones)
    write_reg(port, REG_WINDOW_SIZE, expected);
    read_reg(port, REG_WINDOW_SIZE, value);
    `ASSERT_ERROR(value == expected, "REG_WINDOW_SIZE did not update");

    expected = 0;
    write_reg(port, REG_WINDOW_SIZE, expected);
    read_reg(port, REG_WINDOW_SIZE, value);
    `ASSERT_ERROR(value == expected, "REG_WINDOW_SIZE did not update");

    test.end_test();
  endtask : test_registers


  // Run a single test on the block using random coefficients and packet data.
  //
  //  port        : Port number of the block to test
  //  window_size : Size of the window to use (number of coefficients)
  //  packet_len  : Packet size to input and read out, in samples
  //  num_packets : Number of packets to input and check on the output
  //
  task automatic test_window(int port, int window_size, int packet_len, int num_packets);
    item_t coefficients[$];
    int coeff_index;
    mailbox #(item_queue_t) packets = new();

    $display("Testing: window_size=%04d, packet_len=%04d, num_packets=%04d",
      window_size, packet_len, num_packets);

    write_reg(port, REG_WINDOW_SIZE, window_size);

    // Generate random coefficients 
    for (int count=0; count < window_size; count++) begin
      item_t coeff;

      coeff = $urandom();
      coefficients.push_back(coeff);
      // Load each coefficient, but write the last coefficient to the "last"
      // register.
      if (count == window_size-1)
        write_reg(port, REG_LOAD_COEFF_LAST, coeff);
      else
        write_reg(port, REG_LOAD_COEFF, coeff);
    end

    // Updating the coefficients should restart the address counter
    coeff_index = 0;

    // Generate and enqueue packets for transmission
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      item_t payload[$];

      payload = {};
      for (int sample_count = 0; sample_count < packet_len; sample_count++) begin
        payload.push_back($urandom());
      end
      packets.put(payload);
      blk_ctrl.send_items(port, payload);
    end

    // Receive and check the results
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      item_t sent[$], received[$], expected[$];
      packets.get(sent);

      // Calculate the expected result
      foreach(sent[i]) begin
        expected[i] = mult_rc(coefficients[coeff_index], sent[i]);
        if (coeff_index >= window_size-1) coeff_index = 0;
        else coeff_index++;
      end

      // Retrieve the resulting packet
      blk_ctrl.recv_items(port, received);

      // Check that the packet length matches what was input
      `ASSERT_ERROR(
        expected.size() == received.size(), 
        $sformatf("For packet %0d, received length was incorrect", packet_count)
      );

      // Check that the payload is correct
      foreach(received[i]) begin
        `ASSERT_ERROR(
          received[i] == expected[i], 
          $sformatf("Unexpected result for packet %0d, sample %0d", packet_count, i)
        );
      end
    end

  endtask : test_window


  // Run random tests to look for anything unexpected
  //
  //   port            : Port number of the block to test
  //   num_iterations  : Number of times to repeat a random test
  //   max_window_size : Maximum window size to use (number of coefficients)
  //   max_packet_len  : Maximum packet length to use (in samples)
  //
  task automatic test_random(
    int port,
    int num_iterations  = 100,
    int max_window_size = MAX_WINDOW_SIZE,
    int max_packet_len  = SPP
  );
    test.start_test("Test random", 10ms);

    // Repeat the test num_iterations times
    for (int iteration = 0; iteration < num_iterations; iteration++) begin
      int window_size;
      int packet_len;
      int num_packets;
      int packets_per_window;

      // Choose random attributes for this test
      window_size = $urandom_range(1, MAX_WINDOW_SIZE);
      packet_len  = $urandom_range(1, SPP);

      // Send up to two windows worth of packets
      packets_per_window = $ceil(real'(window_size) / real'(packet_len));
      num_packets = $urandom_range(1, 2*packets_per_window);

      // Run the test
      test_window(port, window_size, packet_len, num_packets);
    end

    test.end_test();
  endtask : test_random


  // Run a few directed tests to check corner cases
  task automatic test_directed(int port);
    test.start_test("Test directed", 100us);
    test_window(port, 16,   1, 33);    // Min packet size
    test_window(port, 16,   8,  5);
    test_window(port, 16,  16,  3);    // Packet size equals window
    test_window(port, 16,  31,  3);    // Packet size larger than window
    test_window(port, 16, SPP,  3);    // Max packet size
    if (MAX_WINDOW_SIZE <= SPP) begin  // Max window size and packet size
      test_window(port, MAX_WINDOW_SIZE, MAX_WINDOW_SIZE, 2);
      test_window(port, MAX_WINDOW_SIZE, SPP, 2);
    end else begin
      test_window(port, MAX_WINDOW_SIZE, SPP, 2);
    end
    test.end_test();
  endtask : test_directed

  // Test with slow BFM slave to make sure back-pressure is working correctly.
  task automatic test_back_pressure(int port);
    test.start_test("Test back pressure", 1ms);
    blk_ctrl.set_slave_stall_prob(port, 90);
    test_window(port, 16, SPP, 20);
    blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    test.end_test();
  endtask : test_back_pressure


  // Test with slow BFM master to make sure AXI-stream flow control is working
  // correctly.
  task automatic test_underflow(int port);
    test.start_test("Test underflow", 1ms);
    blk_ctrl.set_master_stall_prob(port, 90);
    test_window(port, 16, SPP, 20);
    blk_ctrl.set_master_stall_prob(port, STALL_PROB);
    test.end_test();
  endtask : test_underflow


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    int port;

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_window_tb");

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

    // Run register and directed tests on all ports
    for (port = 0; port < NUM_PORTS; port++) begin
      test_registers(port);
      test_directed(port);
    end
    // Run remaining tests on just one port
    port = NUM_PORTS-1;
    test_random(port, 100);
    test_back_pressure(port);
    test_underflow(port);

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_window_tb


`default_nettype wire
