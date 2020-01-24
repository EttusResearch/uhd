//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_fifo_tb();

  /*********************************************
  ** User variables
  *********************************************/
  localparam FIFO_SIZE = 1;
  localparam TEST_VECTOR_SIZE = 10;

  /*********************************************
  ** Clocks & Reset
  *********************************************/
  `define CLOCK_FREQ    200e6
  `define RESET_TIME    100

  reg clk;
  initial clk = 1'b0;
  localparam CLOCK_PERIOD = 1e9/`CLOCK_FREQ;
  always
    #(CLOCK_PERIOD) clk = ~clk;

  reg reset;
  initial begin
    reset = 1'b1;
    #(`RESET_TIME);
    @(posedge clk);
    reset = 1'b0;
  end

  /*********************************************
  ** DUT
  *********************************************/
  reg [31:0] i_tdata;
  reg i_tvalid, o_tready;
  wire i_tready, o_tvalid;
  wire [31:0] o_tdata;
  reg clear;

  axi_fifo #(
    .SIZE(FIFO_SIZE),
    .WIDTH(32))
  dut_axi_fifo (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .space(), .occupied());

  /*********************************************
  ** Testbench
  *********************************************/
  reg [TEST_VECTOR_SIZE-1:0] i_tvalid_sequence;
  reg [TEST_VECTOR_SIZE-1:0] o_tready_sequence;
  integer i,k,n,i_tready_timeout;
  reg [31:0] o_tdata_check;

  initial begin
    i_tdata = {32{1'b1}};
    i_tvalid = 1'b0;
    o_tready = 1'b0;
    i_tready_timeout = 0;
    clear = 1'b0;
    @(negedge reset);
    #(10*CLOCK_PERIOD)
    @(posedge clk);
    $display("*****************************************************");
    $display("**              Begin Assertion Tests              **");
    $display("*****************************************************");
    $display("Test 1 -- Check filling FIFO");
    // Note, if REG_OUTPUT is enabled, the FIFO has space for 1 extra entry
    for (i = 0; i < 2**FIFO_SIZE; i = i + 1) begin
      if (~i_tready) begin
          $display("Test 1 FAILED!");
          $error("FIFO size should be %d entries, but detected %d!",2**FIFO_SIZE,i);
          $stop;
      end
      i_tvalid = 1'b1;
      i_tdata = i_tdata + 32'd1;
      @(posedge clk);
    end
    i_tvalid = 1'b0;
    @(posedge clk);
    if (i_tready) begin
      $display("Test 1 warning!");
      $warning("i_tready still asserted after filling FIFO with %d entries! Might be due to output registering.",i);
      //$stop;
    end
    $display("Test 1 Passed!");
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    $display("Test 2 -- Check emptying FIFO");
    for (i = 0; i < 2**FIFO_SIZE; i = i + 1) begin
      if (~o_tvalid) begin
          $display("Test 2 FAILED!");
          $error("FIFO o_tvalid not asserted! Occured at entry %d",2**FIFO_SIZE-i+1);
          $stop;
      end
      o_tready = 1'b1;
      @(posedge clk);
    end
    o_tready = 1'b0;
    @(posedge clk);
    if (o_tvalid) begin
      $display("Test 1 FAILED!");
      $error("o_tvalid still asserted after emptying FIFO!");
      $stop;
    end
    $display("Test 2 Passed!");
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    $display("Test 3 -- Check for o_tready / i_tready dropping unexpectantly");
    clear = 1'b1;
    i_tdata = {32{1'b1}};
    i_tvalid = 1'b0;
    o_tready = 1'b0;
    @(posedge clk);
    clear = 1'b0;
    @(posedge clk);
    for (i = 0; i < 2**FIFO_SIZE-1; i = i + 1) begin
      i_tvalid = 1'b1;
      i_tdata = i_tdata + 32'd1;
      @(posedge clk);
      i_tvalid = 1'b0;
      // Give some time to propogate
      @(posedge clk);
      @(posedge clk);
      @(posedge clk);
      if (~i_tready) begin
          $display("Test 3 FAILED!");
          $error("i_tready deasserted unexpectantly after writing %d entries!",i+1);
          $stop;
      end
      if (~o_tvalid) begin
          $display("Test 3 FAILED!");
          $error("o_tvalid deasserted unexpectantly after writing %d entries!",i+1);
          $stop;
      end
    end
    // Write final entry
    i_tvalid = 1'b1;
    i_tdata = i_tdata + 32'd1;
    @(posedge clk);
    i_tvalid = 1'b0;
    @(posedge clk);
    if (i_tready) begin
        $display("Test 3 warning!");
        $warning("i_tready still asserted after writing %d entries! Might be due to output registering.",i+1);
        //$stop;
    end
    @(posedge clk);
    for (i = 0; i < 2**FIFO_SIZE-1; i = i + 1) begin
      o_tready = 1'b1;
      @(posedge clk);
      o_tready = 1'b0;
      // Give some time to propogate
      @(posedge clk);
      @(posedge clk);
      @(posedge clk);
      if (~i_tready) begin
          $display("Test 3 FAILED!");
          $error("i_tready deasserted unexpectantly after reading %d entries!",i+1);
          $stop;
      end
      if (~o_tvalid) begin
          $display("Test 3 FAILED!");
          $error("o_tvalid deasserted unexpectantly after reading %d entries!",i+1);
          $stop;
      end
    end
    // Read final entry
    o_tready = 1'b1;
    @(posedge clk);
    o_tready = 1'b0;
    @(posedge clk);
    if (o_tvalid) begin
        $display("Test 3 FAILED!");
        $error("o_tvalid still asserted after reading %d entries!",i+1);
        $stop;
    end
    @(posedge clk);
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    $display("Test 4 -- Check for bubble states");
    clear = 1'b1;
    i_tdata = {32{1'b1}};
    i_tvalid = 1'b0;
    o_tready = 1'b0;
    @(posedge clk);
    clear = 1'b0;
    @(posedge clk);
    // Fill up half way
    for (i = 0; i < (2**FIFO_SIZE + 1)/2; i = i + 1) begin
      i_tvalid = 1'b1;
      i_tdata = i_tdata + 32'd1;
      @(posedge clk);
    end
    // Start reading
    o_tready = 1'b1;
    i_tdata = i_tdata + 32'd1;
    @(posedge clk);
    // Give a clock cycle for latency, but no bubbles should occur after this
    i_tdata = i_tdata + 32'd1;
    @(posedge clk);
    // Continue to write and read at full rate
    for (i = 0; i < (2**FIFO_SIZE + 1)/2; i = i + 1) begin
      if (~i_tready) begin
          $display("Test 4 FAILED!");
          $error("FIFO bubble state detected when writing & reading at full rate!");
          $stop;
      end
      i_tvalid = 1'b1;
      i_tdata = i_tdata + 32'd1;
      @(posedge clk);
    end
    // Read at full, write at half rate
    for (i = 0; i < (2**FIFO_SIZE + 1)/2; i = i + 1) begin
      if (~i_tready | ~o_tvalid) begin
          $display("Test 4 FAILED!");
          $error("FIFO bubble state detected when write at half rate, reading at full rate!");
          $stop;
      end
      i_tvalid = ~i_tvalid;
      if (i_tvalid) i_tdata = i_tdata + 32'd1;
      @(posedge clk);
    end
    // Read at half rate, write at full rate
    for (i = 0; i < (2**FIFO_SIZE + 1)/2; i = i + 1) begin
      if (~i_tready | ~o_tvalid) begin
          $display("Test 4 FAILED!");
          $error("FIFO bubble state detected when write at half rate, reading at full rate!");
          $stop;
      end
      o_tready = ~o_tready;
      i_tvalid = 1'b1;
      i_tdata = i_tdata + 32'd1;
      @(posedge clk);
    end
    $display("Test 4 Passed!");
    //////////////////////////////////////////////////////////////////////////////
    // Tests combinations of i_tvalid / o_tready sequences.
    // Test space depends on TEST_VECTOR_SIZE.
    // Example: TEST_VECTOR_SIZE = 10 => 1024*1024 number of test sequences,
    //          which is every possible 10 bit sequence of i_tvalid / o_tready.
    $display("Test 5 -- Check combinations of i_tvalid / o_tready");
    clear = 1'b1;
    i_tdata = {32{1'b1}};
    i_tvalid = 1'b0;
    o_tready = 1'b0;
    i_tready_timeout = 0;
    i_tvalid_sequence = {TEST_VECTOR_SIZE{1'd0}};
    o_tready_sequence = {TEST_VECTOR_SIZE{1'd0}};
    @(posedge clk);
    clear = 1'b0;
    @(posedge clk);
    for (i = 0; i < 2**TEST_VECTOR_SIZE; i = i + 1) begin
      i_tvalid_sequence = i_tvalid_sequence + 1;
      for (k = 0; k < 2**TEST_VECTOR_SIZE; k = k + 1) begin
        o_tready_sequence = o_tready_sequence + 1;
        for (n = 0; n < TEST_VECTOR_SIZE; n = n + 1) begin
          if (o_tready_sequence[n]) begin
            o_tready = 1'b1;
          end else begin
            o_tready = 1'b0;
          end
          // Special Case: If i_tready timed out, then i_tvalid is still asserted and we cannot
          //               deassert i_tvalid until we see a corresponding i_tready. This is a basic
          //               AXI stream requirement, so we will continue to assert i_tvalid regardless
          //               of what i_tvalid_sequence would have set i_tvalid for this loop.
          if (i_tvalid_sequence[n] | (i_tready_timeout == TEST_VECTOR_SIZE)) begin
            i_tvalid = 1'b1;
            if (i_tready_timeout < TEST_VECTOR_SIZE) begin
              i_tdata = i_tdata + 32'd1;
            end
            @(posedge clk);
            i_tready_timeout = 0;
            // Wait for i_tready until timeout. Timeouts may occur when o_tready_sequence
            // has o_tready not asserted for several clock cycles.
            while(~i_tready & (i_tready_timeout < TEST_VECTOR_SIZE)) begin
              @(posedge clk)
              i_tready_timeout = i_tready_timeout + 1;
            end
          end else begin
            i_tvalid = 1'b0;
            @(posedge clk);
          end
        end
      end
      // Reset starting conditions for the test sequences
      clear = 1'b1;
      i_tdata = {32{1'b1}};
      i_tvalid = 1'b0;
      o_tready = 1'b0;
      i_tready_timeout = 0;
      @(posedge clk);
      clear = 1'b0;
      @(posedge clk);
    end
    $display("Test 5 Passed!");
    $display("All tests PASSED!");
    $stop;
  end

  // Check the input counting sequence independent of
  // i_tvalid / o_tready sequences.
  always @(posedge clk) begin
    if (reset) begin
      o_tdata_check <= 32'd0;
    end else begin
      if (clear) begin
        o_tdata_check <= 32'd0;
      end
      if (o_tready & o_tvalid) begin
        o_tdata_check <= o_tdata_check + 32'd1;
        if (o_tdata != o_tdata_check) begin
          $display("Test FAILED!");
          $error("Incorrect output!");
          $stop;
        end
      end
    end
  end

endmodule
