//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: align_samples
//
// Description:
//
//   This module shifts data words left (i_dir = 0) or right (i_dir = 1) by
//   i_shift samples in order to align samples into the desired position. The
//   module has a fixed latency of PIPE_IN+PIPE_OUT cycles in which i_push is
//   asserted. Here's an example of a left-shift of 2 with a pipeline delay of
//   one clock cycle:
//
//     Input | Output
//    -------|--------
//     ....  |  ....
//     3210  |  ....
//     7564  |  10..
//     BA98  |  5432
//     ....  |  9876
//     ....  |  ..BA
//     ....  |  ....
//
//   The output only updates when a new value is pushed using i_push. Otherwise
//   the output (o_data and o_user) remains the same.
//
//   It also includes a user input/output with timing that matches that of the
//   data. This can be used for any purpose.
//
//   The right shift is implemented as a left shift by SPC-i_shift samples.
//   This puts the data in the same position, but doesn't require the ability
//   to see into the future to get the data to shift in.
//
// Parameters:
//
//   SAMP_W   : Width of each sample.
//   SPC      : Number of samples per clock cycle.
//   USER_W   : Width of user input and output.
//   PIPE_IN  : Enable (1) or disable (0) a pipeline register on the input.
//   PIPE_OUT : Enable (1) or disable (0) a pipeline register on the output.
//
// Signals:
//
//   i_data   : Input data word to write next.
//   i_user   : Input user data.
//   i_push   : Assert for one clock cycle to write a new i_data input and
//              cause the next update on o_data.
//   i_dir    : Direction of shift. 0 = Left, 1 = Right.
//   i_shift  : Number of samples to shift.
//   i_cfg_en : Assert for one clock cycle to write new i_dir and i_shift
//              values.
//   o_data   : Shifted data stream, delayed by PIPE_IN+PIPE_OUT clock cycles.
//   o_user   : Identical to i_user, but with the same delay as the o_data path.
//

`default_nettype none


module align_samples #(
  parameter int SAMP_W   = 32,
  parameter int SPC      = 4,
  parameter int USER_W   = 1,
  parameter bit PIPE_IN  = 1,
  parameter bit PIPE_OUT = 1,

  localparam int DATA_W  = SPC*SAMP_W,
  localparam int SHIFT_W = $clog2(DATA_W/SAMP_W)
) (
  input wire clk,

  // Input Stream
  input  wire [ DATA_W-1:0] i_data,
  input  wire [ USER_W-1:0] i_user,
  input  wire               i_push,

  // Control
  input  wire               i_dir,
  input  wire [SHIFT_W-1:0] i_shift,
  input  wire               i_cfg_en,

  // Output Stream
  output wire [ DATA_W-1:0] o_data,
  output wire [ USER_W-1:0] o_user
);

  localparam int SHIFTER_W = (2*SPC-1)*SAMP_W;
  localparam int CARRY_W   = (SPC-1)*SAMP_W;

  //---------------------------------------------------------------------------
  // Input Register
  //---------------------------------------------------------------------------

  logic [DATA_W-1:0] i_data_reg;
  logic [USER_W-1:0] i_user_reg;
  logic              i_valid_reg;

  if (PIPE_IN) begin : gen_input_reg
    always_ff @(posedge clk) begin : input_pipeline
      if (i_push) begin
        i_data_reg  <= i_data;
        i_user_reg  <= i_user;
        i_valid_reg <= i_push;
      end
    end : input_pipeline
  end else begin : gen_no_input_reg
    assign i_data_reg  = i_data;
    assign i_user_reg  = i_user;
    assign i_valid_reg = i_push;
  end

  //---------------------------------------------------------------------------
  // Control Logic
  //---------------------------------------------------------------------------

  logic [SHIFT_W-1:0] i_shift_reg;
  logic               i_dir_reg;

  always_ff @(posedge clk) begin : input_pipeline
    if (i_cfg_en) begin
      i_dir_reg   <= i_dir;
      i_shift_reg <= i_shift;
    end
  end : input_pipeline

  //---------------------------------------------------------------------------
  // Shift Logic
  //---------------------------------------------------------------------------

  logic [SHIFTER_W-1:0] shifter;
  logic [  CARRY_W-1:0] carry_reg;

  logic [  SHIFT_W-1:0] shift;
  logic [   DATA_W-1:0] carry_mask;

  always_comb begin : shifter_comb
    // Convert a right shift to an equivalent left shift
    shift = i_dir_reg ? (SPC-i_shift_reg) : i_shift_reg;

    // Create a mask of the bits that need to be loaded from the previous clock
    // cycle.
    carry_mask = ((1 << shift*SAMP_W)-1);

    // Shift the input left, then OR it with the data we saved in the previous
    // clock cycle.
    shifter = (i_data_reg << (shift*SAMP_W)) | (carry_reg & carry_mask);
  end  : shifter_comb

  always_ff @(posedge clk) begin : carry_register
    // Save the upper CARRY_W bits for the next clock cycle
    if (i_push) begin
      carry_reg <= shifter[DATA_W +: CARRY_W];
    end
  end : carry_register

  //---------------------------------------------------------------------------
  // Output Register
  //---------------------------------------------------------------------------

  logic [DATA_W-1:0] o_data_reg;
  logic [USER_W-1:0] o_user_reg;

  if (PIPE_OUT) begin : gen_output_reg
    always_ff @(posedge clk) begin : output_pipeline
      if (i_push) begin
        o_data_reg  <= shifter[DATA_W-1:0];
        o_user_reg  <= i_user_reg;
      end
    end : output_pipeline
  end else begin : gen_no_output_reg
    assign o_data_reg  = shifter[DATA_W-1:0];
    assign o_user_reg  = i_user_reg;
  end

  assign o_data  = o_data_reg;
  assign o_user  = o_user_reg;

endmodule : align_samples


`default_nettype wire
