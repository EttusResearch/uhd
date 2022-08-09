//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_wrapper
//
// Description:
//
//   This module computes the complex number e^(j*2*pi*phase). The phase input
//   can be thought of as a 24-bit unsigned fixed-point value with 24
//   fractional bits. In other words, the integer range of the input maps to a
//   phase in the range [0, 1.0). The output consists of two 16-bit signed
//   fixed-point values with 14 fractional bits. The value sin(2*pi*phase) is
//   in the upper 16 bits and cos(2*pi*phase) is in the lower 16-bits. This
//   puts the output in {Q,I} order.
//
//   This is a wrapper for the dds_sin_cos_lut_only IP, which is based on the
//   Xilinx DDS Compiler. This IP has the undesirable behavior that input must
//   be provided to flush out any data stuck in its pipeline. This wrapper
//   hides that behavior so that every input causes a corresponding output,
//   even if the input stops.
//
//   NOTE: The DDS IP requires at least 2 cycles of reset.
//
// Parameters:
//
//   The parameters in this module should not be modified. They match the IP
//   configuration.
//


module dds_wrapper #(
  parameter PHASE_W  = 24,
  parameter OUTPUT_W = 32
) (
  input  wire                clk,
  input  wire                rst,

  // Phase input
  input  wire [ PHASE_W-1:0] s_axis_phase_tdata,
  input  wire                s_axis_phase_tvalid,
  input  wire                s_axis_phase_tlast,
  output wire                s_axis_phase_tready,

  // IQ output (Q in the upper, I in the lower bits)
  output wire [OUTPUT_W-1:0] m_axis_data_tdata,
  output wire                m_axis_data_tvalid,
  output wire                m_axis_data_tlast,
  input  wire                m_axis_data_tready
);

  // Width of number needed to represent the DDS fullness. This value was
  // determined experimentally. The max fullness was 33.
  localparam FULLNESS_W = 6;

  wire [PHASE_W-1:0] phase_tdata;
  wire               phase_tvalid;
  wire               phase_tlast;
  wire               phase_tready;

  wire [OUTPUT_W-1:0] dds_tdata;
  wire                dds_tvalid;
  wire                dds_tlast;
  wire                dds_tready;


  //---------------------------------------------------------------------------
  // DDS Fullness Counter
  //---------------------------------------------------------------------------
  //
  // Count the number of valid samples in the DDS's data pipeline.
  //
  //---------------------------------------------------------------------------

  // The fullness counter must be large enough for DDS's latency.
  reg [FULLNESS_W-1:0] fullness     = 0;
  reg                  dds_has_data = 0;

  wire increment = s_axis_phase_tvalid & s_axis_phase_tready;
  wire decrement = m_axis_data_tvalid  & m_axis_data_tready;

  always @(posedge clk) begin
    if (rst) begin
      fullness     <= 0;
      dds_has_data <= 0;
    end else begin
      if (increment && !decrement) begin
        //synthesis translate_off
        if (fullness+1'b1 == 1'b0) begin
          $display("ERROR: Fullness overflowed!");
        end
        //synthesis translate_on
        fullness <= fullness + 1;
        dds_has_data <= 1;
      end else if (decrement && !increment) begin
        //synthesis translate_off
        if (fullness-1'b1 > fullness) begin
          $display("ERROR: Fullness underflowed!");
        end
        //synthesis translate_on
        fullness <= fullness - 1;
        dds_has_data <= (fullness > 1);
      end else begin
        dds_has_data <= (fullness > 0);
      end
    end
  end


  //---------------------------------------------------------------------------
  // Input Logic
  //---------------------------------------------------------------------------

  assign s_axis_phase_tready = phase_tready;
  assign phase_tlast         = s_axis_phase_tlast;
  assign phase_tdata         = s_axis_phase_tdata;

  // Always input something when the DDS has data stuck inside it so that all
  // data gets flushed out automatically.
  assign phase_tvalid = s_axis_phase_tvalid || dds_has_data;


  //---------------------------------------------------------------------------
  // DDS IP
  //---------------------------------------------------------------------------

  // Use the TUSER path on the DDS IP to indicate if the sample is empty and is
  // just to flush the output.
  wire flush_in = ~s_axis_phase_tvalid; // It's a flush if input is not valid
  wire flush_out;

  dds_sin_cos_lut_only dds_sin_cos_lut_only_i (
    .aclk                (clk),
    .aresetn             (~rst),
    .s_axis_phase_tvalid (phase_tvalid),
    .s_axis_phase_tready (phase_tready),
    .s_axis_phase_tdata  (phase_tdata),
    .s_axis_phase_tlast  (phase_tlast),
    .s_axis_phase_tuser  (flush_in),
    .m_axis_data_tvalid  (dds_tvalid),
    .m_axis_data_tready  (dds_tready),
    .m_axis_data_tdata   (dds_tdata),
    .m_axis_data_tlast   (dds_tlast),
    .m_axis_data_tuser   (flush_out)
  );


  //---------------------------------------------------------------------------
  // Output Logic
  //---------------------------------------------------------------------------

  assign m_axis_data_tdata = dds_tdata;
  assign m_axis_data_tlast = dds_tlast;

  // Discard the current sample if it was for flushing.
  assign m_axis_data_tvalid = dds_tvalid & ~flush_out;
  assign dds_tready         = m_axis_data_tready | flush_out;

endmodule
