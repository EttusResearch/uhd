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


module prot_eng_tx
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [35:0] datain, input src_rdy_i, output dst_rdy_o,
    output [35:0] dataout, output src_rdy_o, input dst_rdy_i);

   wire 	  src_rdy_int1, dst_rdy_int1;
   wire 	  src_rdy_int2, dst_rdy_int2;
   wire [35:0] 	  data_int1, data_int2;

   // Shortfifo on input to guarantee no deadlock
   fifo_short #(.WIDTH(36)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(datain), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o),
      .dataout(data_int1), .src_rdy_o(src_rdy_int1), .dst_rdy_i(dst_rdy_int1),
      .space(),.occupied() );
   
   // Store header values in a small dual-port (distributed) ram
   reg [31:0] 	  header_ram[0:63];
   reg [3:0] 	  state;
   reg [1:0] 	  port_sel;
   
   always @(posedge clk)
     if(set_stb & ((set_addr & 8'hC0) == BASE))
       header_ram[set_addr[5:0]] <= set_data;

   wire [31:0] 	  header_word = header_ram[{port_sel[1:0],state[3:0]}];

   reg [15:0] 	  pre_checksums [0:3];
   always @(posedge clk)
     if(set_stb & ((set_addr & 8'hCF)== (BASE+7)))
       pre_checksums[set_addr[5:4]] <= set_data[15:0];

   wire [15:0] 	  pre_checksum = pre_checksums[port_sel[1:0]];

   // Protocol State Machine
   reg [15:0] length;
   wire [15:0] ip_length = length + 28;  // IP HDR + UDP HDR
   wire [15:0] udp_length = length + 8;  //  UDP HDR
   reg 	       sof_o;
   reg [31:0]  prot_data;
   
   always @(posedge clk)
     if(reset)
       begin
	  state   <= 0;
	  sof_o   <= 0;
       end
     else
       if(src_rdy_int1 & dst_rdy_int2)
	 case(state)
	   0 :
	     begin
		port_sel <= data_int1[18:17];
		length 	<= data_int1[15:0];
		sof_o <= 1;
		if(data_int1[16])
		  state <= 1;
		else
		  state <= 12;
	     end
	   12 :
	     begin
		sof_o <= 0;
		if(data_int1[33]) // eof
		  state <= 0;
	     end
	   default :
	     begin
		sof_o 	<= 0;
		state <= state + 1;
	     end
	 endcase // case (state)

   wire [15:0] ip_checksum;
   add_onescomp #(.WIDTH(16)) add_onescomp 
     (.A(pre_checksum),.B(ip_length),.SUM(ip_checksum));
   reg [15:0]  ip_checksum_reg;
   always @(posedge clk) ip_checksum_reg <= ip_checksum;
   
   always @*
     case(state)
       1 : prot_data <= header_word;  // ETH, top half ignored
       2 : prot_data <= header_word;  // ETH
       3 : prot_data <= header_word;  // ETH
       4 : prot_data <= header_word;  // ETH
       5 : prot_data <= { header_word[31:16], ip_length }; // IP
       6 : prot_data <= header_word; // IP
       7 : prot_data <= { header_word[31:16], (16'hFFFF ^ ip_checksum_reg) }; // IP
       8 : prot_data <= header_word; // IP
       9 : prot_data <= header_word; // IP
       10: prot_data <= header_word;  // UDP 
       11: prot_data <= { udp_length, header_word[15:0]}; // UDP
       default : prot_data <= data_int1[31:0];
     endcase // case (state)

   assign data_int2 = { data_int1[35:33] & {3{state[3]}},  sof_o, prot_data };
   assign dst_rdy_int1 = dst_rdy_int2 & ((state == 0) | (state == 12));
   assign src_rdy_int2 = src_rdy_int1 & (state != 0);
   
   // Shortfifo on output to guarantee no deadlock
   fifo_short #(.WIDTH(36)) tail_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(data_int2), .src_rdy_i(src_rdy_int2), .dst_rdy_o(dst_rdy_int2),
      .dataout(dataout), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i),
      .space(),.occupied() );
   
endmodule // prot_eng_tx
