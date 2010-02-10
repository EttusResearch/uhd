`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module safe_u1e
  (
   input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [2:0] debug_led
   );

   // FPGA-specific pins connections
   wire  clk_fpga;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));
   defparam 	clk_fpga_pin.IOSTANDARD = "LVPECL_25";

   reg [31:0] 	ctr;

   always @(posedge clk_fpga)
     ctr <= ctr + 1;

   assign debug_led = ctr[27:25];

endmodule // safe_u2plus
