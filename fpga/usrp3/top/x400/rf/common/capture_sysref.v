//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: capture_sysref
//
// Description:
//
//   Capture SYSREF and transfer it to the higher clock domain.
//   For X410, module incurs in 2 pll_ref_clk cycles + 1 rfdc_clk
//   cycle of delay. For X440, module synchronizes to each
//   pll_ref_clk and rfdc_clk separately, incurring in 2 cycles
//   for each clock.
//
// Parameters:
//
//   DEVICE_TYPE     : Type of device for which SYSREF is synchronized.
//

module capture_sysref (
  // Clocks
  input  wire pll_ref_clk,
  input  wire rfdc_clk,

  // SYSREF input and control
  input  wire sysref_in,       // Single-ended SYSREF (previously buffered)
  input  wire enable_rclk,     // Enables SYSREF output in the rfdc_clk domain.

  // Captured SYSREF outputs
  output wire sysref_out_pclk, // Debug output (Domain: pll_ref_clk).
  output wire sysref_out_rclk  // RFDC output  (Domain: rfdc_clk).
);

  (* ASYNC_REG = "TRUE" *) reg sysref_neg_pclk_ms = 1'b0;
  (* ASYNC_REG = "TRUE" *) reg sysref_pos_pclk_ms = 1'b0;
  reg sysref_pclk     = 1'b0;
  reg sysref_rclk     = 1'b0;

  // The following waveform illustrates the capture process of SYSREF.
  //
  // PRC                   ___/-----\_____/-----\_____/-----\_____/----
  // SYSREF                ___/----------------------------------------
  // falling edge FF MS    ---------|R|--------------------------------
  // rising edge FF MS     ---------------|R|--------------------------
  // rising edge FF        ---------------------------|R|--------------
  //
  // Capture SYSREF synchronously with the pll_ref_clk, but at the falling edge.
  // This is done to ensure that SYSREF is captured well after its initial
  // rising edge to ensure a stable signal.
  // But to be on the safe side when combining this FPGA code with older
  // LMK04832 MPM configuration (before June 2024), the signal is considered
  // metastable and requires additional double synchronization.
  always @ (negedge pll_ref_clk) begin
    sysref_neg_pclk_ms <= sysref_in;
  end
  
  always @ (posedge pll_ref_clk) begin
    sysref_pos_pclk_ms  <= sysref_neg_pclk_ms;
    sysref_pclk         <= sysref_pos_pclk_ms;
  end

  assign sysref_out_pclk = sysref_pclk;

  // Transfer to faster clock which is edge-aligned with the pll_ref_clk.
  always @ (posedge rfdc_clk) begin
    if (enable_rclk) begin
      sysref_rclk <= sysref_pclk;
    end else begin
      sysref_rclk <= 1'b0;
    end
  end

  assign sysref_out_rclk = sysref_rclk;

endmodule
