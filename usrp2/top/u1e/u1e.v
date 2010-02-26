`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module u1e
  (input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input [2:0] debug_pb, input [7:0] dip_sw, output FPGA_TXD, input FPGA_RXD,

   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,

   inout db_sda, inout db_scl, // I2C
   output overo_gpio144, output overo_gpio145, output overo_gpio146, output overo_gpio147,  // Fifo controls
   inout [15:0] io_tx, inout [15:0] io_rx
   );

   // FPGA-specific pins connections
   wire  clk_fpga;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

   u1e_core u1e_core(.clk_fpga(clk_fpga), .rst_fpga(debug_pb[2]),
		     .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .debug_pb(debug_pb), .dip_sw(dip_sw), .debug_txd(FPGA_TXD), .debug_rxd(FPGA_RXD),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
		     .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
		     .db_sda(db_sda), .db_scl(db_scl),
		     .tx_have_space(overo_gpio144), .tx_underrun(overo_gpio145),
		     .rx_have_data(overo_gpio146), .rx_overrun(overo_gpio147),
		     .io_tx(io_tx), .io_rx(io_rx) );
   
endmodule // u1e
