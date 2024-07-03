//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//
//   Testbench for device_dna and device_dna_ctrlport.
//

`default_nettype none


module device_dna_tb ();

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  localparam real CLK_PERIOD  = 10.0; // ns

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;
  bit rst_dut = 0;

  sim_clock_gen #(.PERIOD(CLK_PERIOD))
    clk_gen (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  logic           i_ctrlport_req_rd   = 1'b0;
  logic [19:0]    i_ctrlport_req_addr = 20'h0;
  logic           o_ctrlport_resp_ack;
  logic [1:0]     o_ctrlport_resp_status;
  logic [31:0]    o_ctrlport_resp_data;

  logic           o_ctrlport_resp_ack_7s;
  logic [1:0]     o_ctrlport_resp_status_7s;
  logic [31:0]    o_ctrlport_resp_data_7s;


  device_dna_ctrlport #(
    .BASE_ADDR(0),
    .DNA_WIDTH(128)
  ) device_dna_dut (
    .ctrlport_clk (clk),
    .reset        (rst | rst_dut),

    .s_ctrlport_req_rd     (i_ctrlport_req_rd     ),
    .s_ctrlport_req_addr   (i_ctrlport_req_addr   ),
    .s_ctrlport_resp_ack   (o_ctrlport_resp_ack   ),
    .s_ctrlport_resp_status(o_ctrlport_resp_status),
    .s_ctrlport_resp_data  (o_ctrlport_resp_data  )
  );

  device_dna_ctrlport #(
    .BASE_ADDR(32),
    .DNA_WIDTH(57),
    .DEVICE_TYPE("7SERIES")
  ) device_dna_dut_7s (
    .ctrlport_clk (clk),
    .reset        (rst | rst_dut),

    .s_ctrlport_req_rd     (i_ctrlport_req_rd        ),
    .s_ctrlport_req_addr   (i_ctrlport_req_addr      ),
    .s_ctrlport_resp_ack   (o_ctrlport_resp_ack_7s   ),
    .s_ctrlport_resp_status(o_ctrlport_resp_status_7s),
    .s_ctrlport_resp_data  (o_ctrlport_resp_data_7s  )
  );

  //--------------------------------
  // Task
  //--------------------------------
  task automatic check_dna(
    int  address,
    int  expected_dna    = 1
  );
    i_ctrlport_req_addr <= address;
    i_ctrlport_req_rd   <= 1;
    @(posedge clk);
    i_ctrlport_req_rd   <= 0;
    while (!o_ctrlport_resp_ack) @(posedge clk);
    test.assert_error(
      o_ctrlport_resp_ack, "CtrlPort response ACK not asserted!");
    test.assert_error(
      !o_ctrlport_resp_status,
      $sformatf("CtrlPort response status not zero (%d)!", o_ctrlport_resp_status));
    test.assert_error(
      o_ctrlport_resp_data == expected_dna,
      $sformatf("Incorrect DNA value: %x! Expected %x.", o_ctrlport_resp_data, expected_dna));
    @(posedge clk);
  endtask;

  task automatic check_dna_7s(
    int  address,
    int  expected_dna    = 1
  );
    i_ctrlport_req_addr <= address;
    i_ctrlport_req_rd   <= 1;
    @(posedge clk);
    i_ctrlport_req_rd   <= 0;
    while (!o_ctrlport_resp_ack_7s) @(posedge clk);
    test.assert_error(
      o_ctrlport_resp_ack_7s, "CtrlPort response ACK not asserted!");
    test.assert_error(
      !o_ctrlport_resp_status_7s,
      $sformatf("CtrlPort response status not zero (%d)!", o_ctrlport_resp_status_7s));
    test.assert_error(
      o_ctrlport_resp_data_7s == expected_dna,
      $sformatf("Incorrect DNA value: %x! Expected %x.", o_ctrlport_resp_data_7s, expected_dna));
    @(posedge clk);
  endtask;

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    tb_name = $sformatf("device_dna_ctrlport");
    test.start_tb(tb_name, 1ms);

    //-------------------------------------------------------
    // Reset clock generator and wait for reset to complete
    //-------------------------------------------------------

    test.start_test("Reset", 100us);
    clk_gen.reset();
    if (rst) @rst;
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    // Reset DNA module and verify that we cannot read back serial immediately after
    test.start_test("Reset DUT only", 2us);
    rst_dut <= 1; // Assert reset for one clock cycle
    @(posedge clk);
    rst_dut <= 0;
    // Now we wait for some amount of clock cycles that is smaller than the time
    // it takes to fully load the DNA value (at least 96 cycles on any UltraScale
    // device)
    repeat (12) @(posedge clk);
    // Request a read and wait for ACK
    i_ctrlport_req_rd <= 1;
    @(posedge clk);
    i_ctrlport_req_rd <= 0;
    while (!o_ctrlport_resp_ack) @(posedge clk);
    test.assert_error(
      o_ctrlport_resp_status == 2'b01,
      $sformatf("Response status should be 1, but is %d", o_ctrlport_resp_status));
    // Oh no! We call reset *again*, in the middle of a read.
    rst_dut <= 1; // Assert reset for one clock cycle
    @(posedge clk);
    rst_dut <= 0;
    // Now wait for status to clear
    while (o_ctrlport_resp_status) @(posedge clk);
    test.end_test();

    // Read back the DNA value
    test.start_test("Reading DNA values", 1ms);
    check_dna(0, 32'h11C0FFEE);
    check_dna(4, 32'hC0D111A0);
    check_dna(8, 32'h012F1110);
    @(posedge clk);
    test.end_test();

    // One more reset and read
    test.start_test("Reading DNA values after 2nd reset", 1ms);
    rst_dut <= 1; // Assert reset for one clock cycle
    @(posedge clk);
    rst_dut <= 0;
    @(posedge clk);
    // Now wait for status to clear
    while (o_ctrlport_resp_status) @(posedge clk);
    check_dna(0, 32'h11C0FFEE);
    test.end_test();

    // Read back the DNA value (7-series)
    test.start_test("Reading DNA values (7-series)", 1ms);
    check_dna_7s(32, 32'hC0DE00FF);
    check_dna_7s(36, 32'h00D111A0);
    @(posedge clk);
    test.end_test();



    //--------------------------------
    // Finish Up
    //--------------------------------

    test.end_tb();

  end : tb_main

endmodule : device_dna_tb

`default_nettype wire
