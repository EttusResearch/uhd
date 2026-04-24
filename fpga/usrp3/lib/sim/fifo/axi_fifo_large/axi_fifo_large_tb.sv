//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo_large_tb
//
// Description:
//
//   Testbench for axi_fifo_large.
//
//   Ensures correct flow of data and accurate reporting of space/occupied
//   counts at start and end of test. Will not fill up the FIFO since this would
//   be very time consuming for larger sizes.
//

`default_nettype none


module axi_fifo_large_tb #(
  int WIDTH               = 72,
  int DEPTH               = 4096,
  string DEVICE           = "ULTRASCALE",
  int MAX_NUM_URAM_BLOCKS = -1
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD = 10.0;  // 100 MHz
  localparam time TIMEOUT    = 50ms;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // AXI-Stream Interfaces and BFM
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(WIDTH), .TKEEP(0), .TUSER(0)) i_axis (.clk(clk), .rst(rst));
  AxiStreamIf #(.DATA_WIDTH(WIDTH), .TKEEP(0), .TUSER(0)) o_axis (.clk(clk), .rst(rst));

  AxiStreamBfm #(.DATA_WIDTH(WIDTH), .TKEEP(0), .TUSER(0)) axis_bfm;


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  wire [19:0] space;
  wire [19:0] occupied;

  // For the single-FIFO path (DEPTH <= 4096) the DUT instantiates axi_fifo whose
  // capacity is the next power of 2 >= DEPTH, plus 1 for the output register.
  // For the cascade path (DEPTH > 4096) capacity is rounded up to the next
  // multiple of 512 (BRAM_MIN_DEPTH).
  localparam int EXPECTED_SPACE = (DEPTH <= 4096) ?
      (1 << $clog2(DEPTH)) + 1 :
      ((DEPTH + 511) / 512) * 512;

  axi_fifo_large #(
    .WIDTH               (WIDTH),
    .DEPTH               (DEPTH),
    .DEVICE              (DEVICE),
    .MAX_NUM_URAM_BLOCKS (MAX_NUM_URAM_BLOCKS)
  ) dut (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  (i_axis.tdata),
    .i_tvalid (i_axis.tvalid),
    .i_tready (i_axis.tready),
    .o_tdata  (o_axis.tdata),
    .o_tvalid (o_axis.tvalid),
    .o_tready (o_axis.tready),
    .space    (space),
    .occupied (occupied)
  );

  // The FIFO has no tlast port. Tie o_axis.tlast high so the BFM slave
  // treats every received word as a complete single-word packet.
  assign o_axis.tlast = 1'b1;


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------
  typedef AxiStreamPacket #(WIDTH, 1) AxisPacket_t;

  // Total number of words to send per test
  // Just a few to test the functionality. Assume each FIFO in there is working
  // fine in itself.
  localparam int NUM_WORDS = 100;

  // Send NUM_WORDS random single-word packets and verify received data matches.
  task automatic run_traffic_test(string test_name, int master_stall, int slave_stall);
    AxisPacket_t tx_pkts[$];
    AxisPacket_t rx_pkt;

    test.start_test(test_name, TIMEOUT);

    axis_bfm.set_master_stall_prob(master_stall);
    axis_bfm.set_slave_stall_prob(slave_stall);

    // Generate and enqueue all TX words
    for (int p = 0; p < NUM_WORDS; p++) begin
      AxisPacket_t pkt = new();
      pkt.data.push_back(Rand#(WIDTH)::rand_bit());
      tx_pkts.push_back(pkt);
      axis_bfm.put(pkt.copy());
    end

    // Receive and compare
    for (int p = 0; p < NUM_WORDS; p++) begin
      axis_bfm.get(rx_pkt);
      `ASSERT_ERROR(
        tx_pkts[p].data[0] === rx_pkt.data[0],
        $sformatf("Word %0d data mismatch! TX=%0h RX=%0h",
                  p, tx_pkts[p].data[0], rx_pkt.data[0])
      )
    end

    // Verify FIFO is empty
    repeat (20) @(posedge clk);
    `ASSERT_ERROR(occupied == 0,
      $sformatf("FIFO not empty after test, occupied = %0d", occupied))
    `ASSERT_ERROR(EXPECTED_SPACE == space,
      $sformatf("FIFO space mismatch after drain, expected %0d got %0d",
                EXPECTED_SPACE, space))

    test.end_test();
  endtask


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : main
    // stop all clock events for simulation performance
    clk_gen.kill();

    begin
      string s;
      $sformat(s, "axi_fifo_large_tb - DEPTH=%0d MAX_NUM_URAM_BLOCKS=%0d WIDTH=%0d", DEPTH, MAX_NUM_URAM_BLOCKS, WIDTH);
      test.start_tb(s);
    end

    clk_gen.revive();

    // Setup BFM
    axis_bfm = new(i_axis.master, o_axis.slave);
    axis_bfm.run();

    // Reset
    clk_gen.start();
    clk_gen.reset(20);
    @(negedge rst);  // Wait for reset to deassert
    repeat (10) @(posedge clk);
    `ASSERT_ERROR(EXPECTED_SPACE == space,
      $sformatf("FIFO space mismatch after reset, expected %0d got %0d",
                EXPECTED_SPACE, space))

    run_traffic_test("Slow master / slow slave", 50, 50);

    //---------------------------------------
    // Done
    //---------------------------------------
    test.end_tb(0);
    clk_gen.kill();
  end : main

endmodule : axi_fifo_large_tb


`default_nettype wire
