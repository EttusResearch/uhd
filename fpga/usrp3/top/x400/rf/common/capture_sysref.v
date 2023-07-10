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

  (* ASYNC_REG = "TRUE" *) reg sysref_pclk_ms = 1'b0;
  reg sysref_pclk = 1'b0;
  reg sysref_rclk = 1'b0;

  // Capture SYSREF synchronously with the pll_ref_clk, but double-sync it just
  // in case static timing isn't met so as not to destroy downstream logic.
  always @ (posedge pll_ref_clk) begin
    sysref_pclk_ms <= sysref_in;
    sysref_pclk    <= sysref_pclk_ms;
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
