//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_moving_avg_tb
//
// Description: Testbench for the moving_avg RFNoC block.
//

`default_nettype none


module rfnoc_block_moving_avg_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  import PkgMovingAverage::MovingAverage;

  `include "rfnoc_block_moving_avg_regs.vh"


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'hAAD20000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;    // CHDR size in bits
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS       = 2;
  localparam int    NUM_PORTS_I     = NUM_PORTS;
  localparam int    NUM_PORTS_O     = NUM_PORTS;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz

  // Divisor data type (signed whole number)
  typedef bit signed [REG_DIVISOR_LEN-1:0] divisor_t;


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

  rfnoc_block_moving_avg #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS)
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

  // Write a 32-bit register
  task automatic write_reg(int port, bit [19:0] addr, bit [31:0] value);
    blk_ctrl.reg_write(port * (2**MOVING_AVG_ADDR_W) + addr, value);
  endtask : write_reg

  // Read a 32-bit register
  task automatic read_reg(int port, bit [19:0] addr, output logic [31:0] value);
    blk_ctrl.reg_read(port * (2**MOVING_AVG_ADDR_W) + addr, value);
  endtask : read_reg


  //---------------------------------------------------------------------------
  // Test Procedures
  //---------------------------------------------------------------------------

  // Run a single test on the block using random packet data.
  //
  //  port        : Port number of the block to test
  //  sum_length  : Number of samples to sum
  //  divisor     : Divisor value to use to get the average (may be negative)
  //  packet_len  : Packet size to input and read out, in samples
  //  num_packets : Number of packets to input and check on the output
  //  value       : If set, use this value of sample instead of random data
  //
  task automatic test_moving_avg(
    int port,
    int sum_length,
    int divisor,
    int packet_len,
    int num_packets,
    item_t sample = 'X
  );
    MovingAverage sum_i = new(), sum_q = new();
    mailbox #(item_queue_t) packets = new();

    $display("Testing: sum_length=%04d, divisor=%08d, packet_len=%04d, num_packets=%04d",
      sum_length, divisor, packet_len, num_packets);

    write_reg(port, REG_SUM_LENGTH, sum_length);
    write_reg(port, REG_DIVISOR, divisor);

    sum_i.set_sum_length(sum_length);
    sum_q.set_sum_length(sum_length);
    sum_i.set_divisor(divisor);
    sum_q.set_divisor(divisor);

    // Generate and enqueue packets for transmission
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      item_t payload[$];

      payload = {};
      for (int sample_count = 0; sample_count < packet_len; sample_count++) begin
        if (sample !== 'X) begin
          payload.push_back(sample);
        end else begin
          payload.push_back($urandom());
        end
      end
      packets.put(payload);
      blk_ctrl.send_items(port, payload);
    end

    // Receive and check the results
    for (int packet_count = 0; packet_count < num_packets; packet_count++) begin
      item_t sent[$], received[$];
      packets.get(sent);

      // Retrieve the resulting packet
      blk_ctrl.recv_items(port, received);

      // Check that the packet length matches what was input
      `ASSERT_ERROR(
        sent.size() == received.size(),
        $sformatf("For packet %0d, received length was incorrect", packet_count)
      );

      // Check that the payload is correct
      foreach(received[i]) begin
        item_t expected_val, sent_val, received_val;

        //$display("Sent %09d,%09d", signed'(sent[i][31:16]), signed'(sent[i][15:0]));

        // Calculate expected result
        sent_val = sent[i];
        sum_i.add_value(signed'(sent_val[31:16]));
        sum_q.add_value(signed'(sent_val[15: 0]));
        expected_val[31:16] = sum_i.get_average();
        expected_val[15: 0] = sum_q.get_average();

        received_val = received[i];
        if(received_val != expected_val) begin
          $display("Received %09d,%09d", signed'(received_val[31:16]), signed'(received_val[15:0]));
          $display("Expected %09d,%09d", signed'(expected_val[31:16]), signed'(expected_val[15:0]));
          `ASSERT_ERROR(
            0,
            $sformatf("Unexpected result for packet %0d, sample %0d.", packet_count, i)
          );
        end
      end
    end

  endtask : test_moving_avg


  // Test the registers for the indicated port. This checks initial values, so
  // it should be run first.
  task automatic test_registers(int port);
    logic [31:0] value;

    test.start_test("Test registers", 1ms);

    // Check initial values, to make sure a previous test didn't affect the
    // wrong port.
    read_reg(port, REG_SUM_LENGTH, value);
    `ASSERT_ERROR(value === {REG_SUM_LENGTH_LEN{1'bX}},
      "REG_SUM_LENGTH initial value didn't match expected value");

    read_reg(port, REG_DIVISOR, value);
    `ASSERT_ERROR(value === {REG_DIVISOR_LEN{1'bX}},
      "REG_DIVISOR_LEN initial value didn't match expected value");

    // Test writing 0 to the registers
    write_reg(port, REG_SUM_LENGTH, 0);
    read_reg(port, REG_SUM_LENGTH, value);
    `ASSERT_ERROR(value == 0, "REG_SUM_LENGTH didn't readback correctly");

    write_reg(port, REG_DIVISOR, 0);
    read_reg(port, REG_DIVISOR, value);
    `ASSERT_ERROR(value == 0, "REG_DIVISOR didn't readback correctly");

    // Test writing the max value to the registers
    write_reg(port, REG_SUM_LENGTH, '1);
    read_reg(port, REG_SUM_LENGTH, value);
    `ASSERT_ERROR(value == {REG_SUM_LENGTH_LEN{1'b1}},
      "REG_SUM_LENGTH didn't readback correctly");

    write_reg(port, REG_DIVISOR, '1);
    read_reg(port, REG_DIVISOR, value);
    `ASSERT_ERROR(value == {REG_DIVISOR_LEN{1'b1}},
      "REG_DIVISOR didn't readback correctly");

    test.end_test();
  endtask : test_registers


  // Run random test and look for anything unexpected
  //
  //   port            : Port number of the block to test
  //   num_iterations  : Number of times to repeat a random test
  //   max_packet_len  : Maximum packet length to use, in samples
  //
  task automatic test_random_config(
    int port,
    int num_iterations  = 100,
    int max_packet_len  = SPP
  );
    test.start_test("Test random", 10ms);

    // Repeat the test num_iterations times
    for (int iteration = 0; iteration < num_iterations; iteration++) begin

      int sum_length;
      int divisor;
      int packet_len;
      int num_packets;

      // Choose random attributes for this test
      sum_length  = $urandom_range(1, 2**REG_SUM_LENGTH_LEN-1);
      // Limit the divisor so we aren't outputting 0 all the time
      divisor     = divisor_t'($urandom_range(1, 20*sum_length));
      if ($urandom_range(0,1)) divisor = -divisor;
      packet_len  = $urandom_range(1, SPP);
      num_packets = $urandom_range(3, 8);

      // Run the test
      test_moving_avg(port, sum_length, divisor, packet_len, num_packets);
    end

    test.end_test();
  endtask : test_random_config


  // Run some quick basic tests
  task automatic test_basic(int port);
    test.start_test($sformatf("Test basic, port %0d", port), 100us);

    // Minimum sum and divisor values (Data in should match data out)
    test_moving_avg(.port(port), .sum_length(1), .divisor(1),
      .packet_len(SPP), .num_packets(3));
    // Input samples negated
    test_moving_avg(.port(port), .sum_length(1), .divisor(-1),
      .packet_len(SPP), .num_packets(3));
    // Input samples divided by 2
    test_moving_avg(.port(port), .sum_length(1), .divisor(2),
      .packet_len(SPP), .num_packets(3));
    // Sum of two samples
    test_moving_avg(.port(port), .sum_length(2), .divisor(1),
      .packet_len(SPP), .num_packets(3));
    // Average of two samples
    test_moving_avg(.port(port), .sum_length(2), .divisor(2),
      .packet_len(SPP), .num_packets(3));

    test.end_test();
  endtask : test_basic


  // Test maximum and minimum of length, divisor, and sample value to check
  // the corner cases of the DUT's computation.
  task automatic test_max_values(int port);
    int max_length, max_divisor, min_divisor;

    test.start_test("Test max values", 1ms);

    max_length  =  2**REG_SUM_LENGTH_LEN-1;
    max_divisor =  2**(REG_DIVISOR_LEN-1)-1;
    min_divisor = -2**(REG_DIVISOR_LEN-1);

    // Send 3*max_length/SPP packets in each case to make sure we fill the
    // history buffer of the DUT.

    // Maximum allowed sum length
    test_moving_avg(.port(port), .sum_length(max_length), .divisor(max_length),
      .packet_len(SPP), .num_packets(3*max_length/SPP));
    // Maximum divisor
    test_moving_avg(.port(port), .sum_length(max_length), .divisor(max_divisor),
      .packet_len(SPP), .num_packets(3*max_length/SPP));
    // Minimum divisor
    test_moving_avg(.port(port), .sum_length(max_length), .divisor(min_divisor),
      .packet_len(SPP), .num_packets(3*max_length/SPP));

    // Make sure we don't overflow the internal sum.
    // Maximum sample value (32767)
    test_moving_avg(.port(port), .sum_length(max_length), .divisor(max_length),
      .packet_len(SPP), .num_packets(3*max_length/SPP), .sample(32'h7FFF_7FFF));
    // Minimum sample value (-32768)
    test_moving_avg(.port(port), .sum_length(max_length), .divisor(max_length),
      .packet_len(SPP), .num_packets(3*max_length/SPP), .sample(32'h8000_8000));

    test.end_test();
  endtask : test_max_values


  // Test with slow BFM slave to make sure back-pressure is working correctly.
  task automatic test_back_pressure(int port);
    test.start_test("Test back pressure", 1ms);
    blk_ctrl.set_slave_stall_prob(port, 90);
    test_moving_avg(port, 16, 16, SPP, 20);
    blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    test.end_test();
  endtask : test_back_pressure


  // Test with slow BFM master to make sure AXI-stream flow control is working
  // correctly.
  task automatic test_underflow(int port);
    test.start_test("Test underflow", 1ms);
    blk_ctrl.set_master_stall_prob(port, 90);
    test_moving_avg(port, 16, 16, SPP, 20);
    blk_ctrl.set_master_stall_prob(port, STALL_PROB);
    test.end_test();
  endtask : test_underflow


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    int port;

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_moving_avg_tb");

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

    // Run the basic tests on all ports
    for (port = 0; port < NUM_PORTS; port++) begin
      test_registers(port);
      test_basic(port);
    end

    // Run remaining tests on a single port
    port = 1;
    test_max_values(port);
    test_back_pressure(port);
    test_underflow(port);
    test_random_config(port, 100);

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_moving_avg_tb


`default_nettype wire
