//
// Copyright 2011-2012 Ettus Research LLC
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



module time_64bit
  #(
    parameter BASE = 0)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,  
    input pps,
    output reg [63:0] vita_time,
    output reg [63:0] vita_time_pps,
    output pps_int,
    input exp_time_in, output exp_time_out,
    output reg good_sync,
    output [31:0] debug
    );
   
   localparam 	   NEXT_TICKS_HI = 0;
   localparam 	   NEXT_TICKS_LO = 1;
   localparam      PPS_POLSRC = 2;
   localparam      PPS_IMM = 3;
   localparam      MIMO_SYNC = 5;
   
   reg [63:0] 	   ticks;

   always @(posedge clk)
     vita_time <= ticks;
   
   wire [63:0] 	   vita_time_rcvd;
   
   wire [63:0] 	   next_ticks_preset;
   wire 	   set_on_pps_trig;
   reg 		   set_on_next_pps;
   wire 	   pps_polarity, pps_source, set_imm;
   reg [1:0] 	   pps_del;
   reg 		   pps_reg_p, pps_reg_n, pps_reg;
   wire 	   pps_edge;

   reg [15:0] 	   sync_counter;
   wire 	   sync_rcvd;
   wire [63:0] 	   mimo_ticks;
   wire 	   mimo_sync_now;
   wire 	   mimo_sync;
   wire [7:0] 	   sync_delay;
   
   setting_reg #(.my_addr(BASE+NEXT_TICKS_LO)) sr_next_ticks_lo
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(next_ticks_preset[31:0]),.changed());
   
   setting_reg #(.my_addr(BASE+NEXT_TICKS_HI)) sr_next_ticks_hi
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(next_ticks_preset[63:32]),.changed(set_on_pps_trig));

   setting_reg #(.my_addr(BASE+PPS_POLSRC), .width(2)) sr_pps_polsrc
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({pps_source,pps_polarity}),.changed());

   setting_reg #(.my_addr(BASE+PPS_IMM), .width(1)) sr_pps_imm
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(set_imm),.changed());

   setting_reg #(.my_addr(BASE+MIMO_SYNC), .at_reset(0), .width(9)) sr_mimosync
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({mimo_sync,sync_delay}),.changed());

   always @(posedge clk)  pps_reg_p <= pps;   
   always @(negedge clk)  pps_reg_n <= pps;
   always @* pps_reg <= pps_polarity ? pps_reg_p : pps_reg_n;
   
   always @(posedge clk)
     if(rst)
       pps_del <= 2'b00;
     else
       pps_del <= {pps_del[0],pps_reg};

   assign pps_edge = pps_del[0] & ~pps_del[1];

   always @(posedge clk)
     if(pps_edge)
       vita_time_pps <= vita_time;
   
   always @(posedge clk)
     if(rst)
       set_on_next_pps <= 0;
     else if(set_on_pps_trig)
       set_on_next_pps <= 1;
     else if(set_imm | pps_edge)
       set_on_next_pps <= 0;

   wire [63:0] 	   ticks_plus_one = ticks + 1;
   
   always @(posedge clk)
     if(rst)
       begin
	  ticks <= 64'd0;
       end
     else if((set_imm | pps_edge) & set_on_next_pps)
       begin
	  ticks <= next_ticks_preset;
       end
     else if(mimo_sync_now)
       begin
	  ticks <= mimo_ticks;
       end
     else
       ticks <= ticks_plus_one;

   assign pps_int = pps_edge;

   // MIMO Connector Time Sync
   wire send_sync = (sync_counter == 59999); // X % 10 = 9

   always @(posedge clk)
     if(rst)
       sync_counter <= 0;
     else
       if(send_sync)
	 sync_counter <= 0;
       else
	 sync_counter <= sync_counter + 1;
   
   time_sender time_sender
     (.clk(clk),.rst(rst),
      .vita_time(vita_time),
      .send_sync(send_sync),
      .exp_time_out(exp_time_out) );

   time_receiver time_receiver
     (.clk(clk),.rst(rst),
      .vita_time(vita_time_rcvd),
      .sync_rcvd(sync_rcvd),
      .exp_time_in(exp_time_in) );

   assign mimo_ticks = vita_time_rcvd[63:0] + {48'd0,sync_delay};
   assign mimo_sync_now = mimo_sync & sync_rcvd;

   assign debug = { { 24'b0} ,
		    { 2'b0, exp_time_in, exp_time_out, mimo_sync, mimo_sync_now, sync_rcvd, send_sync} };

   always @(posedge clk)
     if(rst)
       good_sync <= 0;
     else if(sync_rcvd)
       good_sync <= 1;
   
endmodule // time_64bit
