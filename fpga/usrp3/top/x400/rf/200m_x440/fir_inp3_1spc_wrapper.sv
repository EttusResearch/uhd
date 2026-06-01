//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fir_inp3_1spc_wrapper
//
// Description:
//
//   Wrapper for the Xilinx FIR Compiler IP (fir_inp3_1spc) configured as an
//   interpolate-by-3 filter with 81 taps, 18-bit coefficients, 2 parallel data
//   paths (I/Q), 1 sample per clock input, and 16-bit signed input data. The
//   output is full-precision 35-bit.
//
//   IP Configuration Summary (from fir_inp3_1spc.xci):
//     - Filter Type        : Interpolation
//     - Interpolation Rate : 3
//     - Number of Taps     : 81
//     - Coefficient Width  : 18 (signed)
//     - Data Width         : 16 (signed)
//     - Number of Paths    : 2 (I and Q)
//     - Output Width       : 35 (full precision)
//     - Coefficient Struct : Non-Symmetric
//     - Architecture       : Systolic Multiply-Accumulate
//     - Column Config      : 27
//     - Has ARESETn        : Yes (active-low synchronous reset)
//     - Latency            : 32 clock cycles
//
//   s_axis_data_tdata packing (2 data paths, 1 SPC, 16 bits each):
//     [15:0]    = I sample 0    [31:16]   = Q sample 0
//
//   m_axis_data_tdata packing (2 data paths, 3 SPC, 35 bits each padded to
//   40-bit boundaries):
//     [34:0]    = I sample 0     [39:35]   = padding
//     [74:40]   = Q sample 0     [79:75]   = padding
//     [114:80]  = I sample 1     [119:115] = padding
//     [154:120] = Q sample 1     [159:155] = padding
//     [194:160] = I sample 2     [199:195] = padding
//     [234:200] = Q sample 2     [239:235] = padding
//

module fir_inp3_1spc_wrapper (
  input  logic         aclk,
  input  logic         aresetn,

  // Input DATA channel (AXI4-Stream slave)
  input  logic [ 31:0] s_axis_data_tdata,
  input  logic         s_axis_data_tvalid,
  output logic         s_axis_data_tready,

  // Output DATA channel (AXI4-Stream master)
  output logic [239:0] m_axis_data_tdata,
  output logic         m_axis_data_tvalid
);

  fir_inp3_1spc fir_inp3_i (
    .aclk               (aclk),
    .aresetn            (aresetn),
    .s_axis_data_tdata  (s_axis_data_tdata),
    .s_axis_data_tvalid (s_axis_data_tvalid),
    .s_axis_data_tready (s_axis_data_tready),
    .m_axis_data_tdata  (m_axis_data_tdata),
    .m_axis_data_tvalid (m_axis_data_tvalid)
  );

endmodule : fir_inp3_1spc_wrapper
