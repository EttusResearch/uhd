/////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2017 Ettus Research
//
// Arm Deframer Testbench
//
// Arm Deframer adds a 6 bit padding to the ethernet frame
//
//////////////////////////////////////////////////////////////////////

`timescale 1ns/1ps
`define SIM_RUNTIME_US 0.0001
`define NS_PER_TICK 1
`define NUM_TEST_CASES 3
`include "sim_axis_lib.svh"
`include "sim_exec_report.vh"
`include "sim_rfnoc_lib.svh"
`include "sim_set_rb_lib.svh"

// NOTE: The tesbench is not self-checking
module arm_deframer_tb();
  `TEST_BENCH_INIT("arm_deframer_tb",`NUM_TEST_CASES,`NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e6/166.67e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(rst, 0, 100);

  axis_master #(.DWIDTH(68)) m_axis (.clk(clk));
  axis_slave #(.DWIDTH(68)) s_axis (.clk(clk));

  reg clear;
  wire  [63:0]  c2e_tdata_int;
  wire  [3:0]   c2e_tuser_int;
  wire          c2e_tlast_int;
  wire          c2e_tvalid_int;
  wire          c2e_tready_int;

  arm_deframer inst_arm_deframer (
   .clk               (clk),
   .reset             (rst),
   .clear             (clear),
   .s_axis_tdata      (m_axis.axis.tdata[63:0]),
   .s_axis_tuser      (m_axis.axis.tdata[67:64]),
   .s_axis_tlast      (m_axis.axis.tlast),
   .s_axis_tvalid     (m_axis.axis.tvalid),
   .s_axis_tready     (m_axis.axis.tready),
   .m_axis_tdata      (c2e_tdata_int),
   .m_axis_tuser      (c2e_tuser_int),
   .m_axis_tlast      (c2e_tlast_int),
   .m_axis_tvalid     (c2e_tvalid_int),
   .m_axis_tready     (c2e_tready_int)
  );

  axi_mux4 #(.PRIO(0), .WIDTH(68)) eth_mux
    (.clk(clk), .reset(reset), .clear(clear),
     .i0_tdata({c2e_tuser_int,c2e_tdata_int}), .i0_tlast(c2e_tlast_int), .i0_tvalid(c2e_tvalid_int), .i0_tready(c2e_tready_int),
     .i1_tdata(), .i1_tlast(), .i1_tvalid(), .i1_tready(),
     .i2_tdata(), .i2_tlast(), .i2_tvalid(), .i2_tready(),
     .i3_tdata(), .i3_tlast(), .i3_tvalid(1'b0), .i3_tready(),
     .o_tdata(s_axis.axis.tdata), .o_tlast(s_axis.axis.tlast), .o_tvalid(s_axis.axis.tvalid), .o_tready(1'b1/*s_axis.axis.tready*/));

 reg [63:0] out_tdata;
 reg [3:0] tuser;
 reg tlast;

 initial begin
  forever begin
    s_axis.pull_word({tuser,out_tdata}, tlast);
    $display("tuser = %d, out_tdata = %x, last = %d", tuser, out_tdata, tlast);
  end
 end

 initial begin : tb_main
    `TEST_CASE_START("Wait for reset");
    m_axis.reset;
    while (rst) @(posedge clk);
    `TEST_CASE_DONE(~rst);

    `TEST_CASE_START("Send frame");
    repeat (10) @(posedge clk);
      for (int j = 0; j < 2; j ++) begin
      @(posedge clk);
        $display("----------New Frame---------------");
        for (int i = 0; i < 9; i ++) begin
          m_axis.push_word({4'h0,64'h1111111111111111 * i}, 1'b0);
          //m_axis.push_bubble();
        end
        m_axis.push_word({4'h1,64'h9999999999999999}, 1'b1);
        //m_axis.push_bubble();
      end
      @(posedge clk);
      @(posedge clk);
    `TEST_CASE_DONE(1);
     repeat (10) @(posedge clk);

    `TEST_CASE_START("Send frame with bubble");
    repeat (10) @(posedge clk);
      for (int j = 0; j < 2; j ++) begin
      @(posedge clk);
        $display("----------New Frame---------------");
        for (int i = 0; i < 9; i ++) begin
          m_axis.push_word({4'h0,64'h1111111111111111 * i}, 1'b0);
          m_axis.push_bubble();
        end
        m_axis.push_word({4'h1,64'h9999999999999999}, 1'b1);
        m_axis.push_bubble();
      end
      @(posedge clk);
      @(posedge clk);
    `TEST_CASE_DONE(1);
    `TEST_BENCH_DONE;
 end
endmodule
