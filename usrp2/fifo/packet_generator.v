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



module packet_generator
  (input clk, input reset, input clear,
   output reg [7:0] data_o, output sof_o, output eof_o,
   input [127:0] header,
   output src_rdy_o, input dst_rdy_i);

   localparam len = 32'd2000;

   reg [31:0] state;
   reg [31:0] seq;
   reg [31:0] crc_out;
   wire        calc_crc = src_rdy_o & dst_rdy_i & ~(state[31:2] == 30'h3FFF_FFFF);
   
	
   always @(posedge clk)
     if(reset | clear)
       seq <= 0;
     else
       if(eof_o & src_rdy_o & dst_rdy_i)
	 seq <= seq + 1;
   
   always @(posedge clk)
     if(reset | clear)
       state <= 0;
     else
       if(src_rdy_o & dst_rdy_i)
	 if(state == (len - 1))
	   state <= 32'hFFFF_FFFC;
	 else
	   state <= state + 1;

   always @*
     case(state)
       0 :   data_o <= len[31:24];
       1 :   data_o <= len[23:16];
       2 :   data_o <= len[15:8];
       3 :   data_o <= len[7:0];
       4 :   data_o <= seq[31:24];
       5 :   data_o <= seq[23:16];
       6 :   data_o <= seq[15:8];
       7 :   data_o <= seq[7:0];
       8 :   data_o <= header[7:0];
       9 :   data_o <= header[15:8];
       10 :  data_o <= header[23:16];
       11 :  data_o <= header[31:24];
       12 :  data_o <= header[39:32];
       13 :  data_o <= header[47:40];
       14 :  data_o <= header[55:48];
       15 :  data_o <= header[63:56];
       16 :  data_o <= header[71:64];
       17 :  data_o <= header[79:72];
       18 :  data_o <= header[87:80];
       19 :  data_o <= header[95:88];
       20 :  data_o <= header[103:96];
       21 :  data_o <= header[111:104];
       22 :  data_o <= header[119:112];
       23 :  data_o <= header[127:120];

       32'hFFFF_FFFC : data_o <= crc_out[31:24];
       32'hFFFF_FFFD : data_o <= crc_out[23:16];
       32'hFFFF_FFFE : data_o <= crc_out[15:8];
       32'hFFFF_FFFF : data_o <= crc_out[7:0];
       default : data_o <= state[7:0];
     endcase // case (state)
   
   assign src_rdy_o = 1;
   assign sof_o = (state == 0);
   assign eof_o = (state == 32'hFFFF_FFFF);

   wire        clear_crc = eof_o & src_rdy_o & dst_rdy_i;
   
//   crc crc(.clk(clk), .reset(reset), .clear(clear_crc), .data(data_o), 
//	   .calc(calc_crc), .crc_out(crc_out), .match());
   always @(posedge clk)
     if(reset | clear | clear_crc)
       crc_out <= 0;
     else
       if(calc_crc)
	 crc_out <= crc_out + data_o;
   
endmodule // packet_generator
