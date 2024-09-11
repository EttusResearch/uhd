//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: adc_gearbox_8x1
//
// Description:
//
//   Gearbox ADC data from 8 SPC to 1 SPC and corresponding 1x clock to 8x
//   clock. Also implement data swapping to format packets to fit the FIR
//   filter input requirements.
//
//   This modules incurs one clk1x cycle of delay on the data and valid signals
//   from input on the 1x domain to output on the 8x domain.
//

`default_nettype none

module adc_gearbox_8x1 (
  input  wire        clk1x,
  input  wire        reset_n_1x,
  // Data is _presumed_ to be packed [Sample7, ..., Sample1, Sample0] (Sample0 in LSBs).
  input  wire [127:0] adc_q_in_1x,
  input  wire [127:0] adc_i_in_1x,
  input  wire        valid_in_1x,
  // De-assert enable_1x to clear the data synchronously from this module.
  input  wire        enable_1x,

  input  wire        clk8x,
  // Data is packed [Q,I] (I in LSBs) when swap_iq_1x is '0'.
  input  wire        swap_iq_8x,
  output wire [31:0] adc_out_8x,
  output wire        valid_out_8x
);

  // Re-create the 1x clock in the 8x domain to produce a deterministic
  // crossing.
  reg toggle_1x, toggle_8x = 1'b0, toggle_8x_dly = 1'b0, valid_8x = 1'b0, valid_dly_8x = 1'b0;
  reg [127:0] adc_q_data_in_8x = 128'b0, adc_i_data_in_8x = 128'b0;
  reg [31:0] data_out_8x = 32'b0;
  reg [2:0] counter = 3'b0;

  // Create a toggle in the 1x clock domain (clock divider /2).
  // This signal is used to have a change on the valid signal for each new word.
  // This allows the faster clock domain to detect a new word with the change of
  // the toggle signal.
  always @(posedge clk1x or negedge reset_n_1x) begin
    if ( ! reset_n_1x) begin
      toggle_1x <= 1'b0;
    end else begin
      toggle_1x <= ! toggle_1x;
    end
  end

  // clk1x and clk8x are nominally aligned on their rising edges, but clk8x is
  // more heavily loaded, which results in a later arrival time. That late
  // arrival causes large estimated hold violations after place. The Ultrafast
  // method (UG 949) suggests fixing post-place hold violations that are worse
  // than -0.5 ns.
  // Resampling 1x signals on the falling edge of clk8x provides nominally half
  // a period of setup and half a period of hold. The late arrival of clk8x
  // shifts some of that margin away from hold slack and into setup slack.
  reg        toggle_8x_fall   = 1'b0;
  reg [127:0] adc_q_in_8x_fall = 128'b0;
  reg [127:0] adc_i_in_8x_fall = 128'b0;
  reg        valid_in_8x_fall = 1'b0;
  reg        enable_8x_fall   = 1'b0;

  always @(negedge clk8x) begin
    toggle_8x_fall   <= toggle_1x;
    adc_q_in_8x_fall <= adc_q_in_1x;
    adc_i_in_8x_fall <= adc_i_in_1x;
    valid_in_8x_fall <= valid_in_1x;
    enable_8x_fall   <= enable_1x;
  end

  // Transfer the toggle from the 1x to the 8x domain. Delay the toggle in the
  // 8x domain by one cycle and compare it to the non-delayed version. When
  // they differ a new word has arrived. The counter below is used to determine
  // the index into the input data. See comment below for more details.
  //
  // It is safe to not reset this domain because all of the input signals will
  // be cleared by the 1x reset. Safe default values are assigned to all these
  // registers.
  always @(posedge clk8x) begin
    toggle_8x     <= toggle_8x_fall;
    toggle_8x_dly <= toggle_8x;
    adc_q_data_in_8x <= adc_q_in_8x_fall;
    adc_i_data_in_8x <= adc_i_in_8x_fall;
    // Place Q in the MSBs, I in the LSBs by default, unless swapped = 1.
    if (valid_8x) begin
      if (swap_iq_8x) begin
        data_out_8x[31:16] <= adc_i_data_in_8x[counter*16 +: 16];
        data_out_8x[15: 0] <= adc_q_data_in_8x[counter*16 +: 16];
      end else begin
        // arrival of new data word
        data_out_8x[31:16] <= adc_q_data_in_8x[counter*16 +: 16];
        data_out_8x[15: 0] <= adc_i_data_in_8x[counter*16 +: 16];
      end
    end else begin
      data_out_8x <= 32'b0;
    end

    // The counter is used to determine the index into the input data.
    // An edge on the toggle signal indicated the start of a new word.
    // As the counter is a register it needs to be updated to 1 to be correct
    // for the next cycle.
    // The counter is incremented for each cycle up to 7 and back to 0.
    if (toggle_8x != toggle_8x_dly) begin
      counter <= 3'd1;
    end else begin
      counter <= counter + 1;
    end
    // Valid is simply a transferred version of the 1x clock's valid. Delay it
    // one more cycle to align outputs.
    valid_8x     <= valid_in_8x_fall && enable_8x_fall;
    valid_dly_8x <= valid_8x;
  end

  assign adc_out_8x   = data_out_8x;
  assign valid_out_8x = valid_dly_8x;

endmodule

`default_nettype wire
