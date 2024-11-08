//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_post_processing
//
// Description:
//
//   This module contains the optional post-processing stages of an FFT,
//   including FFT output reordering, magnitude, and magnitude-squared
//   calculations.
//
//   For the magnitude, the result is clipped to a signed 16-bit result in the
//   range [0, 0x7FFF]. The result is placed in the real part of the sc16
//   output (the upper 16 bits) and the imaginary part (the lower 16 bits) is
//   set to 0.
//
//   For the magnitude squared, it computes (i^2 + q^2) / 0x8000, rounding and
//   clipping the result to a signed 16-bit value in the range [0, 0x7FFF]. The
//   division helps to avoid saturation and to put the result in a useful
//   range. The result is placed in the real part of the sc16 output (the upper
//   16 bits) and the imaginary part (the lower 16 bits) is set to 0.
//
//   Note that the I and Q may be swapped in the RFNoC transport adapter, so
//   this order will likely be reversed by the time it makes it back to a host
//   computer.
//
// Parameters:
//
//   EN_FFT_ORDER      : Set to 1 to add the optional FFT reorder core. Set to
//                       0 to remove it and save resources.
//   EN_CP_INSERTION   : Controls whether to include the cyclic prefix
//                       insertion logic, which is a subset of EN_FFT_ORDER.
//   EN_MAGNITUDE      : Set to 1 to add the magnitude output calculation core.
//                       Set to 0 to remove it and save resources.
//   EN_MAGNITUDE_SQ   : Set to 1 to add the magnitude squared output
//                       calculation core. Set to 0 to remove it and save
//                       resources.
//   USE_APPROX_MAG    : Controls which magnitude calculation to use. Set to 1
//                       to use a simpler circuit that gives pretty good
//                       results in order to save resources. Set to 0 to use
//                       the CORDIC IP to calculate the magnitude.
//   MAX_FFT_SIZE_LOG2 : Set to the log base 2 of the maximum FFT size to be
//                       supported. For example, a value of 14 means the
//                       maximum FFT size is 2**14 = 4096.
//
// Signals:
//
//   fft_order_sel : 0 - Normal (0 Hz in the center)
//                   1 - Reverse (same as normal but in reverse)
//                   2 - Natural (0 Hz on the left)
//   magnitude_sel : 0 - Normal complex output (No magnitude calculation)
//                   1 - Magnitude output
//                   2 - Magnitude-squared output
//   fft_size_log2 : Log base-2 of the FFT size. That is, the FFT size is
//                   exactly 2**fft_size_log2. The packet size must match.
//   s_axis_*      : AXI-Stream data input. s_axis_tuser contains the cyclic
//                   prefix length and must be valid on the first transfer of
//                   the packet.

//   m_axis_*      : AXI-Stream data output

