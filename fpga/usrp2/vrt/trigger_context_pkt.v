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



module trigger_context_pkt
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input packet_consumed, output reg trigger);

   wire [23:0] cycles;
   wire [15:0] packets;
   wire [6:0]  dummy1;
   wire [14:0] dummy2;
   wire        enable_cycle, enable_consumed;
   reg [30:0]  cycle_count, packet_count;

   
   setting_reg #(.my_addr(BASE+4), .at_reset(0)) sr_cycles
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({enable_cycle,dummy1,cycles}),.changed());

   setting_reg #(.my_addr(BASE+5), .at_reset(0)) sr_packets
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({enable_consumed,dummy2,packets}),.changed());

   always @(posedge clk)
     if(reset | clear)
       cycle_count <= 0;
     else
       if(trigger)
	 cycle_count <= 0;
       else if(enable_cycle)
	 cycle_count <= cycle_count + 1;

   always @(posedge clk)
     if(reset | clear)
       packet_count <= 0;
     else
       if(trigger)
	 packet_count <= 0;
       else if(packet_consumed & enable_consumed)
	 packet_count <= packet_count + 1;

   always @(posedge clk)
     if(reset | clear)
       trigger <= 0;
     else
       if((cycle_count > cycles)|(packet_count > packets))
	 trigger <= 1;
       else
	 trigger <= 0;
   
endmodule // trigger_context_pkt
