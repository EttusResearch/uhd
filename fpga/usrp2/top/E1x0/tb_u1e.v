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

`timescale 1ps / 1ps
//////////////////////////////////////////////////////////////////////////////////

module tb_u1e();
   
   wire [2:0] debug_led;
   wire [31:0] debug;
   wire [1:0] debug_clk;

   xlnx_glbl glbl (.GSR(),.GTS());

   initial begin
      $dumpfile("tb_u1e.lxt");
      $dumpvars(0,tb_u1e);
   end
    
   // GPMC
   wire       EM_CLK, EM_WAIT0, EM_NCS4, EM_NCS6, EM_NWE, EM_NOE;
   wire [15:0] EM_D;
   wire [10:1] EM_A;
   wire [1:0]  EM_NBE;
   
   reg  clk_fpga = 0, rst_fpga = 1;
   always #15625 clk_fpga = ~clk_fpga;

   initial #200000
     @(posedge clk_fpga)
       rst_fpga <= 0;
   
   u1e_core u1e_core(.clk_fpga(clk_fpga), .rst_fpga(rst_fpga), 
		     .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
		     .EM_NWE(EM_NWE), .EM_NOE(EM_NOE) );

   gpmc_model_async gpmc_model_async
     (.EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
      .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
      .EM_NWE(EM_NWE), .EM_NOE(EM_NOE) );
   
endmodule // tb_u1e
