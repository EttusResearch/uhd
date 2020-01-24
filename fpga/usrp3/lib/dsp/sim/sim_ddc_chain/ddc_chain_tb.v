//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
`timescale 1ns/1ps

module ddc_chain_tb();

   initial $dumpfile("waves.vcd");
   initial $dumpvars(2,ddc_chain_tb.dut_i0);


   // Need these declarations to use the task libarary.
`ifndef CHDR_IN_NUMBER
 `define CHDR_IN_NUMBER 1
`endif
`ifndef CHDR_OUT_NUMBER
 `define CHDR_OUT_NUMBER 1
`endif

   reg [63:0] data_in[`CHDR_IN_NUMBER-1:0];
   reg 	      last_in[`CHDR_IN_NUMBER-1:0];
   reg 	      valid_in[`CHDR_IN_NUMBER-1:0];
   wire       ready_in[`CHDR_IN_NUMBER-1:0];
   wire [63:0] data_out[`CHDR_OUT_NUMBER-1:0];
   wire 	      last_out[`CHDR_OUT_NUMBER-1:0];
   wire 	      valid_out[`CHDR_OUT_NUMBER-1:0];
   reg        ready_out[`CHDR_OUT_NUMBER-1:0];
   //

`include "../../../../../sim/radio_setting_regs.v"
`include "../../../../../sim/task_library.v"

   localparam DSPNO = 0;
   localparam WIDTH = 24;
   localparam NEW_HB_DECIM = 1;
   localparam DEVICE = "SPARTAN6";

   reg clk = 0;
   reg reset;

   reg set_stb;
   reg [7:0] set_addr;
   reg [31:0] set_data;

   wire [WIDTH-1:0] rx_fe_i, rx_fe_q;
   wire [31:0] 	    sample;
   reg 		    run;
   wire 	    strobe;

   reg [11:0] 	    i_in, q_in;

   assign rx_fe_i = {i_in,12'h0};
   assign rx_fe_q = {q_in,12'h0};

   //
   // DUT
   //
   ddc_chain
     #(
     .BASE(SR_RX_DSP),
     .DSPNO(DSPNO),
     .WIDTH(WIDTH),
     .NEW_HB_DECIM(NEW_HB_DECIM),
     .DEVICE("SPARTAN6")
       )
   dut_i0 (
     .clk(clk),
     .rst(reset),
     .clr(1'b0),
     .set_stb(set_stb),
     .set_addr(set_addr),
     .set_data(set_data),

     // From RX frontend
     .rx_fe_i(rx_fe_i),
     .rx_fe_q(rx_fe_q),

     // To RX control
     .sample(sample),
     .run(run),
     .strobe(strobe),
     .debug()
     );



   //
   // Include testbench
   //
`include "simulation_script.v"

endmodule //
