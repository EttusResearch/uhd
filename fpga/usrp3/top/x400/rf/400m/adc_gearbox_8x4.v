//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: adc_gearbox_8x4
//
// Description:
//
//   Gearbox ADC data from 8 SPC to 4 SPC and corresponding 2x clock to 1x
//   clock. Also implement data swapping to format packets to fit the FIR
//   filter input requirements.
//
//   This modules incurs one clk1x cycle of delay on the data and valid signals
//   from input on the 1x domain to output on the 2x domain.
//

`default_nettype none

module adc_gearbox_8x4 (
  input  wire         clk1x,
  input  wire         reset_n_1x,
  // Data is _presumed_ to be packed [Sample7, ..., Sample0] (Sample0 in LSBs).
  input  wire [127:0] adc_q_in_1x,
  input  wire [127:0] adc_i_in_1x,
  input  wire         valid_in_1x,
  // De-assert enable_1x to clear the data valid output synchronously.
  input  wire         enable_1x,

  input  wire         clk2x,
  // Data is packed [Q3,I3, ... , Q0, I0] (I in LSBs) when swap_iq_1x is '0'
  input  wire         swap_iq_2x,
  output wire [127:0] adc_out_2x,
  output wire         valid_out_2x
);

  // Re-create the 1x clock in the 2x domain to produce a deterministic
  // crossing.
  reg toggle_1x, toggle_2x = 1'b0, toggle_2x_dly = 1'b0, valid_2x = 1'b0, valid_dly_2x = 1'b0;
  reg [127:0] data_out_2x = 128'b0, adc_q_data_in_2x = 128'b0, adc_i_data_in_2x = 128'b0;

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
  // they differ, push data_in[63:0] onto the output. When the match, push
  // [127:64] onto the output. The datasheet is unclear on the exact
  // implementation.
  //
  // It is safe to not reset this domain because all of the input signals will
  // be cleared by the 1x reset. Safe default values are assigned to all these
  // registers.
  always @(posedge clk2x) begin
    toggle_2x     <= toggle_1x;
    toggle_2x_dly <= toggle_2x;
    adc_q_data_in_2x <= adc_q_in_1x;
    adc_i_data_in_2x <= adc_i_in_1x;
    data_out_2x <= 128'b0;
    // Place Q in the MSBs, I in the LSBs by default, unless swapped = 1.
    if (valid_2x) begin
      if (swap_iq_2x) begin
        if (toggle_2x != toggle_2x_dly) begin
          data_out_2x <= {adc_i_data_in_2x[63:48], adc_q_data_in_2x[63:48],
                          adc_i_data_in_2x[47:32], adc_q_data_in_2x[47:32],
                          adc_i_data_in_2x[31:16], adc_q_data_in_2x[31:16],
                          adc_i_data_in_2x[15: 0], adc_q_data_in_2x[15: 0]};
        end else begin
          data_out_2x <= {adc_i_data_in_2x[127:112], adc_q_data_in_2x[127:112],
                          adc_i_data_in_2x[111: 96], adc_q_data_in_2x[111: 96],
                          adc_i_data_in_2x[95 : 80], adc_q_data_in_2x[95 : 80],
                          adc_i_data_in_2x[79 : 64], adc_q_data_in_2x[79 : 64]};
        end
      end else begin
        if (toggle_2x != toggle_2x_dly) begin
          data_out_2x <= {adc_q_data_in_2x[63:48], adc_i_data_in_2x[63:48],
                          adc_q_data_in_2x[47:32], adc_i_data_in_2x[47:32],
                          adc_q_data_in_2x[31:16], adc_i_data_in_2x[31:16],
                          adc_q_data_in_2x[15: 0], adc_i_data_in_2x[15: 0]};
        end else begin
          data_out_2x <= {adc_q_data_in_2x[127:112], adc_i_data_in_2x[127:112],
                          adc_q_data_in_2x[111: 96], adc_i_data_in_2x[111: 96],
                          adc_q_data_in_2x[95 : 80], adc_i_data_in_2x[95 : 80],
                          adc_q_data_in_2x[79 : 64], adc_i_data_in_2x[79 : 64]};
        end
      end
    end
    // Valid is simply a transferred version of the 1x clock's valid. Delay it one
    // more cycle to align outputs.
    valid_2x     <= valid_in_1x && enable_1x;
    valid_dly_2x <= valid_2x;
  end

  assign adc_out_2x   = data_out_2x;
  assign valid_out_2x = valid_dly_2x;

endmodule

`default_nettype wire
