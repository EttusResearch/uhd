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


`timescale 1ns / 1ps

module time_transfer_tb();

   reg clk = 0, rst = 1;
   always #5 clk = ~clk;
   
   initial 
     begin
	@(negedge clk);
	@(negedge clk);
	rst <= 0;
     end

   initial $dumpfile("time_transfer_tb.vcd");
   initial $dumpvars(0,time_transfer_tb);
   
   initial #100000000 $finish;

   wire exp_time, pps, pps_rcv;
   wire [63:0] vita_time_rcv;
   reg [63:0]  vita_time = 0;
   reg [63:0]  counter = 0;

   localparam  PPS_PERIOD = 439; // PPS_PERIOD % 10 must = 9
   always @(posedge clk)
     if(counter == PPS_PERIOD)
       counter <= 0;
     else
       counter <= counter + 1;
   assign      pps = (counter == (PPS_PERIOD-1));
   
   always @(posedge clk)
     vita_time <= vita_time + 1;
   
   time_sender time_sender
     (.clk(clk),.rst(rst),
      .vita_time(vita_time),
      .send_sync(pps),
      .exp_time_out(exp_time) );

   time_receiver time_receiver
     (.clk(clk),.rst(rst),
      .vita_time(vita_time_rcv),
      .sync_rcvd(pps_rcv),
      .exp_time_in(exp_time) );

   wire [31:0] delta = vita_time - vita_time_rcv;
endmodule // time_transfer_tb
