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


// Parameter LE tells us if we are little-endian.  
// Little-endian means send lower 16 bits first.
// Default is big endian (network order), send upper bits first.

module fifo72_to_fifo36
  #(parameter LE=0)
   (input clk, input reset, input clear,
    input [71:0] f72_datain,
    input f72_src_rdy_i,
    output f72_dst_rdy_o,
    
    output [35:0] f36_dataout,
    output f36_src_rdy_o,
    input f36_dst_rdy_i );
      
   wire [35:0] f36_data_int;
   wire        f36_src_rdy_int, f36_dst_rdy_int;
   wire [71:0] f72_data_int;
   wire        f72_src_rdy_int, f72_dst_rdy_int;
   
   // Shortfifo on input to guarantee no deadlock
   fifo_short #(.WIDTH(72)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f72_datain), .src_rdy_i(f72_src_rdy_i), .dst_rdy_o(f72_dst_rdy_o),
      .dataout(f72_data_int), .src_rdy_o(f72_src_rdy_int), .dst_rdy_i(f72_dst_rdy_int),
      .space(),.occupied() );

   // Main fifo72_to_fifo36, needs shortfifos to guarantee no deadlock
   wire [2:0]  f72_occ_int  = f72_data_int[68:66];
   wire        f72_sof_int  = f72_data_int[64];
   wire        f72_eof_int  = f72_data_int[65];
   
   reg 	  phase;
   wire   half_line 	   = f72_eof_int & ( (f72_occ_int==1)|(f72_occ_int==2)|(f72_occ_int==3)|(f72_occ_int==4) );
      
   assign f36_data_int[31:0]   = (LE ^ phase) ? f72_data_int[31:0] : f72_data_int[63:32];
   assign f36_data_int[32]     = phase ? 0 : f72_sof_int;
   assign f36_data_int[33]     = phase ? f72_eof_int : half_line;
   assign f36_data_int[35:34]  = f36_data_int[33] ? f72_occ_int[1:0] : 2'b00;
      
   assign f36_src_rdy_int    = f72_src_rdy_int;
   assign f72_dst_rdy_int    = (phase | half_line) & f36_dst_rdy_int;
   
   wire   f36_xfer 	   = f36_src_rdy_int & f36_dst_rdy_int;
   wire   f72_xfer 	   = f72_src_rdy_int & f72_dst_rdy_int;
   
   always @(posedge clk)
     if(reset)
       phase 		  <= 0;
     else if(f72_xfer)
       phase 		  <= 0;
     else if(f36_xfer)
       phase 		  <= 1;
   
   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(36)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f36_data_int), .src_rdy_i(f36_src_rdy_int), .dst_rdy_o(f36_dst_rdy_int),
      .dataout(f36_dataout), .src_rdy_o(f36_src_rdy_o), .dst_rdy_i(f36_dst_rdy_i),
      .space(),.occupied() );

endmodule // fifo72_to_fifo36
