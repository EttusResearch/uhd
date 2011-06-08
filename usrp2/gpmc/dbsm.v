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


module bsm
  (input clk, input reset, input clear,
   input write_done,
   input read_done,
   output readable,
   output writeable);

   reg 	  state;
   localparam ST_WRITEABLE = 0;
   localparam ST_READABLE = 1;
   
   always @(posedge clk)
     if(reset | clear)
       state <= ST_WRITEABLE;
     else
       case(state)
	 ST_WRITEABLE :
	   if(write_done)
	     state <= ST_READABLE;
	 ST_READABLE :
	   if(read_done)
	     state <= ST_WRITEABLE;
       endcase // case (state)

   assign readable = (state == ST_READABLE);
   assign writeable = (state == ST_WRITEABLE);
   
endmodule // bsm

module dbsm
  (input clk, input reset, input clear,
   output reg read_sel, output read_ready, input read_done,
   output reg write_sel, output write_ready, input write_done);

   localparam NUM_BUFS = 2;

   wire       [NUM_BUFS-1:0] readable, writeable, read_done_buf, write_done_buf;
   
   // Two of these buffer state machines
   genvar     i;
   generate
      for(i=0;i<NUM_BUFS;i=i+1)
	begin : BSMS
	   bsm bsm(.clk(clk), .reset(reset), .clear(clear),
		   .write_done((write_sel == i) & write_done),
		   .read_done((read_sel == i) & read_done),
		   .readable(readable[i]), .writeable(writeable[i]));
	end
   endgenerate
   
   reg 	 full;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  write_sel <= 0;
	  full <= 0;
       end
     else
       if(write_done & writeable[write_sel])
	 if(write_sel ==(NUM_BUFS-1))
	   write_sel <= 0;
	 else
	   write_sel <= write_sel + 1;
   
   always @(posedge clk)
     if(reset | clear)
       read_sel <= 0;
     else
       if(read_done & readable[read_sel])
	 if(read_sel==(NUM_BUFS-1))
	   read_sel <= 0;
	 else
	   read_sel <= read_sel + 1;
          
   assign write_ready = writeable[write_sel];
   assign read_ready = readable[read_sel];

endmodule // dbsm
