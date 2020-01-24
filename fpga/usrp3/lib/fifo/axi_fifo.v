//
// Copyright 2012-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// General FIFO block
//  Size == 0: Uses a single stage flop (axi_fifo_flop).
//  Size == 1: Uses a two stage flop (axi_fifo_flop2). Best choice for single stage pipelining.
//             Breaks combinatorial paths on the AXI stream data / control lines at the cost of
//             additional registers. Maps to SLICELs (i.e. does not use distributed RAM).
//  Size <= 5: Uses SRL32 to efficient maps a 32 deep FIFO to SLICEMs (axi_fifo_short). Not
//             recommended for pipelining as most devices have twice as many SLICELs as SLICEMs.
//  Size  > 5: Uses BRAM fifo (axi_fifo_bram)

module axi_fifo
  #(parameter WIDTH=32, SIZE=5)
   (input clk, input reset, input clear,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output i_tready,
    output [WIDTH-1:0] o_tdata,
    output o_tvalid,
    input o_tready,
    
    output [15:0] space,
    output [15:0] occupied);
   
   generate
   if(SIZE==0)
   begin
      axi_fifo_flop #(.WIDTH(WIDTH)) fifo_flop
        (.clk(clk), .reset(reset), .clear(clear),
         .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .space(space[0]), .occupied(occupied[0]));
      assign space[15:1] = 15'd0;
      assign occupied[15:1] = 15'd0;
   end
   else if(SIZE==1)
   begin
      axi_fifo_flop2 #(.WIDTH(WIDTH)) fifo_flop2
        (.clk(clk), .reset(reset), .clear(clear),
         .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .space(space[1:0]), .occupied(occupied[1:0]));
      assign space[15:2] = 14'd0;
      assign occupied[15:2] = 14'd0;
   end
   else if(SIZE<=5)
   begin
      axi_fifo_short #(.WIDTH(WIDTH)) fifo_short
        (.clk(clk), .reset(reset), .clear(clear),
         .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .space(space[5:0]), .occupied(occupied[5:0]));
      assign space[15:6] = 10'd0;
      assign occupied[15:6] = 10'd0;
   end
   else
   begin
      axi_fifo_bram #(.WIDTH(WIDTH), .SIZE(SIZE)) fifo_bram
        (.clk(clk), .reset(reset), .clear(clear),
         .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .space(space), .occupied(occupied));
   end
   endgenerate
      
endmodule // axi_fifo
