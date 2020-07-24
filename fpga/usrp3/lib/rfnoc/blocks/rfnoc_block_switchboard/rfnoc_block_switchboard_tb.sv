//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_switchboard_tb
//
// Description: Testbench for the switchboard RFNoC block.
//

`default_nettype none


module rfnoc_block_switchboard_tb #(
  parameter int CHDR_W              = 64,    // CHDR size in bits
  parameter int NUM_INPUTS          = 1,
  parameter int NUM_OUTPUTS         = 1  
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  `include "rfnoc_block_switchboard_regs.vh"

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [31:0] NOC_ID          = 32'hBE110000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    MTU             = 10;    // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS_I     = NUM_INPUTS;
  localparam int    NUM_PORTS_O     = NUM_OUTPUTS;
  localparam int    ITEM_W          = 32;    // Sample size in bits
  localparam int    SPP             = 64;    // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;    // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;

  // Don't start the clocks automatically (AUTOSTART=0), since we expect
  // multiple instances of this testbench to run in sequence. They will be
  // started before the first test.
  sim_clock_gen #(.PERIOD(CHDR_CLK_PER), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CTRL_CLK_PER), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());

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

  typedef ChdrPacket #(CHDR_W) ChdrPacket_t;

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

  rfnoc_block_switchboard #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_INPUTS          (NUM_INPUTS),
    .NUM_OUTPUTS         (NUM_OUTPUTS)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
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
    blk_ctrl.reg_write((2**SWITCH_ADDR_W)*port + addr, value);
  endtask : write_reg

  // Read a 32-bit register
  task automatic read_reg(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**SWITCH_ADDR_W)*port + addr, value[31:0]);
  endtask : read_reg

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

  // Generate a random CHDR packet with the given number of samples
  function automatic ChdrPacket_t gen_rand_chdr_pkt(int num_samps);
    ChdrPacket_t  packet = new();
    chdr_header_t header;
    chdr_word_t   data[$];

    // Generate a random CHDR packet. I'm not going to randomly change the
    // timestamp or metadata, because the split-stream block doesn't look
    // at any of that.

    // Mostly random header
    header = Rand#($bits(header))::rand_logic();
    header.pkt_type  = CHDR_DATA_NO_TS;
    header.num_mdata = 0;
    header.length    = CHDR_W/8 + num_samps*ITEM_W/8;  // Header + payload

    // Random payload
    repeat (num_samps * ITEM_W / CHDR_W) 
      data.push_back(Rand#(CHDR_W)::rand_logic());
    // Round up to nearest CHDR word
    if (num_samps * ITEM_W % CHDR_W != 0)
      data.push_back(Rand#(CHDR_W)::rand_logic());

    // Build packet
    packet.write_raw(header, data);

    return packet;
  endfunction : gen_rand_chdr_pkt

  // Performs a randomized test, inputting random packets then checking the
  // outputs.
  //
  //   input_port:   Input port to use
  //   output_port:  Output port to use
  //   num_packets:  Number of packets to input
  //   max_samps:    Maximum length of packet to simulate in samples. Packet 
  //                 length is randomly chosen using a uniform distribution.
  //
  task automatic test_stream(
    int input_port    = 0,
    int output_port   = 0,
    int num_packets   = 100,
    int max_samps     = SPP
  );
    // References to the simulation BFMs
    ChdrIfaceBfm #(CHDR_W, ITEM_W) master_bfm;
    ChdrIfaceBfm #(CHDR_W, ITEM_W) slave_bfm;

    // Use mailbox to communicate packets between master and slave processes
    mailbox #(ChdrPacket_t) packets = new();

    // Grab references to the underlying CHDR BFMs
    master_bfm = blk_ctrl.get_master_data_bfm(input_port);
    slave_bfm = blk_ctrl.get_slave_data_bfm(output_port);

    write_reg(input_port, REG_DEMUX_SELECT, output_port);
    write_reg(output_port, REG_MUX_SELECT, input_port);

    fork
      //-----------------------------------------
      // Master
      //-----------------------------------------
      begin : master
        ChdrPacket_t packet;
        repeat (num_packets) begin
          packet = gen_rand_chdr_pkt($urandom_range(max_samps));
          packets.put(packet);
          master_bfm.put_chdr(packet); 
        end
      end

      //-----------------------------------------
      // Slaves
      //-----------------------------------------
      begin : slaves
        ChdrPacket_t expected, packet;

        repeat (num_packets) begin
          // Get the expected packet from the mailbox
          packets.get(expected);
          slave_bfm.get_chdr(packet);
          `ASSERT_ERROR(packet.equal(expected), "Does not match");
        end
      end
    join
  endtask : test_stream

  // Tests 2 connections concurrently
  task automatic test_concurrent(
    int num_packets = 100,
    int max_samps = SPP
  );
    fork
      begin : Stream_A
        test_stream(0, 0, num_packets, max_samps);
      end

      begin : Stream_B
        test_stream(NUM_INPUTS-1, NUM_OUTPUTS-1, num_packets, max_samps);
      end
    join
  endtask : test_concurrent

  // Randomized register test
  task automatic test_reg(
    int port = 0,
    int offset = 0,
    int limit = 0
  );
    int rand_select = $urandom_range(0, limit);
    int read_output;
    write_reg(port, offset, 0);
    read_reg(port, offset, read_output);
    `ASSERT_ERROR(read_output == 0, "Register data does not match");

    write_reg(port, offset, rand_select);
    read_reg(port, offset, read_output);
    `ASSERT_ERROR(read_output == rand_select, "Register data does not match");

    write_reg(port, offset, limit);
    read_reg(port, offset, read_output);
    `ASSERT_ERROR(read_output == limit, "Register data does not match");
  endtask : test_reg

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Initialize the test exec object for this testbench
    tb_name = $sformatf(
      "rfnoc_block_switch_tb\nCHDR_W = %0D, NUM_INPUTS = %0D, NUM_OUTPUTS = %0D",
      CHDR_W, NUM_INPUTS, NUM_OUTPUTS
    );
    test.start_tb(tb_name);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();

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

    // Register Test

    test.start_test("Test Register RW", 10us);
    test_reg(0,             REG_DEMUX_SELECT,   NUM_OUTPUTS-1);
    test_reg(NUM_INPUTS-1,  REG_DEMUX_SELECT,   NUM_OUTPUTS-1);
    test_reg(0,             REG_MUX_SELECT,     NUM_INPUTS-1);
    test_reg(NUM_OUTPUTS-1, REG_MUX_SELECT,     NUM_INPUTS-1);
    test.end_test();

    // Stream Test

    test.start_test("Test short packets", 1ms);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      1000, 4*CHDR_W/ITEM_W);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      1000, 4*CHDR_W/ITEM_W);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      1000, 4*CHDR_W/ITEM_W);
    test.end_test();

    test.start_test("Test long packets", 1ms);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      100, SPP);
    test.end_test();

    test.start_test("Test short packets, fast source, slow sink", 1ms);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      1000, 4*CHDR_W/ITEM_W);
    test.end_test();

    test.start_test("Test long packets, fast source, slow sink", 1ms);
    test_stream($urandom_range(0, NUM_INPUTS-1), $urandom_range(0, NUM_OUTPUTS-1), 
      200, SPP);
    test.end_test();

    if(NUM_INPUTS > 1 && NUM_OUTPUTS > 1) begin
      test.start_test("Test concurrent streams", 1ms);
      test_concurrent(100, 4*CHDR_W/ITEM_W);
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
  end : tb_main

endmodule : rfnoc_block_switchboard_tb


`default_nettype wire
