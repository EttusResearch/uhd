//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// The two clocks are aligned externally in order to eliminate the need for a FIFO.
// A FIFO cannot be used to transition between clock domains because it can cause
// alignment issues between the output of multiple modules.

module capture_ddrlvds #(
  parameter WIDTH            = 14,          //Width of the SS data bus
  parameter PATT_CHECKER     = "FALSE",     //{TRUE, FALSE}: Is the integrated ramp pattern checker  
  parameter DATA_IDELAY_MODE = "BYPASSED",  //{BYPASSED, FIXED, DYNAMIC}
  parameter DATA_IDELAY_VAL  = 16,          //IDELAY value for FIXED mode. In DYNAMIC mode, this value is used by the timing analyzer
  parameter DATA_IDELAY_FREF = 200.0        //Reference clock frequency for the IDELAYCTRL
) (
  // ADC IO Pins
  input             adc_clk_p,
  input             adc_clk_n,
  input [WIDTH-1:0] adc_data_p,
  input [WIDTH-1:0] adc_data_n,
  
  //System synchronous clock
  input             radio_clk,
  
  //IDELAY settings
  input             data_delay_stb,
  input [4:0]       data_delay_val,

  //Capture clock and output data
  output            adc_cap_clk,
  output [(2*WIDTH)-1:0] data_out,
  
  //Pattern checker options (sync to radio_clk)
  input             checker_en,
  output [3:0]      checker_locked,
  output [3:0]      checker_failed
);

  //-------------------------------------------------------------------
  // Clock Path
  
  wire adc_buf_clk;
  
  // Route source synchronous clock to differential input clock buffer
  // then to a global clock buffer. We route to a global buffer because
  // the data bus being capture spans multiple banks.
  IBUFGDS ss_clk_ibufgds_i (
    .I(adc_clk_p), .IB(adc_clk_n),
    .O(adc_buf_clk)
  );

  BUFG ss_clk_bufg_i (
    .I(adc_buf_clk),
    .O(adc_cap_clk)
  );

  //-------------------------------------------------------------------
  // Data Path

  wire [WIDTH-1:0]     adc_data_buf, adc_data_del;
  wire [(2*WIDTH)-1:0] adc_data_aclk;
  reg  [(2*WIDTH)-1:0] adc_data_rclk, adc_data_rclk_sync;

  genvar i;
  generate for(i = 0; i < WIDTH; i = i + 1) begin : gen_lvds_pins

    // Use a differential IO buffer to get the data into the IOB
    IBUFDS ibufds_i (
      .I(adc_data_p[i]), .IB(adc_data_n[i]),
      .O(adc_data_buf[i])
    );

    // Use an optional IDELAY to tune the capture interface from
    // software. This is a clock to data delay calibration so all
    // data bits are delayed by the same amount.
    if (DATA_IDELAY_MODE != "BYPASSED") begin
      // Pipeline IDELAY control signals to ease routing
      reg       data_delay_stb_reg;
      reg [4:0] data_delay_val_reg;
      always @(posedge radio_clk)
        {data_delay_stb_reg, data_delay_val_reg} <= {data_delay_stb, data_delay_val};
      
      IDELAYE2 #(
        .DELAY_SRC("IDATAIN"),              // Delay input (IDATAIN, DATAIN)
        .IDELAY_TYPE(DATA_IDELAY_MODE=="FIXED"?"FIXED":"VAR_LOAD"), // FIXED, VARIABLE, VAR_LOAD, VAR_LOAD_PIPE
        .SIGNAL_PATTERN("DATA"),            // DATA, CLOCK input signal
        .HIGH_PERFORMANCE_MODE("TRUE"),     // Reduced jitter ("TRUE"), Reduced power ("FALSE")
        .PIPE_SEL("FALSE"),                 // Select pipelined mode, FALSE, TRUE
        .CINVCTRL_SEL("FALSE"),             // Enable dynamic clock inversion (FALSE, TRUE)
        .IDELAY_VALUE(DATA_IDELAY_VAL),     // Input delay tap setting (0-31)
        .REFCLK_FREQUENCY(DATA_IDELAY_FREF) // IDELAYCTRL clock input frequency in MHz (190.0-210.0).
      ) idelay_i (
        .DATAIN(1'b0),                      // Internal delay data input
        .IDATAIN(adc_data_buf[i]),          // Data input from the I/O
        .DATAOUT(adc_data_del[i]),          // Delayed data output
        .C(radio_clk),                      // Clock input
        .LD(data_delay_stb_reg),            // Load IDELAY_VALUE input
        .CE(1'b0),                          // Active high enable increment/decrement input
        .INC(1'b0),                         // Increment / Decrement tap delay input
        .CINVCTRL(1'b0),                    // Dynamic clock inversion input
        .CNTVALUEIN(data_delay_val_reg),    // Counter value input
        .CNTVALUEOUT(),                     // Counter value output
        .LDPIPEEN(1'b0),                    // Enable PIPELINE register to load data input
        .REGRST(1'b0)                       // Reset for the pipeline register.Only used in VAR_LOAD_PIPE mode.
      );
    end else begin
      assign adc_data_del[i] = adc_data_buf[i];
    end

    // Use the global ADC clock to capture delayed data into an IDDR.
    // Each IQ sample is transferred in QDR mode i.e. odd and even on
    // a rising and falling edge of the clock
    IDDR #(
      .DDR_CLK_EDGE("SAME_EDGE_PIPELINED")
    ) iddr_i (
      .C(adc_cap_clk), .CE(1'b1),
      .D(adc_data_del[i]), .R(1'b0), .S(1'b0),
      .Q1(adc_data_aclk[2*i]), .Q2(adc_data_aclk[(2*i)+1])
    );
  end endgenerate

  // Transfer data from the source-synchronous ADC clock domian to the
  // system synchronous radio clock domain. We assume that adc_cap_clk
  // and radio_clk are generated from the same source and have the same 
  // frequency however, they have an unknown but constant phase offset.
  // In order to cross domains, we use a simple synchronizer to avoid any
  // sample-sample delay uncertainty introduced by FIFOs.
  // NOTE: The path between adc_data_aclk and adc_data_rclk must be
  //       constrained to prevent build to build variations. Also, the 
  //       phase of the two clocks must be aligned ensure that the data
  //       capture is safe
  always @(posedge radio_clk)
    {adc_data_rclk_sync, adc_data_rclk} <= {adc_data_rclk, adc_data_aclk};

  // The synchronized output is the output of this module
  assign data_out = adc_data_rclk_sync;

  //-------------------------------------------------------------------
  // Checkers

  generate if (PATT_CHECKER == "TRUE") begin
    wire        checker_en_aclk;
    wire [1:0]  checker_locked_aclk, checker_failed_aclk;

    synchronizer #(.INITIAL_VAL(1'b0)) checker_en_aclk_sync_i (
      .clk(adc_cap_clk), .rst(1'b0), .in(checker_en), .out(checker_en_aclk));
    synchronizer #(.INITIAL_VAL(1'b0)) checker_locked_aclk_0_sync_i (
      .clk(radio_clk), .rst(1'b0), .in(checker_locked_aclk[0]), .out(checker_locked[0]));
    synchronizer #(.INITIAL_VAL(1'b0)) checker_locked_aclk_1_sync_i (
      .clk(radio_clk), .rst(1'b0), .in(checker_locked_aclk[1]), .out(checker_locked[1]));
    synchronizer #(.INITIAL_VAL(1'b0)) checker_failed_aclk_0_sync_i (
      .clk(radio_clk), .rst(1'b0), .in(checker_failed_aclk[0]), .out(checker_failed[0]));
    synchronizer #(.INITIAL_VAL(1'b0)) checker_failed_aclk_1_sync_i (
      .clk(radio_clk), .rst(1'b0), .in(checker_failed_aclk[1]), .out(checker_failed[1]));

    cap_pattern_verifier #(   // Q Channel : Synchronous to SSCLK
      .WIDTH(WIDTH), .PATTERN("RAMP"), .HOLD_CYCLES(1),
      .RAMP_START(0), .RAMP_STOP({WIDTH{1'b1}}), .RAMP_INCR(1)
    ) aclk_q_checker_i (
      .clk(adc_cap_clk), .rst(~checker_en_aclk),
      .valid(1'b1), .data(~adc_data_aclk[WIDTH-1:0]),
      .count(), .errors(),
      .locked(checker_locked_aclk[0]), .failed(checker_failed_aclk[0])
    );
  
    cap_pattern_verifier #(   // I Channel : Synchronous to SSCLK
      .WIDTH(WIDTH), .PATTERN("RAMP"), .HOLD_CYCLES(1),
      .RAMP_START(0), .RAMP_STOP({WIDTH{1'b1}}), .RAMP_INCR(1)
    ) aclk_i_checker_i (
      .clk(adc_cap_clk), .rst(~checker_en_aclk),
      .valid(1'b1), .data(~adc_data_aclk[(2*WIDTH)-1:WIDTH]),
      .count(), .errors(),
      .locked(checker_locked_aclk[1]), .failed(checker_failed_aclk[1])
    );
  
    cap_pattern_verifier #(   // Q Channel : Synchronous to Radio CLK
      .WIDTH(WIDTH), .PATTERN("RAMP"), .HOLD_CYCLES(1),
      .RAMP_START(0), .RAMP_STOP({WIDTH{1'b1}}), .RAMP_INCR(1)
    ) rclk_q_checker_i (
      .clk(radio_clk), .rst(~checker_en),
      .valid(1'b1), .data(~adc_data_rclk_sync[WIDTH-1:0]),
      .count(), .errors(),
      .locked(checker_locked[2]), .failed(checker_failed[2])
    );
  
    cap_pattern_verifier #(   // I Channel : Synchronous to Radio CLK
      .WIDTH(WIDTH), .PATTERN("RAMP"), .HOLD_CYCLES(1),
      .RAMP_START(0), .RAMP_STOP({WIDTH{1'b1}}), .RAMP_INCR(1)
    ) rclk_i_checker_i (
      .clk(radio_clk), .rst(~checker_en),
      .valid(1'b1), .data(~adc_data_rclk_sync[(2*WIDTH)-1:WIDTH]),
      .count(), .errors(),
      .locked(checker_locked[3]), .failed(checker_failed[3])
    );
  end endgenerate

endmodule // capture_ddrlvds
