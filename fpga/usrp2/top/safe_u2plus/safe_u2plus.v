`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module safe_u2plus
  (
   input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [5:1] leds,  // LED4 is shared w/INIT_B
   output ETH_LED
   );

   wire   clk_fpga;
   
   IBUFGDS clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));
   defparam 	clk_fpga_pin.IOSTANDARD = "LVPECL_25";

   reg [31:0] 	ctr;

   always @(posedge clk_fpga)
     ctr <= ctr + 1;

   assign {leds,ETH_LED} = ~ctr[29:24];
   
endmodule // safe_u2plus
