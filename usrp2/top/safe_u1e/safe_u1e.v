`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module safe_u1e
  (
   input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,

   // GPMC
   input EM_CLK, input [15:0] EM_D, input [10:1] EM_A, 
   input EM_WAIT0, input EM_NCS4, input EM_NWP, input EM_NWE, input EM_NOE, input EM_NADV_ALE
   );

   // FPGA-specific pins connections
   wire  clk_fpga;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

   // Debug circuitry
   reg [31:0] 	ctr;
   always @(posedge clk_fpga)
     ctr <= ctr + 1;

   
   assign debug_led = ctr[27:25];
   assign debug_clk = { EM_CLK, clk_fpga };
   assign debug = { { EM_WAIT0, EM_NADV_ALE, EM_NWP, EM_NCS4, EM_NWE, EM_NOE, EM_A[10:1] },
		    { EM_D } };
   
endmodule // safe_u2plus
