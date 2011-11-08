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

module dbsm
  (input clk,
   input reset,
   input clear,
   
   output write_ok,
   output write_ptr,
   input write_done,

   output read_ok,
   output read_ptr,
   input read_done,

   output access_ok,
   output access_ptr,
   input access_done,
   input access_skip_read
   );

   localparam PORT_WAIT_0 = 0;
   localparam PORT_USE_0 = 1;
   localparam PORT_WAIT_1 = 2;
   localparam PORT_USE_1 = 3;

   reg [1:0] write_port_state, access_port_state, read_port_state;

   localparam BUFF_WRITABLE = 0;
   localparam BUFF_ACCESSIBLE = 1;
   localparam BUFF_READABLE = 2;
   localparam BUFF_ERROR = 3;

   wire [1:0] buff_state[0:1];
   
   always @(posedge clk)
     if(reset | clear)
       write_port_state <= PORT_WAIT_0;
     else
       case(write_port_state)
	 PORT_WAIT_0 :
	   if(buff_state[0]==BUFF_WRITABLE)
	     write_port_state <= PORT_USE_0;
	 PORT_USE_0 :
	   if(write_done)
	     write_port_state <= PORT_WAIT_1;
	 PORT_WAIT_1 :
	   if(buff_state[1]==BUFF_WRITABLE)
	     write_port_state <= PORT_USE_1;
	 PORT_USE_1 :
	   if(write_done)
	     write_port_state <= PORT_WAIT_0;
       endcase // case (write_port_state)

   assign write_ok = (write_port_state == PORT_USE_0) | (write_port_state == PORT_USE_1);
   assign write_ptr = (write_port_state == PORT_USE_1);
   
   always @(posedge clk)
     if(reset | clear)
       access_port_state <= PORT_WAIT_0;
     else
       case(access_port_state)
	 PORT_WAIT_0 :
	   if(buff_state[0]==BUFF_ACCESSIBLE)
	     access_port_state <= PORT_USE_0;
	 PORT_USE_0 :
	   if(access_done)
	     access_port_state <= PORT_WAIT_1;
	 PORT_WAIT_1 :
	   if(buff_state[1]==BUFF_ACCESSIBLE)
	     access_port_state <= PORT_USE_1;
	 PORT_USE_1 :
	   if(access_done)
	     access_port_state <= PORT_WAIT_0;
       endcase // case (access_port_state)
   
   assign access_ok = (access_port_state == PORT_USE_0) | (access_port_state == PORT_USE_1);
   assign access_ptr = (access_port_state == PORT_USE_1);

   always @(posedge clk)
     if(reset | clear)
       read_port_state <= PORT_WAIT_0;
     else
       case(read_port_state)
	 PORT_WAIT_0 :
	   if(buff_state[0]==BUFF_READABLE)
	     read_port_state <= PORT_USE_0;
	 PORT_USE_0 :
	   if(read_done)
	     read_port_state <= PORT_WAIT_1;
	 PORT_WAIT_1 :
	   if(buff_state[1]==BUFF_READABLE)
	     read_port_state <= PORT_USE_1;
	 PORT_USE_1 :
	   if(read_done)
	     read_port_state <= PORT_WAIT_0;
       endcase // case (read_port_state)
   
   assign read_ok = (read_port_state == PORT_USE_0) | (read_port_state == PORT_USE_1);
   assign read_ptr = (read_port_state == PORT_USE_1);

   buff_sm #(.PORT_USE_FLAG(PORT_USE_0)) buff0_sm
     (.clk(clk), .reset(reset), .clear(clear),
      .write_done(write_done), .access_done(access_done), .access_skip_read(access_skip_read), .read_done(read_done),
      .write_port_state(write_port_state), .access_port_state(access_port_state), .read_port_state(read_port_state),
      .buff_state(buff_state[0]));
   
   buff_sm #(.PORT_USE_FLAG(PORT_USE_1)) buff1_sm
     (.clk(clk), .reset(reset), .clear(clear),
      .write_done(write_done), .access_done(access_done), .access_skip_read(access_skip_read), .read_done(read_done),
      .write_port_state(write_port_state), .access_port_state(access_port_state), .read_port_state(read_port_state),
      .buff_state(buff_state[1]));
   
endmodule // dbsm

module buff_sm
  #(parameter PORT_USE_FLAG=0)
   (input clk, input reset, input clear,
    input write_done, input access_done, input access_skip_read, input read_done,
    input [1:0] write_port_state, input [1:0] access_port_state, input [1:0] read_port_state,
    output reg [1:0] buff_state);
   
   localparam BUFF_WRITABLE = 0;
   localparam BUFF_ACCESSIBLE = 1;
   localparam BUFF_READABLE = 2;
   localparam BUFF_ERROR = 3;

   always @(posedge clk)
     if(reset | clear)
       buff_state <= BUFF_WRITABLE;
     else
       case(buff_state)
	 BUFF_WRITABLE :
	   if(write_done & (write_port_state == PORT_USE_FLAG))
	     buff_state <= BUFF_ACCESSIBLE;
	 BUFF_ACCESSIBLE :
	   if(access_done & (access_port_state == PORT_USE_FLAG))
	     if(access_skip_read)
	       buff_state <= BUFF_WRITABLE;
	     else
	       buff_state <= BUFF_READABLE;
	 BUFF_READABLE :
	   if(read_done & (read_port_state == PORT_USE_FLAG))
	     buff_state <= BUFF_WRITABLE;
	 BUFF_ERROR :
	   ;
       endcase
	 
endmodule // buff_sm
