//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_i2c_sync_ctrl_tb
//
// Description:
//  testbench for ctrlport_to_i2c_sync_ctrl module. Tasks defined for ctrlport write and
//  read transactions. Tests ability to write to SYNC/RFS_EN regs and then
//  initialize i2c_master and i2c peripheral and then configure io through the
//  i2c interface to the io expander.
//
`timescale 1ns / 1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 5
`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`define SIM_TIMEOUT_US 200000 //overwrite this def from sim_exec_report.vh 


module ctrlport_to_i2c_sync_ctrl_tb();
  import PkgRandom::*;

  `TEST_BENCH_INIT("ctrlport_to_i2c_sync_ctrl_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  //sets up clock
  localparam CLK_PERIOD = $ceil(1e9/50.0e6);
  `DEFINE_CLK(clk50, CLK_PERIOD, 50);

  reg reset_clk50;
  reg        s_ctrlport_req_wr;
  reg        s_ctrlport_req_rd;
  reg [19:0] s_ctrlport_req_addr;
  reg [31:0] s_ctrlport_req_data;

  // Response
  wire         s_ctrlport_resp_ack;
  wire  [ 1:0] s_ctrlport_resp_status;
  wire  [31:0] s_ctrlport_resp_data;


  wire i2c_scl_i, i2c_scl_o, i2c_scl_en_o;
  wire i2c_sda_i, i2c_sda_o, i2c_sda_en_o;
  PkgRandom::Rand #(3) rand_sync;
  PkgRandom::Rand #(1) rand_rfs_en;
  reg [2:0] sync1, sync2, sync3, sync4, sync5;
  reg rfs_en;
  `include "../../regmap/rf_sync_regmap_utils.vh"

  task cp_write;
    input [19:0] a;
    input [31:0] d;
    begin
      s_ctrlport_req_addr = a;
      s_ctrlport_req_data = d;
      @(posedge clk50);
      s_ctrlport_req_wr = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_wr = 0;
      @(posedge clk50);
    end
  endtask

  task cp_read;
    input [19:0] a;
    input [31:0] d;
    input debug;
    begin
      s_ctrlport_req_addr = a;
      s_ctrlport_req_rd = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_rd = 0;
      @(posedge clk50);
      if(debug) begin
        $display("status: read addr %h at %t value = %h", a, $time, s_ctrlport_resp_data);
      end
      `ASSERT_ERROR( s_ctrlport_resp_data == d, "CtrlPort read returned unexpected value.");
    end
  endtask
  
  task cp_poll;
    input [19:0] a;
    input [31:0] d;
    input debug;
    begin
      s_ctrlport_req_addr = a;
      s_ctrlport_req_rd = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_rd = 0;
      @(posedge clk50);
      while(s_ctrlport_resp_data != d && s_ctrlport_req_addr == a) begin
        s_ctrlport_req_addr = a;
        s_ctrlport_req_rd = 1;
        @(posedge clk50);
        while(~s_ctrlport_resp_ack) @(posedge clk50);
        s_ctrlport_req_rd = 0;
        @(posedge clk50);
      end
      if(debug) begin
        $display("status: read addr %h at %t value = %h", a, $time, s_ctrlport_resp_data);
      end
    end
  endtask
  

  ctrlport_to_i2c_sync_ctrl #(
    .BASE_ADDRESS(0)
  ) ctrlport_to_i2c_inst (
    .ctrlport_clk              (clk50),
    .ctrlport_rst              (reset_clk50),
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_data       (s_ctrlport_req_data),
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (s_ctrlport_resp_status),
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
    .scl_pad_i                 (i2c_scl_i),
    .scl_pad_o                 (i2c_scl_o),
    .scl_pad_en_o              (i2c_scl_en_o),
    .sda_pad_i                 (i2c_sda_i),
    .sda_pad_o                 (i2c_sda_o),
    .sda_pad_en_o              (i2c_sda_en_o)
  );

  // hookup i2c slave model
  i2c_slave_model #(
    ctrlport_to_i2c_inst.SYNC_IO_SLV_ADR
  ) i2c_slave (
    .scl(i2c_scl_i),
    .sda(i2c_sda_i)
  );

  // create i2c lines
  delay m0_scl (i2c_scl_en_o ? 1'bz : i2c_scl_o, i2c_scl_i),
        m0_sda (i2c_sda_en_o ? 1'bz : i2c_sda_o, i2c_sda_i);

  pullup p1(i2c_scl_i); // pullup scl line
  pullup p2(i2c_sda_i); // pullup sda line

  initial begin : tb_main
    `TEST_CASE_START("initialize UUT");
    clk50 = 0;
    reset_clk50 = 1;
    s_ctrlport_req_wr = 0;
    s_ctrlport_req_rd = 0;
    s_ctrlport_req_addr = 0;
    s_ctrlport_req_data = 0;
    #100
    reset_clk50 = 0;
    $display("status: %t done reset here", $time);
    `TEST_CASE_DONE(~reset_clk50);
    // trigger setup
    `TEST_CASE_START("Initialize core and I2C peripheral");
    cp_write(SETUP_REG, 1);
    cp_poll(SETUP_STATUS_REG,1,1);
    `TEST_CASE_DONE(i2c_slave.mem[7] == 0 & i2c_slave.mem[6] == 0);
    $display("status: %t SETUP DONE", $time);
    // configure SYNC and RFS_EN Regs
    for (int i=0; i<3; i=i+1) begin
      `TEST_CASE_START("configure SYNC and RFS_EN Regs loop");
      $display("loop # %d", i);
      sync1 = rand_sync.rand_bit();
      sync2 = rand_sync.rand_bit();
      sync3 = rand_sync.rand_bit();
      sync4 = rand_sync.rand_bit();
      sync5 = rand_sync.rand_bit();
      rfs_en = rand_rfs_en.rand_bit();
      cp_write(SYNC_1_REG, sync1);
      cp_write(SYNC_2_REG, sync2);
      cp_write(SYNC_3_REG, sync3);
      cp_write(SYNC_4_REG, sync4);
      cp_write(SYNC_5_REG, sync5);
      cp_write(RFS_EN_REG, rfs_en);
      cp_read(SYNC_1_REG, {28'b0, sync1},  1);
      cp_read(SYNC_2_REG, {28'b0, sync2},  1);
      cp_read(SYNC_3_REG, {28'b0, sync3},  1);
      cp_read(SYNC_4_REG, {28'b0, sync4},  1);
      cp_read(SYNC_5_REG, {28'b0, sync5},  1);
      cp_read(RFS_EN_REG, {31'b0, rfs_en}, 1);
      $display("status: %t Configured ControlPort regs, now configure I2C peripheral.", $time);
      cp_write(CONFIG_IO_REG, 1);
      cp_poll(CONFIG_IO_STATUS_REG,32'h1,1);
      `TEST_CASE_DONE(i2c_slave.mem[2] == {sync3[1:0], sync2, sync1} &
                      i2c_slave.mem[3] == {rfs_en, sync5, sync4, sync3[2]});
    end
    `TEST_BENCH_DONE;
end
endmodule

module delay (in, out);
  input  in;
  output out;

  assign out = in;

  specify
    (in => out) = (600,600);
  endspecify
endmodule

