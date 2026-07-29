//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ddc_ms_regs_pkg
//
// Description: Register map for the ddc_ms module.
//

package ddc_ms_regs_pkg;

  // Sampling-rate-change (SR) sub-port: base 0x000 within the ddc_ms window
  localparam int DDC_SR_BASE_ADDR = 'h000;
  localparam int DDC_SR_ADDR_W    = 8;
  localparam int REG_SR_DECIM_ADDR    = 'h00; // Decimation rate (CIC + HBF)
  localparam int REG_SR_CIC_RATE_W    =  8;   // CIC rate - 8 bits
  localparam int REG_SR_HB_EN_W       =  2;   // HB enable bits - 2 bits
  localparam int REG_SR_DECIM_W       =  REG_SR_CIC_RATE_W + REG_SR_HB_EN_W;
  localparam int REG_SR_MUX_ADDR      = 'h04; // Input mux configuration
  localparam int REG_SR_SCALE_IQ_ADDR = 'h08; // IQ scale factor
  localparam int REG_SR_SCALE_IQ_W    =  18;  // 18 bits for IQ scale factor

  // DDS sub-port: base 0x100 within the ddc_ms window
  localparam int DDC_DDS_BASE_ADDR = 'h100;
  localparam int DDC_DDS_ADDR_W    = 8;

endpackage : ddc_ms_regs_pkg
