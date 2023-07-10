//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_i2c_tb
//
// Description:
//  testbench for ctrlport_to_i2c module. Tasks defined for ctrlport write and
//  read transactions as well as a task for waiting on the i2c transaction to
//  finish. Tests i2c write and reads.
//
`timescale 1ns / 1ps

module ctrlport_to_i2c_tb();

  reg clk50;
  reg reset_clk50;
  reg        s_ctrlport_req_wr;
  reg        s_ctrlport_req_rd;
  reg [19:0] s_ctrlport_req_addr;
  reg [31:0] s_ctrlport_req_data;

  // Response
  wire         s_ctrlport_resp_ack;
  wire  [ 1:0] s_ctrlport_resp_status;
  wire  [31:0] s_ctrlport_resp_data;


  wire i2c_scl_i, i2c_scl_o, i2c_sclen_o;
  wire i2c_sda_i, i2c_sda_o, i2c_sdaen_o;

  parameter SADR    = 7'b010_0000;

  `include "../../../rfnoc/core/ctrlport.vh"
  `include "../../../wishbone/i2c_master.vh"

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
    input debug;
    begin
      s_ctrlport_req_addr = a;
      @(posedge clk50);
      s_ctrlport_req_rd = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_rd = 0;
      @(posedge clk50);
      if(debug) begin
        $display("status: read addr %h at %t value = %h", a, $time, s_ctrlport_resp_data);
      end
    end
  endtask

  task i2c_tip;
    begin
    $display("status: %t tip start", $time);
    cp_read('h0+WB_SR, 0);
    #1;
    while(s_ctrlport_resp_data[1])
      cp_read('h0+WB_SR, 0);
    $display("status: %t tip==0", $time);
    end
  endtask

  ctrlport_to_wb_i2c #( .BASE_ADDRESS(0)) ctrlport_to_wb_i2c_inst (
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
  .scl_pad_en_o              (i2c_sclen_o),
  .sda_pad_i                 (i2c_sda_i),
  .sda_pad_o                 (i2c_sda_o),
  .sda_pad_en_o              (i2c_sdaen_o)
  );

  // hookup i2c slave model
  i2c_slave_model #(SADR) i2c_slave (
  .scl(i2c_scl_i),
  .sda(i2c_sda_i)
  );

  // create i2c lines
  delay m0_scl (i2c_sclen_o ? 1'bz : i2c_scl_o, i2c_scl_i),
        m0_sda (i2c_sdaen_o ? 1'bz : i2c_sda_o, i2c_sda_i);

  pullup p1(i2c_scl_i); // pullup scl line
  pullup p2(i2c_sda_i); // pullup sda line

  always #20 clk50 = ~clk50;

  initial begin
    clk50 = 0;
    reset_clk50 = 1;
    s_ctrlport_req_wr = 0;
    s_ctrlport_req_rd = 0;
    s_ctrlport_req_addr = 0;
    s_ctrlport_req_data = 0;
    repeat (5) @(posedge clk50);
    reset_clk50 = 0;
    repeat (5) @(posedge clk50);
    $display("status: %t done reset", $time);
    cp_write('h0+WB_PRER_LO, 'hF4); //F4
    cp_write('h0+WB_PRER_HI, 'h01); //1
    $display("status: %t set prescale to 500", $time);
    cp_read('h0+WB_PRER_LO, 1);
    cp_read('h0+WB_PRER_HI, 1);
    $display("status: %t enable core", $time);
    cp_write('h0+WB_CTR, 'h80);
    cp_write('h0+WB_TXR, {SADR,1'b0}); // present slave address, set write-bit
    cp_write('h0+WB_CR, CR_START_AND_WRITE); // set command (start, write)
    i2c_tip();
    cp_write('h0+WB_TXR, 'h01); // present slave address, set write-bit
    cp_write('h0+WB_CR, CR_WRITE); // set command (write)
    i2c_tip();
    // send memory contents
    cp_write('h0+WB_TXR, 8'ha5); // present data
    cp_write('h0+WB_CR, CR_WRITE); // set command (write)
    $display("status: %t write data a5", $time);
    i2c_tip();
    // send memory contents for next memory address (auto_inc)
    cp_write('h0+WB_TXR, 8'h5a); // present data
    cp_write('h0+WB_CR, CR_WRITE_AND_STOP); // set command (stop, write)
    $display("status: %t write next data 5a, generate 'stop'", $time);
    i2c_tip();
    // drive slave address
    cp_write('h0+WB_TXR, {SADR,1'b0} ); // present slave's address, set write-bit
    cp_write('h0+WB_CR, CR_START_AND_WRITE ); // set command (start, write)
    $display("status: %t generate 'repeated start', write cmd %0h (slave address+write)", $time, {SADR,1'b1} );
    i2c_tip();
    // send memory address
    cp_write('h0+WB_TXR, 8'h01); // present slave's memory address
    cp_write('h0+WB_CR, CR_WRITE); // set command (write)
    $display("status: %t write slave address 01", $time);
    i2c_tip();
    // drive slave address
    cp_write('h0+WB_TXR, {SADR,1'b1} ); // present slave's address, set read-bit
    cp_write('h0+WB_CR, CR_START_AND_WRITE ); // set command (start, write)
    $display("status: %t generate 'repeated start', write cmd %0h (slave address+read)", $time, {SADR,1'b1} );
    i2c_tip();
    // read data from slave
    cp_write('h0+WB_CR, CR_READ_AND_ACK); // set command (read, ack_read)
    $display("status: %t read + ack", $time);
    i2c_tip();
    cp_read('h0+WB_RXR, 1);
    // read data from slave
    cp_write('h0+WB_CR, CR_READ_AND_ACK); // set command (read, ack_read)
    $display("status: %t read + ack", $time);
    i2c_tip();
    cp_read('h0+WB_RXR, 1);
    // read data from slave
    cp_write('h0+WB_CR, CR_READ_AND_ACK); // set command (read, ack_read)
    $display("status: %t read + ack", $time);
    i2c_tip();
    cp_read('h0+WB_RXR, 1);
    $display("status: %t simulation done", $time);
    $finish;
  end

endmodule
