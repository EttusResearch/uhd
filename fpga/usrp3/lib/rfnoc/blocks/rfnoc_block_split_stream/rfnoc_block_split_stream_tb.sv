//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_split_stream_tb
//
// Description: Testbench for the split_stream RFNoC block.
//
// Parameters: These set the parameters applied to the RFNoC block. See the
//             RFNoC block for an explanation of these parameters.
//

`default_nettype none


module rfnoc_block_split_stream_tb #(
  parameter int CHDR_W       = 64,
  parameter int NUM_PORTS    = 1,
  parameter int NUM_BRANCHES = 2
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam [31:0] NOC_ID          = 32'h57570000;
  localparam int    ITEM_W          = 32;
  localparam int    NUM_PORTS_I     = NUM_PORTS;
  localparam int    NUM_PORTS_O     = NUM_PORTS*NUM_BRANCHES;
  localparam int    MTU             = $clog2(512);
  localparam int    SPP             = 128;
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 25;      // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;     // 200 MHz
  localparam real   CTRL_CLK_PER    = 25.0;    // 40 MHz


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

  rfnoc_block_split_stream #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS),
    .NUM_BRANCHES        (NUM_BRANCHES)
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
  // Tready Monitor
  //---------------------------------------------------------------------------
  //
  // Make sure we test the case where the data backs up through the splitter.
  //
  //---------------------------------------------------------------------------

  // Number of stalled cycles before we say the input has completely backed up:
  const int STALL_THRESH = 50;

  // Counters to track how many times the input stalled (externally at the
  // RFNoC block inputs and internally at the splitter).
  int input_stall_count = 0;
  int split_stall_count = 0;

  // Check for stalls on the input to the RFNoC block
  always @(posedge rfnoc_chdr_clk) begin : check_input_stall
    static int count;
    if (s_rfnoc_chdr_tvalid[0] === 1'b1 && s_rfnoc_chdr_tready[0] === 1'b0) begin
      count++;
      if (count == STALL_THRESH) begin
        input_stall_count++;
      end
    end else begin
      count=0;      
    end
  end

  // Check for stalls on the input to the splitter, inside the RFNoC block
  always @(posedge rfnoc_chdr_clk) begin : check_splitter_stall
    static int count;
    if (dut.gen_splitters[0].in_tvalid === 1'b1 && dut.gen_splitters[0].in_tready === 1'b0) begin
      count++;
      if (count == STALL_THRESH) begin
        split_stall_count++;
      end
    end else begin
      count=0;      
    end
  end


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
  //   port:         Port to use
  //   num_packets:  Number of packets to input
  //   max_samps:    Maximum length of packet to simulate in samples. Packet 
  //                 length is randomly chosen using a uniform distribution.
  //   stall_prob_m: Stall probability to use in the master BFM (block input)
  //   stall_prob_s: Stall probability to use in the slave BFMs (block outputs)
  //
  task automatic test_rand(
    int port          = 0,
    int num_packets   = 100,
    int max_samps     = SPP,
    int stall_prob_m  = STALL_PROB,
    int stall_prob_s  = STALL_PROB
  );
    // References to the simulation BFMs
    ChdrIfaceBfm #(CHDR_W, ITEM_W) master_bfm;
    ChdrIfaceBfm #(CHDR_W, ITEM_W) slave_bfm[NUM_BRANCHES];

    // Use mailbox to communicate packets between master and slave processes
    mailbox #(ChdrPacket_t) packets = new();

    // Set the probability of stalling at each interface
    blk_ctrl.set_master_stall_prob(port,    stall_prob_m);
    for (int branch=0; branch < NUM_BRANCHES; branch++)
      blk_ctrl.set_slave_stall_prob(NUM_PORTS*branch + port, stall_prob_s);

    // Grab references to the underlying CHDR BFMs
    master_bfm = blk_ctrl.get_master_data_bfm(port);
    foreach (slave_bfm[branch])
      slave_bfm[branch] = blk_ctrl.get_slave_data_bfm(NUM_PORTS*branch + port);

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

          // Get a packet from each branch of the output and verify its
          // contents.
          foreach (slave_bfm[branch]) begin
            string error_msg;
            $sformat(
              error_msg, 
              "Received packet on branch %d of port %d did not match input packet",
              branch, port
            );
            slave_bfm[branch].get_chdr(packet);
            `ASSERT_ERROR(packet.equal(expected), error_msg);
          end
        end
      end
    join
  endtask : test_rand


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Initialize the test exec object for this testbench
    tb_name = $sformatf(
      "rfnoc_block_split_stream_tb\nCHDR_W = %0D, NUM_PORTS = %0D, NUM_BRANCHES = %0D",
      CHDR_W, NUM_PORTS, NUM_BRANCHES
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

    test.start_test("Test short packets", 1ms);
    test_rand(0, 1000, 4*CHDR_W/ITEM_W);
    test.end_test();

    test.start_test("Test long packets", 1ms);
    test_rand(0, 100, SPP);
    test.end_test();

    test.start_test("Test short packets, fast source, slow sink", 1ms);
    test_rand(0, 1000, 4*CHDR_W/ITEM_W, 10, 90);
    test.end_test();

    test.start_test("Test long packets, fast source, slow sink", 1ms);
    test_rand(0, 200, SPP, 10, 90);
    test.end_test();

    if (NUM_PORTS > 1) begin
      test.start_test("Test another port", 1ms);
      // All ports are identical. Do a random test on last port to make sure
      // additional ports are correctly connected.
      test_rand(NUM_PORTS-1, 100, 4*CHDR_W/ITEM_W);
      test.end_test();
    end

    test.start_test("Check input stall", 1us);
    // Make sure data backed up into the block several times to make sure we've
    // tested flow control of the splitter.
    `ASSERT_ERROR(input_stall_count > 20, "Input never filled during tests");
    `ASSERT_ERROR(split_stall_count > 20, "Splitter never filled during tests");
    test.end_test();

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

endmodule : rfnoc_block_split_stream_tb


`default_nettype wire
