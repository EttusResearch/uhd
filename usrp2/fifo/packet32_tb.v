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



module packet32_tb();

   wire [35:0] data;
   wire       src_rdy, dst_rdy;

   wire       clear = 0;
   reg 	      clk = 0;
   reg 	      reset = 1;

   always #10 clk <= ~clk;
   initial #1000 reset <= 0;

   initial $dumpfile("packet32_tb.vcd");
   initial $dumpvars(0,packet32_tb);

   wire [31:0] total, crc_err, seq_err, len_err;
   
   packet_generator32 pkt_gen (.clk(clk), .reset(reset), .clear(clear),
			       .data_o(data), .src_rdy_o(src_rdy), .dst_rdy_i(dst_rdy));

   packet_verifier32 pkt_ver (.clk(clk), .reset(reset), .clear(clear),
			      .data_i(data), .src_rdy_i(src_rdy), .dst_rdy_o(dst_rdy),
			      .total(total), .crc_err(crc_err), .seq_err(seq_err), .len_err(len_err));

endmodule // packet32_tb
