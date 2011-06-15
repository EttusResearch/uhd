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



// Packet format --
//    Line 1 -- Length, 32 bits
//    Line 2 -- Sequence number, 32 bits
//    Last line -- CRC, 32 bits

module packet_verifier
  (input clk, input reset, input clear,
   input [7:0] data_i, input sof_i, input eof_i, input src_rdy_i, output dst_rdy_o,

   output reg [31:0] total, 
   output reg [31:0] crc_err, 
   output reg [31:0] seq_err, 
   output reg [31:0] len_err);

   reg [31:0] 	     seq_num;
   reg [31:0] 	     length;
   wire 	     first_byte, last_byte;
   reg 		     second_byte, last_byte_d1;
   wire 	     match_crc;
   wire 	     calc_crc = src_rdy_i & dst_rdy_o;
   
   crc crc(.clk(clk), .reset(reset), .clear(last_byte_d1), .data(data_i), 
	   .calc(calc_crc), .crc_out(), .match(match_crc));

   assign first_byte = src_rdy_i & dst_rdy_o & sof_i;
   assign last_byte = src_rdy_i & dst_rdy_o & eof_i;
   assign dst_rdy_o = ~last_byte_d1;

   // stubs for now
   wire 	     match_seq = 1;
   wire 	     match_len = 1;
   
   always @(posedge clk)
     if(reset | clear)
       last_byte_d1 <= 0;
     else 
       last_byte_d1 <= last_byte;

   always @(posedge clk)
     if(reset | clear)
       begin
	  total <= 0;
	  crc_err <= 0;
	  seq_err <= 0;
	  len_err <= 0;
       end
     else
       if(last_byte_d1)
	 begin
	    total <= total + 1;
	    if(~match_crc)
	      crc_err <= crc_err + 1;
	    else if(~match_seq)
	      seq_err <= seq_err + 1;
	    else if(~match_len)
	      seq_err <= len_err + 1;
	 end
   
endmodule // packet_verifier
