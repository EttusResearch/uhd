
// Copyright 2011-2012 Ettus Research LLC
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

module dspengine_16to8
  #(parameter BASE = 0,
    parameter BUF_SIZE = 9)
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

   wire convert;
   setting_reg #(.my_addr(BASE),.width(1)) sr_16to8
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(convert),.changed());

   reg [2:0] 	 dsp_state;
   localparam DSP_IDLE = 0;
   localparam DSP_PARSE_HEADER = 1;
   localparam DSP_CONVERT = 2;
   localparam DSP_CONVERT_DRAIN_PIPE = 3;
   localparam DSP_READ_TRAILER = 4;
   localparam DSP_WRITE_TRAILER = 5;
   localparam DSP_WRITE_HEADER = 6;
   localparam DSP_DONE = 7;

   // Parse VITA header
   wire 	 is_if_data = (access_dat_i[31:29] == 3'b000);
   wire 	 has_streamid = access_dat_i[28];
   wire 	 has_classid = access_dat_i[27];
   wire 	 has_trailer = access_dat_i[26];
   // 25:24 reserved, aka SOB/EOB
   wire 	 has_secs = |access_dat_i[23:22];
   wire 	 has_tics = |access_dat_i[21:20];
   wire [3:0] 	 hdr_length = 1 + has_streamid + has_classid + has_classid + has_secs + has_tics + has_tics;
   
   wire [35:0] 	 prod_i, prod_q;
   wire [15:0] 	 scaled_i, scaled_q;
   wire [7:0] 	 i8, q8;
   reg [7:0] 	 i8_reg, q8_reg;
   wire 	 stb_read, stb_clip, val_read, val_clip;
   wire 	 stb_out, stb_reg;
   reg 		 even;
   
   reg [BUF_SIZE-1:0] read_adr, write_adr;
   reg 		      has_trailer_reg;
   
   wire 	      last = (read_adr + 1) == (access_len - has_trailer_reg);
   wire 	      last_o, even_o;

   wire 	      stb_write = stb_out & (even_o | last_o);
   wire 	      send_to_pipe = ~stb_write & (dsp_state == DSP_CONVERT);
   reg [31:0] 	      new_header, new_trailer, trailer_mask;
   reg [15:0] 	      length;
   reg 		      wait_for_trailer;
   
   always @(posedge clk)
     if(reset | clear)
       dsp_state <= DSP_IDLE;
     else
       case(dsp_state)
	 DSP_IDLE :
	   begin
	      read_adr <= 0;
	      write_adr <= 0;
	      even <= 0;
	      if(access_ok)
		dsp_state <= DSP_PARSE_HEADER;
	   end
	 
	 DSP_PARSE_HEADER :
	   begin
	      has_trailer_reg <= has_trailer;
	      new_header[31:16] <= access_dat_i[31:16];
	      new_header[15:0] <= access_len;
	      length <= access_len;
	      if(is_if_data & convert)
		begin
		   read_adr <= hdr_length;
		   write_adr <= hdr_length;
		   dsp_state <= DSP_CONVERT;
		end
	      else
		dsp_state <= DSP_WRITE_HEADER;
	   end
	 
	 DSP_CONVERT:
	   begin
	      new_header[26] <= 1'b1;  // all converted packets have a trailer
	      if(stb_write)
		write_adr <= write_adr + 1;
	      else if(stb_read)   // should always be 1 if we are here
		begin
		   read_adr <= read_adr + 1;
		   even <= ~even;
		   if(last)
		     begin
			dsp_state <= DSP_CONVERT_DRAIN_PIPE;
			if(~even)
			  trailer_mask <= 32'h00400400;
			else
			  trailer_mask <= 32'h00400000;
		     end
		end
	   end
	 
	 DSP_CONVERT_DRAIN_PIPE :
	   if(stb_write)
	     begin
		write_adr <= write_adr + 1;
		if(last_o)
		  if(has_trailer_reg)
		    begin
		       dsp_state <= DSP_READ_TRAILER;
		       wait_for_trailer <= 0;
		    end
		  else
		    begin
		       dsp_state <= DSP_WRITE_TRAILER;
		       new_trailer <= trailer_mask;
		    end
	     end

	 DSP_READ_TRAILER :
	   begin
	      wait_for_trailer <= 1;
	      if(wait_for_trailer)
		dsp_state <= DSP_WRITE_TRAILER;
	      new_trailer <= access_dat_i[31:0] | trailer_mask;
	   end

	 DSP_WRITE_TRAILER :
	   begin
	      dsp_state <= DSP_WRITE_HEADER;
	      write_adr <= 0;
	      new_header[15:0] <= write_adr + 1;
	   end

	 DSP_WRITE_HEADER :
	   dsp_state <= DSP_DONE;

	 DSP_DONE :
	   begin
	      read_adr <= 0;
	      write_adr <= 0;
	      dsp_state <= DSP_IDLE;
	   end
       endcase // case (dsp_state)

   assign access_skip_read = 0;
   assign access_done = (dsp_state == DSP_DONE);

   assign access_stb = 1;

   assign access_we = (dsp_state == DSP_WRITE_HEADER) | 
		      (dsp_state == DSP_WRITE_TRAILER) |
		      stb_write;

   assign access_dat_o = (dsp_state == DSP_WRITE_HEADER) ? { 4'h1, new_header } :
			 (dsp_state == DSP_WRITE_TRAILER) ? { 4'h2, new_trailer } :
			 (last_o&~even_o) ? {4'h0, 16'd0, i8, q8 } : 
			 {4'h0, i8, q8, i8_reg, q8_reg };
   
   assign access_adr = (stb_write|(dsp_state == DSP_WRITE_HEADER)|(dsp_state == DSP_WRITE_TRAILER)) ? write_adr : read_adr;
      
   // DSP Pipeline
   
   wire [15:0] i16 = access_dat_i[31:16];
   wire [15:0] q16 = access_dat_i[15:0];

   pipectrl #(.STAGES(2), .TAGWIDTH(2)) pipectrl
     (.clk(clk), .reset(reset),
      .src_rdy_i(send_to_pipe), .dst_rdy_o(), // dst_rdy_o will always be 1 since dst_rdy_i is 1, below
      .src_rdy_o(stb_out), .dst_rdy_i(1),   // always accept output of chain
      .strobes({stb_clip,stb_read}), .valids({val_clip,val_read}),
      .tag_i({last,even}), .tag_o({last_o,even_o}));

   always @(posedge clk)
     if(stb_out & ~even_o)
       {i8_reg,q8_reg} <= {i8,q8};

   clip_reg #(.bits_in(16),.bits_out(8),.STROBED(1)) clip_i
     (.clk(clk), .in(i16), .out(i8), .strobe_in(stb_clip), .strobe_out());

   clip_reg #(.bits_in(16),.bits_out(8),.STROBED(1)) clip_q
     (.clk(clk), .in(q16), .out(q8), .strobe_in(stb_clip), .strobe_out());

endmodule // dspengine_16to8
