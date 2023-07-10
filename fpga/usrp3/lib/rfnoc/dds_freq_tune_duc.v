//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_freq_tune_duc
//
// Description:
//
//   Performs a frequency shift on a signal by multiplying it with a complex
//   sinusoid synthesized from a DDS. This module expects samples data to be in
//   {Q,I} order.
//
//   The din input is expected to contain a complex 24-bit signed fixed-point
//   values with 15 fractional bits. The phase input is expected to contain
//   unsigned 24-bit fixed-point with 24 fractional bits, and therefore
//   represents the range [0,1), which corresponds to the range [0,2π) radians.
//   The output will then be a complex 24-bit signed fixed-point with 15
//   fractional bits.
//
//   This version does the same thing as dds_freq_tune, but does not
//   reset/flush the DDS between packets or when an EOB occurs, and it includes
//   a FIFO on the din data path. This separate version was created to avoid
//   affecting the behavior of the DDC.
//
//               ┌───┐
//      phase >──┤DDS├──┐ ┌───────┐
//               └───┘  └─┤Complex│  ┌─────┐
//                        │  Mult ├──┤Round├───> dout
//               ┌────┐ ┌─┤       │  └─────┘
//        din >──┤FIFO├─┘ └───────┘
//               └────┘
//
// Parameters:
//
//   Note: The parameters should NOT be changed, since they depend on the IP
//   configurations.
//
//   INPUT_W  : Width of each component of din.
//   PHASE_W  : Width of the phase input.
//   OUTPUT_W : Width of each component of dout.
//

