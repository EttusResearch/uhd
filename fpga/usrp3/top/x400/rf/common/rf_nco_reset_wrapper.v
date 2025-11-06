//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_nco_reset_wrapper
//
// Verilog wrapper for rf_nco_reset_controller
//

module rf_nco_reset_wrapper(
  input wire config_clk,
  input wire pll_ref_clk,
  input wire data_clk,
  input wire p_sysref,
  input wire d_start_nco_reset,
  input wire [1:0] c_dac0_nco_update_busy,

  output wire c_dac0_nco_update_req,
  output wire c_dac0_sysref_int_gating,
  output wire c_dac0_sysref_int_reenable,

  input wire c_dac1_nco_update_busy,
  output wire c_all_other_nco_update_req,

  input wire c_adc0_nco_update_busy,
  input wire c_adc1_nco_update_busy,
  input wire c_adc2_nco_update_busy,
  input wire c_adc3_nco_update_busy,

  output wire c_nco_phase_rst,
  output wire [5:0] c_nco_update_enable,
  output wire d_nco_reset_done,
  output wire d_nco_synchronization_failed,
  input wire [7:0] d_sysref_wait_cycles
);

  rf_nco_reset_controller
    rf_nco_reset_controller_i
    (
      .config_clk (config_clk),
      .pll_ref_clk (pll_ref_clk),
      .data_clk (data_clk),
      .p_sysref (p_sysref),
      .d_start_nco_reset (d_start_nco_reset),
      .c_dac0_nco_update_busy (c_dac0_nco_update_busy),
      .c_dac0_nco_update_req (c_dac0_nco_update_req),
      .c_dac0_sysref_int_gating (c_dac0_sysref_int_gating),
      .c_dac0_sysref_int_reenable (c_dac0_sysref_int_reenable),
      .c_dac1_nco_update_busy (c_dac1_nco_update_busy),
      .c_all_other_nco_update_req (c_all_other_nco_update_req),
      .c_adc0_nco_update_busy (c_adc0_nco_update_busy),
      .c_adc1_nco_update_busy (c_adc1_nco_update_busy),
      .c_adc2_nco_update_busy (c_adc2_nco_update_busy),
      .c_adc3_nco_update_busy (c_adc3_nco_update_busy),
      .c_nco_phase_rst (c_nco_phase_rst),
      .c_nco_update_en (c_nco_update_enable),
      .d_nco_reset_done (d_nco_reset_done),
      .d_nco_sync_failed (d_nco_synchronization_failed),
      .d_sysref_wait_cycles (d_sysref_wait_cycles)
  );

endmodule
