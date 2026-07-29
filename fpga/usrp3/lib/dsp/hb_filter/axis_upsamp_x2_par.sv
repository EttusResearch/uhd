//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_upsamp_x2_par
//
// Description:
//
//  Parallel x2 upsampler for AXI-Stream. Valid only for SPC_OUT=2*SPC_IN.
//  Each input lane is mapped to one output lane and an inserted zero lanes
//  in between them.
//  Lane ordering controlled by INTERPOLATION_PHASE.
//
//  Lane mapping (for input lanes i = 0..SPC_IN-1):
//
//    INTERPOLATION_PHASE = 0 (data first)
//      out lane (2*i)   <- in lane i
//      out lane (2*i+1) <- 0
//
//      Example with SPC_IN=2, SPC_OUT=4 (stacked by lane index):
//        in lanes
//          lane1 <- in1
//          lane0 <- in0
//        out lanes
//          lane3 <- 0
//          lane2 <- in1
//          lane1 <- 0
//          lane0 <- in0
//
//    INTERPOLATION_PHASE = 1 (zero first)
//      out lane (2*i)   <- 0
//      out lane (2*i+1) <- in lane i
//
//      Example with SPC_IN=2, SPC_OUT=4 (stacked by lane index):
//        in lanes
//          lane1 <- in1
//          lane0 <- in0
//        out lanes
//          lane3 <- in1
//          lane2 <- 0
//          lane1 <- in0
//          lane0 <- 0
//
// Parameters:
//   SAMP_W: Sample width in bits.
//   SPC_IN: Samples per cycle input. Must be a power of 2.
//   SPC_OUT: Samples per cycle output. Must be a power of 2.
//            SPC_OUT must be equal to 2*SPC_IN for this module.
//  INTERPOLATION_PHASE: 0 or 1 to select which interpolation phase to output.
//                        0: data first, 1: zero first.
//
`default_nettype none

module axis_upsamp_x2_par #(
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

  if (SPC_OUT != (2 * SPC_IN)) begin : spc_par_mode_error
    initial begin
      $error("upsamp_x2_par requires SPC_OUT=2*SPC_IN. Got SPC_IN=%0d, SPC_OUT=%0d.", SPC_IN, SPC_OUT);
      $fatal;
    end
  end

  logic [SPC_OUT*SAMP_W-1:0] out_tdata;

  for (genvar samp_idx = 0; samp_idx < SPC_IN; samp_idx++) begin : upsample_map
    if (INTERPOLATION_PHASE == 1'b0) begin : phase_real_first
      assign out_tdata[(2*samp_idx)*SAMP_W +: SAMP_W]   = s_axis_tdata[samp_idx*SAMP_W +: SAMP_W];
      assign out_tdata[(2*samp_idx+1)*SAMP_W +: SAMP_W] = '0;
    end else begin : phase_zero_first
      assign out_tdata[(2*samp_idx)*SAMP_W +: SAMP_W]   = '0;
      assign out_tdata[(2*samp_idx+1)*SAMP_W +: SAMP_W] = s_axis_tdata[samp_idx*SAMP_W +: SAMP_W];
    end
  end : upsample_map

  assign m_axis_tdata  = out_tdata;
  assign m_axis_tvalid = s_axis_tvalid;
  assign m_axis_tlast  = s_axis_tlast;
  assign s_axis_tready = m_axis_tready;

endmodule : axis_upsamp_x2_par

`default_nettype wire
