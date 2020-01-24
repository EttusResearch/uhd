//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This block exists only to wrap the Xilinx IP which has a different interface
//   Xilinx puts Q in the high bits, I in the low bits, and inverts reset

module cmul
  (input clk, input reset,
   input [31:0] a_tdata, input a_tlast, input a_tvalid, output a_tready,
   input [31:0] b_tdata, input b_tlast, input b_tvalid, output b_tready,
   output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   complex_multiplier complex_multiplier
     (.aclk(clk), .aresetn(~reset),
      .s_axis_a_tdata({a_tdata[15:0], a_tdata[31:16]}), .s_axis_a_tlast(a_tlast), .s_axis_a_tvalid(a_tvalid), .s_axis_a_tready(a_tready),
      .s_axis_b_tdata({b_tdata[15:0], b_tdata[31:16]}), .s_axis_b_tlast(b_tlast), .s_axis_b_tvalid(b_tvalid), .s_axis_b_tready(b_tready),
      .s_axis_ctrl_tdata(8'd0), .s_axis_ctrl_tvalid(1'b1), .s_axis_ctrl_tready(),
      .m_axis_dout_tdata({o_tdata[31:0], o_tdata[63:32]}), .m_axis_dout_tlast(o_tlast), .m_axis_dout_tvalid(o_tvalid), .m_axis_dout_tready(o_tready));

endmodule // cmul
