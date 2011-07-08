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


`timescale 1ns/1ns
module rx_dcoffset_tb();
   
   reg clk, rst;

   initial rst = 1;
   initial #1000 rst = 0;
   initial clk = 0;
   always #5 clk = ~clk;
   
   initial $dumpfile("rx_dcoffset_tb.vcd");
   initial $dumpvars(0,rx_dcoffset_tb);

   reg [13:0] adc_in;
   wire [13:0] adc_out;

   always @(posedge clk)
     begin
	if(adc_in[13])
	  $write("-%d,",-adc_in);
	else
	  $write("%d,",adc_in);
	if(adc_out[13])
	  $write("-%d\n",-adc_out);
	else
	  $write("%d\n",adc_out);
     end	
   
   rx_dcoffset #(.WIDTH(14),.ADDR(0), .alpha_shift(8))
     rx_dcoffset(.clk(clk),.rst(rst),.set_stb(0),.set_addr(0),.set_data(0),
		 .in(adc_in),.out(adc_out));

   always @(posedge clk)
     adc_in <= (($random % 473) + 23)/4;
   
endmodule // longfifo_tb
