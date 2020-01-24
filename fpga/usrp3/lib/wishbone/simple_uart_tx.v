//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//



module simple_uart_tx
    #(parameter SIZE=0)
    (input clk, input rst,
     input [7:0] fifo_in, input fifo_write, output [15:0] fifo_level, output fifo_full, 
     input [15:0] clkdiv, output baudclk, output reg tx);

   reg [15:0] 	  baud_ctr;
   reg [3:0] 	  bit_ctr;

   wire 	  read, empty;
   wire [7:0] 	  char_to_send;
   wire 	  i_tready, o_tvalid;

   assign fifo_full = ~i_tready;
   assign empty = ~o_tvalid;

   axi_fifo #(.WIDTH(8), .SIZE(SIZE)) fifo
     (.clk(clk),.reset(rst), .clear(1'b0),
      .i_tdata(fifo_in), .i_tvalid(fifo_write), .i_tready(i_tready),
      .o_tdata(char_to_send),.o_tvalid(o_tvalid),.o_tready(read),
      .space(fifo_level),.occupied() );

   always @(posedge clk)
     if(rst)
       baud_ctr <= 0;
     else if (baud_ctr >= clkdiv)
       baud_ctr <= 1;
     else
       baud_ctr <= baud_ctr + 1;

   always @(posedge clk)
     if(rst)
       bit_ctr <= 0;
     else if(baud_ctr == clkdiv)
       if(bit_ctr == 10)
	 bit_ctr <= 0;
       else if(bit_ctr != 0)
	 bit_ctr <= bit_ctr + 1;
       else if(~empty)
	 bit_ctr <= 1;

   always @(posedge clk)
     if(rst)
       tx <= 1;
     else
       case(bit_ctr)
	 0 : tx <= 1;
	 1 : tx <= 0;
	 2 : tx <= char_to_send[0];
	 3 : tx <= char_to_send[1];
	 4 : tx <= char_to_send[2];
	 5 : tx <= char_to_send[3];
	 6 : tx <= char_to_send[4];
	 7 : tx <= char_to_send[5];
	 8 : tx <= char_to_send[6];
	 9 : tx <= char_to_send[7];
	 default : tx <= 1;
       endcase // case(bit_ctr)

   assign 	  read = (bit_ctr == 9) && (baud_ctr == clkdiv);
   assign 	  baudclk = (baud_ctr == 1);  // Only for debug purposes

endmodule // simple_uart_tx
