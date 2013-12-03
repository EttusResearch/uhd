//
// Copyright 2013 Ettus Research LLC
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

module timekeeper
  #(parameter BASE = 0)
   (input clk, input reset, input pps,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    output reg [63:0] vita_time, output reg [63:0] vita_time_lastpps);

   //////////////////////////////////////////////////////////////////////////
   // timer settings for this module
   //////////////////////////////////////////////////////////////////////////
   wire [63:0] time_at_next_event;
   wire set_time_pps, set_time_now;
   wire cmd_trigger;

   setting_reg #(.my_addr(BASE), .width()) sr_time_hi
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(time_at_next_event[63:32]), .changed());
   
   setting_reg #(.my_addr(BASE+1), .width()) sr_time_lo
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(time_at_next_event[31:0]), .changed());
   
   setting_reg #(.my_addr(BASE+2), .width(2)) sr_ctrl
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_time_pps, set_time_now}), .changed(cmd_trigger));

   //////////////////////////////////////////////////////////////////////////
   // PPS edge detection logic
   //////////////////////////////////////////////////////////////////////////
   reg pps_del, pps_del2;
   always @(posedge clk)
     {pps_del2,pps_del} <= {pps_del, pps};

   wire pps_edge = !pps_del2 & pps_del;

   //////////////////////////////////////////////////////////////////////////
   // track the time at last pps so host can detect the pps
   //////////////////////////////////////////////////////////////////////////
   always @(posedge clk)
     if(pps_edge) vita_time_lastpps <= vita_time;

   //////////////////////////////////////////////////////////////////////////
   // arm the trigger to latch a new time when the ctrl register is written
   //////////////////////////////////////////////////////////////////////////
   reg armed;
   wire time_event = armed && ((set_time_now) || (set_time_pps && pps_edge));
   always @(posedge clk) begin
     if (reset) armed <= 1'b0;
     else if (cmd_trigger) armed <= 1'b1;
     else if (time_event) armed <= 1'b0;
   end

   //////////////////////////////////////////////////////////////////////////
   // vita time tracker - update every tick or when we get an "event"
   //////////////////////////////////////////////////////////////////////////
   always @(posedge clk)
     if(reset)
       vita_time <= 64'h0;
     else if (time_event)
       vita_time <= time_at_next_event;
     else
       vita_time <= vita_time + 64'h1;

endmodule // timekeeper
