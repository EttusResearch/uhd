`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

//`define DCM 1

module u1e
  (output [3:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input debug_pb,

   // GPMC
   input EM_CLK, input [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS5, input EM_NCS6, input EM_NWE, input EM_NOE,
   input EM_NADV_ALE, input EM_NWP
   );

   assign debug_clk = {EM_CLK, EM_NADV_ALE};

   assign debug_led = {EM_NWP, EM_A[9], EM_A[8], debug_pb};

   assign debug = { { EM_NBE[1:0], EM_WAIT0, EM_NCS4, EM_NCS5, EM_NCS6, EM_NWE, EM_NOE },
		    { EM_A[10], EM_A[7:1] },
		    { EM_D[15:8] },
		    { EM_D[7:0] } };
   

endmodule // u1e
