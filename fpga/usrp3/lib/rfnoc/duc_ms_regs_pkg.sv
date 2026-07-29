//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: duc_ms_regs_pkg
//
// Description: Register map for the duc_ms ctrlport window. Defines the
//   placement offsets of the SR and DDS sub-ports within that window, and
//   the SR register addresses (decoded by duc_ms.sv). Included by duc_ms.sv
//   and by rfnoc_block_duc_ms_regs_pkg.sv.
//

package duc_ms_regs_pkg;

  // Sampling-rate-change (SR) sub-port: base 0x000 within the duc_ms window
  localparam int DUC_SR_BASE_ADDR     = 'h000;
  localparam int DUC_SR_ADDR_W        = 8;
  localparam int REG_SR_INTERP_ADDR   = 'h00; // Interpolation rate (CIC + HBF)
  localparam int REG_SR_CIC_RATE_W    =  8;   // CIC rate - 8 bits
  localparam int REG_SR_HB_EN_W       =  2;   // HB enable bits - 2 bits
  localparam int REG_SR_INTERP_W      =  REG_SR_CIC_RATE_W + REG_SR_HB_EN_W;
  localparam int REG_SR_MUX_ADDR      = 'h04; // Input mux configuration
  localparam int REG_SR_MUX_W         =  2;   // 2 bits for input mux selection
  localparam int REG_SR_SCALE_IQ_ADDR = 'h08; // IQ scale factor
  localparam int REG_SR_SCALE_IQ_W    =  18;  // 18 bits for IQ scale factor

endpackage : duc_ms_regs_pkg
