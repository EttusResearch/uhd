//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module const_sreg
  #(parameter BASE=0,
    parameter WIDTH=32)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire [WIDTH-1:0]     const_val;
   
   setting_reg #(.my_addr(BASE), .width(WIDTH)) reg_max
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data), .out(const_val));

   assign o_tdata = const_val;
   assign o_tlast = 1'b0; // FIXME do we want something else here?
   assign o_tvalid = 1'b1; // caution -- will fill up a fifo
   
endmodule // const_sreg
