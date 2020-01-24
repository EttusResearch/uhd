//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module gpif2_to_fifo64
  #(
    parameter FIFO_SIZE = 9,
    parameter MTU = 12
    )
    (
     //input interface
     input gpif_clk,
     input gpif_rst,
     input [31:0] i_tdata,
     input i_tlast,
     input i_tvalid,
     output i_tready,
     output fifo_has_space,
     output fifo_nearly_full,

     //output fifo interface
     input fifo_clk,
     input fifo_rst,
     output [63:0] o_tdata,
     output o_tlast,
     output o_tvalid,
     input o_tready,

     output bus_error,
     output [31:0] debug
     );

   wire [31:0] 	    int_tdata;
   wire 	    int_tlast;
   wire 	    int_tvalid, int_tready;

   wire [31:0] 	    int0_tdata;
   wire 	    int0_tlast, int0_tvalid, int0_tready;

   //
   // Generate flags that show if initial FIFO's can accept a maximum sized burst from the FX3
   // or if the FIFO is about to fill.
   //
   wire [15:0] 	    space;
   assign 	    fifo_has_space = space >= (1 << MTU);
   assign 	    fifo_nearly_full = (space < 6); // 5 spaces left.

   //
   // This FIFO is provdied purely to ease FPGA timing closure as data is coming from I/O pins.
   //
   axi_fifo #(.WIDTH(33), .SIZE(5)) ingress_timing_fifo
     (
      .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
      .i_tdata({i_tlast, i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready), .space(),
      .o_tdata({int0_tlast, int0_tdata}), .o_tvalid(int0_tvalid), .o_tready(int0_tready), .occupied()
      );

   //
   // This FIFO provides space to accept a single burst from FX3 and it's fullness drives flags to GPIF2 logic
   //
   axi_fifo_legacy #(.WIDTH(33), .SIZE(MTU)) min_read_buff
     (
      .clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
      .i_tdata({int0_tlast, int0_tdata}), .i_tvalid(int0_tvalid), .i_tready(int0_tready), .space(space),
      .o_tdata({int_tlast, int_tdata}), .o_tvalid(int_tvalid), .o_tready(int_tready), .occupied()
      );

   //
   // This logic allows signals to cross from the GPIF2 clock domain to the BUS clock domain.
   // It may now be obselete if bus_clk and gpif_clk are merged
   //
   wire [31:0] 	    chk_tdata;
   wire 	    chk_tlast;
   wire 	    chk_tvalid, chk_tready;

   axi_fifo_2clk #(.WIDTH(33), .SIZE(FIFO_SIZE)) cross_clock_fifo
     (
      .reset(fifo_rst | gpif_rst),
      .i_aclk(gpif_clk), .i_tdata({int_tlast, int_tdata}), .i_tvalid(int_tvalid), .i_tready(int_tready),
      .o_aclk(fifo_clk), .o_tdata({chk_tlast, chk_tdata}), .o_tvalid(chk_tvalid), .o_tready(chk_tready)
      );

   //
   // Performs basic tests on incomming packets such as testing if size on the wire patches
   // the internal size field. Uses axi_packet_gate internally so can back pressure upstream if
   // packet needs to be dropped.
   //
   wire [31:0] 	    o32_tdata;
   wire 	    o32_tlast;
   wire 	    o32_tvalid, o32_tready;

   gpif2_error_checker #(.SIZE(MTU)) checker
     (
      .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .i_tdata(chk_tdata), .i_tlast(chk_tlast), .i_tvalid(chk_tvalid), .i_tready(chk_tready),
      .o_tdata(o32_tdata), .o_tlast(o32_tlast), .o_tvalid(o32_tvalid), .o_tready(o32_tready),
      .bus_error(bus_error), .debug()
      );

   //assign o32_tdata = chk_tdata;
   //assign o32_tlast = chk_tlast;
   //assign o32_tvalid = chk_tvalid;
   //assign chk_tready = o32_tready;

   //
   // Convert 32bit AXIS bus to 64bit
   //
   axi_fifo32_to_fifo64 fifo32_to_fifo64
     (
      .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .i_tdata(o32_tdata), .i_tuser(2'b0/*always 32 bits*/), .i_tlast(o32_tlast), .i_tvalid(o32_tvalid), .i_tready(o32_tready),
      .o_tdata(o_tdata), .o_tuser(/*ignored cuz vita has len*/), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready)
      );


   /////////////////////////////////////////////
   //
   // Debug logic only
   //
   /////////////////////////////////////////////

   reg 		    o_tready_debug;
   reg 		    o_tvalid_debug;
   reg 		    o_tlast_debug;
   reg 		    i_tready_debug;
   reg 		    i_tvalid_debug;
   reg 		    i_tlast_debug;

   always @(posedge gpif_clk) begin
   	    o_tready_debug <= o_tready;
 	    o_tvalid_debug <= o_tvalid;
 	    o_tlast_debug <= o_tlast;
  	    i_tready_debug <= i_tready;
  	    i_tvalid_debug <= i_tvalid;
	    i_tlast_debug <= i_tlast;
   end

   assign debug = {26'h0,
		   o_tready_debug,
    		   o_tvalid_debug,
    		   o_tlast_debug,
    		   i_tready_debug,
    		   i_tvalid_debug,
    		   i_tlast_debug
		   };


endmodule //fifo_to_gpif2
