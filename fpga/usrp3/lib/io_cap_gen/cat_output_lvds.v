//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_output_lvds
// Description: Transmit interface to AD9361 in LVDS mode.
//
// The frame_sample signal controls the expected frame signal timing. When 
// frame_sample is 0, the period of the ddr_frame signal will be equal to two 
// samples (e.g., one from each channel). When frame_sample is 1, the frame 
// period will be equal to the length of one sample. This allows the module to 
// be used for 2R2T (frame_sample = 1) or 1R1T mode (frame_sample = 0).


module cat_output_lvds #(
  parameter INVERT_FRAME_TX   = 0,
  parameter INVERT_DATA_TX    = 6'b00_0000,
  parameter USE_CLOCK_DELAY   = 1,
  parameter USE_DATA_DELAY    = 1,
  parameter CLOCK_DELAY_MODE = "VAR_LOAD",
  parameter DATA_DELAY_MODE  = "VAR_LOAD",
  parameter CLOCK_DELAY       = 0,
  parameter DATA_DELAY        = 0,
  parameter WIDTH             = 6,
  parameter GROUP             = "DEFAULT"
) (
  input clk200,
  input rst,

  // Data and frame timing (synchronous to radio_clk)
  input frame_sample,  // Two samples per frame period (frame_sample=0) or one sample per frame (frame_sample=1)

  // Region local Clocks for I/O cells.
  input ddr_clk,
  input sdr_clk,

  // Source synchronous external input clock
  output ddr_clk_p,
  output ddr_clk_n,

  // Source synchronous data lines
  output [WIDTH-1:0] ddr_data_p,
  output [WIDTH-1:0] ddr_data_n,
  output             ddr_frame_p,
  output             ddr_frame_n,

  // Delay control interface
  input       ctrl_clk,
  input [4:0] ctrl_data_delay,
  input [4:0] ctrl_clk_delay,
  input       ctrl_ld_data_delay,
  input       ctrl_ld_clk_delay,

  // Global input clock
  input radio_clk,

  // SDR data buses
  input [(WIDTH*2)-1:0] i0,
  input [(WIDTH*2)-1:0] q0,
  input [(WIDTH*2)-1:0] i1,
  input [(WIDTH*2)-1:0] q1

);


  //------------------------------------------------------------------
  // UG471 says take reset high asynchronously, and de-assert
  // synchronized to CLKDIV (sdr_clk) for SERDES.
  //------------------------------------------------------------------
  reg rst_sdr_sync;

  always @(posedge sdr_clk or posedge rst)
    if (rst)
      rst_sdr_sync <= 1'b1;
    else
      rst_sdr_sync <= 1'b0;

  //------------------------------------------------------------------
  // Route radio data to SERDES for SISO and MIMO modes.
  //------------------------------------------------------------------
  reg [(WIDTH*2)-1:0] radio_data_i0, radio_data_q0;
  reg [(WIDTH*2)-1:0] radio_data_i1, radio_data_q1;
  reg [(WIDTH*2)-1:0] data_i0, data_q0, data_i1, data_q1;
  //
  //
  always @(posedge radio_clk)
  begin
    // When in 2R2T mode, data mapping is the same for SISO and MIMO. The data
    // for channel 1 will be ignored.
    radio_data_i0 <= i0;
    radio_data_q0 <= q0;
    radio_data_i1 <= i1;
    radio_data_q1 <= q1;
  end

  //
  // Cross data into sdr_clock domain.
  // sdr_clock leads radio_clk in phase by insertion delay of BUFG.
  // We can transfer data in this direction with no special logic.
  // Path length must be radio_clk period - BUFG delay to make timing.
  //
  always @(posedge sdr_clk)
  begin
    data_i0 <= radio_data_i0 ^ {INVERT_DATA_TX,INVERT_DATA_TX};
    data_q0 <= radio_data_q0 ^ {INVERT_DATA_TX,INVERT_DATA_TX};
    data_i1 <= radio_data_i1 ^ {INVERT_DATA_TX,INVERT_DATA_TX};
    data_q1 <= radio_data_q1 ^ {INVERT_DATA_TX,INVERT_DATA_TX};
  end

  //------------------------------------------------------------------
  // Clock output
  //------------------------------------------------------------------
  wire ddr_clk_out, ddr_clk_dly;

  OSERDESE2 #(
    .DATA_RATE_OQ   ("DDR"),    // DDR, SDR
    .DATA_RATE_TQ   ("DDR"),    // DDR, BUF, SDR
    .DATA_WIDTH     (8),        // Parallel data width (2-8,10,14)
    .INIT_OQ        (1'b0),     // Initial value of OQ output (1'b0,1'b1)
    .INIT_TQ        (1'b0),     // Initial value of TQ output (1'b0,1'b1)
    .SERDES_MODE    ("MASTER"), // MASTER, SLAVE
    .SRVAL_OQ       (1'b0),     // OQ output value when SR is used (1'b0,1'b1)
    .SRVAL_TQ       (1'b0),     // TQ output value when SR is used (1'b0,1'b1)
    .TBYTE_CTL      ("FALSE"),  // Enable tristate byte operation (FALSE, TRUE)
    .TBYTE_SRC      ("FALSE"),  // Tristate byte source (FALSE, TRUE)
    .TRISTATE_WIDTH (1)         // 3-state converter width (1,4)
  ) ddr_clk_oserdese2 (
    .OFB       (),             // High-speed data output to ODELAYE2
    .OQ        (ddr_clk_out),  // High-speed data output direct to OBUF
    // SHIFTOUT1 / SHIFTOUT2: 1-bit (each) output: Data output expansion (1-bit each)
    .SHIFTOUT1 (),
    .SHIFTOUT2 (),
    .TBYTEOUT  (),
    .TFB       (),
    .TQ        (),
    .CLK       (ddr_clk),
    .CLKDIV    (sdr_clk),
    // D1 - D8: 1-bit (each) input: Parallel data inputs (1-bit each)
    .D1        (1'b1),         // Canned Clock waveform synthesized as data.
    .D2        (1'b0),
    .D3        (1'b1),
    .D4        (1'b0),
    .D5        (1'b1),
    .D6        (1'b0),
    .D7        (1'b1),
    .D8        (1'b0),
    .OCE       (1'b1),         // Active high clock enable
    .RST       (rst_sdr_sync),
    // SHIFTIN1 / SHIFTIN2: 1-bit (each) input: Data input expansion (1-bit each)
    .SHIFTIN1  (1'b0),
    .SHIFTIN2  (1'b0),
    // T1 - T4: 1-bit (each) input: Parallel 3-state inputs
    .T1        (1'b0),
    .T2        (1'b0),
    .T3        (1'b0),
    .T4        (1'b0),
    .TBYTEIN   (1'b0),
    .TCE       (1'b0)
  );

  generate
    if (USE_CLOCK_DELAY) begin : gen_clock_odelay
      (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
      ODELAYE2 #(
        .CINVCTRL_SEL          ("FALSE"),          // Enable dynamic clock inversion (FALSE, TRUE)
        .DELAY_SRC             ("ODATAIN"),        // Delay input (ODATAIN, CLKIN)
        .HIGH_PERFORMANCE_MODE ("FALSE"),          // Reduced jitter ("TRUE"), Reduced power ("FALSE")
        .ODELAY_TYPE           (CLOCK_DELAY_MODE), // FIXED, VARIABLE, VAR_LOAD, VAR_LOAD_PIPE
        .ODELAY_VALUE          (CLOCK_DELAY),      // Output delay tap setting (0-31)
        .PIPE_SEL              ("FALSE"),          // Select pipelined mode, FALSE, TRUE
        .REFCLK_FREQUENCY      (200.0),            // IDELAYCTRL clock input frequency in MHz (190.0-210.0, 290.0-310.0).
        .SIGNAL_PATTERN        ("CLOCK")           // DATA, CLOCK input signal
      ) ddr_clk_odelaye2 (
        .CNTVALUEOUT (),                  // 5-bit output: Counter value output
        .DATAOUT     (ddr_clk_dly),       // 1-bit output: Delayed data/clock output
        .C           (ctrl_clk),          // 1-bit input: Clock input
        .CE          (1'b0),              // 1-bit input: Active high enable increment/decrement input
        .CINVCTRL    (1'b0),              // 1-bit input: Dynamic clock inversion input
        .CLKIN       (1'b0),              // 1-bit input: Clock delay input
        .CNTVALUEIN  (ctrl_clk_delay),    // 5-bit input: Counter value input
        .INC         (1'b0),              // 1-bit input: Increment / Decrement tap delay input
        .LD          (ctrl_ld_clk_delay), // 1-bit input: Loads ODELAY_VALUE tap delay in VARIABLE mode, in VAR_LOAD or
                                          //              VAR_LOAD_PIPE mode, loads the value of CNTVALUEIN
        .LDPIPEEN    (1'b0),              // 1-bit input: Enables the pipeline register to load data
        .ODATAIN     (ddr_clk_out),       // 1-bit input: Output delay data input
        .REGRST      (1'b0)               // 1-bit input: Active-high reset tap-delay input
      );
    end else begin
      assign ddr_clk_dly = ddr_clk_out;
    end
  endgenerate



  OBUFDS ddr_clk_obuf (
    .O  (ddr_clk_p),   // Diff_p output (connect directly to top-level port)
    .OB (ddr_clk_n),   // Diff_n output (connect directly to top-level port)
    .I  (ddr_clk_dly)  // Buffer input
  );



  //------------------------------------------------------------------
  // Frame Signal
  //------------------------------------------------------------------
  wire ddr_frame, ddr_frame_dly;

  OSERDESE2 #(
    .DATA_RATE_OQ   ("DDR"),    // DDR, SDR
    .DATA_RATE_TQ   ("DDR"),    // DDR, BUF, SDR
    .DATA_WIDTH     (8),        // Parallel data width (2-8,10,14)
    .INIT_OQ        (1'b0),     // Initial value of OQ output (1'b0,1'b1)
    .INIT_TQ        (1'b0),     // Initial value of TQ output (1'b0,1'b1)
    .SERDES_MODE    ("MASTER"), // MASTER, SLAVE
    .SRVAL_OQ       (1'b0),     // OQ output value when SR is used (1'b0,1'b1)
    .SRVAL_TQ       (1'b0),     // TQ output value when SR is used (1'b0,1'b1)
    .TBYTE_CTL      ("FALSE"),  // Enable tristate byte operation (FALSE, TRUE)
    .TBYTE_SRC      ("FALSE"),  // Tristate byte source (FALSE, TRUE)
    .TRISTATE_WIDTH (1)         // 3-state converter width (1,4)
  ) ddr_frame_oserdese2 (
    .OFB       (),                 // High-speed data output to ODELAYE2
    .OQ        (ddr_frame),        // High-speed data output direct to OBUF
    // SHIFTOUT1 / SHIFTOUT2: 1-bit (each) output: Data output expansion (1-bit each)
    .SHIFTOUT1 (),
    .SHIFTOUT2 (),
    .TBYTEOUT  (),
    .TFB       (),
    .TQ        (),
    .CLK       (ddr_clk),
    .CLKDIV    (sdr_clk),
    // D1 - D8: 1-bit (each) input: Parallel data inputs (1-bit each). Frame is 
    // either 11110000 or 11001100 depending on if frame_sample is true or 
    // false, respectively, and it can be inverted by INVERT_FRAME_TX, becoming 
    // 00001111 or 00110011.
    .D1        (~INVERT_FRAME_TX[0]),
    .D2        (~INVERT_FRAME_TX[0]),
    .D3        (INVERT_FRAME_TX[0] ~^ frame_sample),
    .D4        (INVERT_FRAME_TX[0] ~^ frame_sample),
    .D5        (INVERT_FRAME_TX[0]  ^ frame_sample),
    .D6        (INVERT_FRAME_TX[0]  ^ frame_sample),
    .D7        (INVERT_FRAME_TX[0]),
    .D8        (INVERT_FRAME_TX[0]),
    .OCE       (1'b1),             // Active high clock enable
    .RST       (rst_sdr_sync),
    // SHIFTIN1 / SHIFTIN2: 1-bit (each) input: Data input expansion (1-bit each)
    .SHIFTIN1  (1'b0),
    .SHIFTIN2  (1'b0),
    // T1 - T4: 1-bit (each) input: Parallel 3-state inputs
    .T1        (1'b0),
    .T2        (1'b0),
    .T3        (1'b0),
    .T4        (1'b0),
    .TBYTEIN   (1'b0),
    .TCE       (1'b0)
  );

  generate
    if (USE_DATA_DELAY) begin : gen_frame_odelay
      (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
      ODELAYE2 #(
        .CINVCTRL_SEL          ("FALSE"),         // Enable dynamic clock inversion (FALSE, TRUE)
        .DELAY_SRC             ("ODATAIN"),       // Delay input (ODATAIN, CLKIN)
        .HIGH_PERFORMANCE_MODE ("FALSE"),         // Reduced jitter ("TRUE"), Reduced power ("FALSE")
        .ODELAY_TYPE           (DATA_DELAY_MODE), // FIXED, VARIABLE, VAR_LOAD, VAR_LOAD_PIPE
        .ODELAY_VALUE          (DATA_DELAY),      // Output delay tap setting (0-31)
        .PIPE_SEL              ("FALSE"),         // Select pipelined mode, FALSE, TRUE
        .REFCLK_FREQUENCY      (200.0),           // IDELAYCTRL clock input frequency in MHz (190.0-210.0, 290.0-310.0).
        .SIGNAL_PATTERN        ("DATA")           // DATA, CLOCK input signal
      ) ddr_frame_odelaye2 (
        .CNTVALUEOUT (),                   // 5-bit output: Counter value output
        .DATAOUT     (ddr_frame_dly),      // 1-bit output: Delayed data/clock output
        .C           (ctrl_clk),           // 1-bit input: Clock input
        .CE          (1'b0),               // 1-bit input: Active high enable increment/decrement input
        .CINVCTRL    (1'b0),               // 1-bit input: Dynamic clock inversion input
        .CLKIN       (1'b0),               // 1-bit input: Clock delay input
        .CNTVALUEIN  (ctrl_data_delay),    // 5-bit input: Counter value input
        .INC         (1'b0),               // 1-bit input: Increment / Decrement tap delay input
        .LD          (ctrl_ld_data_delay), // 1-bit input: Loads ODELAY_VALUE tap delay in VARIABLE mode, in VAR_LOAD or
                                           //              VAR_LOAD_PIPE mode, loads the value of CNTVALUEIN
        .LDPIPEEN    (1'b0),               // 1-bit input: Enables the pipeline register to load data
        .ODATAIN     (ddr_frame),          // 1-bit input: Output delay data input
        .REGRST      (1'b0)                // 1-bit input: Active-high reset tap-delay input
      );
    end else begin
      assign ddr_frame_dly = ddr_frame;
    end
  endgenerate

  OBUFDS ddr_frame_obuf (
    .O  (ddr_frame_p),   // Diff_p output (connect directly to top-level port)
    .OB (ddr_frame_n),   // Diff_n output (connect directly to top-level port)
    .I  (ddr_frame_dly)  // Buffer input
  );


  //------------------------------------------------------------------
  // Data Bus
  //------------------------------------------------------------------
  wire [WIDTH-1:0] ddr_data;
  wire [WIDTH-1:0] ddr_data_dly ;


  // wire [(WIDTH*2)-1:0] sdr_data_i;
  // wire [(WIDTH*2)-1:0] sdr_data_q;

  genvar i;
  generate
    for (i=0 ; i<WIDTH ; i=i+1) begin : generate_data_bus

      OSERDESE2 #(
        .DATA_RATE_OQ   ("DDR"),    // DDR, SDR
        .DATA_RATE_TQ   ("DDR"),    // DDR, BUF, SDR
        .DATA_WIDTH     (8),        // Parallel data width (2-8,10,14)
        .INIT_OQ        (1'b0),     // Initial value of OQ output (1'b0,1'b1)
        .INIT_TQ        (1'b0),     // Initial value of TQ output (1'b0,1'b1)
        .SERDES_MODE    ("MASTER"), // MASTER, SLAVE
        .SRVAL_OQ       (1'b0),     // OQ output value when SR is used (1'b0,1'b1)
        .SRVAL_TQ       (1'b0),     // TQ output value when SR is used (1'b0,1'b1)
        .TBYTE_CTL      ("FALSE"),  // Enable tristate byte operation (FALSE, TRUE)
        .TBYTE_SRC      ("FALSE"),  // Tristate byte source (FALSE, TRUE)
        .TRISTATE_WIDTH (1)         // 3-state converter width (1,4)
      ) ddr_data_oserdese2 (
        .OFB       (),                 // High-speed data output to ODELAYE2
        .OQ        (ddr_data[i]),      // High-speed data output direct to OBUF
        // SHIFTOUT1 / SHIFTOUT2: 1-bit (each) output: Data output expansion (1-bit each)
        .SHIFTOUT1 (),
        .SHIFTOUT2 (),
        .TBYTEOUT  (),
        .TFB       (),
        .TQ        (),
        .CLK       (ddr_clk),
        .CLKDIV    (sdr_clk),
        // D1 - D8: 1-bit (each) input: Parallel data inputs (1-bit each)
        .D1        (data_i0[WIDTH+i]),
        .D2        (data_q0[WIDTH+i]),
        .D3        (data_i0[i]),
        .D4        (data_q0[i]),
        .D5        (data_i1[WIDTH+i]),
        .D6        (data_q1[WIDTH+i]),
        .D7        (data_i1[i]),
        .D8        (data_q1[i]),
        .OCE       (1'b1),             // Active high clock enable
        .RST       (rst_sdr_sync),
        // SHIFTIN1 / SHIFTIN2: 1-bit (each) input: Data input expansion (1-bit each)
        .SHIFTIN1  (1'b0),
        .SHIFTIN2  (1'b0),
        // T1 - T4: 1-bit (each) input: Parallel 3-state inputs
        .T1        (1'b0),
        .T2        (1'b0),
        .T3        (1'b0),
        .T4        (1'b0),
        .TBYTEIN   (1'b0),
        .TCE       (1'b0)
      );

      if (USE_DATA_DELAY) begin : gen_data_odelay
        (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
        ODELAYE2 #(
          .CINVCTRL_SEL          ("FALSE"),         // Enable dynamic clock inversion (FALSE, TRUE)
          .DELAY_SRC             ("ODATAIN"),       // Delay input (ODATAIN, CLKIN)
          .HIGH_PERFORMANCE_MODE ("FALSE"),         // Reduced jitter ("TRUE"), Reduced power ("FALSE")
          .ODELAY_TYPE           (DATA_DELAY_MODE), // FIXED, VARIABLE, VAR_LOAD, VAR_LOAD_PIPE
          .ODELAY_VALUE          (DATA_DELAY),      // Output delay tap setting (0-31)
          .PIPE_SEL              ("FALSE"),         // Select pipelined mode, FALSE, TRUE
          .REFCLK_FREQUENCY      (200.0),           // IDELAYCTRL clock input frequency in MHz (190.0-210.0, 290.0-310.0).
          .SIGNAL_PATTERN        ("DATA")           // DATA, CLOCK input signal
        ) ddr_data_odelaye2 (
          .CNTVALUEOUT (),                   // 5-bit output: Counter value output
          .DATAOUT     (ddr_data_dly[i]),    // 1-bit output: Delayed data/clock output
          .C           (ctrl_clk),           // 1-bit input: Clock input
          .CE          (1'b0),               // 1-bit input: Active high enable increment/decrement input
          .CINVCTRL    (1'b0),               // 1-bit input: Dynamic clock inversion input
          .CLKIN       (1'b0),               // 1-bit input: Clock delay input
          .CNTVALUEIN  (ctrl_data_delay),    // 5-bit input: Counter value input
          .INC         (1'b0),               // 1-bit input: Increment / Decrement tap delay input
          .LD          (ctrl_ld_data_delay), // 1-bit input: Loads ODELAY_VALUE tap delay in VARIABLE mode, in VAR_LOAD or
                                             //              VAR_LOAD_PIPE mode, loads the value of CNTVALUEIN
          .LDPIPEEN    (1'b0),               // 1-bit input: Enables the pipeline register to load data
          .ODATAIN     (ddr_data[i]),        // 1-bit input: Output delay data input
          .REGRST      (1'b0)                // 1-bit input: Active-high reset tap-delay input
        );
      end else begin
        assign ddr_data_dly[i] = ddr_data[i];
      end

      OBUFDS ddr_data_obuf (
        .O  (ddr_data_p[i]),   // Diff_p output (connect directly to top-level port)
        .OB (ddr_data_n[i]),   // Diff_n output (connect directly to top-level port)
        .I  (ddr_data_dly[i])  // Buffer input
      );
    end // block: generate_data_bus

  endgenerate

endmodule // cat_output_lvds
