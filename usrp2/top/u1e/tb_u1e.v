`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module tb_u1e();
   
   wire [2:0] debug_led;
   wire [31:0] debug;
   wire [1:0] debug_clk;
   
   
   // GPMC
   wire       EM_CLK, EM_WAIT0, EM_NCS4, EM_NCS6, EM_NWE, EM_NOE;
   wire [15:0] EM_D;
   wire [10:1] EM_A;
   wire [1:0]  EM_NBE;
   
   reg  clk_fpga = 0;
   always #100 clk_fpga = ~clk_fpga;
   
   u1e_core u1e_core(.clk_fpga(clk_fpga), .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
		     .EM_NWE(EM_NWE), .EM_NOE(EM_NOE) );
   
endmodule // u1e
