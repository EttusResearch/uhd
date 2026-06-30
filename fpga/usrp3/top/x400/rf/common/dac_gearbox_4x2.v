//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dac_gearbox_4x2
//
// Description:
//
//   Gearbox DAC data from 4 SPC to 2 SPC and corresponding 2x clock to 1x
//   clock.
//   This module incurs in one clk1x cycle of delay on the data and valid
//   signals from input on the 1x domain to output on the 2x domain.
//

`default_nettype none

module dac_gearbox_4x2 (
  input  wire         clk1x,
  input  wire         reset_n_1x,
  // Data is _presumed_ to be packed [Q3,I3,Q2,I2,Q1,I1,Q0,I0]
  input  wire [127:0] data_in_1x,
  input  wire         valid_in_1x,
  output wire         ready_out_1x,

  input  wire         clk2x,
  // Data is packed [Q1,I1,Q0,I0] (I in LSBs)
  output wire [ 63:0] data_out_2x,
  output wire         valid_out_2x
);

  // Re-create the 1x clock in the 2x domain to produce a deterministic
  // crossing.
  reg toggle_1x, toggle_2x = 1'b0, toggle_2x_dly = 1'b0, valid_2x = 1'b0, valid_dly_2x = 1'b0;
  reg [127:0] data_in_2x_dly0 = 128'b0, data_in_2x_dly1 = 32'b0;
  reg [63 :0] data_2x_dly = 64'b0;

  // Create a toggle in the 1x clock domain (clock divider /2).
  always @(posedge clk1x or negedge reset_n_1x) begin
    if ( ! reset_n_1x) begin
      toggle_1x <= 1'b0;
    end else begin
      toggle_1x <= ! toggle_1x;
    end
  end

  // Transfer the toggle from the 1x to the 2x domain. Delay the toggle in the
  // 2x domain by one cycle and compare it to the non-delayed version. When
  // they differ, push data_in[63:0] onto the output. When they match, push
  // [127:64] onto the output.
  //
  // It is safe to not reset this domain because all of the input signals will
  // be cleared by the 1x reset. Safe default values are assigned to all these
  // registers.
  always @(posedge clk2x) begin
    toggle_2x     <= toggle_1x;
    toggle_2x_dly <= toggle_2x;
    data_in_2x_dly0 <= data_in_1x;
    data_in_2x_dly1 <= data_in_2x_dly0 ;
    data_2x_dly <= 64'b0;

    if (valid_2x) begin
      data_2x_dly <= data_in_2x_dly1[127:64];
      if (toggle_2x != toggle_2x_dly) begin
        data_2x_dly <= data_in_2x_dly0[63:0];
      end
    end
    // Valid is simply a transferred version of the 1x clock's valid. Delay it
    // one more cycle to align outputs.
    valid_2x     <= valid_in_1x;
    valid_dly_2x <= valid_2x;
  end

  assign valid_out_2x = valid_dly_2x;
  assign data_out_2x = data_2x_dly;
  assign ready_out_1x = 1'b1;

endmodule

`default_nettype wire
