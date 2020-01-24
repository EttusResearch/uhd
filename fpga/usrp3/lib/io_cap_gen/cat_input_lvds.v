//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_input_lvds
//
// Description:
//
// Receive interface to AD9361 (Catalina) in LVDS mode.
//
// Use Xilinx SERDES to deserialize interleaved sample data off
// half-word-width LVDS differential data bus from the Catalina.
//
// Use FRAME signal to initially synchronize to incoming data after reset
// de-asserts.
//
// In all modes (SISO or MIMO) we output a clock of 1/4 the frequency
// of the Catalina source-synchronous bus clock to be used as the radio_clk.
//
// In SISO mode, every cycle of the radio_clk supplies a new RX sample which
// is routed to both radios, even if only one is actively receiving.
//
// In MIMO mode, every cycle of the radio clock supplies a pair of
// time aligned MIMO samples which are routed to different radios.
//
// The frame_sample signal controls the expected frame signal timing. When 
// frame_sample is 0, the period of the ddr_frame signal is expected to equal 
// two samples (e.g., one from each channel). When frame_sample is 1, the frame 
// period is expected to equal the length of one sample. This allows the module 
// to be used for 2R2T (frame_sample = 1) or 1R1T mode (frame_sample = 0).
//


module cat_input_lvds #(
  parameter INVERT_FRAME_RX  = 0,
  parameter INVERT_DATA_RX   = 6'b00_0000,
  parameter USE_CLOCK_DELAY  = 1,
  parameter USE_DATA_DELAY   = 1,
  parameter CLOCK_DELAY_MODE = "VAR_LOAD",
  parameter DATA_DELAY_MODE  = "VAR_LOAD",
  parameter CLOCK_DELAY      = 0,
  parameter DATA_DELAY       = 0,
  parameter WIDTH            = 6,
  parameter GROUP            = "DEFAULT",
  parameter USE_BUFG         = 1
) (
  input clk200,
  input rst,

  // Data and frame timing (synchronous to radio_clk)
  input mimo,          // Output one channel (MIMO=0) or two (MIMO=1)
  input frame_sample,  // Two samples per frame period (frame_sample=0) or one sample per frame (frame_sample=1)

  // Region local Clocks for I/O cells.
  output ddr_clk,
  output sdr_clk,

  // Source Synchronous external input clock
  input ddr_clk_p,
  input ddr_clk_n,

  // Source Synchronous data lines
  input [WIDTH-1:0] ddr_data_p,
  input [WIDTH-1:0] ddr_data_n,
  input             ddr_frame_p,
  input             ddr_frame_n,

  // Delay control interface
  input       ctrl_clk,
  input [4:0] ctrl_data_delay,
  input [4:0] ctrl_clk_delay,
  input       ctrl_ld_data_delay,
  input       ctrl_ld_clk_delay,

  // Global output clocks, ddr_clk/4 & ddr_clk/2
  output radio_clk,
  output radio_clk_2x,

  // SDR Data buses
  output reg [(WIDTH*2)-1:0] i0,
  output reg [(WIDTH*2)-1:0] q0,
  output reg [(WIDTH*2)-1:0] i1,
  output reg [(WIDTH*2)-1:0] q1,
  output reg                 rx_aligned

);

  //------------------------------------------------------------------
  // UG471 says take reset high asynchronously, and de-assert
  // synchronized to CLKDIV (sdr_clk) for SERDES.
  //------------------------------------------------------------------
  (* ASYNC_REG = "TRUE" *) reg rst_sdr_sync, rst_sdr_sync_ms;

  always @(posedge sdr_clk or posedge rst)
    if (rst) begin
      rst_sdr_sync_ms <= 1'b1;
      rst_sdr_sync    <= 1'b1;
    end else begin
      rst_sdr_sync_ms <= 1'b0;
      rst_sdr_sync    <= rst_sdr_sync_ms;
    end


  //------------------------------------------------------------------
  // IDELAY is calibrated using (mandatory) IDELAYCTRL cell.
  // Must be feed stable free running clock specified by: FIDELAYCTRL_REF.(200MHz)
  // Mandatory async reset required, min pulse of: TIDELAYCTRL_RPW (~60nS)
  //------------------------------------------------------------------
  (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
  IDELAYCTRL IDELAYCTRL_i0 (
    .REFCLK (clk200),
    .RST    (rst_sdr_sync),
    .RDY    ()
  );


  //------------------------------------------------------------------
  // Clock input
  //------------------------------------------------------------------
  wire ddr_clk_dly, ddr_clk_unbuf;

  IBUFDS #(
    .DIFF_TERM("TRUE")
  ) clk_ibufds (
    .O(ddr_clk_unbuf),
    .I(ddr_clk_p),
    .IB(ddr_clk_n)
  );

  generate
    if (USE_CLOCK_DELAY) begin : gen_clock_delay
      (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
      IDELAYE2 #(
        .CINVCTRL_SEL          ("FALSE"),     // Enable dynamic clock inversion (FALSE, TRUE)
        .DELAY_SRC             ("IDATAIN"),   // Delay input (IDATAIN, DATAIN)
        .HIGH_PERFORMANCE_MODE ("FALSE"),     // Reduced jitter ("TRUE"), Reduced power ("FALSE")
        .IDELAY_TYPE           (CLOCK_DELAY_MODE),
        .IDELAY_VALUE          (CLOCK_DELAY),
        .PIPE_SEL              ("FALSE"),
        .REFCLK_FREQUENCY      (200.0),
        .SIGNAL_PATTERN        ("CLOCK")
      ) ddr_clk_idelaye2 (
        .CNTVALUEOUT (),                  // 5-bit output: Counter value output
        .DATAOUT     (ddr_clk_dly),       // 1-bit output: Delayed data output
        .C           (ctrl_clk),          // 1-bit input: Clock input
        .CE          (1'b0),              // 1-bit input: Active high enable increment/decrement input
        .CINVCTRL    (1'b0),              // 1-bit input: Dynamic clock inversion input
        .CNTVALUEIN  (ctrl_clk_delay),    // 5-bit input: Counter value input
        .DATAIN      (1'b0),              // 1-bit input: Internal delay data input
        .IDATAIN     (ddr_clk_unbuf),     // 1-bit input: Data input from the I/O
        .INC         (1'b0),              // 1-bit input: Increment / Decrement tap delay input
        .LD          (ctrl_ld_clk_delay), // 1-bit input: Load IDELAY_VALUE input
        .LDPIPEEN    (1'b0),              // 1-bit input: Enable PIPELINE register to load data input
        .REGRST      (1'b0)               // 1-bit input: Active-high reset tap-delay input
      ); 
    end
    else begin
      assign ddr_clk_dly = ddr_clk_unbuf;
    end

  endgenerate

  // IO CLock is DDR freq. This drives SERDES and other I/O elements with minimal clock skew.
  BUFIO ddr_clk_bufio (.O(ddr_clk),.I(ddr_clk_dly));

  // SDR clock is one quarter DDR freq and local to regio using BUFR
  // BUFR is a constraint of the SERDES since we need frequency agnostic clock division.
  // UG471 states can use BUFIO and BUFR divided to directly drive a SERDES legally.
  // (Other option is pair of BUFG's plus an MMCM - But MMCM has fixed frequency)
  wire sdr_clk_2x;

  BUFR #(
    .BUFR_DIVIDE ("2"),
    .SIM_DEVICE  ("7SERIES")
  ) sdr_clk_2x_bufr (
    .O   (sdr_clk_2x),
    .CE  (1'b1),
    .CLR (1'b0),
    .I   (ddr_clk_dly)
  );

  BUFR #(
    .BUFR_DIVIDE("4"),
    .SIM_DEVICE("7SERIES")
  ) sdr_clk_bufr (
    .O(sdr_clk),
    .CE(1'b1),
    .CLR(1'b0),
    .I(ddr_clk_dly)
  );

  generate
    if (USE_BUFG) begin : gen_BUFG
      // radio_clock is sdr_clk re-buffered with BUFG, and radio_clk_2x is
      // sdr_clk_2x re-buffered, so both can be used globally. This introduces skew
      // between sdr_clk -> radio_clock so we must hand data between them carefully
      // even though they have a fixed phase relationship.
      BUFG radio_clk_1x_bufg (.O(radio_clk), .I(sdr_clk));
      BUFG radio_clk_2x_bufg (.O(radio_clk_2x), .I(sdr_clk_2x));  
    end else begin
      assign radio_clk = sdr_clk;
      assign radio_clk_2x = sdr_clk_2x;
    end
  endgenerate



  //------------------------------------------------------------------
  // Frame Signal
  //------------------------------------------------------------------
  wire       ddr_frame, ddr_frame_dly;
  wire [7:0] des_frame; // deserialized  frame signal
  reg        bitslip;
  reg        aligned;


  //
  // Use FRAME signal to get bitstream word aligned.
  //
  // In MIMO mode, FRAME is asserted during the entirety of channel 0, and
  // deasserts during the entirety of channel 1.
  //
  localparam IDLE   = 0;
  localparam SEARCH = 1;
  localparam SLIP1  = 3;
  localparam SLIP2  = 2;
  localparam SLIP3  = 4;
  localparam SLIP4  = 5;
  localparam SYNC   = 6;


  reg [2:0] frame_state;

  //
  // Delay start of framesync operation for 64 clocks after reset de-asserts to
  // SERDES to be sure they are in a steady state.
  //
  // Each time we assert bitslip we then have to wait 2 cycles before we can
  // examine the results.
  //
  // Checking for 0xF0 and 0xCC allows us to support 1R1T and 2R2T timing, 
  // which have different frame periods.
  wire frame_is_aligned = 
          (!frame_sample && (des_frame[7:0] == (INVERT_FRAME_RX ? 8'h0F : 8'hF0))) ||
          ( frame_sample && (des_frame[7:0] == (INVERT_FRAME_RX ? 8'h33 : 8'hCC)));

  reg [5:0] sync_delay;
  reg       run_sync;


  always @(posedge sdr_clk)
    if (rst_sdr_sync) begin
      sync_delay <= 6'h0;
      run_sync <= 1'b0;
    end else if (sync_delay == 6'h3F)
      run_sync <= 1'b1;
    else
      sync_delay <= sync_delay + 1'b1;

  always @(posedge sdr_clk)
  begin
    if (!run_sync) begin
      frame_state <= IDLE;
      bitslip <= 1'b0;
      aligned <= 1'b0;
    end else begin
      case (frame_state)
        IDLE: begin
          bitslip <= 1'b0;
          aligned <= 1'b0;
          frame_state <= SEARCH;
        end

        SEARCH: begin
          if (frame_is_aligned) begin
            frame_state <= SYNC;
            bitslip <= 1'b0;
            aligned <= 1'b1;
          end else begin
            // Bitslip until captured frame is aligned
            bitslip <= 1'b1;
            frame_state <= SLIP1;
            aligned <= 1'b0;
          end
        end

        SLIP1: begin
          frame_state <= SLIP2;
          bitslip <= 1'b0;
          aligned <= 1'b0;
        end

        SLIP2: begin
          frame_state <= SLIP3;
          bitslip <= 1'b0;
          aligned <= 1'b0;
        end

        SLIP3: begin
          frame_state <= SLIP4;
          bitslip <= 1'b0;
          aligned <= 1'b0;
        end

        SLIP4: begin
          frame_state <= SEARCH;
          bitslip <= 1'b0;
          aligned <= 1'b0;
        end

        SYNC: begin
          if (frame_is_aligned) begin
            frame_state <= SYNC;
            aligned <= 1'b1;
          end else begin
            frame_state <= SEARCH;
            aligned <= 1'b0;
          end
        end

      endcase // case(frame_state)

    end
  end


  IBUFDS #(
    .DIFF_TERM ("TRUE")
  ) ddr_frame_ibufds (
    .O  (ddr_frame),
    .I  (ddr_frame_p),
    .IB (ddr_frame_n)
  );

  generate
    if (USE_DATA_DELAY) begin : gen_frame_delay
      (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
      IDELAYE2 #(
        .CINVCTRL_SEL          ("FALSE"),    // Enable dynamic clock inversion (FALSE, TRUE)
        .DELAY_SRC             ("IDATAIN"),  // Delay input (IDATAIN, DATAIN)
        .HIGH_PERFORMANCE_MODE ("FALSE"),    // Reduced jitter ("TRUE"), Reduced power ("FALSE")
        .IDELAY_TYPE           (DATA_DELAY_MODE),
        .IDELAY_VALUE          (DATA_DELAY),
        .PIPE_SEL              ("FALSE"),
        .REFCLK_FREQUENCY      (200.0),
        .SIGNAL_PATTERN        ("DATA")
      ) ddr_frame_idelaye2 (
        .CNTVALUEOUT (),                   // 5-bit output: Counter value output
        .DATAOUT     (ddr_frame_dly),      // 1-bit output: Delayed data output
        .C           (ctrl_clk),           // 1-bit input: Clock input
        .CE          (1'b0),               // 1-bit input: Active high enable increment/decrement input
        .CINVCTRL    (1'b0),               // 1-bit input: Dynamic clock inversion input
        .CNTVALUEIN  (ctrl_data_delay),    // 5-bit input: Counter value input
        .DATAIN      (1'b0),               // 1-bit input: Internal delay data input
        .IDATAIN     (ddr_frame),          // 1-bit input: Data input from the I/O
        .INC         (1'b0),               // 1-bit input: Increment / Decrement tap delay input
        .LD          (ctrl_ld_data_delay), // 1-bit input: Load IDELAY_VALUE input
        .LDPIPEEN    (1'b0),               // 1-bit input: Enable PIPELINE register to load data input
        .REGRST      (1'b0)                // 1-bit input: Active-high reset tap-delay input
      );
    end
    else begin
      assign ddr_frame_dly = ddr_frame;
    end
  endgenerate

  ISERDESE2 #(
    .DATA_RATE         ("DDR"),        // DDR, SDR
    .DATA_WIDTH        (8),            // Parallel data width (2-8,10,14)
    .DYN_CLKDIV_INV_EN ("FALSE"),      // Enable DYNCLKDIVINVSEL inversion (FALSE, TRUE)
    .DYN_CLK_INV_EN    ("FALSE"),      // Enable DYNCLKINVSEL inversion (FALSE, TRUE)
    // INIT_Q1 - INIT_Q4: Initial value on the Q outputs (0/1)
    .INIT_Q1           (1'b0),
    .INIT_Q2           (1'b0),
    .INIT_Q3           (1'b0),
    .INIT_Q4           (1'b0),
    .INTERFACE_TYPE    ("NETWORKING"),
    .IOBDELAY          ("BOTH"),
    .NUM_CE            (1),
    .OFB_USED          ("FALSE"),
    .SERDES_MODE       ("MASTER"),
    // SRVAL_Q1 - SRVAL_Q4: Q output values when SR is used (0/1)
    .SRVAL_Q1          (1'b0),
    .SRVAL_Q2          (1'b0),
    .SRVAL_Q3          (1'b0),
    .SRVAL_Q4          (1'b0)
  ) ddr_frame_serdese2 (
    .O            (),              // 1-bit output: Combinatorial output
    // Q1 - Q8: 1-bit (each) output: Registered data outputs
    .Q1           (des_frame[0]),
    .Q2           (des_frame[1]),
    .Q3           (des_frame[2]),
    .Q4           (des_frame[3]),
    .Q5           (des_frame[4]),
    .Q6           (des_frame[5]),
    .Q7           (des_frame[6]),
    .Q8           (des_frame[7]),
    // SHIFTOUT1, SHIFTOUT2: 1-bit (each) output: Data width expansion output ports
    .SHIFTOUT1    (),
    .SHIFTOUT2    (),
    // 1-bit input: The BITSLIP pin performs a Bitslip operation synchronous to
    // CLKDIV when asserted (active High). Subsequently, the data seen on the Q1
    // to Q8 output ports will shift, as in a barrel-shifter operation, one
    // position every time Bitslip is invoked (DDR operation is different from SDR)
    .BITSLIP      (bitslip),
    // CE1, CE2: 1-bit (each) input: Data register clock enable inputs
    .CE1          (1'b1),
    .CE2          (1'b1),
    .CLKDIVP      (1'b0),          // 1-bit input: TBD
    // Clocks: 1-bit (each) input: ISERDESE2 clock input ports
    .CLK          (ddr_clk),       // 1-bit input: High-speed clock
    .CLKB         (~ddr_clk),      // 1-bit input: High-speed secondary clock
    .CLKDIV       (sdr_clk),       // 1-bit input: Divided clock
    .OCLK         (1'b0),          // 1-bit input: High-speed output clock used when INTERFACE_TYPE="MEMORY"
    // Dynamic Clock Inversions: 1-bit (each) input: Dynamic clock inversion pins to switch clock polarity
    .DYNCLKDIVSEL (1'b0),          // 1-bit input: Dynamic CLKDIV inversion
    .DYNCLKSEL    (1'b0),          // 1-bit input: Dynamic CLK/CLKB inversion
    // Input Data: 1-bit (each) input: ISERDESE2 data input ports
    .D            (1'b0),          // 1-bit input: Data input
    .DDLY         (ddr_frame_dly), // 1-bit input: Serial data from IDELAYE2
    .OFB          (1'b0),          // 1-bit input: Data feedback from OSERDESE2
    .OCLKB        (1'b0),          // 1-bit input: High-speed negative edge output clock
    .RST          (rst_sdr_sync),  // 1-bit input: Active high asynchronous reset
    // SHIFTIN1, SHIFTIN2: 1-bit (each) input: Data width expansion input ports
    .SHIFTIN1     (1'b0),
    .SHIFTIN2     (1'b0)
  );


  //------------------------------------------------------------------
  // Data Bus
  //------------------------------------------------------------------
  wire [WIDTH-1:0] ddr_data;
  wire [WIDTH-1:0] ddr_data_dly;
  wire [(WIDTH*2)-1:0] data_i0, data_i1;
  wire [(WIDTH*2)-1:0] data_q0, data_q1;


  genvar i;
  generate
    for (i=0 ; i<WIDTH ; i=i+1) begin : generate_data_bus

      IBUFDS #(
        .DIFF_TERM ("TRUE")
      ) ddr_data_ibufds (
        .O  (ddr_data[i]),
        .I  (ddr_data_p[i]),
        .IB (ddr_data_n[i])
      );

      if (USE_DATA_DELAY) begin : gen_data_delay
        (* IODELAY_GROUP = GROUP *) // Specifies group name for associated IDELAYs/ODELAYs and IDELAYCTRL
        IDELAYE2 #(
          .CINVCTRL_SEL          ("FALSE"),    // Enable dynamic clock inversion (FALSE, TRUE)
          .DELAY_SRC             ("IDATAIN"),  // Delay input (IDATAIN, DATAIN)
          .HIGH_PERFORMANCE_MODE ("FALSE"),    // Reduced jitter ("TRUE"), Reduced power ("FALSE")
          .IDELAY_TYPE           (DATA_DELAY_MODE),
          .IDELAY_VALUE          (DATA_DELAY),
          .PIPE_SEL              ("FALSE"),
          .REFCLK_FREQUENCY      (200.0),
          .SIGNAL_PATTERN        ("DATA")
        ) ddr_data_idelaye2 (
          .CNTVALUEOUT (),                   // 5-bit output: Counter value output
          .DATAOUT     (ddr_data_dly[i]),    // 1-bit output: Delayed data output
          .C           (ctrl_clk),           // 1-bit input: Clock input
          .CE          (1'b0),               // 1-bit input: Active high enable increment/decrement input
          .CINVCTRL    (1'b0),               // 1-bit input: Dynamic clock inversion input
          .CNTVALUEIN  (ctrl_data_delay),    // 5-bit input: Counter value input
          .DATAIN      (1'b0),               // 1-bit input: Internal delay data input
          .IDATAIN     (ddr_data[i]),        // 1-bit input: Data input from the I/O
          .INC         (1'b0),               // 1-bit input: Increment / Decrement tap delay input
          .LD          (ctrl_ld_data_delay), // 1-bit input: Load IDELAY_VALUE input
          .LDPIPEEN    (1'b0),               // 1-bit input: Enable PIPELINE register to load data input
          .REGRST      (1'b0)                // 1-bit input: Active-high reset tap-delay input
        );
      end
      else begin
        assign ddr_data_dly[i] = ddr_data[i];
      end

      ISERDESE2 #(
        .DATA_RATE         ("DDR"),        // DDR, SDR
        .DATA_WIDTH        (8),            // Parallel data width (2-8,10,14)
        .DYN_CLKDIV_INV_EN ("FALSE"),      // Enable DYNCLKDIVINVSEL inversion (FALSE, TRUE)
        .DYN_CLK_INV_EN    ("FALSE"),      // Enable DYNCLKINVSEL inversion (FALSE, TRUE)
        // INIT_Q1 - INIT_Q4: Initial value on the Q outputs (0/1)
        .INIT_Q1           (1'b0),
        .INIT_Q2           (1'b0),
        .INIT_Q3           (1'b0),
        .INIT_Q4           (1'b0),
        .INTERFACE_TYPE    ("NETWORKING"),
        .IOBDELAY          ("BOTH"),
        .NUM_CE            (1),
        .OFB_USED          ("FALSE"),
        .SERDES_MODE       ("MASTER"),
        // SRVAL_Q1 - SRVAL_Q4: Q output values when SR is used (0/1)
        .SRVAL_Q1          (1'b0),
        .SRVAL_Q2          (1'b0),
        .SRVAL_Q3          (1'b0),
        .SRVAL_Q4          (1'b0)
      ) ddr_data_serdese2 (
        .O            (),                 // 1-bit output: Combinatorial output
        // Q1 - Q8: 1-bit (each) output: Registered data outputs
        .Q1           (data_q1[i]),
        .Q2           (data_i1[i]),
        .Q3           (data_q1[WIDTH+i]),
        .Q4           (data_i1[WIDTH+i]),
        .Q5           (data_q0[i]),
        .Q6           (data_i0[i]),
        .Q7           (data_q0[WIDTH+i]),
        .Q8           (data_i0[WIDTH+i]),
        // SHIFTOUT1, SHIFTOUT2: 1-bit (each) output: Data width expansion output ports
        .SHIFTOUT1    (),
        .SHIFTOUT2    (),
        // 1-bit input: The BITSLIP pin performs a Bitslip operation synchronous to
        // CLKDIV when asserted (active High). Subsequently, the data seen on the Q1
        // to Q8 output ports will shift, as in a barrel-shifter operation, one
        // position every time Bitslip is invoked (DDR operation is different from SDR)
        .BITSLIP      (bitslip),
        // CE1, CE2: 1-bit (each) input: Data register clock enable inputs
        .CE1          (1'b1),
        .CE2          (1'b1),
        .CLKDIVP      (1'b0),             // 1-bit input: TBD
        // Clocks: 1-bit (each) input: ISERDESE2 clock input ports
        .CLK          (ddr_clk),          // 1-bit input: High-speed clock
        .CLKB         (~ddr_clk),         // 1-bit input: High-speed secondary clock
        .CLKDIV       (sdr_clk),          // 1-bit input: Divided clock
        .OCLK         (1'b0),             // 1-bit input: High-speed output clock used when INTERFACE_TYPE="MEMORY"
        // Dynamic Clock Inversions: 1-bit (each) input: Dynamic clock inversion pins to switch clock polarity
        .DYNCLKDIVSEL (1'b0),             // 1-bit input: Dynamic CLKDIV inversion
        .DYNCLKSEL    (1'b0),             // 1-bit input: Dynamic CLK/CLKB inversion
        // Input Data: 1-bit (each) input: ISERDESE2 data input ports
        .D            (1'b0),             // 1-bit input: Data input
        .DDLY         (ddr_data_dly[i]),  // 1-bit input: Serial data from IDELAYE2
        .OFB          (1'b0),             // 1-bit input: Data feedback from OSERDESE2
        .OCLKB        (1'b0),             // 1-bit input: High-speed negative edge output clock
        .RST          (rst_sdr_sync),     // 1-bit input: Active high asynchronous reset
        // SHIFTIN1, SHIFTIN2: 1-bit (each) input: Data width expansion input ports
        .SHIFTIN1     (1'b0),
        .SHIFTIN2     (1'b0)
      );

    end // block: generate_data_bus
  endgenerate

  //
  // Cross these cycles to radio_clk using negative edge. This give 1/2 a radio
  // clock period + BUFG insertion delay for signals to propagate. Thats > 6nS.
  //
  reg [(WIDTH*2)-1:0] radio_data_i0, radio_data_i1, radio_data_q0, radio_data_q1;
  reg                 radio_aligned;

  always @(negedge radio_clk)
    begin
      radio_data_i0[(WIDTH*2)-1:0] <= data_i0[(WIDTH*2)-1:0] ^ {INVERT_DATA_RX,INVERT_DATA_RX};
      radio_data_q0[(WIDTH*2)-1:0] <= data_q0[(WIDTH*2)-1:0] ^ {INVERT_DATA_RX,INVERT_DATA_RX};
      radio_data_i1[(WIDTH*2)-1:0] <= data_i1[(WIDTH*2)-1:0] ^ {INVERT_DATA_RX,INVERT_DATA_RX};
      radio_data_q1[(WIDTH*2)-1:0] <= data_q1[(WIDTH*2)-1:0] ^ {INVERT_DATA_RX,INVERT_DATA_RX};
      radio_aligned                <= aligned;
    end

  always @(posedge radio_clk)
    begin
      i0 <= radio_data_i0;
      q0 <= radio_data_q0;
      if (mimo) { i1, q1 } <= { radio_data_i1, radio_data_q1 };
      else { i1, q1 } <= { radio_data_i0, radio_data_q0 }; // dup single valid channel to both radios
      rx_aligned <= radio_aligned;
    end

  /*******************************************************************
   * Debug only logic below here.
   ******************************************************************/
/*-----\/----- EXCLUDED -----\/-----
  (* keep = "true", max_fanout = 10 *) reg [7:0] des_frame_reg;
  (* keep = "true", max_fanout = 10 *) reg rst_sdr_sync_reg;
  (* keep = "true", max_fanout = 10 *) reg run_sync_reg;
  (* keep = "true", max_fanout = 10 *) reg bitslip_reg;
  (* keep = "true", max_fanout = 10 *) reg aligned_reg;
  (* keep = "true", max_fanout = 10 *) reg [2:0] frame_state_reg;

  always @(posedge sdr_clk)
    begin
      des_frame_reg    <= des_frame;
      rst_sdr_sync_reg <= rst_sdr_sync;
      run_sync_reg     <= run_sync;
      bitslip_reg      <= bitslip;
      aligned_reg      <= aligned;
      frame_state_reg  <= frame_state;
    end

  ila64 ila64_i (
    .clk(sdr_clk),        // input clk
    .probe0(
      {
        des_frame_reg,
        rst_sdr_sync_reg,
        run_sync_reg,
        bitslip_reg,
        aligned_reg,
        frame_state_reg
      }
    )
  );
  -----/\----- EXCLUDED -----/\----- */

endmodule

