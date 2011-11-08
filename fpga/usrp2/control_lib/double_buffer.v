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

module double_buffer
  #(parameter BUF_SIZE = 9)
   (input clk, input reset, input clear,

    // Random access interface to RAM
    input access_we,
    input access_stb,
    output access_ok,
    input access_done,
    input access_skip_read,
    input [BUF_SIZE-1:0] access_adr,
    output [BUF_SIZE-1:0] access_len,
    input [35:0] access_dat_i,   
    output [35:0] access_dat_o,
    
    // Write FIFO Interface
    input [35:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    
    // Read FIFO Interface
    output [35:0] data_o,
    output src_rdy_o,
    input dst_rdy_i
    );

   wire [35:0] data_o_0, data_o_1;

   wire        read, read_ok, read_ptr, read_done;
   wire        write, write_ok, write_ptr, write_done;
   
   wire [BUF_SIZE-1:0] rw0_adr, rw1_adr;
   reg [BUF_SIZE-1:0]  read_adr, write_adr;
   reg [BUF_SIZE-1:0]  len0, len1;
 
   assign data_o = read_ptr ? data_o_1 : data_o_0;
   assign rw0_adr = (write_ok & ~write_ptr) ? write_adr : read_adr;
   assign rw1_adr = (write_ok & write_ptr) ? write_adr : read_adr;
   
   wire [35:0] 	       access_dat_o_0, access_dat_o_1;
   wire 	       access_ptr;
   assign access_dat_o = access_ptr? access_dat_o_1 : access_dat_o_0;
   
   dbsm dbsm 
     (.clk(clk), .reset(reset), .clear(clear),
      .write_ok(write_ok), .write_ptr(write_ptr), .write_done(write_done),
      .access_ok(access_ok), .access_ptr(access_ptr), .access_done(access_done), .access_skip_read(access_skip_read),
      .read_ok(read_ok), .read_ptr(read_ptr), .read_done(read_done));
   
   // Port A for random access, Port B for FIFO read and write
   ram_2port #(.DWIDTH(36),.AWIDTH(BUF_SIZE)) buffer0
     (.clka(clk),.ena(access_stb & access_ok & (access_ptr == 0)),.wea(access_we),
      .addra(access_adr),.dia(access_dat_i),.doa(access_dat_o_0),
      .clkb(clk),.enb((read & read_ok & ~read_ptr)|(write & write_ok & ~write_ptr) ),.web(write&write_ok&~write_ptr),
      .addrb(rw0_adr),.dib(data_i),.dob(data_o_0));
   
   ram_2port #(.DWIDTH(36),.AWIDTH(BUF_SIZE)) buffer1
     (.clka(clk),.ena(access_stb & access_ok & (access_ptr == 1)),.wea(access_we),
      .addra(access_adr),.dia(access_dat_i),.doa(access_dat_o_1),
      .clkb(clk),.enb((read & read_ok & read_ptr)|(write & write_ok & write_ptr) ),.web(write&write_ok&write_ptr),
      .addrb(rw1_adr),.dib(data_i),.dob(data_o_1));

   // Write into buffers
   assign dst_rdy_o =  write_ok;
   assign write = src_rdy_i & write_ok;
   assign write_done = write & data_i[33]; // done
   always @(posedge clk)
     if(reset | clear)
       write_adr <= 0;
     else
       if(write_done)
	 begin
	    write_adr <= 0;
	    if(write_ptr)
	      len1 <= write_adr + 1;
	    else
	      len0 <= write_adr + 1;
	 end
       else if(write)
	 write_adr <= write_adr + 1;

   assign access_len = access_ptr ? len1 : len0;
   
   reg [1:0] 	       read_state;
   localparam IDLE = 0;
   localparam PRE_READ = 1;
   localparam READING = 2;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  read_state <= IDLE;
	  read_adr <= 0;
       end
     else
       case(read_state)
	 IDLE :
	   begin
	      read_adr <= 0;
	      if(read_ok)
		read_state <= PRE_READ;
	   end
	 PRE_READ :
	   begin
	      read_state <= READING;
	      read_adr <= 1;
	   end
	 
	 READING :
	   if(dst_rdy_i)
	     begin
		read_adr <= read_adr + 1;
		if(data_o[33])
		  read_state <= IDLE;
	     end
       endcase // case (read_state)
      
   assign read = ~((read_state==READING)& ~dst_rdy_i);
   assign read_done = data_o[33] & dst_rdy_i & src_rdy_o;
   assign src_rdy_o = (read_state == READING);
   
endmodule // double_buffer
