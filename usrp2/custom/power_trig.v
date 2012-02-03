//
// Copyright 2012 Ettus Research LLC
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

//CUSTOMIZE ME!

//The following module effects the IO of the DDC chain.
//By default, this entire module is a simple pass-through.

//To implement DSP logic before the DDC:
//Implement custom DSP between frontend and ddc input.

//To implement DSP logic after the DDC:
//Implement custom DSP between ddc output and baseband.

//To bypass the DDC with custom logic:
//Implement custom DSP between frontend and baseband.

module power_trig
  #(parameter BASE=0)
   (//control signals
    input clk, input reset, input enable,

    // Setting Bus
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    input run,
    
    //strobed samples {I16,Q16} from the RX DDC chain
    input [31:0] ddc_out_sample,
    input ddc_out_strobe, //high on valid sample

    //strobed baseband samples {I16,Q16} from this module
    output [31:0] bb_sample,
    output bb_strobe, //high on valid sample

    //debug output (optional)
    output [31:0] debug
    );

   reg [8:0] 	  wr_addr;
   wire [8:0] 	  rd_addr;
   reg 		  triggered, triggerable;
   wire 	  trigger;
   
   wire [31:0] 	  delayed_sample;
   wire [31:0] 	  thresh;
   
   setting_reg #(.my_addr(BASE+0)) sr_0
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(thresh),.changed());

   assign rd_addr = wr_addr + 1; // FIXME adjustable delay

   ram_2port  #(.DWIDTH(32),.AWIDTH(9)) delay_line
     (.clka(clk),.ena(1),.wea(ddc_out_strobe),.addra(wr_addr),.dia(ddc_out_sample),.doa(),
      .clkb(clk),.enb(ddc_out_strobe),.web(1'b0),.addrb(rd_addr),.dib(32'hFFFF),.dob(delayed_sample));

   always @(posedge clk)
     if(reset | ~run)
       wr_addr <= 0;
     else
       if(ddc_out_strobe)
	 wr_addr <= wr_addr + 1;

   always @(posedge clk)
     if(reset | ~run)
       triggerable <= 0;
     else if(wr_addr == 9'h1FF)  // Wait till we're nearly full
       triggerable <= 1;
   

   reg 			      stb_d1, stb_d2;
   always @(posedge clk) stb_d1 <= ddc_out_strobe;
   always @(posedge clk) stb_d2 <= stb_d1;
   
   assign bb_sample = delayed_sample;
   assign bb_strobe = stb_d1 & triggered;

   // Compute Mag
   wire [17:0] 		      mult_in = stb_d1 ? { ddc_out_sample[15],ddc_out_sample[15:0], 1'b0 } : 
			      { ddc_out_sample[31], ddc_out_sample[31:16], 1'b0 };
   wire [35:0] 		      prod;
   reg [31:0] 		      sum;
   
   MULT18X18S mult (.P(prod), .A(mult_in), .B(mult_in), .C(clk), .CE(ddc_out_strobe | stb_d1), .R(reset) );

   always @(posedge clk)
     if(stb_d1)
       sum <= prod[35:4];
     else if(stb_d2)
       sum <= sum + prod[35:4];

   always @(posedge clk)
     if(reset | ~run | ~triggerable)
       triggered <= 0;
     else if(trigger)
       triggered <= 1;

   assign trigger = (sum > thresh);
   
endmodule // power_trig
