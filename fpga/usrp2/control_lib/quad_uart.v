//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


module quad_uart
  #(parameter TXDEPTH = 1,
    parameter RXDEPTH = 1)
    (input clk_i, input rst_i,
     input we_i, input stb_i, input cyc_i, output reg ack_o,
     input [4:0] adr_i, input [31:0] dat_i, output reg [31:0] dat_o,
     output [3:0] rx_int_o, output [3:0] tx_int_o, 
     output [3:0] tx_o, input [3:0] rx_i, output [3:0] baud_o
     );
   
   // Register Map
   localparam SUART_CLKDIV = 0;
   localparam SUART_TXLEVEL = 1;
   localparam SUART_RXLEVEL = 2;
   localparam SUART_TXCHAR = 3;
   localparam SUART_RXCHAR = 4;
   
   wire       wb_acc = cyc_i & stb_i;            // WISHBONE access
   wire       wb_wr  = wb_acc & we_i;            // WISHBONE write access
   
   reg [15:0] clkdiv[0:3];
   wire [7:0] rx_char[0:3];
   wire  [3:0] tx_fifo_full, rx_fifo_empty;   
   wire [7:0] tx_fifo_level[0:3], rx_fifo_level[0:3];

   always @(posedge clk_i)
     if (rst_i)
       ack_o 			    <= 1'b0;
     else
       ack_o 			    <= wb_acc & ~ack_o;

   integer    i;
   always @(posedge clk_i)
     if (rst_i)
       for(i=0;i<4;i=i+1)
	 clkdiv[i] <= 0;
     else if (wb_wr)
       case(adr_i[2:0])
	 SUART_CLKDIV : clkdiv[adr_i[4:3]] <= dat_i[15:0];
       endcase // case(adr_i)
   
   always @(posedge clk_i)
     case (adr_i[2:0])
       SUART_TXLEVEL : dat_o <= tx_fifo_level[adr_i[4:3]];
       SUART_RXLEVEL : dat_o <= rx_fifo_level[adr_i[4:3]];
       SUART_RXCHAR : dat_o <= rx_char[adr_i[4:3]];
     endcase // case(adr_i)

   genvar     j;
   generate
      for(j=0;j<4;j=j+1)
	begin : gen_uarts
	   simple_uart_tx #(.DEPTH(TXDEPTH)) simple_uart_tx
	    (.clk(clk_i),.rst(rst_i),
	     .fifo_in(dat_i[7:0]),.fifo_write(ack_o && wb_wr && (adr_i[2:0] == SUART_TXCHAR) && (adr_i[4:3]==j)),
	     .fifo_level(tx_fifo_level[j]),.fifo_full(tx_fifo_full[j]),
	     .clkdiv(clkdiv[j]),.baudclk(baud_o[j]),.tx(tx_o[j]));
	   
	   simple_uart_rx #(.DEPTH(RXDEPTH)) simple_uart_rx
	     (.clk(clk_i),.rst(rst_i),
	      .fifo_out(rx_char[j]),.fifo_read(ack_o && ~wb_wr && (adr_i[2:0] == SUART_RXCHAR) && (adr_i[4:3]==j)),
	      .fifo_level(rx_fifo_level[j]),.fifo_empty(rx_fifo_empty[j]),
	      .clkdiv(clkdiv[j]),.rx(rx_i[j]));
	end // block: gen_uarts
   endgenerate
   
   assign     tx_int_o 	= ~tx_fifo_full;  // Interrupt for those that have space
   assign     rx_int_o 	= ~rx_fifo_empty; // Interrupt for those that have data
   
endmodule // quad_uart
