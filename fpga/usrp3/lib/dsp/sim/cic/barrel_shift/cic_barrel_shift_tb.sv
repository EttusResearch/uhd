//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_barrel_shift_tb
//
// Description:
//
//   Direct testbench for cic_barrel_shift. Drives signed I/Q values with
//   selected discarded-bit residues and checks round-to-nearest behavior on
//   every SPC lane.
//

`default_nettype none

module cic_barrel_shift_tb #(
  int SPC      = 4,
  int IN_WIDTH = 96,
  int OUT_WIDTH = 32,
  int MAX_RATE = 255,
  int ORDER    = 4
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import cic_filter_test_pkg::*;

  localparam real CLK_PERIOD = 10.0;
  localparam int IN_COMP_W = IN_WIDTH / 2;
  localparam int OUT_COMP_W = OUT_WIDTH / 2;
  localparam int RATE_W = $clog2(MAX_RATE + 1);
  localparam int TEST_RATE = (MAX_RATE >= 2) ? 2 : 1;
  localparam int TEST_SHIFT = $clog2(longint'(TEST_RATE) ** ORDER);
  localparam longint signed OUTPUT_MAX =
    (longint'(1) << (OUT_COMP_W - 1)) - 1;
  localparam longint signed HALF_LSB = longint'(1) << (TEST_SHIFT - 1);

  typedef cic_filter_quantizer #(
    .ACCUM_W(IN_WIDTH),
    .SAMP_W (OUT_WIDTH)
  ) quantizer_c;
  typedef AxiStreamPacket #(SPC * IN_WIDTH) input_pkt_t;
  typedef AxiStreamPacket #(SPC * OUT_WIDTH) output_pkt_t;

  logic clk;
  logic rst;
  logic clear = 1'b0;
  logic [RATE_W-1:0] rate_factor;

  sim_clock_gen #(
    .PERIOD   (CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamIf #(
    .DATA_WIDTH(SPC * IN_WIDTH),
    .TKEEP     (0),
    .TUSER     (0)
  ) i_data (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamIf #(
    .DATA_WIDTH(SPC * OUT_WIDTH),
    .TKEEP     (0),
    .TUSER     (0)
  ) o_data (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamBfm #(
    .DATA_WIDTH(SPC * IN_WIDTH),
    .TKEEP     (0),
    .TUSER     (0)
  ) input_bfm = new(
    .master(i_data),
    .slave (null)
  );

  AxiStreamBfm #(
    .DATA_WIDTH(SPC * OUT_WIDTH),
    .TKEEP     (0),
    .TUSER     (0)
  ) output_bfm = new(
    .master(null),
    .slave (o_data)
  );

  cic_barrel_shift #(
    .SPC      (SPC),
    .MAX_RATE (MAX_RATE),
    .ORDER    (ORDER)
  ) dut (
    .clk        (clk),
    .rst        (rst),
    .clear      (clear),
    .rate_factor(rate_factor),
    .data_in    (i_data),
    .data_out   (o_data)
  );

  longint signed vector_i[SPC];
  longint signed vector_q[SPC];

  task automatic check_vector(input int R, input string test_name);
    input_pkt_t input_pkt;
    output_pkt_t output_pkt;
    logic [SPC * IN_WIDTH-1:0] input_word;
    logic [SPC * OUT_WIDTH-1:0] expected_word;
    int shift_amount;

    input_word = '0;
    expected_word = '0;
    rate_factor = R;
    shift_amount = $clog2(longint'(R) ** ORDER);

    // Wait for the rate lookup pipeline to select the requested rate.
    clk_gen.clk_wait_r(4);

    for (int lane = 0; lane < SPC; lane++) begin
      input_word[IN_WIDTH*lane +: IN_COMP_W] = IN_COMP_W'(vector_q[lane]);
      input_word[IN_WIDTH*lane + IN_COMP_W +: IN_COMP_W] =
        IN_COMP_W'(vector_i[lane]);
      expected_word[OUT_WIDTH*lane +: OUT_WIDTH] =
        quantizer_c::round_shift_sample(
          input_word[IN_WIDTH*lane +: IN_WIDTH], shift_amount);
    end

    input_pkt = new();
    input_pkt.data.push_back(input_word);
    input_bfm.put(input_pkt);
    input_bfm.wait_complete();
    output_bfm.get(output_pkt);

    `ASSERT_ERROR(output_pkt.data.size() == 1,
      $sformatf("%s: expected one output word, got %0d",
                test_name, output_pkt.data.size()))
    if (output_pkt.data.size() == 1) begin
      `ASSERT_ERROR(output_pkt.data[0] === expected_word,
        $sformatf("%s: R=%0d expected 0x%0h, got 0x%0h",
                  test_name, R, expected_word, output_pkt.data[0]))
    end

    clk_gen.reset();
    @(negedge rst);
  endtask : check_vector

  task automatic run_testcases();
    clk_gen.start();
    input_bfm.run();
    output_bfm.run();
    input_bfm.set_master_stall_prob(0);
    output_bfm.set_slave_stall_prob(0);
    clk_gen.reset();
    @(negedge rst);

    test.start_test("Signed CIC rounding residues", 10us);
    for (int lane = 0; lane < SPC; lane++) begin
      case (lane % 4)
        0: begin
          vector_i[lane] = (2 * (longint'(1) << TEST_SHIFT)) + HALF_LSB - 1;
          vector_q[lane] = -vector_i[lane];
        end
        1: begin
          vector_i[lane] = (2 * (longint'(1) << TEST_SHIFT)) + HALF_LSB;
          vector_q[lane] = -vector_i[lane];
        end
        default: begin
          vector_i[lane] = -((OUTPUT_MAX + 1) << TEST_SHIFT) + HALF_LSB + 1;
          vector_q[lane] = -vector_i[lane];
        end
      endcase
    end
    check_vector(TEST_RATE, "rounding residues");
    test.end_test();

    test.start_test("R=1 output conversion", 10us);
    for (int lane = 0; lane < SPC; lane++) begin
      vector_i[lane] = 32 + lane;
      vector_q[lane] = -32 - lane;
    end
    check_vector(1, "R=1");
    test.end_test();

    if (MAX_RATE >= 3) begin
      test.start_test("Non-power-of-two rate", 10us);
      for (int lane = 0; lane < SPC; lane++) begin
        vector_i[lane] = 16 + lane;
        vector_q[lane] = -16 - lane;
      end
      check_vector(3, "R=3");
      test.end_test();
    end
  endtask : run_testcases

  initial begin
    test.start_tb($sformatf(
      "CIC barrel shift rounding (SPC=%0d, IN_WIDTH=%0d, OUT_WIDTH=%0d, ORDER=%0d)",
      SPC, IN_WIDTH, OUT_WIDTH, ORDER));
    run_testcases();
    test.end_tb(0);
    clk_gen.kill();
  end

endmodule : cic_barrel_shift_tb

`default_nettype wire
