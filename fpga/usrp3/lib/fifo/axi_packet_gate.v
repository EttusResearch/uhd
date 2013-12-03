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

// Hold packets in fifo until they are complete.  This prevents slowly-built packets
//  from clogging up the downstream.  This block will hold up to 255 packets.
//  Will permanently block if a single packet is bigger than the fifo.
//  Will also drop any packet with an error signalled on the last line.
//  This is useful after an ethernet interface to drop packets with bad CRCs.

module axi_packet_gate
  #(parameter WIDTH=68,
    parameter SIZE=10)
   (input clk, 
    input reset, 
    input clear,
    input [WIDTH-1:0] i_tdata,
    input i_tlast,
    input i_terror,
    input i_tvalid,
    output i_tready,
    output [WIDTH-1:0] o_tdata,
    output o_tlast,
    output o_tvalid,
    input o_tready
    );

   reg [7:0] num_packets;
   reg 	     dump;
   
   wire      o_tvalid_int, o_tready_int, i_tvalid_int, i_tready_int;

   assign i_tvalid_int = (~dump & (num_packets != 8'hFF)) ? i_tvalid : 1'b0;
   assign i_tready = (~dump & (num_packets != 8'hFF)) ? i_tready_int : 1'b0;

   assign o_tvalid = (num_packets != 8'h0) ? o_tvalid_int : 1'b0;
   assign o_tready_int = (num_packets != 8'h0) ? o_tready : 1'b0;
   
   wire      last_in = i_tvalid_int & i_tready_int & i_tlast;
   wire      last_out = o_tvalid_int & o_tready_int & o_tlast;

   always @(posedge clk)
     if(reset | clear)
       begin
	  num_packets <= 8'd0;
	  dump <= 1'b0;
       end
     else
       if(dump)
	 if(num_packets != 8'd0)
	   if(last_out)
	     num_packets <= num_packets - 8'd1;
	   else
	     ;
	 else
	   dump <= 1'b0;
       else
	 if(last_in)
	   if(i_terror)
	     begin
		dump <= 1'b1;
		if(last_out)
		  num_packets <= num_packets - 8'd1;
	     end
	   else if(~last_out)
	     num_packets <= num_packets + 8'd1;
	   else
	     ;
   	 else if(last_out)
	   num_packets <= num_packets - 8'd1;
   
   axi_fifo #(.SIZE(SIZE), .WIDTH(WIDTH+1)) axi_fifo
     (.clk(clk), .reset(reset), .clear(clear | (dump & (num_packets == 8'd0))),
      .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid_int), .i_tready(i_tready_int),
      .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid_int), .o_tready(o_tready_int));
      
endmodule // axi_packet_gate
