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



module clock_control_tb();
   
  clock_control clock_control
    (.reset(reset),
     .aux_clk(aux_clk),  
     .clk_fpga(clk_fpga),
     .clk_en(clk_en),    
     .clk_sel(clk_sel),  
     .clk_func(clk_func), 
     .clk_status(clk_status),
     
     .sen(sen),   
     .sclk(sclk), 
     .sdi(sdi),
     .sdo(sdo)
     );

   reg reset, aux_clk;
   
   wire [1:0] clk_sel, clk_en;
   
   initial reset = 1'b1;
   initial #1000 reset = 1'b0;
   
   initial aux_clk = 1'b0;
   always #10 aux_clk = ~aux_clk;
   
   initial $dumpfile("clock_control_tb.vcd");
   initial $dumpvars(0,clock_control_tb);
   
   initial #10000 $finish;
   
endmodule // clock_control_tb
