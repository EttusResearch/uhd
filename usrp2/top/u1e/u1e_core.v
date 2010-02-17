`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module u1e_core
  (input clk_fpga, output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,

   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE
   );

   // Debug circuitry
   reg [31:0] 	ctr=0;
   always @(posedge clk_fpga)
     ctr <= ctr + 1;
   
   assign debug_led = ctr[27:25];
   assign debug_clk = { EM_CLK, clk_fpga };
   assign debug = { { 1'b0, EM_WAIT0, EM_NCS6, EM_NCS4, EM_NWE, EM_NOE, EM_A[10:1] },
		    { EM_D } };

   wire 	wb_clk, wb_rst;
   wire 	wb_cyc, wb_stb, wb_we, wb_ack;
   wire [1:0] 	wb_sel;
   wire [10:0] 	wb_adr;
   wire [15:0] 	wb_dat_mosi, wb_dat_miso;
   
   gpmc gpmc (.EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
	      .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), .EM_NWE(EM_NWE), 
	      .EM_NOE(EM_NOE),

	      .wb_clk(wb_clk), .wb_rst(wb_rst),
	      .wb_adr_o(wb_adr), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso),
	      .wb_sel_o(wb_sel), .wb_cyc_o(wb_cyc), .wb_stb_o(wb_stb), .wb_we_o(wb_we),
	      .wb_ack_i(wb_ack));

   assign wb_clk = clk_fpga;
   reg [15:0] 	reg_fast, reg_slow;

   localparam [10:0] WB_ADR_REG_FAST = 11'd36;
   localparam [10:0] WB_ADR_REG_SLOW = 38;
   
   always @(posedge wb_clk)
     if(wb_cyc & wb_stb & wb_we & (wb_adr == WB_ADR_REG_FAST))
       reg_fast <= wb_dat_mosi;

   assign wb_dat_miso = (wb_adr == WB_ADR_REG_FAST) ? reg_fast : 16'bx;

   assign wb_ack = wb_stb & wb_cyc;
      
endmodule // u1e_core
