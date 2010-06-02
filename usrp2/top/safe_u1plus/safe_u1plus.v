`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module safe_u1plus
  (input CLK_FPGA_P, input CLK_FPGA_N,
   input reset_n,
   output [2:0] debug_led  // LED4 is shared w/INIT_B
   );
   
   // FPGA-specific pins connections
   wire 	clk_fpga, dsp_clk, clk_div, dcm_out, wb_clk, clock_ready;
   
   IBUFGDS clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));
   defparam 	clk_fpga_pin.IOSTANDARD = "LVPECL_25";
   
   reg [31:0] 	ctr;
   
   always @(posedge clk_fpga)
     ctr <= ctr + 1;
   
   assign debug_led[1:0] = ~ctr[26:25];

   assign debug_led[2] = ~reset_n;
   
endmodule // safe_u1plus
