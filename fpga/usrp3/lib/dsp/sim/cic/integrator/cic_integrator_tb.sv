//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_integrator_tb
//
// Description:
//
//   Testbench for cic_integrator: drives AXI-Stream packets through the
//   DUT and compares every output sample against the bit-true
//   cic_integrator_model. Accumulator state persists across test
//   boundaries (matching the RTL); only explicit reset or clear tears it down.
//
// Parameters:
//
//   SPC     : Number of samples per clock cycle on DUT.
//   ACCUM_W : Width of each sample at the DUT interface (I+Q packed).
//   SAMP_W  : I/Q component width used to generate bounded test inputs.
//             Must be <= ACCUM_W.
//   ORDER   : CIC filter order (number of integrator stages).
//

`default_nettype none

module cic_integrator_tb #(
  int SPC     = 2,
  int ACCUM_W = 48,
  int SAMP_W  = 32,
  int ORDER   = 4
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  import cic_utils_pkg::*;
  import cic_test_pkg::*;

  //---------------------------------------------------------------------------
  // Testbench parameters and type definitions
  //---------------------------------------------------------------------------
  localparam real CLK_PERIOD         = 10.0;
  localparam int  STALL_PROB_LOW     = 10;
  localparam int  STALL_PROB_DEFAULT = 50;
  localparam int  STALL_PROB_HIGH    = 80;
  localparam int  NUM_TESTS          = 10;
  localparam bit  VERBOSE            = 0;

  localparam int DATA_W   = ACCUM_W * SPC;
  localparam int COMP_W   = SAMP_W / 2;
  // Flush words: ORDER stages, each with (SPC/2)+1 pipeline depth headroom.
  localparam int LEN_FLUSH_WORDS = ORDER * ($clog2(SPC) + 2);

  // Parametrized type definitions
  typedef cic_utils#(
    .SPC(SPC),
    .SAMP_W(ACCUM_W)
  ) util_c;

  typedef cic_test_utils#(
    .SPC(SPC),
    .ACCUM_W(ACCUM_W),
    .COMP_W(COMP_W)
  ) test_utils;

  typedef util_c::sample_t           sample_t;
  typedef util_c::comp_t             comp_t;
  typedef util_c::word_t             word_t;
  typedef test_utils::sample_queue_t sample_queue_t;
  typedef test_utils::axis_pkt_t     axis_pkt_t;

  localparam comp_t MAX_COMP = comp_t'((1 << (COMP_W - 1)) - 1);
  localparam comp_t MIN_COMP = comp_t'(-(1 << (COMP_W - 1)));

  //---------------------------------------------------------------------------
  // Parameter checks
  //---------------------------------------------------------------------------
  if (COMP_W > ACCUM_W / 2) begin
    $error(
      "SAMP_W/2 (%0d) must be <= ACCUM_W/2 (%0d)", COMP_W, ACCUM_W / 2
    );
  end

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------
  logic clk;
  logic rst;

  sim_clock_gen #(
    .PERIOD(CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------
  AxiStreamIf #(DATA_W) i_data (.clk(clk), .rst(rst));
  AxiStreamIf #(DATA_W) o_data (.clk(clk), .rst(rst));

  AxiStreamBfm #(DATA_W) axis_bfm = new(.master(i_data), .slave(o_data));

  typedef AxiStreamBfm#(DATA_W)::AxisPacket_t pkt_t;

  //---------------------------------------------------------------------------
  // Device Under Test
  //---------------------------------------------------------------------------
  cic_integrator #(
    .SAMP_W(ACCUM_W),
    .SPC   (SPC),
    .ORDER (ORDER)
  ) dut (
    .clk     (clk),
    .rst     (rst),
    .clr     (1'b0),
    .data_in (i_data),
    .data_out(o_data)
  );

  //---------------------------------------------------------------------------
  // Reference model (persistent state across test cases)
  //---------------------------------------------------------------------------
  cic_integrator_model #(
    .SAMP_W (ACCUM_W),
    .ACCUM_W(ACCUM_W),
    .ORDER  (ORDER)
  ) model = new();

  //---------------------------------------------------------------------------
  // Testcase helper
  //
  // Sends pkt to the DUT, waits for the pipeline to drain, collects the
  // output packet, and compares it sample-by-sample against the reference
  // model.  The model's state persists between calls.
  //---------------------------------------------------------------------------
  task automatic run_test(pkt_t pkt);
    pkt_t           rx_pkt;
    sample_queue_t  inp_samples, exp_samples, dut_samples;

    // Derive expected output from reference model before sending (non-destructive copy)
    inp_samples = test_utils::pkt_to_samples(axis_pkt_t'(pkt.copy()));
    model.process_packet(inp_samples, exp_samples);

    // Send to DUT
    axis_bfm.put(pkt);
    axis_bfm.wait_complete(-1);

    // Flush pipeline
    clk_gen.clk_wait_r(LEN_FLUSH_WORDS);

    // Receive and compare
    axis_bfm.get(rx_pkt);
    dut_samples = test_utils::pkt_to_samples(axis_pkt_t'(rx_pkt.copy()));

    // BEGIN DEBUG — print full sample table before any assertion
    if (VERBOSE) begin : dbg_samples
      automatic int n_exp  = exp_samples.size();
      automatic int n_dut  = dut_samples.size();
      automatic sample_queue_t exp_copy = exp_samples;
      automatic sample_queue_t dut_copy = dut_samples;
      automatic int idx = 0;
      $display("[DBG] run_test  SPC=%0d  ORDER=%0d  exp=%0d  dut=%0d samples",
               SPC, ORDER, n_exp, n_dut);
      while (exp_copy.size() > 0 || dut_copy.size() > 0) begin
        automatic sample_t  e = '0, d = '0;
        automatic string    exp_str = "<missing>", dut_str = "<missing>", mark = "";
        if (exp_copy.size() > 0) begin
          e = exp_copy.pop_front();
          exp_str = $sformatf("0x%0h (I=%0d Q=%0d)",
            e, $signed(e[ACCUM_W-1 -: ACCUM_W/2]), $signed(e[ACCUM_W/2-1 -: ACCUM_W/2]));
        end
        if (dut_copy.size() > 0) begin
          d = dut_copy.pop_front();
          dut_str = $sformatf("0x%0h (I=%0d Q=%0d)",
            d, $signed(d[ACCUM_W-1 -: ACCUM_W/2]), $signed(d[ACCUM_W/2-1 -: ACCUM_W/2]));
          if (exp_str != "<missing>") mark = (e === d) ? " OK" : " <<< MISMATCH";
        end
        $display("[DBG]   [%0d] EXP=%s  DUT=%s%s", idx, exp_str, dut_str, mark);
        idx++;
      end
    end : dbg_samples
    // END DEBUG

    `ASSERT_ERROR(dut_samples.size() == exp_samples.size(),
                 $sformatf("Output size mismatch: expected %0d, got %0d",
                           exp_samples.size(), dut_samples.size()));

    // Compare each sample with the next expected sample.
    begin
      automatic int idx = 0;
      while (exp_samples.size() > 0) begin
        automatic sample_t exp_s;
        automatic sample_t dut_s;
        exp_s = exp_samples.pop_front();
        dut_s = dut_samples.pop_front();
        if (VERBOSE) begin
          $display("[DBG]   [%0d] EXP=0x%0h  DUT=0x%0h", idx, exp_s, dut_s);
        end
        `ASSERT_ERROR(dut_s === exp_s,
                     $sformatf("Sample[%0d] mismatch: expected 0x%0h, got 0x%0h", idx, exp_s, dut_s));
        idx++;
      end
    end
  endtask : run_test

  // Helper: build a pkt_t from an axis_pkt_t
  function automatic pkt_t axis_pkt_to_raw(axis_pkt_t src);
    pkt_t dst = new();
    foreach (src.data[i]) begin
      dst.data.push_back(src.data[i]);
    end
    return dst;
  endfunction : axis_pkt_to_raw

  //---------------------------------------------------------------------------
  // Reset DUT and reference model
  //---------------------------------------------------------------------------
  task automatic do_reset();
    clk_gen.reset();
    @(negedge rst);
    model.reset();
  endtask : do_reset

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------
  task testbench_main();
    string test_name;
    test_name = $sformatf(
      "CIC N-stage Integrator\nSPC=%0d  ACCUM_W=%0d  ORDER=%0d",
      SPC, ACCUM_W, ORDER
    );
    test.start_tb(test_name, 100ms);

    clk_gen.start();
    axis_bfm.run();
    axis_bfm.set_master_stall_prob(STALL_PROB_DEFAULT);
    axis_bfm.set_slave_stall_prob(STALL_PROB_DEFAULT);

    do_reset();

    //------------------------------------------------------------------
    test.start_test("Constant +1 input", 1ms);
    run_test(axis_pkt_to_raw(test_utils::generate_constant_packet(16, 1, 1)));
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    test.start_test("Constant -1 input", 1ms);
    run_test(axis_pkt_to_raw(test_utils::generate_constant_packet(16, -1, -1)));
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    test.start_test("Ramp input", 1ms);
    run_test(axis_pkt_to_raw(test_utils::generate_ramp_packet(16)));
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    test.start_test("Impulse input", 1ms);
    run_test(axis_pkt_to_raw(test_utils::generate_impulse_packet(4)));
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    test.start_test("Random input (multiple packets)", NUM_TESTS * 1ms);
    repeat (NUM_TESTS) begin
      run_test(axis_pkt_to_raw(test_utils::generate_ramp_packet(32, 0, 1)));
    end
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    // High output stall: stress backpressure path
    axis_bfm.set_master_stall_prob(STALL_PROB_LOW);
    axis_bfm.set_slave_stall_prob(STALL_PROB_HIGH);
    test.start_test("Random input, high output stall", NUM_TESTS * 5ms);
    repeat (NUM_TESTS) begin
      run_test(axis_pkt_to_raw(test_utils::generate_random_packet(32)));
    end
    do_reset();
    test.end_test();

    //------------------------------------------------------------------
    // High input stall: stress input rate variation
    axis_bfm.set_master_stall_prob(STALL_PROB_HIGH);
    axis_bfm.set_slave_stall_prob(STALL_PROB_LOW);
    test.start_test("Random input, high input stall", NUM_TESTS * 5ms);
    repeat (NUM_TESTS) begin
      run_test(axis_pkt_to_raw(test_utils::generate_random_packet(32)));
    end
    do_reset();
    test.end_test();

    clk_gen.kill();

    test.end_tb(0);
  endtask : testbench_main

  //---------------------------------------------------------------------------
  // Execution
  //---------------------------------------------------------------------------
  initial begin : tb_main
    testbench_main();
  end

endmodule : cic_integrator_tb

`default_nettype wire
