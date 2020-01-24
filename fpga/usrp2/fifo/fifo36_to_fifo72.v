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

module fifo36_to_fifo72
  #(parameter LE=0)
   (input clk, input reset, input clear,
    input [35:0] f36_datain,
    input f36_src_rdy_i,
    output f36_dst_rdy_o,

    output [71:0] f72_dataout,
    output f72_src_rdy_o,
    input f72_dst_rdy_i,
    output [31:0] debug
    );
   
   // Shortfifo on input to guarantee no deadlock
   wire [35:0] 	  f36_data_int;
   wire 	  f36_src_rdy_int, f36_dst_rdy_int;
   
   fifo_short #(.WIDTH(36)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f36_datain), .src_rdy_i(f36_src_rdy_i), .dst_rdy_o(f36_dst_rdy_o),
      .dataout(f36_data_int), .src_rdy_o(f36_src_rdy_int), .dst_rdy_i(f36_dst_rdy_int),
      .space(),.occupied() );

   // Actual f36 to f72 which could deadlock if not connected to shortfifos
   reg 		  f72_sof_int, f72_eof_int;
   reg [2:0] 	  f72_occ_int;
   wire [71:0] 	  f72_data_int;
   wire 	  f72_src_rdy_int, f72_dst_rdy_int;
      
   reg [1:0] 	  state;
   reg [31:0] 	  dat0, dat1;

   wire 	  f36_sof_int  = f36_data_int[32];
   wire 	  f36_eof_int  = f36_data_int[33];
   wire [1:0]	  f36_occ_int  = f36_data_int[35:34];

   wire 	  xfer_out = f72_src_rdy_int & f72_dst_rdy_int;

   always @(posedge clk)
     if(f36_src_rdy_int & ((state==0)|xfer_out))
       f72_sof_int 	<= f36_sof_int;

   always @(posedge clk)
     if(f36_src_rdy_int & ((state != 2)|xfer_out))
       f72_eof_int 	<= f36_eof_int;

   always @(posedge clk)
     if(reset)
       begin
	  state 	<= 0;
	  f72_occ_int   <= 0;
       end
     else
       if(f36_src_rdy_int)
	 case(state)
	   0 : 
	     begin
		dat0 <= f36_data_int;
		if(f36_eof_int)
		  begin
		     state <= 2;
		     case (f36_occ_int)
		       0 : f72_occ_int <= 3'd4;
		       1 : f72_occ_int <= 3'd1;
		       2 : f72_occ_int <= 3'd2;
		       3 : f72_occ_int <= 3'd3;
		     endcase // case (f36_occ_int)
		  end
		else
		  state <= 1;
	     end
	   1 : 
	     begin
		dat1 <= f36_data_int;
		state <= 2;
		if(f36_eof_int)
		  case (f36_occ_int)
		    0 : f72_occ_int <= 3'd0;
		    1 : f72_occ_int <= 3'd5;
		    2 : f72_occ_int <= 3'd6;
		    3 : f72_occ_int <= 3'd7;
		  endcase // case (f36_occ_int)
	     end
	   2 : 
	     if(xfer_out)
	       begin
		  dat0 <= f36_data_int;
		  if(f36_eof_int) // remain in state 2 if we are at eof
		    case (f36_occ_int)
		      0 : f72_occ_int <= 3'd4;
		      1 : f72_occ_int <= 3'd1;
		      2 : f72_occ_int <= 3'd2;
		      3 : f72_occ_int <= 3'd3;
		    endcase // case (f36_occ_int)
		  else
		    state 	   <= 1;
	       end
	 endcase // case(state)
       else
	 if(xfer_out)
	   begin
	      state 	   <= 0;
	      f72_occ_int  <= 0;
	   end
   
   assign    f36_dst_rdy_int  = xfer_out | (state != 2);
   assign    f72_data_int     = LE ? {3'b000,f72_occ_int[2:0],f72_eof_int,f72_sof_int,dat1,dat0} :
				{3'b000,f72_occ_int[2:0],f72_eof_int,f72_sof_int,dat0,dat1};
   assign    f72_src_rdy_int  = (state == 2);

   assign    debug = state;

   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(72)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f72_data_int), .src_rdy_i(f72_src_rdy_int), .dst_rdy_o(f72_dst_rdy_int),
      .dataout(f72_dataout), .src_rdy_o(f72_src_rdy_o), .dst_rdy_i(f72_dst_rdy_i),
      .space(),.occupied() );
   
endmodule // fifo36_to_fifo72
