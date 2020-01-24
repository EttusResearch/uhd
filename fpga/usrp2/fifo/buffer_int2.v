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


// FIFO Interface to the 2K buffer RAMs
// Read port is read-acknowledge
// FIXME do we want to be able to interleave reads and writes?

module buffer_int2
  #(parameter BASE = 0,
    parameter BUF_SIZE = 9)
    (input clk, input rst,
     input set_stb, input [7:0] set_addr, input [31:0] set_data,
     output [31:0] status,

     // Wishbone interface to RAM
     input wb_clk_i,
     input wb_rst_i,
     input wb_we_i,
     input wb_stb_i,
     input [15:0] wb_adr_i,
     input [31:0] wb_dat_i,   
     output [31:0] wb_dat_o,
     output reg wb_ack_o,

     // Write FIFO Interface
     input [35:0] wr_data_i,
     input wr_ready_i,
     output wr_ready_o,
     
     // Read FIFO Interface
     output [35:0] rd_data_o,
     output rd_ready_o,
     input rd_ready_i
     );

   reg [15:0]         rd_addr, wr_addr;    // Handle pkt bigger than buffer
   wire [15:0] 	      rd_addr_next = rd_addr + 1;
   reg [15:0] 	      rd_length;

   wire [31:0] 	      ctrl;
   wire 	      wr_done, wr_error, wr_idle;
   wire 	      rd_done, rd_error, rd_idle;
   wire 	      we, en, go;

   wire 	      read = ctrl[3];
   wire 	      rd_clear = ctrl[2];
   wire 	      write = ctrl[1];
   wire 	      wr_clear = ctrl[0];
   
   reg [2:0] 	      rd_state, wr_state;
   reg 		      rd_sop, rd_eop;
   wire 	      wr_sop, wr_eop;
   reg [1:0] 	      rd_occ;
   wire [1:0] 	      wr_occ;
   
   localparam IDLE = 3'd0;
   localparam PRE_READ = 3'd1;
   localparam READING = 3'd2;
   localparam WRITING = 3'd3;
   localparam ERROR = 3'd4;
   localparam DONE = 3'd5;

   // read state machine
   always @(posedge clk)
     if(rst | (rd_clear & go))
       begin
	  rd_state <= IDLE;
	  rd_sop <= 0;
	  rd_eop <= 0;
	  rd_occ <= 0;
       end
     else
       case(rd_state)
	 IDLE :
	   if(go & read)
	     begin
		rd_addr <= 0;
		rd_state <= PRE_READ;
		rd_length <= ctrl[31:16];
	     end
	 
	 PRE_READ :
	   begin
	      rd_state <= READING;
	      rd_addr <= rd_addr_next;
	      rd_occ <= 2'b00;
	      rd_sop <= 1;
	      rd_eop <= 0;
	   end
	 
	 READING :
	   if(rd_ready_i)
	     begin
		rd_sop <= 0;
		rd_addr <= rd_addr_next;
		if(rd_addr_next == rd_length)
		  begin
		     rd_eop <= 1;
		     // FIXME assign occ here
		     rd_occ <= 0;
		  end
		else
		  rd_eop <= 0;
		if(rd_eop)
		  rd_state <= DONE;
	     end
	 
       endcase // case(rd_state)
   
   // write state machine
   always @(posedge clk)
     if(rst | (wr_clear & go))
       wr_state <= IDLE;
     else 
       case(wr_state)
	 IDLE :
	   if(go & write)
	     begin
		wr_addr <= 0;
		wr_state <= WRITING;
	     end
	 
	 WRITING :
	   if(wr_ready_i)
	     begin
		wr_addr <= wr_addr + 1;
		if(wr_sop & wr_eop)
		  wr_state <= ERROR;  // Should save OCC flags here
		else if(wr_eop)
		  wr_state <= DONE;
	     end // if (wr_ready_i)
       endcase // case(wr_state)
   
   assign     rd_data_o[35:32] = { rd_occ[1:0], rd_eop, rd_sop };
   assign     rd_ready_o = (rd_state == READING);
   
   assign     wr_sop = wr_data_i[32];
   assign     wr_eop = wr_data_i[33];
   assign     wr_occ = wr_data_i[35:34];
   assign     wr_ready_o = (wr_state == WRITING);

   assign     we = (wr_state == WRITING); // always write to avoid timing issue
   assign     en = ~((rd_state==READING)& ~rd_ready_i);   // FIXME potential critical path
   
   assign     rd_done = (rd_state == DONE);
   assign     wr_done = (wr_state == DONE);
   assign     rd_error = (rd_state == ERROR);
   assign     wr_error = (wr_state == ERROR);
   assign     rd_idle = (rd_state == IDLE);
   assign     wr_idle = (wr_state == IDLE);

   wire [BUF_SIZE-1:0] wr_addr_clip = (|wr_addr[15:BUF_SIZE]) ? {BUF_SIZE{1'b1}} : wr_addr[BUF_SIZE-1:0];
   
   ram_2port #(.DWIDTH(32),.AWIDTH(BUF_SIZE)) buffer_in // CPU reads here
     (.clka(wb_clk_i),.ena(wb_stb_i),.wea(1'b0),
      .addra(wb_adr_i[BUF_SIZE+1:2]),.dia(0),.doa(wb_dat_o),
      .clkb(clk),.enb(1'b1),.web(we),
      .addrb(wr_addr_clip),.dib(wr_data_i[31:0]),.dob());
   
   ram_2port #(.DWIDTH(32),.AWIDTH(BUF_SIZE)) buffer_out // CPU writes here
     (.clka(wb_clk_i),.ena(wb_stb_i),.wea(wb_we_i),
      .addra(wb_adr_i[BUF_SIZE+1:2]),.dia(wb_dat_i),.doa(),
      .clkb(clk),.enb(en),.web(1'b0),
      .addrb(rd_addr[BUF_SIZE-1:0]),.dib(0),.dob(rd_data_o[31:0]));
   
   always @(posedge wb_clk_i)
     if(wb_rst_i)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

   setting_reg #(.my_addr(BASE)) 
   sreg(.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),.in(set_data),
	.out(ctrl),.changed(go));
   
   assign status = { wr_addr,
		     8'b0,1'b0,rd_idle,rd_error,rd_done, 1'b0,wr_idle,wr_error,wr_done};

endmodule // buffer_int2
