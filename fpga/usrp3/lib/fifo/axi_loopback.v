//
// Copyright 2012 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

//
// axi_loopback.v
//
// Loopback all data assuming it's in CHDR format, and swap SRC/DST in the SID in the process
// thus reflecting it back to it's origin...in theory!
//

module axi_loopback
 #(
   parameter WIDTH = 64
  )
  (
   input clk,
   input reset,
   // Input AXIS
   input [WIDTH-1:0] i_tdata,
   input i_tlast,
   input i_tvalid,
   output i_tready,
   // Output AXIS
   output [WIDTH-1:0] o_tdata,
   output o_tlast,
   output o_tvalid,
   input o_tready
   );

   wire [WIDTH-1:0]     fifoin_tdata,fifoout_tdata,dmux_tdata;
   wire                 fifoin_tlast,dmux_tlast;
   wire                 fifoin_tvalid,dmux_tvalid;
   wire                 fifoin_tready,dmux_tready;
   
   // Since most real endpoints go via Demux4 place one in here to look for bugs.
   axi_demux4 #(.ACTIVE_CHAN(4'b0001), .WIDTH(WIDTH)) demux
     (.clk(clk), .reset(reset), .clear(1'b0),
      .header(), .dest(2'b00),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o0_tdata(dmux_tdata), .o0_tlast(dmux_tlast), .o0_tvalid(dmux_tvalid), .o0_tready(dmux_tready), 
      .o1_tdata(), .o1_tlast(), .o1_tvalid(), .o1_tready(1'b1), 
      .o2_tdata(), .o2_tlast(), .o2_tvalid(), .o2_tready(1'b1), 
      .o3_tdata(), .o3_tlast(), .o3_tvalid(), .o3_tready(1'b1));

   axi_fifo_short #(.WIDTH(WIDTH+1)) axi_fifo_short1
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({dmux_tlast,dmux_tdata}), .i_tvalid(dmux_tvalid), .i_tready(dmux_tready),
      .o_tdata({fifoin_tlast,fifoin_tdata}), .o_tvalid(fifoin_tvalid), .o_tready(fifoin_tready),
      .space(), .occupied());

   reg header;
   always @(posedge clk) begin
     if(reset) begin
       header <= 1'b1;
     end else if (header) begin
       if(fifoin_tvalid & fifoin_tready & ~fifoin_tlast) header <= 1'b0;
     end else begin
       if(fifoin_tvalid & fifoin_tready & fifoin_tlast) header <= 1'b1;
     end
   end

   assign fifoout_tdata = header ? 
			       {fifoin_tdata[63:32] ,fifoin_tdata[15:0],fifoin_tdata[31:16]} :
			       fifoin_tdata;
   
   axi_fifo_short #(.WIDTH(WIDTH+1)) axi_fifo_short2
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({fifoin_tlast,fifoout_tdata}), .i_tvalid(fifoin_tvalid), .i_tready(fifoin_tready),
      .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied());

endmodule // axi_loopback
