//
// Copyright 2013-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module timekeeper_legacy
  #(parameter SR_TIME_HI = 0,
    parameter SR_TIME_LO = 1,
    parameter SR_TIME_CTRL = 2,
    parameter INCREMENT = 64'h1)
   (input clk, input reset, input pps, input sync_in, input strobe,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    output reg [63:0] vita_time, output reg [63:0] vita_time_lastpps,
    output reg sync_out);

   //////////////////////////////////////////////////////////////////////////
   // timer settings for this module
   //////////////////////////////////////////////////////////////////////////
   wire [63:0] time_at_next_event;
   wire set_time_pps, set_time_now, set_time_sync;
   wire cmd_trigger;

   setting_reg #(.my_addr(SR_TIME_HI), .width(32)) sr_time_hi
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(time_at_next_event[63:32]), .changed());

   setting_reg #(.my_addr(SR_TIME_LO), .width(32)) sr_time_lo
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out(time_at_next_event[31:0]), .changed());

   setting_reg #(.my_addr(SR_TIME_CTRL), .width(3)) sr_ctrl
     (.clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
      .out({set_time_sync, set_time_pps, set_time_now}), .changed(cmd_trigger));

   //////////////////////////////////////////////////////////////////////////
   // PPS edge detection logic
   //////////////////////////////////////////////////////////////////////////
   reg pps_del, pps_del2;
   always @(posedge clk)
     {pps_del2,pps_del} <= {pps_del, pps};

   wire pps_edge = !pps_del2 & pps_del;

   //////////////////////////////////////////////////////////////////////////
   // arm the trigger to latch a new time when the ctrl register is written
   //////////////////////////////////////////////////////////////////////////
   reg armed;
   wire time_event = armed && ((set_time_now) || (set_time_pps && pps_edge) || (set_time_sync && sync_in));
   always @(posedge clk) begin
     if (reset) armed <= 1'b0;
     else if (cmd_trigger) armed <= 1'b1;
     else if (time_event) armed <= 1'b0;
   end

   //////////////////////////////////////////////////////////////////////////
   // vita time tracker - update every tick or when we get an "event"
   //////////////////////////////////////////////////////////////////////////
   always @(posedge clk) begin
     sync_out <= 1'b0;
     if(reset) begin
       vita_time <= 64'h0;
     end else begin
         if (time_event) begin
           sync_out <= 1'b1;
           vita_time <= time_at_next_event;
         end else if (strobe) begin
	   vita_time <= vita_time + INCREMENT;
         end
     end
   end

   //////////////////////////////////////////////////////////////////////////
   // track the time at last pps so host can detect the pps
   //////////////////////////////////////////////////////////////////////////
   always @(posedge clk)
     if(reset)
       vita_time_lastpps <= 64'h0;
     else if(pps_edge)
       if(time_event)
         vita_time_lastpps <= time_at_next_event;
       else
         vita_time_lastpps <= vita_time + INCREMENT;

endmodule // timekeeper_legacy
