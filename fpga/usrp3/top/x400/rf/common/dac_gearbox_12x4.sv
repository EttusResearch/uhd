//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dac_gearbox_12x4
//
// Description:
//
//   Gearbox DAC data from 12 SPC to 4 SPC and corresponding 3x clock to 1x
//   clock.
//

module dac_gearbox_12x4 (
  input  logic         clk1x,
  input  logic         reset_n_1x,
  // Data is _presumed_ to be packed [Q11,I11,...,Q1,I1,Q0,I0]
  input  logic [383:0] data_in_1x,
  input  logic         valid_in_1x,

  input  logic         clk3x,
  // Data is packed [Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
  output logic [127:0] data_out_3x,
  output logic         valid_out_3x
);

  // Re-create the 1x clock in the 3x domain to produce a deterministic
  // crossing.
  logic toggle_1x, toggle_3x = '0, toggle_3x_dly = '0, valid_3x = '0, valid_dly_3x = '0;
  logic [1:0] phase_3x = '0;
  logic [383:0] data_in_3x = '0;
  logic [127:0] data_3x_dly = '0;

  // Create a toggle in the 1x clock domain (clock divider /2).
  always_ff @(posedge clk1x or negedge reset_n_1x) begin
    if (!reset_n_1x) begin
      toggle_1x <= '0;
    end else begin
      toggle_1x <= ! toggle_1x;
    end
  end

  // Transfer the toggle from the 1x to the 3x domain. Delay the toggle in the
  // 3x domain by one cycle and compare it to the non-delayed version. When
  // they differ, a new 1x period has started (phase 0). A phase counter tracks
  // the three output phases within each 1x period:
  //   Phase 0 (edge): output [127:0]   - samples 0-3
  //   Phase 1:        output [255:128] - samples 4-7
  //   Phase 2:        output [383:256] - samples 8-11
  //
  // It is safe to not reset this domain because all of the input signals will
  // be cleared by the 1x reset. Safe default values are assigned to all these
  // registers.
  always_ff @(posedge clk3x) begin
    toggle_3x <= toggle_1x;
    toggle_3x_dly <= toggle_3x;
    data_in_3x <= data_in_1x;
    data_3x_dly <= '0;

    // Phase counter: reset on toggle edge, count 0-1-2 otherwise.
    if (toggle_3x != toggle_3x_dly) begin
      phase_3x <= 2'd1;
    end else if (phase_3x == 2'd2) begin
      phase_3x <= '0;
    end else begin
      phase_3x <= phase_3x + 2'd1;
    end

    // Update data path unconditionally depending only on the phase.
    if (phase_3x == 2'd0) begin
      data_3x_dly <= data_in_3x[127:0];
    end else if (phase_3x == 2'd1) begin
      data_3x_dly <= data_in_3x[255:128];
    end else if (phase_3x == 2'd2) begin
      data_3x_dly <= data_in_3x[383:256];
    end

    // Valid is simply a transferred version of the 1x clock's valid. Delay it
    // one more cycle to align outputs.
    valid_3x     <= valid_in_1x;
    valid_dly_3x <= valid_3x;
  end

  assign valid_out_3x = valid_dly_3x;
  assign data_out_3x = data_3x_dly;

endmodule
