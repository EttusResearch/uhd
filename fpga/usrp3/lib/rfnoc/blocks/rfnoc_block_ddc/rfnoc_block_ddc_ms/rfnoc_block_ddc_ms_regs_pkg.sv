//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: rfnoc_block_ddc_ms_regs_pkg
//
// Description: Register offsets, bitfields and constants for the multisample
// RFNoC DDC block. This is the SystemVerilog package version of
// rfnoc_block_ddc_regs.vh, scoped to the multisample DDC implementation.
//

package rfnoc_block_ddc_ms_regs_pkg;

  //=============================================================================
  // Multisample DDC registers (new DDC)
  //=============================================================================
  //-----------------------------------------------------------------------------
  // Shared Register Offsets + Block-Level Address Space Layout
  //-----------------------------------------------------------------------------
  // Shared register window (one set per DDC NoC block)
  localparam int DDC_SHARED_BASE_ADDR = 20'h00; // Base address for shared DDC registers
  localparam int DDC_SHARED_ADDR_W    = 8;      // Address space size for shared registers
  // Shared registers
  localparam int REG_COMPAT_NUM    = 'h00; // Compatibility number register offset
  localparam int REG_NUM_HB        = 'h04; // Number of halfband filter stages in the DDC
  localparam int REG_CIC_MAX_DECIM = 'h08; // Maximum decimation rate supported by the CIC filter
  localparam int REG_SPC           = 'h0C; // Number of samples per clock cycle
  // Per-port register window (one set per DDC port)
  localparam int DDC_PORT_BASE_ADDR = 20'h100; // Base address of first port
  localparam int DDC_PORT_ADDR_W    = 11;      // Address space size per port
  localparam int DDC_PORT_RANGE_W   = 19;      // Address space for all ports

  //-----------------------------------------------------------------------------
  // DDC Register Offsets (One Set Per Port)
  //-----------------------------------------------------------------------------
  //
  // These registers are replicated depending on the number of DDC ports.
  // They start at DDC_PORT_BASE_ADDR and each port's register set occupies
  // 2**DDC_PORT_ADDR_W bytes before the next port's registers begin.
  //
  // WARNING: All registers larger than a single 32-bit word must be read and
  //          written least significant word first to guarantee coherency.
  //
  //-----------------------------------------------------------------------------
  // port-specific registers
  // axi rate change sub-port
  localparam int DDC_PORT_AXI_RATE_OFFSET = 'h000;
  localparam int DDC_PORT_AXI_RATE_ADDR_W = 8;
  // sampling rate change sub-port
  localparam int DDC_PORT_SR_OFFSET  = 'h100;
  localparam int DDC_PORT_SR_ADDR_W  = 8;
  // dds sub-port
  localparam int DDC_PORT_DDS_OFFSET = 'h200;
  localparam int DDC_PORT_DDS_ADDR_W = 8;
  // axi_tag_time_ms sub-port - handles DDS freq shift register
  import axi_tag_time_ms_pkg::*;
  localparam int REG_DDS_FREQ_ADDR   = REG_TAG_ADDR;

endpackage : rfnoc_block_ddc_ms_regs_pkg
