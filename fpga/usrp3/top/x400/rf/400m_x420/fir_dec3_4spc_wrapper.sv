//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fir_dec3_4spc_wrapper
//
// Description:
//
//   Wrapper for the Xilinx FIR Compiler IP (fir_dec3_4spc) configured as a
//   decimate-by-3 filter with 77 symmetric taps, 18-bit coefficients,
//   2 parallel data paths (I/Q), 4 samples per clock (super-sample rate),
//   and 16-bit signed input data. The output is full-precision 34-bit.
//
//  The FIR filter coefficients are reused from
//  fpga/usrp3/top/x400/ip/adc_400m_bd/adc_400m_bd.tcl
//
//   IP Configuration Summary (from fir_dec3_4spc.xci):
//     - Filter Type        : Decimation
//     - Decimation Rate    : 3
//     - Number of Taps     : 77
//     - Coefficient Width  : 18 (signed)
//     - Data Width         : 16 (signed)
//     - Number of Paths    : 2 (I and Q)
//     - Output Width       : 34 (full precision)
//     - Coefficient Struct : Symmetric
//     - Architecture       : Systolic Multiply-Accumulate
//     - Has ARESETn        : Yes (active-low synchronous reset)
//     - Latency            : 20 clock cycles
//
//   s_axis_data_tdata packing (8 internal data paths, 16 bits each):
//     [15:0]    = I sample 0    [31:16]   = Q sample 0
//     [47:32]   = I sample 1    [63:48]   = Q sample 1
//     [79:64]   = I sample 2    [95:80]   = Q sample 2
//     [111:96]  = I sample 3    [127:112] = Q sample 3
//
//   m_axis_data_tdata packing (4 output data paths, 34 bits each
//   padded to 40-bit boundaries):
//     [33:0]    = I sample 0    [39:34]   = padding
//     [73:40]   = Q sample 0    [79:74]   = padding
//     [113:80]  = I sample 1    [119:114] = padding
//     [153:120] = Q sample 1    [159:154] = padding
//
//   m_axis_data_tuser[0] = data_valid (indicates filter memory is fully
//   flushed after reset).
//

module fir_dec3_4spc_wrapper (
  input  logic         aclk,
  input  logic         aresetn,

  // Input DATA channel (AXI4-Stream slave)
  input  logic [127:0] s_axis_data_tdata,
  input  logic         s_axis_data_tvalid,
  output logic         s_axis_data_tready,

  // Output DATA channel (AXI4-Stream master)
  output logic [159:0] m_axis_data_tdata,
  output logic         m_axis_data_tvalid,
  output logic         m_axis_data_tuser
);

  fir_dec3_4spc fir_dec3_4spc_i (
    .aclk               (aclk),
    .aresetn            (aresetn),
    .s_axis_data_tdata  (s_axis_data_tdata),
    .s_axis_data_tvalid (s_axis_data_tvalid),
    .s_axis_data_tready (s_axis_data_tready),
    .m_axis_data_tdata  (m_axis_data_tdata),
    .m_axis_data_tvalid (m_axis_data_tvalid),
    .m_axis_data_tuser  (m_axis_data_tuser)
  );

endmodule : fir_dec3_4spc_wrapper
