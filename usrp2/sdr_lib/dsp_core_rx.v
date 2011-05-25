//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


module dsp_core_rx
  #(parameter BASE = 160)
  (input clk, input rst,
   input set_stb, input [7:0] set_addr, input [31:0] set_data,

   input [23:0] adc_i, input adc_ovf_i,
   input [23:0] adc_q, input adc_ovf_q,
   
   output [31:0] sample,
   input run,
   output strobe,
   output [31:0] debug
   );

   wire [31:0] phase_inc;
   reg [31:0]  phase;

   wire [24:0] i_cordic, q_cordic;
   wire [23:0] i_cordic_clip, q_cordic_clip;
   wire [23:0] i_cic, q_cic;
   wire [23:0] i_hb1, q_hb1;
   wire [23:0] i_hb2, q_hb2;
   
   wire        strobe_cic, strobe_hb1, strobe_hb2;
   wire        enable_hb1, enable_hb2;
   wire [7:0]  cic_decim_rate;

   reg [23:0]  adc_i_mux, adc_q_mux;
   wire        realmode;
   wire        swap_iq;
   
   setting_reg #(.my_addr(BASE+0)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(phase_inc),.changed());
   
   /*
   setting_reg #(.my_addr(BASE+1)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({scale_i,scale_q}),.changed());
   */
   
   setting_reg #(.my_addr(BASE+2), .width(10)) sr_2
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({enable_hb1, enable_hb2, cic_decim_rate}),.changed());

   setting_reg #(.my_addr(BASE+3), .width(2)) sr_3
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({realmode,swap_iq}),.changed());

   // MUX so we can do realmode signals on either input
   
   always @(posedge clk)
     if(swap_iq)
       begin
	  adc_i_mux <= adc_q;
	  adc_q_mux <= realmode ? 24'd0 : adc_i;
       end
     else
       begin
	  adc_i_mux <= adc_i;
	  adc_q_mux <= realmode ? 24'd0 : adc_q;
       end

   // NCO
   always @(posedge clk)
     if(rst)
       phase <= 0;
     else if(~run)
       phase <= 0;
     else
       phase <= phase + phase_inc;

   // CORDIC  24-bit I/O
   cordic_z24 #(.bitwidth(25))
     cordic(.clock(clk), .reset(rst), .enable(run),
	    .xi({adc_i_mux[23],adc_i_mux}),. yi({adc_q_mux[23],adc_q_mux}), .zi(phase[31:8]),
	    .xo(i_cordic),.yo(q_cordic),.zo() );

   clip_reg #(.bits_in(25), .bits_out(24)) clip_i (.clk(clk), .in(i_cordic), .out(i_cordic_clip));
   clip_reg #(.bits_in(25), .bits_out(24)) clip_q (.clk(clk), .in(q_cordic), .out(q_cordic_clip));

   // CIC decimator  24 bit I/O
   cic_strober cic_strober(.clock(clk),.reset(rst),.enable(run),.rate(cic_decim_rate),
			   .strobe_fast(1),.strobe_slow(strobe_cic) );

   cic_decim #(.bw(24))
     decim_i (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(i_cordic_clip),.signal_out(i_cic));
   
   cic_decim #(.bw(24))
     decim_q (.clock(clk),.reset(rst),.enable(run),
	      .rate(cic_decim_rate),.strobe_in(1'b1),.strobe_out(strobe_cic),
	      .signal_in(q_cordic_clip),.signal_out(q_cic));

   // First (small) halfband  24 bit I/O
   small_hb_dec #(.WIDTH(24)) small_hb_i
     (.clk(clk),.rst(rst),.bypass(~enable_hb1),.run(run),
      .stb_in(strobe_cic),.data_in(i_cic),.stb_out(strobe_hb1),.data_out(i_hb1));
   
   small_hb_dec #(.WIDTH(24)) small_hb_q
     (.clk(clk),.rst(rst),.bypass(~enable_hb1),.run(run),
      .stb_in(strobe_cic),.data_in(q_cic),.stb_out(),.data_out(q_hb1));

   // Second (large) halfband  24 bit I/O
   wire [8:0]  cpi_hb = enable_hb1 ? {cic_decim_rate,1'b0} : {1'b0,cic_decim_rate};
   hb_dec #(.WIDTH(24)) hb_i
     (.clk(clk),.rst(rst),.bypass(~enable_hb2),.run(run),.cpi(cpi_hb),
      .stb_in(strobe_hb1),.data_in(i_hb1),.stb_out(strobe_hb2),.data_out(i_hb2));

   hb_dec #(.WIDTH(24)) hb_q
     (.clk(clk),.rst(rst),.bypass(~enable_hb2),.run(run),.cpi(cpi_hb),
      .stb_in(strobe_hb1),.data_in(q_hb1),.stb_out(),.data_out(q_hb2));

   // Round final answer to 16 bits
   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_i
     (.clk(clk),.reset(rst), .in(i_hb2),.strobe_in(strobe_hb2), .out(sample[31:16]), .strobe_out(strobe));

   round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_q
     (.clk(clk),.reset(rst), .in(q_hb2),.strobe_in(strobe_hb2), .out(sample[15:0]), .strobe_out());
   
   assign      debug = {enable_hb1, enable_hb2, run, strobe, strobe_cic, strobe_hb1, strobe_hb2};
   
endmodule // dsp_core_rx
