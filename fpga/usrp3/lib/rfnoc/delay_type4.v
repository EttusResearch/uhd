//
// Copyright 2018 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This delay doesn't use a fifo, and solves pipeline bubble issues.
// fixes some issues that seemed to occur with delay_type2:
// - o_tvalid is set to 0 when delay_done is 0
// - added the clear signal
// - i_tvalid is a combinational input to incrementing delay_count

module delay_type4
  #(parameter MAX_LEN_LOG2=4,
    parameter WIDTH=16,
    parameter DELAY_VAL=0)
  (input clk, input reset, input clear,
    input [MAX_LEN_LOG2-1:0] len,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

  reg [MAX_LEN_LOG2-1:0] delay_count;

  wire delay_done = delay_count >= len;

  always @(posedge clk)
    if(reset | clear)
      delay_count <= 0;
    else
      if(~delay_done & i_tvalid & o_tready)
        delay_count <= delay_count + 1;

  assign o_tdata = delay_done ? i_tdata : DELAY_VAL;
  assign o_tlast = delay_done ? i_tlast : 1'b0;      // FIXME (carried over from delay_type2) think about this more, no answer is perfect in all situations
  assign o_tvalid = delay_done ? i_tvalid : 1'b0;
  assign i_tready = delay_done ? o_tready : 1'b0;

endmodule // delay_type4
