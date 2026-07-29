//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_upsamp_x2
//
// Description: Wrapper for x2 upsamples. Depending on SPC_IN and SPC_OUT,
//              this module instantiates either a serial (time domain for SPC_OUT = SPC_IN = 1)
//              or parallel x2 upsampler (lane domain, SPC_OUT = 2 * SPC_IN).
//
// Parameters:
//   SAMP_W: Sample width in bits.
//   SPC_IN: Samples per cycle input. Must be a power of 2 (including 1).
//   SPC_OUT: Samples per cycle output. Must be a power of 2 (including 1).
//   INTERPOLATION_PHASE: 0 or 1 to select which interpolation phase to output
//                        for the inserted zero samples.
//                        0: data first, 1: zero first.
//
`default_nettype none

module axis_upsamp_x2 #(
  parameter int SAMP_W              = 32,
  parameter int SPC_IN              = 4,
  parameter int SPC_OUT             = 8,
  parameter bit INTERPOLATION_PHASE = 1'b0
) (
  input  wire                       clk,
  input  wire                       rst,
  input  wire                       clr,
  input  wire [SPC_IN*SAMP_W-1:0]   s_axis_tdata,
  input  wire                       s_axis_tvalid,
  input  wire                       s_axis_tlast,
  output logic                      s_axis_tready,
  output logic [SPC_OUT*SAMP_W-1:0] m_axis_tdata,
  output logic                      m_axis_tvalid,
  output logic                      m_axis_tlast,
  input  wire                       m_axis_tready
);

  generate
    if ((SPC_IN == 1) && (SPC_OUT == 1)) begin : gen_ser
      axis_upsamp_x2_ser #(
        .SAMP_W             (SAMP_W),
        .INTERPOLATION_PHASE(INTERPOLATION_PHASE)
      ) upsamp_x2_ser_x (
        .clk          (clk),
        .rst          (rst),
        .clr          (clr),
        .s_axis_tdata (s_axis_tdata),
        .s_axis_tvalid(s_axis_tvalid),
        .s_axis_tlast (s_axis_tlast),
        .s_axis_tready(s_axis_tready),
        .m_axis_tdata (m_axis_tdata),
        .m_axis_tvalid(m_axis_tvalid),
        .m_axis_tlast (m_axis_tlast),
        .m_axis_tready(m_axis_tready)
      );
    end else if (SPC_OUT == (2 * SPC_IN)) begin : gen_par
      axis_upsamp_x2_par #(
        .SAMP_W             (SAMP_W),
        .SPC_IN             (SPC_IN),
        .SPC_OUT            (SPC_OUT),
        .INTERPOLATION_PHASE(INTERPOLATION_PHASE)
      ) upsamp_x2_par_x (
        .clk          (clk),
        .rst          (rst),
        .clr          (clr),
        .s_axis_tdata (s_axis_tdata),
        .s_axis_tvalid(s_axis_tvalid),
        .s_axis_tlast (s_axis_tlast),
        .s_axis_tready(s_axis_tready),
        .m_axis_tdata (m_axis_tdata),
        .m_axis_tvalid(m_axis_tvalid),
        .m_axis_tlast (m_axis_tlast),
        .m_axis_tready(m_axis_tready)
      );
    end else begin : gen_invalid
      initial begin
        $error({"Unsupported upsampler SPC configuration. ",
                "Expected (SPC_IN,SPC_OUT)=(1,1) or SPC_OUT=2*SPC_IN. ",
                "Got SPC_IN=%0d, SPC_OUT=%0d."}, SPC_IN, SPC_OUT);
        $fatal;
      end

      assign s_axis_tready = 1'b0;
      assign m_axis_tdata  = '0;
      assign m_axis_tvalid = 1'b0;
      assign m_axis_tlast  = 1'b0;
    end
  endgenerate

endmodule : axis_upsamp_x2

`default_nettype wire
