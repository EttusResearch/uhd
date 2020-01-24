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


module fifo_to_wb
  #(parameter PKT_LEN = 16)
   (input clk, input reset, input clear,
    input [18:0] data_i, input src_rdy_i, output dst_rdy_o,
    output [18:0] data_o, output src_rdy_o, input dst_rdy_i,
    output reg [15:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    input [7:0] triggers,
    output [31:0] debug0, output [31:0] debug1);
   
   wire [18:0] 	  ctrl_data;
   reg [18:0] 	  resp_data;
   wire 	  ctrl_src_rdy, ctrl_dst_rdy, resp_src_rdy, resp_dst_rdy;
   
   fifo_short #(.WIDTH(19)) ctrl_sfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o),
      .dataout(ctrl_data), .src_rdy_o(ctrl_src_rdy), .dst_rdy_i(ctrl_dst_rdy));
   
   fifo_short #(.WIDTH(19)) resp_sfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(resp_data), .src_rdy_i(resp_src_rdy), .dst_rdy_o(resp_dst_rdy),
      .dataout(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
   
   // Need a programmable state machine here.  The program is the fifo contents.
   // All words are 16 bits wide
   //    Word 0   Command  { Read, Write, Triggers[5:0], Seqno [7:0] }
   //    Word 1   Length
   //    Word 2   Address LSW
   //    Word 3   Address MSW (should all be 0)
   
   localparam RESP_IDLE = 0;
   localparam RESP_LEN = 1;
   localparam RESP_ADDR_LSW = 2;
   localparam RESP_ADDR_MSW = 3;
   localparam RESP_WRITE = 4;
   localparam RESP_DUMP = 5;
   localparam RESP_WAIT_READ = 6;
   localparam RESP_RCMD = 7;
   localparam RESP_RLEN = 8;
   localparam RESP_RADDR_LSW = 9;
   localparam RESP_RADDR_MSW = 10;
   localparam RESP_READ = 11;
   
   reg [3:0] 	  resp_state;
   reg 		  rd, wr;
   reg [15:0] 	  base_addr;
   reg [15:0] 	  length;
   reg [15:0] 	  count;
   reg [7:0] 	  seqnum;
   reg [5:0] 	  which_trig;
   
   always @(posedge clk)
     if(reset | clear)
       resp_state <= RESP_IDLE;
     else
       case(resp_state)
	 RESP_IDLE :
	   if(ctrl_src_rdy)
	     begin
		{ rd, wr, which_trig[5:0], seqnum[7:0] } <= ctrl_data[15:0];
		if(ctrl_data[16])    // WAIT for start of packet, clean out otherwise
		  resp_state <= RESP_LEN;
	     end
	 RESP_LEN :
	   if(ctrl_src_rdy)
	     begin
		length <= ctrl_data[15:0];
		count <= ctrl_data[15:0];
		resp_state <= RESP_ADDR_LSW;
	     end
	 RESP_ADDR_LSW :
	   if(ctrl_src_rdy)
	     begin
		base_addr <= ctrl_data[15:0];
		wb_adr_o <= ctrl_data[15:0];
		resp_state <= RESP_ADDR_MSW;
	     end
	 RESP_ADDR_MSW :
	   if(ctrl_src_rdy)
	     if(wr)
	       resp_state <= RESP_WRITE;
	     else
	       resp_state <= RESP_DUMP;
	 RESP_WRITE :
	   if(ctrl_src_rdy & wb_ack_i)
	     if(count==1)
	       if(ctrl_data[17]) //eof
		 resp_state <= RESP_IDLE;
	       else  // clean out padding
		 resp_state <= RESP_DUMP;
	     else
	       begin
		  wb_adr_o <= wb_adr_o + 2;
		  count <= count - 1;
	       end
	 RESP_DUMP :
	   if(ctrl_src_rdy & ctrl_data[17])
	     if(rd)
	       resp_state <= RESP_WAIT_READ;
	     else
	       resp_state <= RESP_IDLE;
	 RESP_WAIT_READ :
	   begin
	      wb_adr_o <= base_addr;
	      count <= length;
	      if( &(triggers | ~which_trig) )
		resp_state <= RESP_RCMD;
	   end
	 RESP_RCMD :
	   if(resp_dst_rdy)
	     resp_state <= RESP_RLEN;
	 RESP_RLEN :
	   if(resp_dst_rdy)
	     resp_state <= RESP_RADDR_LSW;
	 RESP_RADDR_LSW :
	   if(resp_dst_rdy)
	     resp_state <= RESP_RADDR_MSW;
	 RESP_RADDR_MSW :
	   if(resp_dst_rdy)
	     resp_state <= RESP_READ;
	 RESP_READ :
	   if(resp_dst_rdy & wb_ack_i)
	     if(count==1)
	       resp_state <= RESP_IDLE;
	     else
	       begin
		  wb_adr_o <= wb_adr_o + 2;
		  count <= count - 1;
	       end
       endcase // case (resp_state)

   always @*
     case(resp_state)
       RESP_RCMD : resp_data <= { 3'b001, 8'hAA, seqnum };
       RESP_RLEN : resp_data <= { 3'b000, length };
       RESP_RADDR_LSW : resp_data <= { 3'b000, base_addr };
       RESP_RADDR_MSW : resp_data <= { 3'b000, 16'd0 };
       default : resp_data <= { 1'b0, (count==1), 1'b0, wb_dat_miso };
     endcase // case (resp_state)
   
   assign ctrl_dst_rdy = (resp_state == RESP_IDLE) |
			 (resp_state == RESP_LEN) |
			 (resp_state == RESP_ADDR_LSW) |
			 (resp_state == RESP_ADDR_MSW) |
			 ((resp_state == RESP_WRITE) & wb_ack_i) |
			 (resp_state == RESP_DUMP);
   
   assign resp_src_rdy = (resp_state == RESP_RCMD) |
			 (resp_state == RESP_RLEN) |
			 (resp_state == RESP_RADDR_LSW) |
			 (resp_state == RESP_RADDR_MSW) |
			 ((resp_state == RESP_READ) & wb_ack_i);
   
   assign wb_dat_mosi = ctrl_data[15:0];
   assign wb_we_o = (resp_state == RESP_WRITE);
   assign wb_cyc_o = wb_stb_o;
   assign wb_sel_o = 2'b11;
   assign wb_stb_o = (ctrl_src_rdy & (resp_state == RESP_WRITE)) | (resp_dst_rdy & (resp_state == RESP_READ));
   
   
   assign debug0 = { 14'd0, ctrl_data[17:0] };
   assign debug1 = { ctrl_src_rdy, ctrl_dst_rdy, resp_src_rdy, resp_dst_rdy, 2'b00, ctrl_data[17:16] };
   
endmodule // fifo_to_wb
