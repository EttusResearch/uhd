//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_load_split_merge_tb
//
// Description:
//
//   Testbench for axis_load_split and axis_load_merge modules. It tests both
//   modules together but taking an input data stream and splitting it into
//   multiple streams, then combining them back together to recover the
//   original data stream.
//
// Parameters:
//
//   MAX_PKT_LEN   : Maximum packet length to test, in number of DATA_W words.
//   DATA_W        : Width of the input data port to axis_load_split and the
//                   output data port of axis_load_merge.
//   USER_W        : Width of TUSER to test.
//   INT_DATA_W    : Internal data width, at the output of axis_load_split and
//                   the input to axis_load_merge.
//   NUM_PORTS     : The number of ports to split into and merge.
//   INT_FIFO_SIZE : Log base 2 of the FIFO size (in units of DATA_W sized
//                   words) to configure for the output FIFOs in
//                   axis_load_split and the input FIFOs in axis_load_merge.
//   EXT_FIFO_SIZE : Log base 2 of the FIFO size (in units of DATA_W sized
//                   words) to configure for the input FIFO in axis_load_split
//                   and the output FIFO in axis_load_merge.
//

module axis_load_split_merge_tb #(
  int MAX_PKT_LEN   = 16,
  int DATA_W        = 32,
  int USER_W        = 4,
  int INT_DATA_W    = 8,
  int NUM_PORTS     = DATA_W/INT_DATA_W,
  int INT_FIFO_SIZE = $clog2(MAX_PKT_LEN),
  int EXT_FIFO_SIZE = 1
);

  `include "test_exec.svh"
  `include "usrp_utils.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;

  localparam real CLK_PER = 5;

  // Number of packets to test
  localparam NUM_PKTS = NUM_PORTS*100;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PER), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // AXI-Stream Interfaces
  AxiStreamIf #(DATA_W, USER_W) i_axis (clk, rst);
  AxiStreamIf #(DATA_W, USER_W) o_axis (clk, rst);

  // AXI-Stream BFMs
  typedef AxiStreamBfm #(DATA_W, USER_W) bfm_t;
  typedef bfm_t::AxisPacket_t pkt_t;
  bfm_t bfm = new(i_axis, o_axis);


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic [INT_DATA_W-1:0] split_tdata  [NUM_PORTS];
  logic [    USER_W-1:0] split_tuser  [NUM_PORTS];
  logic                  split_tlast  [NUM_PORTS];
  logic                  split_tvalid [NUM_PORTS];
  logic                  split_tready [NUM_PORTS];

  // Convert the internal FIFO size from the external word size (DATA_W) to the
  // internal word size (INT_DATA_W).
  localparam int INT_WORD_FIFO_SIZE = INT_FIFO_SIZE + $clog2(DATA_W/INT_DATA_W);

  axis_load_split #(
    .IN_DATA_W    (DATA_W            ),
    .IN_FIFO_SIZE (EXT_FIFO_SIZE     ),
    .OUT_DATA_W   (INT_DATA_W        ),
    .OUT_FIFO_SIZE(INT_WORD_FIFO_SIZE),
    .OUT_NUM_PORTS(NUM_PORTS         ),
    .USER_W       (USER_W            )
  ) axis_load_split_i (
    .clk     (clk          ),
    .rst     (rst          ),
    .i_tdata (i_axis.tdata ),
    .i_tuser (i_axis.tuser ),
    .i_tlast (i_axis.tlast ),
    .i_tvalid(i_axis.tvalid),
    .i_tready(i_axis.tready),
    .o_tdata (split_tdata  ),
    .o_tuser (split_tuser  ),
    .o_tlast (split_tlast  ),
    .o_tvalid(split_tvalid ),
    .o_tready(split_tready )
  );

  axis_load_merge #(
    .IN_DATA_W    (INT_DATA_W        ),
    .IN_FIFO_SIZE (INT_WORD_FIFO_SIZE),
    .IN_NUM_PORTS (NUM_PORTS         ),
    .OUT_DATA_W   (DATA_W          ),
    .OUT_FIFO_SIZE(EXT_FIFO_SIZE     ),
    .USER_W       (USER_W            )
  ) axis_load_merge_i (
    .clk     (clk          ),
    .rst     (rst          ),
    .i_tdata (split_tdata  ),
    .i_tuser (split_tuser  ),
    .i_tlast (split_tlast  ),
    .i_tvalid(split_tvalid ),
    .i_tready(split_tready ),
    .o_tdata (o_axis.tdata ),
    .o_tuser (o_axis.tuser ),
    .o_tlast (o_axis.tlast ),
    .o_tvalid(o_axis.tvalid),
    .o_tready(o_axis.tready)
  );


  //---------------------------------------------------------------------------
  // Test Procedures
  //---------------------------------------------------------------------------

  task automatic test_packets(int num_pkts = NUM_PKTS);
    pkt_t::data_t data_count = 0;
    mailbox pkt_mb = new();

    fork
      begin : send_thread
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin
          pkt_t pkt = new();
          pkt.data = new [$urandom_range(1,MAX_PKT_LEN)];
          foreach(pkt.data[idx]) begin
            pkt.data[idx] = data_count;
            pkt.user[idx] = ~data_count;
            data_count++;
          end
          bfm.put(pkt);
          pkt_mb.put(pkt.copy());
        end
      end
      begin : recv_thread
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin
          pkt_t act_pkt, exp_pkt;
          bfm.get(act_pkt);
          act_pkt.keep = {};  // Not using keep, so remove the X's
          pkt_mb.get(exp_pkt);

          // Verify that we got the expected data
          `ASSERT_ERROR(
            exp_pkt.equal(act_pkt),
            //bfm_t::packets_equal(exp_pkt, act_pkt),   // **** AND WHY DOESN'T THIS WORK!!!!!!!!!!!!!!!!!!
            $sformatf("On packet %0d, actual does not match expected", pkt_count)
          );
        end
      end
    join
  endtask : test_packets


  //---------------------------------------------------------------------------
  // Main
  //---------------------------------------------------------------------------

  initial begin : main
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf({
      "rfnoc_block_fft_tb\n",
      "\tMAX_PKT_LEN   = %0d\n",
      "\tDATA_W        = %0d\n",
      "\tINT_DATA_W    = %0d\n",
      "\tNUM_PORTS     = %0d\n",
      "\tINT_FIFO_SIZE = %0d\n",
      "\tEXT_FIFO_SIZE = %0d"},
      MAX_PKT_LEN, DATA_W, INT_DATA_W, NUM_PORTS, INT_FIFO_SIZE, EXT_FIFO_SIZE
    );

    // Initialize the test exec object for this testbench
    test.start_tb(tb_name);

    // Start the clocks
    clk_gen.start();

    // Start the BFMs
    bfm.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset");
    clk_gen.reset();
    @(negedge rst);
    test.end_test();

    //--------------------------------
    // Tests
    //--------------------------------

    test.start_test("Packet tests (normal)");
    bfm.set_slave_stall_prob(50);
    bfm.set_master_stall_prob(50);
    test_packets();
    test.end_test();

    test.start_test("Packet tests (back-pressure)");
    bfm.set_slave_stall_prob(95);
    bfm.set_master_stall_prob(5);
    test_packets();
    test.end_test();

    test.start_test("Packet tests (underflow)");
    bfm.set_slave_stall_prob(95);
    bfm.set_master_stall_prob(5);
    test_packets();
    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();
  end

endmodule
