//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_demux_tb.sv
//
// Description:
//
//   A testbench for the chdr_channel_demux module.
//

`default_nettype none

module chdr_channel_demux_tb #(
  parameter int NUM_PORTS      = 2,
  parameter int CHDR_W         = 64,
  parameter int CHANNEL_OFFSET = 0
) ();

  `include "usrp_utils.svh"
  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrBfm::*;
  import PkgRandom::*;


  //---------------------------------------------------------------------------
  // Test Parameters
  //---------------------------------------------------------------------------

  localparam int  PRE_FIFO_SIZE  = 1;
  localparam int  POST_FIFO_SIZE = 1;
  localparam real CLK_PERIOD     = 10.0;
  localparam int  MIN_PYLD_SIZE  = 0;
  localparam int  MAX_PYLD_SIZE  = CHDR_W/8 * 16;
  localparam int  MIN_MDATA_SIZE = 0;
  localparam int  MAX_MDATA_SIZE = 3;
  localparam int  DEF_STALL_PROB = 38;

  localparam int NUM_TESTS     = 10;
  localparam int PKTS_PER_TEST = 100;


  //---------------------------------------------------------------------------
  // Type Definitions
  //---------------------------------------------------------------------------

  typedef ChdrPacket#(.CHDR_W(CHDR_W)) chdr_pkt_t;
  typedef chdr_pkt_t                   chdr_pkt_queue_t[$];


  //---------------------------------------------------------------------------
  // Local Signals
  //---------------------------------------------------------------------------

  logic                             clk;
  logic                             rst;
  logic [   CHDR_W-1:0]             in_tdata;
  logic                             in_tvalid;
  logic                             in_tlast;
  logic                             in_tready;
  logic [NUM_PORTS-1:0][CHDR_W-1:0] out_tdata;
  logic [NUM_PORTS-1:0]             out_tvalid;
  logic [NUM_PORTS-1:0]             out_tlast;
  logic [NUM_PORTS-1:0]             out_tready;


  //---------------------------------------------------------------------------
  // Clocking and Reset
  //---------------------------------------------------------------------------

  sim_clock_gen #(
    .PERIOD   (CLK_PERIOD),
    .AUTOSTART(0         )
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // AXI Stream interface for the DUT inputs
  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W)
  ) to_dut (
    .clk(clk),
    .rst(rst)
  );

  // AXI Stream interface for the DUT output
  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W)
  ) from_dut[NUM_PORTS] (
    .clk(clk),
    .rst(rst)
  );

  // Chdr BFMs
  ChdrBfm #(.CHDR_W(CHDR_W)) chdr_bfm[NUM_PORTS];

  // Connect the BFMs to the interfaces
  generate
    // Use BFM 0 for both the single input port and the first output port of
    // the DUT.
    initial begin
      chdr_bfm[0] = new(to_dut, from_dut[0]);
    end
    // Connect the remaining BFMs to only the corresponding output ports of the
    // DUT.
    for (genvar port = 1; port < NUM_PORTS; port++) begin : gen_bfm_connections
      initial begin
        chdr_bfm[port] = new(null, from_dut[port]);
      end
    end
    // Connect the interfaces to signals
    assign in_tdata      = to_dut.tdata;
    assign in_tvalid     = to_dut.tvalid;
    assign in_tlast      = to_dut.tlast;
    assign to_dut.tready = in_tready;
    for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_input_connections
      assign from_dut[port].tdata  = out_tdata[port];
      assign from_dut[port].tvalid = out_tvalid[port];
      assign from_dut[port].tlast  = out_tlast[port];
      assign out_tready[port]      = from_dut[port].tready;
    end
    endgenerate


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  chdr_channel_demux #(
    .NUM_PORTS     (NUM_PORTS     ),
    .CHDR_W        (CHDR_W        ),
    .CHANNEL_OFFSET(CHANNEL_OFFSET),
    .PRE_FIFO_SIZE (PRE_FIFO_SIZE ),
    .POST_FIFO_SIZE(POST_FIFO_SIZE)
  ) dut (
    .clk       (clk       ),
    .rst       (rst       ),
    .in_tdata  (in_tdata  ),
    .in_tvalid (in_tvalid ),
    .in_tlast  (in_tlast  ),
    .in_tready (in_tready ),
    .out_tdata (out_tdata ),
    .out_tvalid(out_tvalid),
    .out_tlast (out_tlast ),
    .out_tready(out_tready)
  );


  //---------------------------------------------------------------------------
  // Helper Tasks and Functions
  //---------------------------------------------------------------------------


  // Creates and returns a randomly generated CHDR packet
  function automatic chdr_pkt_t random_chdr_packet();
    chdr_pkt_t pkt = new();
    int pyld_size;
    int num_mdata;
    int num_words;

    // Choose random metadata and payload lengths
    pyld_size = $urandom_range(MIN_PYLD_SIZE, MAX_PYLD_SIZE);
    num_mdata = $urandom_range(MIN_MDATA_SIZE, MAX_MDATA_SIZE);
    num_words = $ceil(real'(pyld_size) / (CHDR_W/8));

    // Create header for this random packet. Only the VC really value matters
    // for our testbench, but we'll set the length fields too to make sure the
    // BFMs are happy.
    pkt.header = Rand#(CHDR_HEADER_W)::rand_bit();
    pkt.timestamp = Rand#(CHDR_TIMESTAMP_W)::rand_bit();
    pkt.header.vc = $urandom_range(CHANNEL_OFFSET, CHANNEL_OFFSET + NUM_PORTS - 1);
    pkt.header.num_mdata = num_mdata;
    pkt.header = chdr_update_length(CHDR_W, pkt.header, pyld_size);

    repeat (num_mdata) pkt.metadata.push_back(Rand#(CHDR_W)::rand_bit());
    repeat (num_words) pkt.data.push_back(Rand#(CHDR_W)::rand_bit());

    return pkt;
  endfunction


  //---------------------------------------------------------------------------
  // Testcases
  //---------------------------------------------------------------------------

  // Tests writing the indicated number of packets through the DUT while
  // simultaneously monitoring the output and checking for errors.
  task automatic test_rand_packets(int num_packets);
    semaphore pkt_count_sm = new (1);
    int pkt_count = 0;

    // Mailboxes to communicate the expected packets to the readers
    mailbox #(chdr_pkt_t) exp_pkts_mb [NUM_PORTS];

    // Semaphore to know when all the readers are done checking their ports
    semaphore reader_sm = new();

    foreach (exp_pkts_mb[port]) begin
      exp_pkts_mb[port] = new();
    end

    fork
      //-----------------------------------------------------------------------
      // Input Thread
      //-----------------------------------------------------------------------

      begin : writer
        chdr_pkt_t pkt;

        // Write num_packets, choosing a random VC for each one
        for (int count = 0; count < num_packets; count++) begin
          pkt = random_chdr_packet();
          chdr_bfm[0].put_chdr(pkt.copy());
          exp_pkts_mb[pkt.header.vc-CHANNEL_OFFSET].put(pkt);
        end

        // Let the reader threads know we're done by sending a null packet to
        // each reader.
        foreach (exp_pkts_mb[ch]) begin
          exp_pkts_mb[ch].put(null);
        end
      end

      //-----------------------------------------------------------------------
      // Output Thread and Checker
      //-----------------------------------------------------------------------

      // Create one thread per output port
      for (int port_count = 0; port_count < NUM_PORTS; port_count++) begin
        fork : reader_thread
          // This line below ensures that each thread knows which port it is.
          automatic int port = port_count;
          begin
            chdr_pkt_t act_pkt, exp_pkt;  // Actual and expected packets
            int out_pkt_count = 0;
            forever begin
              // Wait for next packet from write thread. If it's null, the last
              // packet was already sent.
              exp_pkts_mb[port].get(exp_pkt);
              if (exp_pkt == null) break;

              // Grab the next packet from the DUT
              chdr_bfm[port].get_chdr(act_pkt);

              // Make sure it matches what we sent
              pkt_count_sm.get();
              `ASSERT_ERROR(act_pkt.equal(exp_pkt), $sformatf(
                "Output packet %0d on port %0d (input packet %0d) does not match",
                out_pkt_count, port, pkt_count));
              pkt_count++;
              pkt_count_sm.put();
              out_pkt_count++;
            end
            reader_sm.put(1);
          end
        join_none : reader_thread
      end
    join

    // Wait for all readers to finish
    reader_sm.get(NUM_PORTS);

    // As a sanity check, make sure we verified the expected number of packets.
    `ASSERT_ERROR(pkt_count == num_packets,
      $sformatf("Only verified %0d packets out of %0d!", pkt_count, num_packets));

  endtask


  //---------------------------------------------------------------------------
  // Test Execution
  //---------------------------------------------------------------------------

  initial begin
    string tb_name;

    typedef struct {
      int m_prob;
      int s_prob;
    } stall_prob_t;

    automatic stall_prob_t stall_speeds [3] = '{
      '{DEF_STALL_PROB, DEF_STALL_PROB}, // Equal write/read
      '{10, 90},                         // Fast write, slow read
      '{90, 10}                          // Slow write, fast ready
    };

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf({
      "rfnoc_block_fft_tb\n",
      "\tNUM_PORTS                = %0d\n",
      "\tCHDR_W                   = %0d\n",
      "\tCHANNEL_OFFSET           = %0d\n"},
      NUM_PORTS, CHDR_W, CHANNEL_OFFSET
    );

    test.start_tb(tb_name, 10ms);

    // Start the clocks
    clk_gen.start();

    // Start the BFMs
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].run();
    end

    foreach (stall_speeds[idx]) begin
      // Configure the BFM rates
      foreach (chdr_bfm[ch]) begin
        chdr_bfm[ch].set_master_stall_prob(stall_speeds[idx].m_prob);
        chdr_bfm[ch].set_slave_stall_prob(stall_speeds[idx].s_prob);
      end

      // Run the tests with these rates
      for (int test_idx = 0; test_idx < NUM_TESTS; test_idx++) begin
        test.start_test($sformatf(
          "Random iteration %0d (%0d, %0d)", test_idx,
          stall_speeds[idx].m_prob, stall_speeds[idx].s_prob)
        );

        // Reset between tests
        clk_gen.reset();
        @(negedge rst);

        // Run the test
        test_rand_packets(PKTS_PER_TEST);

        test.end_test();
      end
    end

    // Cleanup after all tests have run
    test.end_tb(0);
    clk_gen.kill();

  end

endmodule

`default_nettype wire