`default_nettype none


module dds_freq_tune_duc #(
  parameter INPUT_W  = 24,
  parameter PHASE_W  = 24,
  parameter OUTPUT_W = 24
) (
  input wire clk,
  input wire reset,

  // IQ input (Q in the upper, I in the lower bits)
  input  wire [INPUT_W*2-1:0] s_axis_din_tdata,
  input  wire                 s_axis_din_tlast,
  input  wire                 s_axis_din_tvalid,
  output wire                 s_axis_din_tready,

  // Phase input from NCO
  input  wire [PHASE_W-1:0] s_axis_phase_tdata,
  input  wire               s_axis_phase_tlast,
  input  wire               s_axis_phase_tvalid,
  output wire               s_axis_phase_tready,

  // IQ output (Q in the upper, I in the lower bits)
  output wire [OUTPUT_W*2-1:0] m_axis_dout_tdata,
  output wire                  m_axis_dout_tlast,
  output wire                  m_axis_dout_tvalid,
  input  wire                  m_axis_dout_tready
);

  //---------------------------------------------------------------------------
  // Reset Generation
  //---------------------------------------------------------------------------

  reg reset_d1, reset_int;

  // Create a local reset, named reset_int, which will always be asserted for
  // at least 2 clock cycles, which is required by Xilinx DDS and complex
  // multiplier IP.
  always @(posedge clk) begin
    reset_d1  <= reset;
    reset_int <= reset | reset_d1;
  end


  //---------------------------------------------------------------------------
  // Data Input FIFO
  //---------------------------------------------------------------------------
  //
  // We want the din and phase inputs paths to be balanced, so that a new
  // data/phase pair can be input on each clock cycles. This FIFO allows the
  // din data path to queue up samples while the DDS is processing.
  //
  //---------------------------------------------------------------------------

  wire [INPUT_W*2-1:0] s_axis_fifo_tdata;
  wire                 s_axis_fifo_tlast;
  wire                 s_axis_fifo_tvalid;
  wire                 s_axis_fifo_tready;

  axi_fifo #(
    .WIDTH (2*INPUT_W+1),
    .SIZE  (5)
  ) axi_fifo_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({ s_axis_din_tlast, s_axis_din_tdata }),
    .i_tvalid (s_axis_din_tvalid),
    .i_tready (s_axis_din_tready),
    .o_tdata  ({ s_axis_fifo_tlast, s_axis_fifo_tdata }),
    .o_tvalid (s_axis_fifo_tvalid),
    .o_tready (s_axis_fifo_tready),
    .space    (),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // DDS/NCO
  //---------------------------------------------------------------------------

  // Width of each component of the DDS output. This width is fixed by the IP
  // configuration.
  localparam DDS_W = 16;

  wire               m_axis_dds_tlast;
  wire               m_axis_dds_tvalid;
  wire               m_axis_dds_tready;
  wire [DDS_W*2-1:0] m_axis_dds_tdata;

  // DDS to convert the phase input to a unit-length complex number with that
  // phase. It takes in an unsigned 24-bit phase with 24 fractional bits and
  // outputs two signed 16-bit fixed point values with 14 fractional bits. The
  // output has sin(2*pi*phase) in the upper 16 bits and cos(2*pi*phase) in the
  // lower 16-bits.
  dds_wrapper dds_wrapper_i (
    .clk                 (clk),
    .rst                 (reset_int),
    .s_axis_phase_tdata  (s_axis_phase_tdata),
    .s_axis_phase_tvalid (s_axis_phase_tvalid),
    .s_axis_phase_tlast  (s_axis_phase_tlast),
    .s_axis_phase_tready (s_axis_phase_tready),
    .m_axis_data_tdata   (m_axis_dds_tdata),
    .m_axis_data_tvalid  (m_axis_dds_tvalid),
    .m_axis_data_tlast   (m_axis_dds_tlast),
    .m_axis_data_tready  (m_axis_dds_tready)
  );


  //---------------------------------------------------------------------------
  // Complex Multiplier
  //---------------------------------------------------------------------------
  //
  // Use a complex multiplier to multiply the DDS complex sinusoid by the input
  // data samples.
  //
  //---------------------------------------------------------------------------

  // Width of each component on the output of the complex_multiplier_dds IP.
  // This width is fixed by the IP configuration.
  localparam MULT_OUT_W = 32;

  // Width is set by the IP
  wire [2*MULT_OUT_W-1:0] mult_out_tdata;
  wire                    mult_out_tvalid;
  wire                    mult_out_tready;
  wire                    mult_out_tlast;

  // The complex multiplier IP is configured so that the A input is 21 bits
  // with 15 fractional bits, and the B input (dds) is 16 bits with 14
  // fractional bits. Due to AXI-Stream requirements, A is rounded up to 24
  // bits in width. The full multiplier output result would be 21+16+1 = 38
  // bits, but the output is configured for 32, dropping the lower 6 bits.
  // Therefore, the result has 15+14-6 = 23 fractional bits.
  //
  // The IP is configured to pass the TLAST from port A through, but we connect
  // the B path anyway for completeness.
  complex_multiplier_dds complex_multiplier_dds_i (
    .aclk               (clk),
    .aresetn            (~reset_int),
    .s_axis_a_tvalid    (s_axis_fifo_tvalid),
    .s_axis_a_tready    (s_axis_fifo_tready),
    .s_axis_a_tlast     (s_axis_fifo_tlast),
    .s_axis_a_tdata     (s_axis_fifo_tdata),
    .s_axis_b_tvalid    (m_axis_dds_tvalid),
    .s_axis_b_tready    (m_axis_dds_tready),
    .s_axis_b_tlast     (m_axis_dds_tlast),
    .s_axis_b_tdata     (m_axis_dds_tdata),
    .m_axis_dout_tvalid (mult_out_tvalid),
    .m_axis_dout_tready (mult_out_tready),
    .m_axis_dout_tlast  (mult_out_tlast),
    .m_axis_dout_tdata  (mult_out_tdata)
  );


  //---------------------------------------------------------------------------
  // Round
  //---------------------------------------------------------------------------
  //
  // Round the 32-bit multiplier result down to 24 bits. This moves the binary
  // point so that we go from 23 fractional bits down to 15 fractional bits.
  //
  //---------------------------------------------------------------------------

  axi_round_complex #(
    .WIDTH_IN  (MULT_OUT_W),
    .WIDTH_OUT (OUTPUT_W)
  ) axi_round_complex_i (
    .clk      (clk),
    .reset    (reset_int),
    .i_tdata  (mult_out_tdata),
    .i_tlast  (mult_out_tlast),
    .i_tvalid (mult_out_tvalid),
    .i_tready (mult_out_tready),
    .o_tdata  (m_axis_dout_tdata),
    .o_tlast  (m_axis_dout_tlast),
    .o_tvalid (m_axis_dout_tvalid),
    .o_tready (m_axis_dout_tready)
  );

endmodule


`default_nettype wire
