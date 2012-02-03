//
// Copyright 2012 Ettus Research LLC
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


module power_trig_tb();
   initial $dumpfile("power_trig_tb.vcd");
   initial $dumpvars(0,power_trig_tb);

   reg clk = 0;
   always #10 clk <= ~clk;
   reg rst = 1;
   initial #100 rst <= 0;

   initial
     begin
	set_stb <= 0;
	#1000;
	set_stb <= 1;
     end
   
   reg [31:0] sample_in;
   reg 	      strobe_in;
   wire [31:0] sample_out;
   wire        strobe_out;
   reg 	       set_stb, run;
   
   power_trig #(.BASE(0)) power_trig
     (.clk(clk), .reset(rst), .enable(1),
      .set_stb(set_stb), .set_addr(0), .set_data(32'h000B_B000),
      .run(run),

      .ddc_out_sample(sample_in), .ddc_out_strobe(strobe_in),
      .bb_sample(sample_out), .bb_strobe(strobe_out));

   initial sample_in <= 32'h0100_0300;
   
   always @(posedge clk)
     if(~strobe_in)
       sample_in <= sample_in + 32'h0001_0001;

   initial
     #100000 $finish;

   initial 
     begin
	run <= 0;
	#2000 run <= 1;
	#30000 run <= 0;
     end

   always @(posedge clk)
     if(rst | ~run)
       strobe_in <= 0;
     else
       strobe_in <= ~strobe_in;
   
endmodule // power_trig_tb
