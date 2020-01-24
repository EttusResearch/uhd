
// Copyright 2012-2013 Ettus Research LLC
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

module dspengine_8to16
  #(parameter BASE = 0,
    parameter BUF_SIZE = 9,
    parameter HEADER_OFFSET = 0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    output access_we,
    output access_stb,
    input access_ok,
    output access_done,
    output access_skip_read,
    output [BUF_SIZE-1:0] access_adr,
    input [BUF_SIZE-1:0] access_len,
    output [35:0] access_dat_o,
    input [35:0] access_dat_i
    );

   wire 	 convert;
   
   setting_reg #(.my_addr(BASE),.width(1)) sr_8to16
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(convert),.changed());
   
   reg [3:0] 	 dsp_state;
   localparam DSP_IDLE = 0;
   localparam DSP_IDLE_RD = 1;
   localparam DSP_PARSE_HEADER = 2;
   localparam DSP_READ = 3;
   localparam DSP_READ_WAIT = 4;
   localparam DSP_WRITE_1 = 5;
   localparam DSP_WRITE_0 = 6;
   localparam DSP_READ_TRAILER = 7;
   localparam DSP_WRITE_TRAILER = 8;
   localparam DSP_WRITE_HEADER = 9;
   localparam DSP_DONE = 10;
   
   // Parse VITA header
   wire 	 is_if_data = (access_dat_i[31:29] == 3'b000);
   wire 	 has_streamid = access_dat_i[28];
   wire 	 has_classid = access_dat_i[27];
   wire 	 has_trailer = access_dat_i[26];
   // 25:24 reserved, aka SOB/EOB
   wire 	 has_secs = |access_dat_i[23:22];
   wire 	 has_tics = |access_dat_i[21:20];
   wire [3:0] 	 hdr_length = 1 + has_streamid + has_classid + has_classid + has_secs + has_tics + has_tics;
   reg [15:0] 	 hdr_length_reg;
 	 
   reg 		 odd;
   
   reg [BUF_SIZE-1:0] read_adr, write_adr;
   reg 		      has_trailer_reg;
   
   reg [31:0] 	      new_header, new_trailer, trailer_mask;
   reg 		      wait_for_trailer;
   reg [15:0] 	      data_in_len;
   wire       	      is_odd = access_dat_i[22] & access_dat_i[10];
   wire [15:0] 	      data_in_lenx2 = {data_in_len[14:0], 1'b0} - is_odd;

   reg [7:0] 	      i8_0, q8_0;
   wire [7:0] 	      i8_1 = access_dat_i[15:8];
   wire [7:0] 	      q8_1 = access_dat_i[7:0];
   reg 		      skip;
   

   always @(posedge clk)
     { i8_0, q8_0 } <= access_dat_i[31:16];
   
   always @(posedge clk)
     if(reset | clear)
       dsp_state <= DSP_IDLE;
     else
       case(dsp_state)
	 DSP_IDLE :
	   begin
	      read_adr <= HEADER_OFFSET;
	      write_adr <= HEADER_OFFSET;
	      if(access_ok)
		dsp_state <= DSP_IDLE_RD;
	   end

	 DSP_IDLE_RD: //extra idle state for read to become valid
		dsp_state <= DSP_PARSE_HEADER;

	 DSP_PARSE_HEADER :
	   begin
	      has_trailer_reg <= has_trailer;
	      new_header[31:0] <= access_dat_i[31:0];
	      hdr_length_reg <= hdr_length;
	      if(~is_if_data | ~convert | ~has_trailer)
		// ~convert is valid (16 bit mode) but both ~trailer and ~is_if_data are both
		// really error conditions on the TX side.  We shouldn't ever see them in the TX chain
		dsp_state <= DSP_WRITE_HEADER;  
	      else
		begin
		   read_adr <= access_dat_i[15:0] + HEADER_OFFSET - 1; // point to trailer
		   dsp_state <= DSP_READ_TRAILER;
		   wait_for_trailer <= 0;
		   data_in_len <= access_dat_i[15:0] - hdr_length - 1 /*trailer*/;
		end
	   end
	 
	 DSP_READ_TRAILER :
	   begin
	      wait_for_trailer <= 1;
	      if(wait_for_trailer)
		dsp_state <= DSP_WRITE_TRAILER;
	      new_trailer <= access_dat_i[31:0]; // Leave trailer unchanged
	      odd <= is_odd;
	      write_adr <= hdr_length_reg + data_in_lenx2 + HEADER_OFFSET;
	   end

	 DSP_WRITE_TRAILER :
	   begin
	      dsp_state <= DSP_READ;
	      write_adr <= write_adr - 1;
	      read_adr <= read_adr - 1;
	      new_header[15:0] <= write_adr + (1 - HEADER_OFFSET); // length = addr of trailer + 1
	   end

	 DSP_READ :
	   begin
	      read_adr <= read_adr - 1;
	      if(odd)
		dsp_state <= DSP_READ_WAIT;
	      else
		dsp_state <= DSP_WRITE_1;
	      odd <= 0;
	   end

	 DSP_READ_WAIT :
	   dsp_state <= DSP_WRITE_0;
	 
	 DSP_WRITE_1 :
	   begin
	      write_adr <= write_adr - 1;
	      if(write_adr == (hdr_length_reg+HEADER_OFFSET))
		begin
		   write_adr <= HEADER_OFFSET;
		   dsp_state <= DSP_WRITE_HEADER;
		end
	      dsp_state <= DSP_WRITE_0;
	   end

	 DSP_WRITE_0 :
	   begin
	      write_adr <= write_adr - 1;
	      if(write_adr == (hdr_length_reg+HEADER_OFFSET))
		begin
		   write_adr <= HEADER_OFFSET;
		   dsp_state <= DSP_WRITE_HEADER;
		end
	      else
		dsp_state <= DSP_READ;
	   end

	 DSP_WRITE_HEADER :
	   dsp_state <= DSP_DONE;

	 DSP_DONE :
	   begin
	      read_adr <= HEADER_OFFSET;
	      write_adr <= HEADER_OFFSET;
	      dsp_state <= DSP_IDLE;
	   end
       endcase // case (dsp_state)

   assign access_skip_read = 0;
   assign access_done = (dsp_state == DSP_DONE);

   assign access_stb = 1;

   assign access_we = (dsp_state == DSP_WRITE_HEADER) | 
		      (dsp_state == DSP_WRITE_TRAILER) |
		      (dsp_state == DSP_WRITE_0) |
		      (dsp_state == DSP_WRITE_1);
   
   assign access_dat_o = (dsp_state == DSP_WRITE_HEADER) ? { 4'h0, new_header } :
			 (dsp_state == DSP_WRITE_TRAILER) ? { 4'h2, new_trailer } :
			 (dsp_state == DSP_WRITE_0) ? { 4'h0, i8_0, 8'd0, q8_0, 8'd0 } :
			 (dsp_state == DSP_WRITE_1) ? { 4'h0, i8_1, 8'd0, q8_1, 8'd0 } :
			 34'h0DEADBEEF;
         
   assign access_adr = access_we ? write_adr : read_adr;
      
endmodule // dspengine_16to8