`default_nettype none


module fft_post_processing #(
  bit EN_FFT_ORDER        = 1,
  bit EN_CP_INSERTION     = 1,
  bit EN_MAGNITUDE        = 1,
  bit EN_MAGNITUDE_SQ     = 1,
  bit USE_APPROX_MAG      = 1,
  int MAX_FFT_SIZE_LOG2   = 12,

  localparam int FFT_SIZE_LOG2_W = $clog2(MAX_FFT_SIZE_LOG2+1),
  localparam int CP_LEN_W = MAX_FFT_SIZE_LOG2
) (
  input wire clk,
  input wire rst,

  input wire [1:0] fft_order_sel,
  input wire [1:0] magnitude_sel,

  input wire [FFT_SIZE_LOG2_W-1:0] fft_size_log2,

  input  wire [        31:0] s_axis_tdata,
  input  wire [CP_LEN_W-1:0] s_axis_tuser,
  input  wire                s_axis_tlast,
  input  wire                s_axis_tvalid,
  output wire                s_axis_tready,

  output wire [31:0] m_axis_tdata,
  output wire        m_axis_tlast,
  output wire        m_axis_tvalid,
  input  wire        m_axis_tready
);

  //---------------------------------------------------------------------------
  // FFT Reorder
  //---------------------------------------------------------------------------

  import fft_reorder_pkg::*;

  wire [31:0] reorder_tdata;
  wire        reorder_tlast;
  wire        reorder_tvalid;
  wire        reorder_tready;

  if (EN_FFT_ORDER) begin : gen_fft_reorder
    logic [1:0]                 old_fft_order_sel;
    logic [FFT_SIZE_LOG2_W-1:0] old_fft_size_log2;
    logic                       fft_cfg_wr;

    // Update the FFT config whenever it changes
    always_ff @(posedge clk) begin
      fft_cfg_wr <= 0;
      if (
        (old_fft_order_sel != fft_order_sel) ||
        (old_fft_size_log2 != fft_size_log2)
      ) begin
        fft_cfg_wr <= 1;
      end
      old_fft_order_sel <= fft_order_sel;
      old_fft_size_log2 <= fft_size_log2;
    end

    fft_reorder #(
      .INPUT_ORDER     (BIT_REVERSE),
      .MAX_FFT_LEN_LOG2(MAX_FFT_SIZE_LOG2),
      .DATA_W          (32),
      .EN_CP_INSERTION (EN_CP_INSERTION)
    ) fft_reorder_i (
      .clk          (clk),
      .rst          (rst),
      .fft_cfg_wr   (fft_cfg_wr),
      .fft_len_log2 (fft_size_log2),
      .fft_out_order(fft_order_t'(fft_order_sel)),
      .i_tdata      (s_axis_tdata),
      .i_tuser      (s_axis_tuser),
      .i_tlast      (s_axis_tlast),
      .i_tvalid     (s_axis_tvalid),
      .i_tready     (s_axis_tready),
      .o_tdata      (reorder_tdata),
      .o_tlast      (reorder_tlast),
      .o_tvalid     (reorder_tvalid),
      .o_tready     (reorder_tready)
    );
  end else begin : gen_no_fft_reorder
    // Pass the data directly through when reordering is disabled.
    assign reorder_tdata  = s_axis_tdata;
    assign reorder_tlast  = s_axis_tlast;
    assign reorder_tvalid = s_axis_tvalid;
    assign s_axis_tready  = reorder_tready;
  end

  //---------------------------------------------------------------------------
  // Demultiplex Magnitude Options
  //---------------------------------------------------------------------------

  wire [31:0] mag_bypass_tdata;
  wire        mag_bypass_tlast;
  wire        mag_bypass_tvalid;
  wire        mag_bypass_tready;

  wire [31:0] mag_in_tdata;
  wire        mag_in_tlast;
  wire        mag_in_tvalid;
  wire        mag_in_tready;

  wire [31:0] mag_sq_in_tdata;
  wire        mag_sq_in_tlast;
  wire        mag_sq_in_tvalid;
  wire        mag_sq_in_tready;

  if (EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin : gen_mag_demux
    axi_demux #(
      .WIDTH         (32),
      .SIZE          (3),
      .PRE_FIFO_SIZE (0),
      .POST_FIFO_SIZE(0)
    ) axi_demux_i (
      .clk     (clk),
      .reset   (rst),
      .clear   (1'b0),
      .header  (),
      .dest    (magnitude_sel),
      .i_tdata (reorder_tdata),
      .i_tlast (reorder_tlast),
      .i_tvalid(reorder_tvalid),
      .i_tready(reorder_tready),
      .o_tdata ({mag_sq_in_tdata , mag_in_tdata , mag_bypass_tdata }),
      .o_tlast ({mag_sq_in_tlast , mag_in_tlast , mag_bypass_tlast }),
      .o_tvalid({mag_sq_in_tvalid, mag_in_tvalid, mag_bypass_tvalid}),
      .o_tready({mag_sq_in_tready, mag_in_tready, mag_bypass_tready})
    );
  end

  //---------------------------------------------------------------------------
  // Magnitude
  //---------------------------------------------------------------------------

  wire [31:0] mag_out_tdata;
  wire        mag_out_tlast;
  wire        mag_out_tvalid;
  wire        mag_out_tready;

  if (EN_MAGNITUDE) begin : gen_magnitude
    wire [16:0] round_in_tdata;
    wire [31:0] round_in_tdata_tmp;
    wire        round_in_tlast;
    wire        round_in_tvalid;
    wire        round_in_tready;

    wire [15:0] mag_out_tdata_tmp;

    if (!USE_APPROX_MAG) begin : gen_cordic
      wire [47:0] m_axis_dout_tdata;
      // The CORDIC IP below inputs/outputs its data as signed numbers having 2
      // whole bits and 15 fractional bits (17 total bits). To be compliant
      // with AXI, each value is stuffed into a 24-bit vector. On the input, we
      // resize our sc16 inputs to be 24 bits (the upper 7 bits will be ignored
      // by the IP). On the output side, we only need the magnitude, which is
      // in the lower 17 bits. The phase, in the upper bits, is left unused.
      complex_to_magphase_int17 complex_to_magphase_int17_i (
        .aclk                   (clk),
        .aresetn                (~rst),
        .s_axis_cartesian_tvalid(mag_in_tvalid),
        .s_axis_cartesian_tlast (mag_in_tlast),
        .s_axis_cartesian_tready(mag_in_tready),
        .s_axis_cartesian_tdata ({ 24'(signed'(mag_in_tdata[31:16])),
                                   24'(signed'(mag_in_tdata[15:0])) }),
        .m_axis_dout_tvalid     (round_in_tvalid),
        .m_axis_dout_tlast      (round_in_tlast),
        .m_axis_dout_tdata      (m_axis_dout_tdata),
        .m_axis_dout_tready     (round_in_tready)
      );
      assign round_in_tdata_tmp = 32'(m_axis_dout_tdata[16:0]);
    end else if (USE_APPROX_MAG) begin : gen_approx
      complex_to_mag_approx complex_to_mag_approx_i (
        .clk     (clk),
        .reset   (rst),
        .clear   (1'b0),
        .i_tvalid(mag_in_tvalid),
        .i_tlast (mag_in_tlast),
        .i_tready(mag_in_tready),
        .i_tdata (mag_in_tdata),
        .o_tvalid(round_in_tvalid),
        .o_tlast (round_in_tlast),
        .o_tready(round_in_tready),
        .o_tdata (round_in_tdata_tmp[15:0])
      );
      assign round_in_tdata_tmp[31:16] = '0;
    end

    // The magnitude is always positive, so we set the MSB to 0 then clip the
    // result to a signed 16-bit value.
    assign round_in_tdata = {1'b0, round_in_tdata_tmp[15:0]};

    axi_round_and_clip #(
      .WIDTH_IN (17),
      .WIDTH_OUT(16),
      .CLIP_BITS(1)
    ) axi_round_and_clip_i (
      .clk     (clk),
      .reset   (rst),
      .i_tdata (round_in_tdata),
      .i_tlast (round_in_tlast),
      .i_tvalid(round_in_tvalid),
      .i_tready(round_in_tready),
      .o_tdata (mag_out_tdata_tmp),
      .o_tlast (mag_out_tlast),
      .o_tvalid(mag_out_tvalid),
      .o_tready(mag_out_tready)
    );

    // Put the resulting magnitude in the "real" part of the output
    assign mag_out_tdata = {mag_out_tdata_tmp, 16'd0};

  end else begin : gen_no_magnitude
    assign mag_out_tdata  = '0;
    assign mag_out_tlast  = '0;
    assign mag_out_tvalid = '0;
    assign mag_in_tready  = '1;
  end

  //---------------------------------------------------------------------------
  // Magnitude Squared
  //---------------------------------------------------------------------------

  wire [31:0] mag_sq_out_tdata;
  wire        mag_sq_out_tlast;
  wire        mag_sq_out_tvalid;
  wire        mag_sq_out_tready;

  if (EN_MAGNITUDE_SQ) begin : gen_magnitude_squared
    wire [31:0] round_in_tdata;
    wire        round_in_tlast;
    wire        round_in_tvalid;
    wire        round_in_tready;

    wire [15:0] mag_sq_out_tdata_tmp;

    complex_to_magsq complex_to_magsq_i (
      .clk     (clk),
      .reset   (rst),
      .clear   (1'b0),
      .i_tvalid(mag_sq_in_tvalid),
      .i_tlast (mag_sq_in_tlast),
      .i_tready(mag_sq_in_tready),
      .i_tdata (mag_sq_in_tdata),
      .o_tvalid(round_in_tvalid),
      .o_tlast (round_in_tlast),
      .o_tready(round_in_tready),
      .o_tdata (round_in_tdata)
    );

    axi_round_and_clip #(
      .WIDTH_IN (32),
      .WIDTH_OUT(16),
      .CLIP_BITS(1)
    ) axi_round_and_clip_i (
      .clk     (clk),
      .reset   (rst),
      .i_tdata (round_in_tdata),
      .i_tlast (round_in_tlast),
      .i_tvalid(round_in_tvalid),
      .i_tready(round_in_tready),
      .o_tdata (mag_sq_out_tdata_tmp),
      .o_tlast (mag_sq_out_tlast),
      .o_tvalid(mag_sq_out_tvalid),
      .o_tready(mag_sq_out_tready)
    );

    assign mag_sq_out_tdata = {mag_sq_out_tdata_tmp, 16'd0};

  end else begin : gen_no_magnitude_squared
    assign mag_sq_out_tdata  = '0;
    assign mag_sq_out_tlast  = '0;
    assign mag_sq_out_tvalid = '0;
    assign mag_sq_in_tready  = '1;
  end

  //---------------------------------------------------------------------------
  // Combine Magnitude Options
  //---------------------------------------------------------------------------

  if (EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin : gen_mag_mux
    axi_mux #(
      .PRIO          (1),
      .WIDTH         (32),
      .SIZE          (3),
      .PRE_FIFO_SIZE (0),
      .POST_FIFO_SIZE(1)
    ) axi_demux_i (
      .clk     (clk),
      .reset   (rst),
      .clear   (1'b0),
      .i_tdata ({mag_sq_out_tdata , mag_out_tdata , mag_bypass_tdata }),
      .i_tlast ({mag_sq_out_tlast , mag_out_tlast , mag_bypass_tlast }),
      .i_tvalid({mag_sq_out_tvalid, mag_out_tvalid, mag_bypass_tvalid}),
      .i_tready({mag_sq_out_tready, mag_out_tready, mag_bypass_tready}),
      .o_tdata (m_axis_tdata),
      .o_tlast (m_axis_tlast),
      .o_tvalid(m_axis_tvalid),
      .o_tready(m_axis_tready)
    );
  end else begin : gen_no_mag_mux
    assign m_axis_tdata   = reorder_tdata;
    assign m_axis_tlast   = reorder_tlast;
    assign m_axis_tvalid  = reorder_tvalid;
    assign reorder_tready = m_axis_tready;
  end


endmodule : fft_post_processing


`default_nettype wire
