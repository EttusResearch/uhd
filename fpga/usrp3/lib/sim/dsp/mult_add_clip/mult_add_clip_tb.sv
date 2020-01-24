//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Checks a few cases of A*B+C against precomputed values
// Configurations checked include widths and binary points for
// rx_frontend_gen3 and tx_frontend_gen3. All latencies for those
// configurations are checked.


`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 2

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"

module mult_add_clip_tb();
  `TEST_BENCH_INIT("mult_add_clip_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/200e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  // {RX config, TX config}
  localparam integer AW[0:1]   = {18,16};
  localparam integer A_PT[0:1] = {17,15};
  localparam integer BW[0:1]   = {18,18};
  localparam integer B_PT[0:1] = {17,17};
  localparam integer CW[0:1]   = {24,16};
  localparam integer C_PT[0:1] = {23,15};
  localparam integer OW        = 24;
  localparam integer O_PT      = 23;
  localparam integer NUM_LATENCY = 4;
  localparam integer NUM_CASES   = 2;

  localparam [23:0] A_VECTOR[0:7] = {24'h000000,
                                     24'h800000,
                                     24'h7fffff,
                                     24'h111111,
                                     24'h333333,
                                     24'h111111,
                                     24'h333333,
                                     24'h333333};
  localparam [17:0] B_VECTOR[0:7] = {18'h00000,
                                     18'h1ffff,
                                     18'h1ffff,
                                     18'h10000,
                                     18'h3ffff,
                                     18'h3ffff,
                                     18'h10000,
                                     18'h10000};
  localparam [23:0] C_VECTOR[0:7] = {24'h000000,
                                     24'h880000,
                                     24'h70ffff,
                                     24'h000000,
                                     24'h000000,
                                     24'h101010,
                                     24'h101010,
                                     24'hfffff1};
  localparam [OW-1:0] O_VECTOR_RX[0:7] = {24'h000000,
                                          24'h800000,
                                          24'h7fffff,
                                          24'h088880,
                                          24'hffffe6,
                                          24'h101007,
                                          24'h29a990,
                                          24'h199971};
  localparam [OW-1:0] O_VECTOR_TX[0:7] = {24'h000000,
                                          24'h800000,
                                          24'h7fffff,
                                          24'h088880,
                                          24'hffffe6,
                                          24'h100ff7,
                                          24'h29a980,
                                          24'h199880};

  logic [23:0] a;
  logic [17:0] b;
  logic [23:0] c;
  wire  [OW-1:0] o[0:NUM_LATENCY*NUM_CASES-1];

  task check_result(integer samp, integer inst);
    begin
      string s;
      if ((samp >= inst) && (samp-inst < 8)) begin
        $sformat(s, "Incorrect data for RX. Samp = %0d, Latency = %0d, Data = %06x, Expected = %06x", samp, inst, o[inst], O_VECTOR_RX[samp-inst]);
        `ASSERT_ERROR(o[inst] == O_VECTOR_RX[samp-inst], s);
        $sformat(s, "Incorrect data for TX. Samp = %0d, Latency = %0d, Data = %06x, Expected = %06x", samp, inst, o[inst+NUM_LATENCY], O_VECTOR_TX[samp-inst]);
        `ASSERT_ERROR(o[inst+NUM_LATENCY] == O_VECTOR_TX[samp-inst], s);
      end
    end
  endtask

  genvar k, l;
  generate for (k = 0; k < NUM_CASES; k = k + 1) begin: inst
    for (l = 0; l < NUM_LATENCY; l = l + 1) begin: latency
      mult_add_clip #(
        .WIDTH_A(AW[k]),
        .BIN_PT_A(A_PT[k]),
        .WIDTH_B(BW[k]),
        .BIN_PT_B(B_PT[k]),
        .WIDTH_C(CW[k]),
        .BIN_PT_C(C_PT[k]),
        .WIDTH_O(OW),
        .BIN_PT_O(O_PT),
        .LATENCY(l+1)
      ) dut (
        .clk(clk), .reset(reset), .CE(1'b1),
        .A(a[23 -: AW[k]]),
        .B(b[17 -: BW[k]]),
        .C(c[23 -: CW[k]]),
        .O(o[k*NUM_LATENCY+l])
      );
    end
  end endgenerate

  initial begin : tb_main
    `TEST_CASE_START("Wait for Reset");
      while (reset) @ (posedge clk);
    `TEST_CASE_DONE(~reset);

    `TEST_CASE_START("Check products");
    for (int i = 0; i < (8 + NUM_LATENCY); i = i + 1) begin
      @(negedge clk);
      for (int j = 0; j < NUM_LATENCY; j = j + 1) begin
        check_result(i-1, j); // i-1 because inst = latency - 1
      end
      a = A_VECTOR[i % 8];
      b = B_VECTOR[i % 8];
      c = C_VECTOR[i % 8];
    end
    `TEST_CASE_DONE(1);
    `TEST_BENCH_DONE;
  end

endmodule // mult_add_clip_tb
