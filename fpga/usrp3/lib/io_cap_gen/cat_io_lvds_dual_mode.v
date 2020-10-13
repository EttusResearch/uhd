//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_io_lvds_dual_mode
//
// Description:
//
// This is an LVDS interface for the AD9361 (Catalina). It uses the cat_io_lvds
// module to implement the interface, but supports both 1R1T and 2R2T timing
// modes while using full LVDS bandwidth. That is, it can support 1R1T at twice
// the sample rate of 2R2T.
//
// This is controlled by the a_mimo control signal. When MIMO = 0 (1R1T mode),
// the radio_clk frequency equals that of rx_clk/2 and the data is output to
// both radio channels. If MIMO = 1 (2R2T), the frequency of radio_clk equals
// rx_clk/4 and the data stream is split between channel 0 and channel 1. This is used
// for 2R2T mode.
//

module cat_io_lvds_dual_mode #(
  parameter INVERT_FRAME_RX    = 0,
  parameter INVERT_DATA_RX     = 6'b00_0000,
  parameter INVERT_FRAME_TX    = 0,
  parameter INVERT_DATA_TX     = 6'b00_0000,
  parameter USE_CLOCK_IDELAY   = 1,
  parameter USE_DATA_IDELAY    = 1,
  parameter DATA_IDELAY_MODE   = "FIXED",
  parameter CLOCK_IDELAY_MODE  = "FIXED",
  parameter INPUT_CLOCK_DELAY  = 16,
  parameter INPUT_DATA_DELAY   = 0,
  parameter USE_CLOCK_ODELAY   = 0,
  parameter USE_DATA_ODELAY    = 0,
  parameter DATA_ODELAY_MODE   = "FIXED",
  parameter CLOCK_ODELAY_MODE  = "FIXED",
  parameter OUTPUT_CLOCK_DELAY = 16,
  parameter OUTPUT_DATA_DELAY  = 0
) (
  input clk200,

  // Data and frame timing (asynchronous, glitch free)
  input a_mimo,     // MIMO vs. SISO mode
  input a_tx_ch,    // Which channel to transmit when MIMO=0

  // Delay Control Interface
  input       ctrl_clk,
  input [4:0] ctrl_in_data_delay,
  input [4:0] ctrl_in_clk_delay,
  input       ctrl_ld_in_data_delay,
  input       ctrl_ld_in_clk_delay,
  input [4:0] ctrl_out_data_delay,
  input [4:0] ctrl_out_clk_delay,
  input       ctrl_ld_out_data_delay,
  input       ctrl_ld_out_clk_delay,

  // Baseband sample interface
  input  radio_rst,           // Glitch-free, synchronous to radio_clk
  output radio_clk,
  //
  output reg    rx_aligned,
  output [11:0] rx_i0,
  output [11:0] rx_q0,
  output [11:0] rx_i1,
  output [11:0] rx_q1,
  //
  input  [11:0] tx_i0,
  input  [11:0] tx_q0,
  input  [11:0] tx_i1,
  input  [11:0] tx_q1,

  // Catalina LVDS interface
  input        rx_clk_p,
  input        rx_clk_n,
  input        rx_frame_p,
  input        rx_frame_n,
  input  [5:0] rx_d_p,
  input  [5:0] rx_d_n,
  //
  output       tx_clk_p,
  output       tx_clk_n,
  output       tx_frame_p,
  output       tx_frame_n,
  output [5:0] tx_d_p,
  output [5:0] tx_d_n
);

  wire radio_clk_1x;    // rx_clk_p divided by 4
  wire radio_clk_2x;    // rx_clk_p divided by 2


  //---------------------------------------------------------------------------
  // Mode Selection
  //---------------------------------------------------------------------------

  wire r_mimo;
  wire r_tx_ch;

  // Double synchronize the MIMO signal
  synchronizer mimo_sync (
    .clk(radio_clk_1x),
    .rst(1'b0),
    .in(a_mimo),
    .out(r_mimo));

  // Double synchronize the Tx channel signal
  synchronizer tx_ch_sync (
    .clk(radio_clk_1x),
    .rst(1'b0),
    .in(a_tx_ch),
    .out(r_tx_ch));


  //---------------------------------------------------------------------------
  // Clock Mux
  //---------------------------------------------------------------------------

  // Select the source for radio_clk. Use radio_clk_1x when MIMO = 1 or
  // radio_clk_2x when MIMO = 0.
  BUFGCTRL BUFGCTRL_radio_clk (
    .I0      (radio_clk_1x),
    .I1      (radio_clk_2x),
    .S0      (r_mimo),
    .S1      (~r_mimo),
    .CE0     (1),
    .CE1     (1),
    .O       (radio_clk),
    .IGNORE0 (0),
    .IGNORE1 (0)
  );


  //---------------------------------------------------------------------------
  // Generate Alignment Strobes
  //---------------------------------------------------------------------------
  //
  // The LVDS input logic generates the following two clocks:
  //
  //   radio_clk_1x   |‾‾‾‾‾|_____|‾‾‾‾‾|_____|‾‾‾‾‾|_____|‾‾‾‾‾|_____|‾‾‾‾‾|
  //
  //   radio_clk_2x   |‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|‾‾|__|
  //
  //
  // Using simple logic, we create the following two signals from these clocks:
  //
  //   align_1x       |‾‾‾‾‾‾‾‾‾‾‾|___________|‾‾‾‾‾‾‾‾‾‾‾|___________|‾‾‾‾‾‾
  //
  //   align_2x       ______|‾‾‾‾‾‾‾‾‾‾‾|___________|‾‾‾‾‾‾‾‾‾‾‾|___________|
  //
  // These two alignment signals allow us to tell where in the frame period we
  // are so that we can deserialize in the correct order.
  //
  //---------------------------------------------------------------------------

  reg align_1x = 0;
  reg align_2x = 0;

  always @(posedge radio_clk_1x)
  begin
    align_1x <= ~align_1x;
  end

  always @(posedge radio_clk_2x)
  begin
    // Align data capture to 1x clock so that we stay in sync with data.
    // Otherwise, the data might be serialized in the wrong order.
    align_2x <= align_1x;
  end


  //---------------------------------------------------------------------------
  // Rx MIMO/SISO Serialization
  //---------------------------------------------------------------------------
  //
  // This block of code takes the dual outputs when in SISO mode and serializes
  // them. Because we use the 2x clock when in SISO mode, this allows us to
  // double the data rate when using a single channel.
  //
  //---------------------------------------------------------------------------

  wire rx_aligned_t;
  reg rx_aligned_reg;

  wire [11:0] rx_i0_t;
  wire [11:0] rx_q0_t;
  wire [11:0] rx_i1_t;
  wire [11:0] rx_q1_t;

  reg [11:0] rx_i0_ser;
  reg [11:0] rx_q0_ser;
  reg [11:0] rx_i1_ser;
  reg [11:0] rx_q1_ser;

  reg [11:0] rx_i0_out;
  reg [11:0] rx_q0_out;
  reg [11:0] rx_i1_out;
  reg [11:0] rx_q1_out;

  reg rx_out_val;

  always @(posedge radio_clk_2x)
  begin
    rx_aligned_reg <= rx_aligned_t;

    if (align_1x ^ align_2x) begin
      // This clock cycle corresponds to the first 1x cycle in which two
      // samples are output, so grab data from port 0.
      rx_i0_ser <= rx_i0_t;
      rx_q0_ser <= rx_q0_t;
      rx_i1_ser <= rx_i0_t;
      rx_q1_ser <= rx_q0_t;
    end else begin
      // This radio_clk_2x cycle corresponds to the second 1x cycle in which
      // two samples are output, so grab data from port 1.
      rx_i0_ser <= rx_i1_t;
      rx_q0_ser <= rx_q1_t;
      rx_i1_ser <= rx_i1_t;
      rx_q1_ser <= rx_q1_t;
    end

    // Select the correct Rx output based on MIMO setting
    if (r_mimo) begin
      // In MIMO mode, we get new data for both channels every other
      // radio_clk_2x clock cycle.
      rx_out_val <= ~rx_out_val;
      rx_i0_out <= rx_i0_t;
      rx_q0_out <= rx_q0_t;
      rx_i1_out <= rx_i1_t;
      rx_q1_out <= rx_q1_t;
    end else begin
      // In SISO mode, we get new data for one channel on every radio_clk_2x
      // cock cycle.
      rx_out_val <= 1'b1;
      rx_i0_out <= rx_i0_ser;
      rx_q0_out <= rx_q0_ser;
      rx_i1_out <= rx_i1_ser;
      rx_q1_out <= rx_q1_ser;
    end
  end


  //---------------------------------------------------------------------------
  // Cross RX Data from radio_clk_2x to radio_clk Domain
  //---------------------------------------------------------------------------
  //
  //  The clocks are synchronous and the data input rate matches the data
  //  output rate, so this FIFO should never overflow or underflow once it is
  //  primed and starts being read.
  //
  //---------------------------------------------------------------------------

  wire rx_fifo_full;
  wire rx_fifo_empty;
  reg  rx_fifo_rd_en;

  fifo_short_2clk fifo_short_2clk_rx (
    .rst           (radio_rst),         // Asynchronous reset input
    .wr_clk        (radio_clk_2x),
    .rd_clk        (radio_clk),
    .din           ({rx_i1_out, rx_q1_out, rx_i0_out, rx_q0_out}),
    .wr_en         (rx_out_val & ~rx_fifo_full & rx_aligned_reg),
    .rd_en         (rx_fifo_rd_en & ~rx_fifo_empty),
    .dout          ({rx_i1, rx_q1, rx_i0, rx_q0 }),
    .full          (rx_fifo_full),
    .empty         (rx_fifo_empty),
    .rd_data_count (),
    .wr_data_count ()
  );

  // Wait until the FIFO is partially filled before we start reading out data.
  // Go back to waiting if the FIFO empties.
  always @(posedge radio_clk) begin
    if (radio_rst) begin
      rx_fifo_rd_en <= 1'b0;
      rx_aligned    <= 1'b0;
    end else begin
      if (!rx_fifo_empty) begin
        rx_fifo_rd_en <= 1'b1;
        rx_aligned    <= 1'b1;
      end else if (rx_fifo_empty) begin
        rx_fifo_rd_en <= 1'b0;
        rx_aligned    <= 1'b0;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Cross TX Data from radio_clk domain to radio_clk_2x Domain
  //---------------------------------------------------------------------------
  //
  //  The clocks are synchronous and the data input rate matches the data
  //  output rate, so this FIFO should never overflow or underflow once it is
  //  primed and starts being read.
  //
  //---------------------------------------------------------------------------

  // Cross the radio_rst to radio_clk_2x
  synchronizer #(
    .INITIAL_VAL (1'b1)
  ) synchronizer_radio_rst_2x (
    .clk (radio_clk_2x),
    .rst (1'b0),
    .in  (radio_rst),
    .out (radio_rst_2x)
  );

  wire [11:0] tx_i0_del0;
  wire [11:0] tx_q0_del0;
  wire [11:0] tx_i1_del0;
  wire [11:0] tx_q1_del0;

  wire tx_fifo_full;
  wire tx_fifo_empty;
  reg  tx_fifo_rd_en;

  fifo_short_2clk fifo_short_2clk_tx (
    .rst           (radio_rst),         // Asynchronous reset input
    .wr_clk        (radio_clk),
    .rd_clk        (radio_clk_2x),
    .din           ({tx_i1, tx_q1, tx_i0, tx_q0}),
    .wr_en         (~tx_fifo_full),
    .rd_en         (tx_fifo_rd_en & ~tx_fifo_empty),
    .dout          ({tx_i1_del0, tx_q1_del0, tx_i0_del0, tx_q0_del0}),
    .full          (tx_fifo_full),
    .empty         (tx_fifo_empty),
    .rd_data_count (),
    .wr_data_count ()
  );

  // Wait until the FIFO is partially filled before we start reading out data.
  // Go back to waiting if the FIFO empties.
  always @(posedge radio_clk_2x) begin
    if (radio_rst_2x) begin
      tx_fifo_rd_en <= 1'b0;
    end else begin
      if (!tx_fifo_empty) begin
        tx_fifo_rd_en <= 1'b1;
      end else if (tx_fifo_empty) begin
        tx_fifo_rd_en <= 1'b0;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Tx MIMO/SISO Deserialization
  //---------------------------------------------------------------------------
  //
  // This block of code takes the serialized output from the radios and
  // parallelizes it onto the two radio ports of the Catalina interface. It
  // also takes the radio data, output on the radio_clk domain, and crosses it
  // to the radio_clk_1x domain.
  //
  //---------------------------------------------------------------------------

  reg [11:0] tx_i0_del1;
  reg [11:0] tx_q0_del1;
  reg [11:0] tx_i1_del1;
  reg [11:0] tx_q1_del1;

  reg [11:0] tx_i0_t;
  reg [11:0] tx_q0_t;
  reg [11:0] tx_i1_t;
  reg [11:0] tx_q1_t;

  always @(posedge radio_clk_2x)
  begin
    // Capture copy of the data delayed by one radio_clk_2x cycle.
    tx_i0_del1 <= tx_i0_del0;
    tx_q0_del1 <= tx_q0_del0;
    tx_i1_del1 <= tx_i1_del0;
    tx_q1_del1 <= tx_q1_del0;
  end

  always @(posedge radio_clk_1x)
  begin
    if (r_mimo) begin
      // In MIMO mode, radio_clk is radio_clk_1x, so we just capture the same
      // data for each radio_clk_1x cycle.
      tx_i0_t <= tx_i0_del0;
      tx_q0_t <= tx_q0_del0;
      tx_i1_t <= tx_i1_del0;
      tx_q1_t <= tx_q1_del0;
    end else begin
      // In SISO mode, data is updated every radio_clk_2x cycle, so we output
      // the data from the previous radio_clk_2x cycle onto channel 0 and the
      // data from the current radio_clk_2x cycle onto channel 1. This puts the
      // data in the correct order when in 1R1T mode.
      if (r_tx_ch == 0) begin
        tx_i0_t <= tx_i0_del1;
        tx_q0_t <= tx_q0_del1;
        tx_i1_t <= tx_i0_del0;
        tx_q1_t <= tx_q0_del0;
      end else begin
        tx_i0_t <= tx_i1_del1;
        tx_q0_t <= tx_q1_del1;
        tx_i1_t <= tx_i1_del0;
        tx_q1_t <= tx_q1_del0;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Catalina TX/RX Interface
  //---------------------------------------------------------------------------

  cat_io_lvds #(
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
    .OUTPUT_DATA_DELAY  (OUTPUT_DATA_DELAY),
    .USE_BUFG           (0)
  ) cat_io_lvds_i0 (
    .rst    (radio_rst),
    .clk200 (clk200),

    // Data and frame timing
    .mimo         (1),       // Set to 1 to always return all samples
    .frame_sample (~r_mimo), // Frame timing corresponds to SISO/MIMO setting

    // Delay control interface
    .ctrl_clk               (ctrl_clk),
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
    .radio_clk    (radio_clk_1x),
    .radio_clk_2x (radio_clk_2x),
    .rx_aligned   (rx_aligned_t),
    //
    .rx_i0        (rx_i0_t),
    .rx_q0        (rx_q0_t),
    .rx_i1        (rx_i1_t),
    .rx_q1        (rx_q1_t),
    //
    .tx_i0        (tx_i0_t),
    .tx_q0        (tx_q0_t),
    .tx_i1        (tx_i1_t),
    .tx_q1        (tx_q1_t),

    // Catalina interface
    .rx_clk_p   (rx_clk_p),
    .rx_clk_n   (rx_clk_n),
    .rx_frame_p (rx_frame_p),
    .rx_frame_n (rx_frame_n),
    .rx_d_p     (rx_d_p),
    .rx_d_n     (rx_d_n),
    //
    .tx_clk_p   (tx_clk_p),
    .tx_clk_n   (tx_clk_n),
    .tx_frame_p (tx_frame_p),
    .tx_frame_n (tx_frame_n),
    .tx_d_p     (tx_d_p),
    .tx_d_n     (tx_d_n)
  );

endmodule  // cat_io_lvds_dual_mode
