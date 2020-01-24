//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`timescale 1ns/1ps

module ddc_chain_x300_tb();
   
`ifdef ISIM
`else //iverilog implied.
//   xlnx_glbl glbl (.GSR(),.GTS());
`endif
   
   localparam SR_TX_DSP        = 8;

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("ddc_chain_x300_tb.vcd");
   initial $dumpvars(0,ddc_chain_x300_tb);
   reg run = 0;
   wire strobe;
   
   initial
     begin
	#1000 reset = 0;
	@(posedge clk);
	set_addr <= 0; set_data <= 32'd8434349; set_stb <= 1; @(posedge clk); // CORDIC
	set_addr <= 1; set_data <= 18'd19800; set_stb <= 1; @(posedge clk); // Scale factor
	set_addr <= 2; set_data <= 10'h003; set_stb <= 1; @(posedge clk); // Decim control
	set_addr <= 3; set_data <= 0; set_stb <= 1; @(posedge clk); // Swap iq
	set_addr <= 4; set_data <= 0; set_stb <= 1; @(posedge clk); // filter taps
	set_stb <= 0;
		
	repeat(10)
	  @(posedge clk);
	run <= 1'b1;
	#30000;
	$finish;
     end
   
   reg [7:0]   set_addr;
   reg [31:0]  set_data;
   reg 	       set_stb = 1'b0;

   wire [15:0] i_out, q_out;
   wire [23:0] rx_fe_i, rx_fe_q;

   assign rx_fe_i = 24'd8388607;
   assign rx_fe_q = 24'd8388607;
   //assign rx_fe_q = 0;
   
   ddc_chain_x300 #(.BASE(0), .DSPNO(0), .WIDTH(24)) ddc_chain
     (.clk(clk), .rst(reset), .clr(1'b0),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .rx_fe_i(rx_fe_i),.rx_fe_q(rx_fe_q),
      .sample({i_out,q_out}), .run(run), .strobe(strobe),
      .debug() );
   
endmodule // new_tx_tb
