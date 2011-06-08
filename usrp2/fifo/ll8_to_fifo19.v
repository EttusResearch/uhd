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


module ll8_to_fifo19
  (input clk, input reset, input clear,
   input [7:0] ll_data,
   input ll_sof,
   input ll_eof,
   input ll_src_rdy,
   output ll_dst_rdy,

   output [18:0] f19_data,
   output f19_src_rdy_o,
   input f19_dst_rdy_i );

   // Short FIFO on input to guarantee no deadlock
   wire [7:0] ll_data_int;
   wire       ll_sof_int, ll_eof_int, ll_src_rdy_int, ll_dst_rdy_int;
   
   ll8_shortfifo head_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(ll_data), .sof_i(ll_sof), .eof_i(ll_eof),
      .error_i(0), .src_rdy_i(ll_src_rdy), .dst_rdy_o(ll_dst_rdy),
      .dataout(ll_data_int), .sof_o(ll_sof_int), .eof_o(ll_eof_int),
      .error_o(), .src_rdy_o(ll_src_rdy_int), .dst_rdy_i(ll_dst_rdy_int));

   // Actual ll8_to_fifo19 which could deadlock if not connected to a shortfifo
   localparam XFER_EMPTY       = 0;
   localparam XFER_HALF        = 1;
   localparam XFER_HALF_WRITE  = 3;
   
   wire [18:0] f19_data_int;
   wire        f19_sof_int, f19_eof_int, f19_occ_int, f19_src_rdy_int, f19_dst_rdy_int;

   wire        xfer_out    = f19_src_rdy_int & f19_dst_rdy_int;
   wire        xfer_in 	   = ll_src_rdy_int & ll_dst_rdy_int; 
   reg 	       hold_sof;
   
   reg [1:0]   state;
   reg [7:0]   hold_reg;
   
   always @(posedge clk)
     if(ll_src_rdy_int & (state==XFER_EMPTY))
       hold_reg 	      <= ll_data_int;
   
   always @(posedge clk)
     if(ll_sof_int & (state==XFER_EMPTY))
       hold_sof 	      <= 1;
     else if(xfer_out)
       hold_sof 	      <= 0;
   
   always @(posedge clk)
     if(reset | clear)
       state 		      <= XFER_EMPTY;
     else
       case(state)
	 XFER_EMPTY :
	   if(ll_src_rdy_int)
	     if(ll_eof_int)
	       state 	      <= XFER_HALF_WRITE;
	     else
	       state 	      <= XFER_HALF;
	 XFER_HALF :
	   if(ll_src_rdy_int & f19_dst_rdy_int)
	       state 	      <= XFER_EMPTY;
         XFER_HALF_WRITE :
	   if(f19_dst_rdy_int)
	     state 	<= XFER_EMPTY;
       endcase // case (state)
      
   assign ll_dst_rdy_int = (state==XFER_EMPTY) | ((state==XFER_HALF)&f19_dst_rdy_int);
   assign f19_src_rdy_int= (state==XFER_HALF_WRITE) | ((state==XFER_HALF)&ll_src_rdy_int);
   
   assign f19_sof_int 	 = hold_sof | (ll_sof_int & (state==XFER_HALF));
   assign f19_eof_int 	 = (state == XFER_HALF_WRITE) | ll_eof_int;
   assign f19_occ_int 	 = (state == XFER_HALF_WRITE);
   
   assign f19_data_int 	 = {f19_occ_int,f19_eof_int,f19_sof_int,hold_reg,ll_data_int};

   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(19)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f19_data_int), .src_rdy_i(f19_src_rdy_int), .dst_rdy_o(f19_dst_rdy_int),
      .dataout(f19_data), .src_rdy_o(f19_src_rdy_o), .dst_rdy_i(f19_dst_rdy_i),
      .space(),.occupied() );
      
   
endmodule // ll8_to_fifo19

