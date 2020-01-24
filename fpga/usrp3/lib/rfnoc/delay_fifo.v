//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module delay_fifo
  #(parameter MAX_LEN=1023,
    parameter WIDTH=16)
   (input clk, input reset, input clear,
    input [$clog2(MAX_LEN+1)-1:0] len,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   reg [$clog2(MAX_LEN+1)-1:0] full_count;
   wire 		  full = full_count == len;
   
   wire 		  do_op = i_tvalid & o_tready;

   assign i_tready = o_tready;
   assign o_tvalid = i_tvalid;

   wire [WIDTH-1:0] 		    fifo_out;
   
   axi_fifo #(.WIDTH(WIDTH), .SIZE($clog2(MAX_LEN+1))) sample_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tvalid(do_op), .i_tready(),
      .o_tdata(fifo_out), .o_tvalid(), .o_tready(do_op&full));

   always @(posedge clk)
     if(reset | clear)
       full_count <= 0;
     else
       if(do_op & ~full)
	 full_count <= full_count + 1;     // FIXME careful if len changes during operation you must clear
   
   assign o_tdata = full ? fifo_out : 0;
   assign o_tlast = i_tlast;

endmodule // delay_fifo
