//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//




module simple_uart_rx
    #(parameter SIZE=8)
    (input clk, input rst,
     output [7:0] fifo_out, input fifo_read, output [15:0] fifo_level, output fifo_empty,
     input [15:0] clkdiv, input rx);

   reg 		  rx_d1, rx_d2;
   always @(posedge clk)
     if(rst)
       {rx_d2,rx_d1} <= 0;
     else
       {rx_d2,rx_d1} <= {rx_d1,rx};

   reg [15:0] 	  baud_ctr;
   reg [3:0] 	  bit_ctr;
   reg [7:0] 	  sr;

   wire 	  neg_trans = rx_d2 & ~rx_d1;
   wire 	  shift_now = baud_ctr == (clkdiv>>1);
   wire 	  stop_now = (bit_ctr == 10) && shift_now;
   wire 	  go_now = (bit_ctr == 0) && neg_trans;

   always @(posedge clk)
     if(rst)
       sr <= 0;
     else if(shift_now)
       sr <= {rx_d2,sr[7:1]};

   always @(posedge clk)
     if(rst)
       baud_ctr <= 0;
     else
       if(go_now)
	 baud_ctr <= 1;
       else if(stop_now)
	 baud_ctr <= 0;
       else if(baud_ctr >= clkdiv)
	 baud_ctr <= 1;
       else if(baud_ctr != 0)
	 baud_ctr <= baud_ctr + 1;

   always @(posedge clk)
     if(rst)
       bit_ctr <= 0;
     else
       if(go_now)
	 bit_ctr <= 1;
       else if(stop_now)
	 bit_ctr <= 0;
       else if(baud_ctr == clkdiv)
	 bit_ctr <= bit_ctr + 1;

   wire 	  i_tready, o_tvalid;
   wire 	  full = ~i_tready;
   wire 	  write = ~full & stop_now;
   assign fifo_empty = ~o_tvalid;

   axi_fifo #(.WIDTH(8), .SIZE(SIZE)) fifo
     (.clk(clk),.reset(rst), .clear(1'b0),
      .i_tdata(sr),.i_tvalid(write),.i_tready(i_tready),
      .o_tdata(fifo_out),.o_tvalid(o_tvalid),.o_tready(fifo_read),
      .space(),.occupied(fifo_level) );

endmodule // simple_uart_rx
