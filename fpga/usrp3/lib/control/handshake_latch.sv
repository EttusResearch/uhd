//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: handshake_latch
//
// Description:
//
//   This is a wrapper for handshake that automatically latches the output each
//   time it updates. See handshake.v for details.
//
//   For example, suppose you don't care about the valid signals, and just need a
//   copy of one bus from the clk_a domain in the clk_b domain. In that case, you
//   could do this:
//
//   handshake_latch #(
//     .WIDTH(32)
//   ) handshake_i (
//     .clk_a  (clk_a       ),
//     .rst_a  (0           ),
//     .valid_a(1           ),    // Always update as soon as possible
//     .data_a (my_sig_clk_a),
//     .busy_a (            ),
//     .clk_b  (clk_b       ),
//     .valid_b(            ),
//     .data_b (my_sig_clk_b)
//   );
//
//   In this case, the valid_b output can be ignored, but it will pulse every
//   time the output updates.
//
// Parameters:
//
//   WIDTH : Width of the data bus to handshake
//

`default_nettype none


module handshake_latch #(
  parameter int WIDTH = 32
) (
  // Source clock domain
  input  wire              clk_a,
  input  wire              rst_a,
  input  wire              valid_a,
  input  wire  [WIDTH-1:0] data_a,
  output logic             busy_a,

  // Target clock domain
  input  wire              clk_b,
  output logic             valid_b,
  output logic [WIDTH-1:0] data_b
);
  logic             valid_b_tmp;
  logic [WIDTH-1:0] data_b_tmp;

  handshake #(
    .WIDTH(WIDTH)
  ) handshake_i (
    .clk_a  (clk_a      ),
    .rst_a  (rst_a      ),
    .valid_a(valid_a    ),
    .data_a (data_a     ),
    .busy_a (busy_a     ),
    .clk_b  (clk_b      ),
    .valid_b(valid_b_tmp),
    .data_b (data_b_tmp )
  );

  // Using the ASYNC_REG property effectively turns this into the last stage of
  // a triple synchronizer.
  (* ASYNC_REG = "TRUE" *) reg [WIDTH-1:0] data_b_reg;

  always_ff @(posedge clk_b) begin : latch_reg
    valid_b <= valid_b_tmp;
    if (valid_b_tmp) begin
      data_b_reg <= data_b_tmp;
    end
  end : latch_reg

  assign data_b = data_b_reg;

endmodule : handshake_latch


`default_nettype wire
