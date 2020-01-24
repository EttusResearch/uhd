//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_io_lvds_tb
//
// Description: Testbench for cat_io_lvds.
//

`timescale 1ns/1ps

module cat_io_lvds_tb();

  localparam CLK_PERIOD    = 10;
  localparam CLK200_PERIOD = 2.5;

  localparam FRAME_SAMPLE = 0;

  localparam USE_CLOCK_IDELAY   = 1;
  localparam USE_DATA_IDELAY    = 1;
  localparam DATA_IDELAY_MODE   = "VAR_LOAD";
  localparam CLOCK_IDELAY_MODE  = "VAR_LOAD";
  localparam INPUT_CLOCK_DELAY  = 0;
  localparam INPUT_DATA_DELAY   = 0;
  localparam USE_CLOCK_ODELAY   = 1;
  localparam USE_DATA_ODELAY    = 1;
  localparam DATA_ODELAY_MODE   = "VAR_LOAD";
  localparam CLOCK_ODELAY_MODE  = "VAR_LOAD";
  localparam OUTPUT_CLOCK_DELAY = 0;
  localparam OUTPUT_DATA_DELAY  = 0;

  reg [8*19:0] test_status;

  reg       clk      = 0;
  reg       rx_clk   = 0;
  reg       clk200   = 0;

  reg       reset;
  reg       mimo;
  reg [5:0] rx_d;
  reg       rx_frame;
  reg [7:0] count;

//  initial $dumpfile("catcap_ddr_lvds_tb.vcd");
//  initial $dumpvars(0,catcap_ddr_lvds_tb);

  wire [11:0] i0 = {4'hA,count};
  wire [11:0] q0 = {4'hB,count};
  wire [11:0] i1 = {4'hC,count};
  wire [11:0] q1 = {4'hD,count};

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

  task BURST;
    input [31:0] len;
    input        do_mimo;
    begin
      count <= 0;
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
          rx_frame <= ~FRAME_SAMPLE;
          @(posedge clk);
          rx_d <= q0[5:0];
          rx_frame <= ~FRAME_SAMPLE;

          if (do_mimo) begin
            // Channel 1 sample
            @(posedge clk);
            rx_d <= i1[11:6];
            rx_frame <= FRAME_SAMPLE;
            @(posedge clk);
            rx_d <= q1[11:6];
            rx_frame <= FRAME_SAMPLE;
            @(posedge clk);
            rx_d <= i1[5:0];
            rx_frame <= 0;
            @(posedge clk);
            rx_d <= q1[5:0];
            rx_frame <= 0;
          end else begin
            if (!FRAME_SAMPLE) begin
              // When we frame every two samples (one from each channel), in
              // MIMO mode, we should only grab channel 0. So input garbage on
              // channel 1 to make sure the data doesn't get used.
              @(posedge clk);
              rx_d <= 6'bXXXXXX;
              rx_frame <= FRAME_SAMPLE;
              @(posedge clk);
              rx_d <= 6'bXXXXXX;
              rx_frame <= FRAME_SAMPLE;
              @(posedge clk);
              rx_d <= 6'bXXXXXX;
              rx_frame <= 0;
              @(posedge clk);
              rx_d <= 6'bXXXXXX;
              rx_frame <= 0;
            end else begin
              // When every sample is framed, we might sync align to either
              // channel (no way to tell them apart), so input the channel 1
              // data, but we should only see one channel's data duplicated on
              // both outputs.
              @(posedge clk);
              rx_d <= i1[11:6];
              rx_frame <= FRAME_SAMPLE;
              @(posedge clk);
              rx_d <= q1[11:6];
              rx_frame <= FRAME_SAMPLE;
              @(posedge clk);
              rx_d <= i1[5:0];
              rx_frame <= 0;
              @(posedge clk);
              rx_d <= q1[5:0];
              rx_frame <= 0;
            end
          end

          count <= count + 1;
        end
    end
  endtask // BURST


  //---------------------------------------------------------------------------
  // Test Procedure
  //---------------------------------------------------------------------------

  initial
    begin
      // Initial values
      test_status <= "Reset";
      reset = 1;
      mimo = 1;
      ctrl_in_data_delay = 5'd0;
      ctrl_in_clk_delay = 5'd8;
      ctrl_ld_in_data_delay = 1'b0;
      ctrl_ld_in_clk_delay = 1'b0;
      ctrl_out_data_delay = 5'd0;
      ctrl_out_clk_delay = 5'd16;
      ctrl_ld_out_data_delay = 1'b0;
      ctrl_ld_out_clk_delay = 1'b0;
      repeat(10) @(negedge rx_clk);
      reset = 0;
      @(negedge rx_clk);

      // Load new input delay values
      test_status <= "Load input delays";
      ctrl_ld_in_data_delay = 1'b1;
      ctrl_ld_in_clk_delay = 1'b1;
      @(negedge rx_clk);
      ctrl_ld_in_data_delay = 1'b0;
      ctrl_ld_in_clk_delay = 1'b0;

      // Load new output delay values
      test_status <= "Load output delays";
      ctrl_ld_out_data_delay = 1'b1;
      ctrl_ld_out_clk_delay = 1'b1;
      @(negedge rx_clk);
      ctrl_ld_out_data_delay = 1'b0;
      ctrl_ld_out_clk_delay = 1'b0;

      // Input data until the Rx circuit aligns
      test_status <= "Wait align";
      while (!rx_aligned) begin
        BURST(1,1);
      end

      // Input some new samples
      test_status <= "Burst 1 (MIMO)";
      BURST(30, 1);

      // Reset and do another burst
      test_status <= "Reset 2";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);

      // Input data until the Rx circuit aligns
      test_status <= "Wait align 2";
      while (!rx_aligned) begin
        BURST(1,1);
      end

      // Input some new samples
      test_status <= "Burst 2 (MIMO)";
      BURST(30, 1);

      // Reset and do another burst
      test_status <= "Reset 3";
      reset = 1;
      repeat(20) @(negedge rx_clk);
      reset = 0;
      repeat(2) @(negedge rx_clk);

      // Input data until the Rx circuit aligns in SISO mode
      test_status <= "Wait align 3";
      while (!rx_aligned) begin
        BURST(1,0);
      end

      // Switch to SISO mode
      test_status <= "Burst 3 (SISO)";
      BURST(25,0);

      repeat(50) @(negedge rx_clk);


      $finish;
    end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  // Loop the Rx interface of cat_io_lvds back to its Tx interface
  always @(posedge radio_clk) begin
    tx_i0 = rx_i0;
    tx_q0 = rx_q0;
    tx_i1 = rx_i1;
    tx_q1 = rx_q1;
  end

  cat_io_lvds #(
    .INVERT_FRAME_RX    (0),
    .INVERT_DATA_RX     (6'b00_0000),
    .INVERT_FRAME_TX    (0),
    .INVERT_DATA_TX     (6'b00_0000),
    .USE_CLOCK_IDELAY   (USE_CLOCK_IDELAY  ),
    .USE_DATA_IDELAY    (USE_DATA_IDELAY   ),
    .DATA_IDELAY_MODE   (DATA_IDELAY_MODE  ),
    .CLOCK_IDELAY_MODE  (CLOCK_IDELAY_MODE ),
    .INPUT_CLOCK_DELAY  (INPUT_CLOCK_DELAY ),
    .INPUT_DATA_DELAY   (INPUT_DATA_DELAY  ),
    .USE_CLOCK_ODELAY   (USE_CLOCK_ODELAY  ),
    .USE_DATA_ODELAY    (USE_DATA_ODELAY   ),
    .DATA_ODELAY_MODE   (DATA_ODELAY_MODE  ),
    .CLOCK_ODELAY_MODE  (CLOCK_ODELAY_MODE ),
    .OUTPUT_CLOCK_DELAY (OUTPUT_CLOCK_DELAY),
    .OUTPUT_DATA_DELAY  (OUTPUT_DATA_DELAY )
  ) cat_io_lvds_i0 (
    .rst    (reset),
    .clk200 (clk200),

    // Data and frame timing
    .mimo         (mimo),
    .frame_sample (FRAME_SAMPLE[0]),

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
    .radio_clk    (radio_clk),
    .radio_clk_2x (),
    .rx_aligned   (rx_aligned),
    //
    .rx_i0        (rx_i0),
    .rx_q0        (rx_q0),
    .rx_i1        (rx_i1),
    .rx_q1        (rx_q1),
    //
    .tx_i0        (tx_i0),
    .tx_q0        (tx_q0),
    .tx_i1        (tx_i1),
    .tx_q1        (tx_q1),

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

endmodule // cat_io_lvds_tb
