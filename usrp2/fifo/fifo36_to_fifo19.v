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

module fifo36_to_fifo19
  #(parameter LE=0)
   (input clk, input reset, input clear,
    input [35:0] f36_datain,
    input f36_src_rdy_i,
    output f36_dst_rdy_o,
    
    output [18:0] f19_dataout,
    output f19_src_rdy_o,
    input f19_dst_rdy_i );
      
   wire [18:0] f19_data_int;
   wire        f19_src_rdy_int, f19_dst_rdy_int;
   wire [35:0] f36_data_int;
   wire        f36_src_rdy_int, f36_dst_rdy_int;
   
   // Shortfifo on input to guarantee no deadlock
   fifo_short #(.WIDTH(36)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f36_datain), .src_rdy_i(f36_src_rdy_i), .dst_rdy_o(f36_dst_rdy_o),
      .dataout(f36_data_int), .src_rdy_o(f36_src_rdy_int), .dst_rdy_i(f36_dst_rdy_int),
      .space(),.occupied() );

   // Main fifo36_to_fifo19, needs shortfifos to guarantee no deadlock
   wire [1:0]  f36_occ_int  = f36_data_int[35:34];
   wire        f36_sof_int  = f36_data_int[32];
   wire        f36_eof_int  = f36_data_int[33];
   
   reg 	  phase;
   wire   half_line 	   = f36_eof_int & ((f36_occ_int==1)|(f36_occ_int==2));
   
   assign f19_data_int[15:0] = (LE ^ phase) ? f36_data_int[15:0] : f36_data_int[31:16];
   assign f19_data_int[16]   = phase ? 0 : f36_sof_int;
   assign f19_data_int[17]   = phase ? f36_eof_int : half_line;
   assign f19_data_int[18]   = f19_data_int[17] & ((f36_occ_int==1)|(f36_occ_int==3));
   
   assign f19_src_rdy_int    = f36_src_rdy_int;
   assign f36_dst_rdy_int    = (phase | half_line) & f19_dst_rdy_int;
   
   wire   f19_xfer 	   = f19_src_rdy_int & f19_dst_rdy_int;
   wire   f36_xfer 	   = f36_src_rdy_int & f36_dst_rdy_int;
   
   always @(posedge clk)
     if(reset)
       phase 		  <= 0;
     else if(f36_xfer)
       phase 		  <= 0;
     else if(f19_xfer)
       phase 		  <= 1;
   
   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(19)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f19_data_int), .src_rdy_i(f19_src_rdy_int), .dst_rdy_o(f19_dst_rdy_int),
      .dataout(f19_dataout), .src_rdy_o(f19_src_rdy_o), .dst_rdy_i(f19_dst_rdy_i),
      .space(),.occupied() );

endmodule // fifo36_to_fifo19
