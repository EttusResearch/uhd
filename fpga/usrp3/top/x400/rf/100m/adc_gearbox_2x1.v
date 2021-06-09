//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: adc_gearbox_2x1
//
// Description:
//
//   Gearbox ADC data from 2 SPC to 1 SPC and corresponding 2x clock to 1x
//   clock. Also implement data swapping to format packets to fit the FIR
//   filter input requirements.
//
//   This modules incurs one clk1x cycle of delay on the data and valid signals
//   from input on the 1x domain to output on the 2x domain.
//

`default_nettype none

module adc_gearbox_2x1 (
  input  wire        clk1x,
  input  wire        reset_n_1x,
  // Data is _presumed_ to be packed [Sample1, Sample0] (Sample0 in LSBs).
  input  wire [31:0] adc_q_in_1x,
  input  wire [31:0] adc_i_in_1x,
  input  wire        valid_in_1x,
  // De-assert enable_1x to clear the data synchronously from this module.
  input  wire        enable_1x,

  input  wire        clk2x,
  // Data is packed [Q,I] (I in LSBs) when swap_iq_1x is '0'.
  input  wire        swap_iq_2x,
  output wire [31:0] adc_out_2x,
  output wire        valid_out_2x
);

  // Re-create the 1x clock in the 2x domain to produce a deterministic
  // crossing.
  reg toggle_1x, toggle_2x = 1'b0, toggle_2x_dly = 1'b0, valid_2x = 1'b0, valid_dly_2x = 1'b0;
  reg [31:0] data_out_2x = 32'b0, adc_q_data_in_2x = 32'b0, adc_i_data_in_2x = 32'b0;

  // Create a toggle in the 1x clock domain (clock divider /2).
  always @(posedge clk1x or negedge reset_n_1x) begin
    if ( ! reset_n_1x) begin
      toggle_1x <= 1'b0;
    end else begin
      toggle_1x <= ! toggle_1x;
    end
  end

  // clk1x and clk2x are nominally aligned on their rising edges, but clk2x is
  // more heavily loaded, which results in a later arrival time. That late
  // arrival causes large estimated hold violations after place. The Ultrafast
  // method (UG 949) suggests fixing post-place hold violations that are worse
  // than -0.5 ns.
  // Resampling 1x signals on the falling edge of clk2x provides nominally half
  // a period of setup and half a period of hold. The late arrival of clk2x
  // shifts some of that margin away from hold slack and into setup slack.
  reg        toggle_2x_fall   = 1'b0;
  reg [31:0] adc_q_in_2x_fall = 32'b0;
  reg [31:0] adc_i_in_2x_fall = 32'b0;
  reg        valid_in_2x_fall = 1'b0;
  reg        enable_2x_fall   = 1'b0;

  always @(negedge clk2x) begin
    toggle_2x_fall   <= toggle_1x;
    adc_q_in_2x_fall <= adc_q_in_1x;
    adc_i_in_2x_fall <= adc_i_in_1x;
    valid_in_2x_fall <= valid_in_1x;
    enable_2x_fall   <= enable_1x;
  end

  // Transfer the toggle from the 1x to the 2x domain. Delay the toggle in the
  // 2x domain by one cycle and compare it to the non-delayed version. When
  // they differ, push data_in[15:0] onto the output. When the match, push
  // [31:16] onto the output. The datasheet is unclear on the exact
  // implementation.
  //
  // It is safe to not reset this domain because all of the input signals will
  // be cleared by the 1x reset. Safe default values are assigned to all these
  // registers.
  always @(posedge clk2x) begin
    toggle_2x     <= toggle_2x_fall;
    toggle_2x_dly <= toggle_2x;
    adc_q_data_in_2x <= adc_q_in_2x_fall;
    adc_i_data_in_2x <= adc_i_in_2x_fall;
    // Place Q in the MSBs, I in the LSBs by default, unless swapped = 1.
    if (valid_2x) begin
      if (swap_iq_2x) begin
        if (toggle_2x != toggle_2x_dly) begin
          data_out_2x[31:16] <= adc_i_data_in_2x[15:0];
          data_out_2x[15: 0] <= adc_q_data_in_2x[15:0];
        end else begin
          data_out_2x[31:16] <= adc_i_data_in_2x[31:16];
          data_out_2x[15: 0] <= adc_q_data_in_2x[31:16];
        end
      end else begin
        if (toggle_2x != toggle_2x_dly) begin
          data_out_2x[31:16] <= adc_q_data_in_2x[15:0];
          data_out_2x[15: 0] <= adc_i_data_in_2x[15:0];
        end else begin
          data_out_2x[31:16] <= adc_q_data_in_2x[31:16];
          data_out_2x[15: 0] <= adc_i_data_in_2x[31:16];
        end
      end
    end else begin
      data_out_2x <= 32'b0;
    end
    // Valid is simply a transferred version of the 1x clock's valid. Delay it
    // one more cycle to align outputs.
    valid_2x     <= valid_in_2x_fall && enable_2x_fall;
    valid_dly_2x <= valid_2x;
  end

  assign adc_out_2x   = data_out_2x;
  assign valid_out_2x = valid_dly_2x;

endmodule

`default_nettype wire
