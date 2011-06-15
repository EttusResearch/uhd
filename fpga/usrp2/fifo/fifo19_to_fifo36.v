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

module fifo19_to_fifo36
  #(parameter LE=0)
   (input clk, input reset, input clear,
    input [18:0] f19_datain,
    input f19_src_rdy_i,
    output f19_dst_rdy_o,

    output [35:0] f36_dataout,
    output f36_src_rdy_o,
    input f36_dst_rdy_i,
    output [31:0] debug
    );
   
   // Shortfifo on input to guarantee no deadlock
   wire [18:0] 	  f19_data_int;
   wire 	  f19_src_rdy_int, f19_dst_rdy_int;
   
   fifo_short #(.WIDTH(19)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f19_datain), .src_rdy_i(f19_src_rdy_i), .dst_rdy_o(f19_dst_rdy_o),
      .dataout(f19_data_int), .src_rdy_o(f19_src_rdy_int), .dst_rdy_i(f19_dst_rdy_int),
      .space(),.occupied() );

   // Actual f19 to f36 which could deadlock if not connected to shortfifos
   reg 		  f36_sof_int, f36_eof_int;
   reg [1:0] 	  f36_occ_int;
   wire [35:0] 	  f36_data_int;
   wire 	  f36_src_rdy_int, f36_dst_rdy_int;
      
   reg [1:0] 	  state;
   reg [15:0] 	  dat0, dat1;

   wire 	  f19_sof_int  = f19_data_int[16];
   wire 	  f19_eof_int  = f19_data_int[17];
   wire 	  f19_occ_int  = f19_data_int[18];

   wire 	  xfer_out = f36_src_rdy_int & f36_dst_rdy_int;

   always @(posedge clk)
     if(f19_src_rdy_int & ((state==0)|xfer_out))
       f36_sof_int 	<= f19_sof_int;

   always @(posedge clk)
     if(f19_src_rdy_int & ((state != 2)|xfer_out))
       f36_eof_int 	<= f19_eof_int;

   always @(posedge clk)
     if(reset)
       begin
	  state 	<= 0;
	  f36_occ_int <= 0;
       end
     else
       if(f19_src_rdy_int)
	 case(state)
	   0 : 
	     begin
		dat0 <= f19_data_int;
		if(f19_eof_int)
		  begin
		     state <= 2;
		     f36_occ_int <= f19_occ_int ? 2'b01 : 2'b10;
		  end
		else
		  state <= 1;
	     end
	   1 : 
	     begin
		dat1 <= f19_data_int;
		state <= 2;
		if(f19_eof_int)
		  f36_occ_int <= f19_occ_int ? 2'b11 : 2'b00;
	     end
	   2 : 
	     if(xfer_out)
	       begin
		  dat0 <= f19_data_int;
		  if(f19_eof_int) // remain in state 2 if we are at eof
		    f36_occ_int <= f19_occ_int ? 2'b01 : 2'b10;
		  else
		    state 	   <= 1;
	       end
	 endcase // case(state)
       else
	 if(xfer_out)
	   begin
	      state 	   <= 0;
	      f36_occ_int <= 0;
	   end
   
   assign    f19_dst_rdy_int  = xfer_out | (state != 2);
   assign    f36_data_int     = LE ? {f36_occ_int,f36_eof_int,f36_sof_int,dat1,dat0} :
				{f36_occ_int,f36_eof_int,f36_sof_int,dat0,dat1};
   assign    f36_src_rdy_int  = (state == 2);

   assign    debug = state;

   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(36)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f36_data_int), .src_rdy_i(f36_src_rdy_int), .dst_rdy_o(f36_dst_rdy_int),
      .dataout(f36_dataout), .src_rdy_o(f36_src_rdy_o), .dst_rdy_i(f36_dst_rdy_i),
      .space(),.occupied() );
   
endmodule // fifo19_to_fifo36
