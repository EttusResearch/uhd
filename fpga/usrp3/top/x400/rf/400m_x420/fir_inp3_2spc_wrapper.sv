//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fir_inp3_2spc_wrapper
//
// Description:
//
//   Wrapper for the Xilinx FIR Compiler IP (fir_inp3_2spc) configured as an
//   interpolate-by-3 filter with 81 taps, 18-bit coefficients,
//   2 parallel data paths (I/Q), 2 samples per clock (super sample rate),
//   and 16-bit signed input data. The output is full-precision 35-bit.
//
//  The FIR filter coefficients are reused from
//  fpga/usrp3/top/x400/ip/dac_400m_bd/dac_400m_bd.tcl
//
//   IP Configuration Summary (from fir_inp3_2spc.xci):
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
//   s_axis_data_tdata packing (4 internal data paths, 16 bits each):
//     [15:0]    = I sample 0    [31:16]   = Q sample 0
//     [47:32]   = I sample 1    [63:48]   = Q sample 1
//
//   m_axis_data_tdata packing (12 output data paths, 35 bits each
//   padded to 40-bit boundaries):
//     [34:0]    = I sample 0     [39:35]   = padding
//     [74:40]   = Q sample 0     [79:75]   = padding
//     [114:80]  = I sample 1     [119:115] = padding
//     [154:120] = Q sample 1     [159:155] = padding
//     [194:160] = I sample 2     [199:195] = padding
//     [234:200] = Q sample 2     [239:235] = padding
//     [274:240] = I sample 3     [279:275] = padding
//     [314:280] = Q sample 3     [319:315] = padding
//     [354:320] = I sample 4     [359:355] = padding
//     [394:360] = Q sample 4     [399:395] = padding
//     [434:400] = I sample 5     [439:435] = padding
//     [474:440] = Q sample 5     [479:475] = padding
//
//   m_axis_data_tuser[0] = data_valid (indicates filter memory is fully
//   flushed after reset).
//

module fir_inp3_2spc_wrapper (
  input  logic         aclk,
  input  logic         aresetn,

  // Input DATA channel (AXI4-Stream slave)
  input  logic [ 63:0] s_axis_data_tdata,
  input  logic         s_axis_data_tvalid,
  output logic         s_axis_data_tready,

  // Output DATA channel (AXI4-Stream master)
  output logic [479:0] m_axis_data_tdata,
  output logic         m_axis_data_tvalid,
  output logic         m_axis_data_tuser
);

  fir_inp3_2spc fir_inp3_2spc_i (
    .aclk               (aclk),
    .aresetn            (aresetn),
    .s_axis_data_tdata  (s_axis_data_tdata),
    .s_axis_data_tvalid (s_axis_data_tvalid),
    .s_axis_data_tready (s_axis_data_tready),
    .m_axis_data_tdata  (m_axis_data_tdata),
    .m_axis_data_tvalid (m_axis_data_tvalid),
    .m_axis_data_tuser  (m_axis_data_tuser)
  );

endmodule : fir_inp3_2spc_wrapper
