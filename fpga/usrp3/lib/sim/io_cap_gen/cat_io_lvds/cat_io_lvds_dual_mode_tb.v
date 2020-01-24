//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_io_lvds_dual_mode_tb
//
// Description: Testbench for cat_io_lvds_dual_mode. 
//

`timescale 1ns/1ps

module cat_io_lvds_dual_mode_tb();

  localparam CLK_PERIOD    = 10;
  localparam CLK200_PERIOD = 2.5;

  localparam USE_CLOCK_IDELAY   = 1;
  localparam USE_DATA_IDELAY    = 1;
  localparam DATA_IDELAY_MODE   = "FIXED";
  localparam CLOCK_IDELAY_MODE  = "FIXED";
  localparam INPUT_CLOCK_DELAY  = 16;
  localparam INPUT_DATA_DELAY   = 0;
  localparam USE_CLOCK_ODELAY   = 1;
  localparam USE_DATA_ODELAY    = 1;
  localparam DATA_ODELAY_MODE   = "FIXED";
  localparam CLOCK_ODELAY_MODE  = "FIXED";
  localparam OUTPUT_CLOCK_DELAY = 31;
  localparam OUTPUT_DATA_DELAY  = 0;

  reg [8*19:0] test_status;
  reg          check_enabled;   // Controls when output checking is performed

  reg       clk    = 0;
  reg       rx_clk = 0;
  reg       clk200 = 0;

  reg       reset;
  reg       mimo;
  reg       tx_ch;
  reg [5:0] rx_d;
  reg       rx_frame;
  reg [7:0] rx_count = 0;

  // Each channel's data begins with a unique identifier (A../B.. or C../D..) 
  // followed by a count, which should always be sequential.
  wire [11:0] i0 = { 4'hA, rx_count };
  wire [11:0] q0 = { 4'hB, rx_count };
  wire [11:0] i1 = { 4'hC, rx_count };
  wire [11:0] q1 = { 4'hD, rx_count };

  wire radio_clk;

  reg [11:0] tx_i0;
  reg [11:0] tx_q0;
  reg [11:0] tx_i1;
  reg [11:0] tx_q1;

  wire [11:0] rx_i0;
  wire [11:0] rx_q0;
  wire [11:0] rx_i1;
  wire [11:0] rx_q1;

  wire rx_aligned;

  wire        tx_clk_p, tx_clk_n;
  wire        tx_frame_p, tx_frame_n;
  wire [5:0]  tx_d_p, tx_d_n;

  reg [4:0]  ctrl_in_data_delay;
  reg [4:0]  ctrl_in_clk_delay;
  reg        ctrl_ld_in_data_delay;
  reg        ctrl_ld_in_clk_delay;

  reg [4:0]  ctrl_out_data_delay;
  reg [4:0]  ctrl_out_clk_delay;
  reg        ctrl_ld_out_data_delay;
  reg        ctrl_ld_out_clk_delay;


  //---------------------------------------------------------------------------
  // Clock Generation
  //---------------------------------------------------------------------------

  // IODELAYCTRL reference clock
  always #(CLK200_PERIOD) clk200 = ~clk200;

  // Create an internal clock we'll use to drive the data
  always #(CLK_PERIOD) clk = ~clk;

  // RF interface clock. Half the rate of clk and out of phase
  always @(negedge clk) rx_clk <= ~rx_clk;


  //---------------------------------------------------------------------------
  // Tasks
  //---------------------------------------------------------------------------

  // Output a single burst of 2*len samples. In MIMO mode, this consists of len 
  // samples on each channel. In SISO mode, this consists of 2*len samples on  
  // the same channel.
  task Burst;
    input [31:0] len;
    input        do_mimo;
    begin
      repeat(len)
        begin
          mimo <= do_mimo;

          // Channel 0 sample
          @(posedge clk);
          rx_d <= i0[11:6];
          rx_frame <= 1;
          @(posedge clk);
          rx_d <= q0[11:6];
          rx_frame <= 1;
          @(posedge clk);
          rx_d <= i0[5:0];
          rx_frame <= do_mimo;
          @(posedge clk);
          rx_d <= q0[5:0];
          rx_frame <= do_mimo;

          // Channel 1 sample / Second channel 0 sample
          @(posedge clk);
          rx_d <= i1[11:6];
          rx_frame <= ~do_mimo;
          @(posedge clk);
          rx_d <= q1[11:6];
          rx_frame <= ~do_mimo;
          @(posedge clk);
          rx_d <= i1[5:0];
          rx_frame <= 0;
          @(posedge clk);
          rx_d <= q1[5:0];
          rx_frame <= 0;

          rx_count <= rx_count + 1;
        end
    end
  endtask // Burst


  // Test receiving/transmitting 2*len samples, checking len-2 for correctness. 
  // The output is checked by the Tx and Rx Output Checkers below. We have to 
  // be a little bit careful when we enable output checking, because it takes a 
  // few clock cycles for data to propagate through, and we don't want to check 
  // the outputs when the outputs are not valid.
  task TestBurst;
    input [31:0] len;
    input        do_mimo;
    begin
      if (len <= 2) begin
        $display("ERROR @%0t in %m: In TestBurst, len must be > 2", $time);
        $finish;
      end

      // Input several bursts, to fill the pipeline and cause results on the 
      // outputs before we start checking.
      Burst(1, do_mimo);

      // Enable output checking
      check_enabled <= 1'b1;

      // Do the requested length, minus 1
      Burst(len-2, do_mimo);

      // Disable output checking
      check_enabled <= 1'b0;

      // Give an extra output to allow data to propagate to the output
      Burst(1, do_mimo);
    end
  endtask // TestBurst


  //---------------------------------------------------------------------------
  // Test Procedure
  //---------------------------------------------------------------------------

  initial
    begin
      // Initial values
      check_enabled <= 1'b0;
      test_status <= "Reset";
      reset = 1;
      mimo  = 1;
      ctrl_in_clk_delay      = INPUT_CLOCK_DELAY;
      ctrl_in_data_delay     = INPUT_DATA_DELAY;
      ctrl_ld_in_data_delay  = 1'b0;
      ctrl_ld_in_clk_delay   = 1'b0;
      ctrl_out_clk_delay     = OUTPUT_CLOCK_DELAY;
      ctrl_out_data_delay    = OUTPUT_DATA_DELAY;
      ctrl_ld_out_data_delay = 1'b0;
      ctrl_ld_out_clk_delay  = 1'b0;
      repeat(10) @(negedge rx_clk);
      reset = 0;
      @(negedge rx_clk);

      //-----------------------------------------------------------------------
      // Test Changing Delays

      test_status <= "Load IO delays";

      if (CLOCK_IDELAY_MODE == "VAR_LOAD") begin
        ctrl_ld_in_clk_delay  = 1'b1;
        @(negedge rx_clk);
        ctrl_ld_in_clk_delay  = 1'b0;
        @(negedge rx_clk);
      end

      if (DATA_IDELAY_MODE == "VAR_LOAD") begin
        ctrl_ld_in_data_delay = 1'b1;
        @(negedge rx_clk);
        ctrl_ld_in_data_delay = 1'b0;
        @(negedge rx_clk);
      end

      if (CLOCK_ODELAY_MODE == "VAR_LOAD") begin
        ctrl_ld_out_clk_delay  = 1'b1;
        @(negedge rx_clk);
        ctrl_ld_out_clk_delay  = 1'b0;
        @(negedge rx_clk);
      end

      if (DATA_ODELAY_MODE == "VAR_LOAD") begin
        ctrl_ld_out_data_delay = 1'b1;
        @(negedge rx_clk);
        ctrl_ld_out_data_delay = 1'b0;
        @(negedge rx_clk);
      end

      //-----------------------------------------------------------------------
      // Startup

      test_status <= "Startup";

      // Pump a few clock cycles to get things started (flush out X values)
      Burst(2,1);

      //-----------------------------------------------------------------------
      // Test MIMO

      // Input data until the Rx circuit aligns
      test_status <= "Wait align 1";
      while (!rx_aligned) begin
        Burst(1,1);
      end

      // Input some new samples
      test_status <= "Burst 1 (MIMO)";
      TestBurst(30, 1);

      // Reset and do another burst
      test_status <= "Reset 2";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);      

      // Input data until the Rx circuit aligns
      test_status <= "Wait align 2";
      while (!rx_aligned) begin
        Burst(1,1);
      end

      // Input some new samples
      test_status <= "Burst 2 (MIMO)";
      TestBurst(23, 1);

      //-----------------------------------------------------------------------
      // Test SISO (transmit channel 0)

      tx_ch <= 1'b0;

      // Reset and do another burst
      test_status <= "Reset 3";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);      

      // Input data until the Rx circuit aligns in SISO mode
      test_status <= "Wait align 3";
      while (!rx_aligned) begin
        Burst(1,0);
      end

      // Test SISO mode
      test_status <= "Burst 3 (SISO, Ch 0)";
      TestBurst(25, 0);

      // Reset and do another burst
      test_status <= "Reset 4";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);      

      // Input data until the Rx circuit aligns in SISO mode
      test_status <= "Wait align 4";
      while (!rx_aligned) begin
        Burst(1,0);
      end

      // Test SISO mode
      test_status <= "Burst 4 (SISO, Ch 0)";
      TestBurst(27, 0);

      //-----------------------------------------------------------------------
      // Test SISO (transmit channel 1)

      tx_ch <= 1'b1;

      // Reset and do another burst
      test_status <= "Reset 5";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);      

      // Input data until the Rx circuit aligns in SISO mode
      test_status <= "Wait align 5";
      while (!rx_aligned) begin
        Burst(1,0);
      end

      // Test SISO mode
      test_status <= "Burst 5 (SISO, Ch 1)";
      TestBurst(25, 0);

      // Reset and do another burst
      test_status <= "Reset 6";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);      

      // Input data until the Rx circuit aligns in SISO mode
      test_status <= "Wait align 6";
      while (!rx_aligned) begin
        Burst(1,0);
      end

      // Test SISO mode
      test_status <= "Burst 6 (SISO, Ch 1)";
      TestBurst(27, 0);

      //-----------------------------------------------------------------------
      // Done

      test_status <= "Finished";
      repeat(50) @(negedge rx_clk);

      $finish;
    end


  //---------------------------------------------------------------------------
  // Rx Output Checker
  //---------------------------------------------------------------------------
  //
  // In MIMO mode, we expect to see:
  //
  //   rx_i0:  A00, A01, A02, A03, ...
  //   rx_q0:  B00, B01, B02, B03, ...
  //   rx_i1:  C00, C01, C02, C03, ...
  //   rx_q1:  D00, D01, D02, D03, ...
  //
  // In SISO mode, we expect to see (with twice the clock rate):
  //
  //   rx_i0:  A00, C00, A01, C01, ...
  //   rx_q0:  B00, D00, B01, D01, ...
  //   rx_i1:  A00, C00, A01, C01, ...
  //   rx_q1:  B00, D00, B01, D01, ...
  //
  //---------------------------------------------------------------------------

  reg first_rx_check = 1'b1;
  reg [11:0] rx_i0_del1, rx_i0_del2;
  reg [11:0] rx_q0_del1, rx_q0_del2;
  reg [11:0] rx_i1_del1, rx_i1_del2;
  reg [11:0] rx_q1_del1, rx_q1_del2;

  always @(posedge radio_clk)
  begin
    if (check_enabled) begin
      if (!first_rx_check) begin

        if (mimo) begin

          // Check prefix for channel 0
          if (rx_i0[11:8] != 4'hA || rx_q0[11:8] != 4'hB) begin
            $display("ERROR @%0t in %m: Rx channel 0 didn't have expected A/B prefix in MIMO mode", $time);
            $finish;
          end

          // Check prefix for channel 1
          if (rx_i1[11:8] != 4'hC || rx_q1[11:8] != 4'hD) begin
            $display("ERROR @%0t in %m: Rx channel 1 didn't have expected C/D in MIMO mode", $time);
            $finish;
          end

          // All outputs should have the same count in MIMO mode
          if (! (rx_i0[7:0] == rx_q0[7:0] &&
                 rx_i0[7:0] == rx_i1[7:0] && 
                 rx_i0[7:0] == rx_q1[7:0]) ) begin
            $display("ERROR @%0t in %m: Rx data counts didn't match on all outputs in MIMO mode", $time);
            $finish;
          end

          // Make sure the count increments
          if (rx_i0[7:0] != rx_i0_del1[7:0] + 8'd1 || rx_q0[7:0] != rx_q0_del1[7:0] + 8'd1 ||
              rx_i1[7:0] != rx_i1_del1[7:0] + 8'd1 || rx_q1[7:0] != rx_q1_del1[7:0] + 8'd1) begin
            $display("ERROR @%0t in %m: Rx data count didn't increment as expected", $time);
            $finish;
          end

        end else begin  // if (mimo)

          // In SISO mode, both outputs should be the same
          if (rx_i0 != rx_i1 || rx_q0 != rx_q1) begin
            $display("ERROR @%0t in %m: Rx channel 0 and 1 don't match in SISO mode", $time);
            $finish;
          end

          // Check channel 0 prefix. No need to check channel 1, since we 
          // already checked that the channels match.
          if (!((rx_i0[11:8] == 4'hA && rx_q0[11:8] == 4'hB) || 
                (rx_i0[11:8] == 4'hC && rx_q0[11:8] == 4'hD))) begin
            $display("ERROR @%0t in %m: Rx data didn't have expected A/B or C/D prefix in SISO mode", $time);
            $finish;
          end

          // Make sure we're alternating between channel data. No need to check 
          // channel 1, since we already checked that the channels match.
          if (!((rx_i0[11:8] == 4'hA && rx_i0_del1[11:8] == 4'hC) ||
                (rx_i0[11:8] == 4'hC && rx_i0_del1[11:8] == 4'hA) ||
                (rx_q0[11:8] == 4'hB && rx_q0_del1[11:8] == 4'hD) ||
                (rx_q0[11:8] == 4'hD && rx_q0_del1[11:8] == 4'hB))) begin
            $display("ERROR @%0t in %m: Rx data not toggling between channel data in SISO mode", $time);
            $finish;
          end

          // Make sure the counts are the same for both I and Q. No need to 
          // check channel 1, since we already checked that the channels match.
          if (rx_i0[7:0] != rx_q0[7:0]) begin
            $display("ERROR @%0t in %m: Rx data counts didn't match on all outputs in SISO mode", $time);
            $finish;
          end

          // Make sure the count increments every other clock cycle. No need to 
          // check channel 1, since we already checked that the channels match.
          if (!(
              rx_i0[7:0] != rx_i0_del2[7:0] + 8'd1 && (rx_i0[7:0] == rx_i0_del1[7:0] || rx_i0[7:0] == rx_i0_del1[7:0] + 8'd1) &&
              rx_q0[7:0] != rx_q0_del2[7:0] + 8'd1 && (rx_q0[7:0] == rx_q0_del1[7:0] || rx_q0[7:0] == rx_q0_del1[7:0] + 8'd1)
            )) begin
            $display("ERROR @%0t in %m: Rx data count didn't increment as expected", $time);
            $finish;
          end

        end  // if (mimo)
      end  // if (!first_rx_check)

      // Make sure we've captured at least one set of values, so we have a 
      // previous set to look back to.
      first_rx_check <= 1'b0;

    end else begin  // if (check_enabled)
      first_rx_check <= 1'b1;
    end  // if (check_enabled)

    // Save values seen this cycle
    rx_i0_del1 <= rx_i0;
    rx_q0_del1 <= rx_q0;
    rx_i1_del1 <= rx_i1;
    rx_q1_del1 <= rx_q1;
    rx_i0_del2 <= rx_i0_del2;
    rx_q0_del2 <= rx_q0_del2;
    rx_i1_del2 <= rx_i1_del2;
    rx_q1_del2 <= rx_q1_del2;
  end


  //---------------------------------------------------------------------------
  // Tx Output Checker
  //---------------------------------------------------------------------------
  //
  // The code implements a loopback, so the output should match the input. In 
  // SISO mode, however, the frame signal may not be aligned.
  //
  //---------------------------------------------------------------------------

  reg first_tx_check;
  reg [11:0] tx_i0_del1;
  reg [11:0] tx_q0_del1;
  reg [11:0] tx_i1_del1;
  reg [11:0] tx_q1_del1;
  reg        tx_frame_del1;

  reg [11:0] tx_i0_check;
  reg [11:0] tx_q0_check;
  reg [11:0] tx_i1_check;
  reg [11:0] tx_q1_check;
  reg [7:0]  tx_frame_check;


  always @(posedge tx_clk_p)
  begin
    tx_frame_del1 <= tx_frame_p;
  end


  always @(posedge tx_clk_p)
  begin
    if (tx_frame_p && !tx_frame_del1) begin
      //-----------------------------------------------------------------------
      // Grab two samples from the output, starting at frame boundary
      //-----------------------------------------------------------------------

      // Channel 0 sample
      tx_i0_check[11:6] <= tx_d_p;
      tx_frame_check[7] <= tx_frame_p;
      @(posedge tx_clk_n);
      tx_q0_check[11:6] <= tx_d_p;
      tx_frame_check[6] <= tx_frame_p;
      @(posedge tx_clk_p);
      tx_i0_check[5:0] <= tx_d_p;
      tx_frame_check[5] <= tx_frame_p;
      @(posedge tx_clk_n);
      tx_q0_check[5:0] <= tx_d_p;
      tx_frame_check[4] <= tx_frame_p;

      // Channel 1 sample / Second channel 0 sample
      @(posedge tx_clk_p);
      tx_i1_check[11:6] <= tx_d_p;
      tx_frame_check[3] <= tx_frame_p;
      @(posedge tx_clk_n);
      tx_q1_check[11:6] <= tx_d_p;
      tx_frame_check[2] <= tx_frame_p;
      @(posedge tx_clk_p);
      tx_i1_check[5:0] <= tx_d_p;
      tx_frame_check[1] <= tx_frame_p;
      @(posedge tx_clk_n);
      tx_q1_check[5:0] <= tx_d_p;
      tx_frame_check[0] <= tx_frame_p;

      #1   // Minimum delay for *_check registers to update in simulation

      if (check_enabled) begin
        if (!first_tx_check) begin
   
          if (mimo) begin
            //-----------------------------------------------------------------
            // Check MIMO output
            //-----------------------------------------------------------------

            // Check that the frame signal is correct
            if (tx_frame_check != 8'b11110000) begin
              $display("ERROR @%0t in %m: Tx frame was not correct in MIMO mode", $time);
              $finish;
            end
  
            // Check prefix for channel 0
            if (tx_i0_check[11:8] != 4'hA || tx_q0_check[11:8] != 4'hB) begin
              $display("ERROR @%0t in %m: Tx channel 0 didn't have expected A/B prefix in MIMO mode", $time);
              $finish;
            end
  
            // Check prefix for channel 1
            if (tx_i1_check[11:8] != 4'hC || tx_q1_check[11:8] != 4'hD) begin
              $display("ERROR @%0t in %m: Tx channel 1 didn't have expected C/D in MIMO mode", $time);
              $finish;
            end
  
            // All outputs should have the same count in MIMO mode
            if (! (tx_i0_check[7:0] == tx_q0_check[7:0] && 
                   tx_i0_check[7:0] == tx_i1_check[7:0] &&
                   tx_i0_check[7:0] == tx_q1_check[7:0]) ) begin
              $display("ERROR @%0t in %m: Rx data counts didn't match on all outputs in MIMO mode", $time);
              $finish;
            end
  
            // Make sure the count increments
            if (tx_i0_check[7:0] != tx_i0_del1[7:0] + 8'd1 || tx_q0_check[7:0] != tx_q0_del1[7:0] + 8'd1 ||
                tx_i1_check[7:0] != tx_i1_del1[7:0] + 8'd1 || tx_q1_check[7:0] != tx_q1_del1[7:0] + 8'd1) begin
              $display("ERROR @%0t in %m: Rx data count didn't increment as expected", $time);
              $finish;
            end
          
          end else begin  
            //-----------------------------------------------------------------
            // Check SISO Output
            //-----------------------------------------------------------------

            // Check that the frame signal is correct
            if (tx_frame_check != 8'b11001100) begin
              $display("ERROR @%0t in %m: Tx frame was not correct in SISO mode", $time);
              $finish;
            end


            // In SISO mode, the data we get depends on which channel is 
            // selected.
            //
            //        Channel 0:                   Channel 1:
            //  ...,A01,B01,A02,B02,...  OR  ...,C01,D01,C02,D02,...
            // 
            // So we should receive
            //
            //       A01 A03 A05
            //   ... B01 B03 B05 ...
            //       A02 B04 A06
            //       B02 B04 A07
            //
            // or
            //       C01 C03 C05
            //   ... D01 D03 D05 ...
            //       C02 C04 C06
            //       D02 D04 D07
            //

            // Check prefixes
            if (!(
              // Either A,B on channel 0 or C,D on channel 1
              ((tx_ch == 0 &&
                tx_i0_check[11:8] == 4'hA &&
                tx_q0_check[11:8] == 4'hB) ||
               (tx_ch == 1 && 
                tx_i0_check[11:8] == 4'hC &&
                tx_q0_check[11:8] == 4'hD)) && 
              // Samples 0 and 1 prefixes equal samples 2 and 3 prefixes
              (tx_i0_check[11:8] == tx_i1_check[11:8]  && 
               tx_q0_check[11:8] == tx_q1_check[11:8])
              )) begin
              $display("ERROR @%0t in %m: Tx channel didn't have expected prefixes in SISO mode", $time);
              $finish;
            end
  
            // Check that the data count matches between samples
            if (!(
              tx_i0_check[7:0] == tx_q0_check[7:0] &&
              tx_i1_check[7:0] == tx_q1_check[7:0] &&
              tx_i0_check[7:0] == tx_i1_check[7:0] - 8'd1
              )) begin
              $display("ERROR @%0t in %m: Tx channel data counts didn't correlate in SISO mode", $time);
              $finish;
            end

            // Make sure the count increments form one burst to the next
            if (tx_i0_check[7:0] != tx_i0_del1[7:0] + 8'd2 || 
                tx_q0_check[7:0] != tx_q0_del1[7:0] + 8'd2 ||
                tx_i1_check[7:0] != tx_i1_del1[7:0] + 8'd2 ||
                tx_q1_check[7:0] != tx_q1_del1[7:0] + 8'd2) begin
              $display("ERROR @%0t in %m: Tx data count didn't increment as expected", $time);
              $finish;
            end
          
          end

        end else begin  // if (!first_tx_check)
          // Make sure we've captured at least one set of values, so we have a 
          // previous set to look back to.
          first_tx_check <= 1'b0;
        end  // if (!first_tx_check)

        // Save values seen this cycle
        tx_i0_del1 <= tx_i0_check;
        tx_q0_del1 <= tx_q0_check;
        tx_i1_del1 <= tx_i1_check;
        tx_q1_del1 <= tx_q1_check;

      end else begin  // if (check_enabled)        
        first_tx_check <= 1'b1;

      end  // if (check_enabled)

    end  // if (tx_frame_p && !tx_frame_del1)

  end


  //---------------------------------------------------------------------------
  // Tx Input Data Generation
  //---------------------------------------------------------------------------
  //
  // Input a known data pattern similar to the Rx patten.
  //
  //   I0:     A01 A02 A03
  //   Q0: ... B01 B02 B03 ...
  //   I1:     C01 C02 C03
  //   Q1:     D01 D02 D03
  //
  //---------------------------------------------------------------------------

  reg [7:0] tx_count = 0;

  // Loop the Rx interface of DUT back to its Tx interface
  always @(posedge radio_clk) begin
    tx_i0 <= { 4'hA, tx_count };
    tx_q0 <= { 4'hB, tx_count };
    tx_i1 <= { 4'hC, tx_count };
    tx_q1 <= { 4'hD, tx_count };
    tx_count <= tx_count + 7'd1;
  end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  cat_io_lvds_dual_mode #(
    .INVERT_FRAME_RX    (0),
    .INVERT_DATA_RX     (6'b00_0000),
    .INVERT_FRAME_TX    (0),
    .INVERT_DATA_TX     (6'b00_0000),
    .USE_CLOCK_IDELAY   (USE_CLOCK_IDELAY),
    .USE_DATA_IDELAY    (USE_DATA_IDELAY),
    .DATA_IDELAY_MODE   (DATA_IDELAY_MODE),
    .CLOCK_IDELAY_MODE  (CLOCK_IDELAY_MODE),
    .INPUT_CLOCK_DELAY  (INPUT_CLOCK_DELAY),
    .INPUT_DATA_DELAY   (INPUT_DATA_DELAY),
    .USE_CLOCK_ODELAY   (USE_CLOCK_ODELAY),
    .USE_DATA_ODELAY    (USE_DATA_ODELAY),
    .DATA_ODELAY_MODE   (DATA_ODELAY_MODE),
    .CLOCK_ODELAY_MODE  (CLOCK_ODELAY_MODE),
    .OUTPUT_CLOCK_DELAY (OUTPUT_CLOCK_DELAY),
    .OUTPUT_DATA_DELAY  (OUTPUT_DATA_DELAY)
  ) cat_io_lvds_dual_mode_dut (
    .rst    (reset),
    .clk200 (clk200),
    
    // Data and frame timing
    .a_mimo  (mimo),
    .a_tx_ch (tx_ch),
    
    // Delay control interface
    .ctrl_clk               (rx_clk),
    //
    .ctrl_in_data_delay     (ctrl_in_data_delay),
    .ctrl_in_clk_delay      (ctrl_in_clk_delay),
    .ctrl_ld_in_data_delay  (ctrl_ld_in_data_delay),
    .ctrl_ld_in_clk_delay   (ctrl_ld_in_clk_delay),
    //
    .ctrl_out_data_delay    (ctrl_out_data_delay),
    .ctrl_out_clk_delay     (ctrl_out_clk_delay),
    .ctrl_ld_out_data_delay (ctrl_ld_out_data_delay),
    .ctrl_ld_out_clk_delay  (ctrl_ld_out_clk_delay),
    
    // Baseband sample interface
    .radio_clk  (radio_clk),
    .rx_aligned (rx_aligned),
    //
    .rx_i0      (rx_i0),
    .rx_q0      (rx_q0),
    .rx_i1      (rx_i1),
    .rx_q1      (rx_q1),
    //
    .tx_i0      (tx_i0),
    .tx_q0      (tx_q0),
    .tx_i1      (tx_i1),
    .tx_q1      (tx_q1),
    
    // Catalina interface
    .rx_clk_p   (rx_clk),
    .rx_clk_n   (~rx_clk),
    .rx_frame_p (rx_frame),
    .rx_frame_n (~rx_frame),
    .rx_d_p     (rx_d),
    .rx_d_n     (~rx_d),
    //
    .tx_clk_p   (tx_clk_p),
    .tx_clk_n   (tx_clk_n),
    .tx_frame_p (tx_frame_p),
    .tx_frame_n (tx_frame_n),
    .tx_d_p     (tx_d_p),
    .tx_d_n     (tx_d_n)
  );

endmodule // cat_io_lvds_dual_mode_tb
