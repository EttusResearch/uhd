//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


//selectable conversion chain

module chdr_16sc_to_xxxx_chain
  #(parameter BASE = 0)
   (input 	  clk, input reset,

    input 	  set_stb, input [7:0] set_addr, input [31:0] set_data,

    input [63:0]  i_tdata,
    input 	  i_tlast,
    input 	  i_tvalid,
    output 	  i_tready,

    output [63:0] o_tdata,
    output 	  o_tlast,
    output 	  o_tvalid,
    input 	  o_tready,

    output [31:0] debug
    );

   //------------------------------------------------------------------
   // Demux destination setting register - safe switch for demux
   //------------------------------------------------------------------
   wire [1:0] 	  demux_dst;
   setting_reg #(.my_addr(BASE), .width(2), .at_reset(2'b00)) sr_demux_dst
     (.clk(clk),.rst(reset),
      .strobe(set_stb),.addr(set_addr), .in(set_data),
      .out({demux_dst}),.changed());

   //------------------------------------------------------------------
   // All FIFO IO lines
   //------------------------------------------------------------------
   wire [63:0] 	  i0_tdata; wire i0_tlast, i0_tvalid, i0_tready;
   wire [63:0] 	  i1_tdata; wire i1_tlast, i1_tvalid, i1_tready;
   wire [63:0] 	  i2_tdata; wire i2_tlast, i2_tvalid, i2_tready;
   wire [63:0] 	  i3_tdata; wire i3_tlast, i3_tvalid, i3_tready;

   wire [63:0] 	  o0_tdata; wire o0_tlast, o0_tvalid, o0_tready;
   wire [63:0] 	  o1_tdata; wire o1_tlast, o1_tvalid, o1_tready;
   wire [63:0] 	  o2_tdata; wire o2_tlast, o2_tvalid, o2_tready;
   wire [63:0] 	  o3_tdata; wire o3_tlast, o3_tvalid, o3_tready;

   //------------------------------------------------------------------
   // Instantiate converters
   //------------------------------------------------------------------
   assign {o0_tdata, o0_tlast, o0_tvalid, i0_tready} = {i0_tdata, i0_tlast, i0_tvalid, o0_tready};
   //assign {o1_tdata, o1_tlast, o1_tvalid, i1_tready} = {i1_tdata, i1_tlast, i1_tvalid, o1_tready};
   //assign {o2_tdata, o2_tlast, o2_tvalid, i2_tready} = {i2_tdata, i2_tlast, i2_tvalid, o2_tready};
   //assign {o3_tdata, o3_tlast, o3_tvalid, i3_tready} = {i3_tdata, i3_tlast, i3_tvalid, o3_tready};

   //leave path 0 for pass through
   
   chdr_16sc_to_12sc
     #(.BASE(89)) convert_16sc_to_12sc
      
       (.clk(clk), .reset(reset),.set_data(0), .set_stb(0), .set_addr(0),
       .i_tdata(i1_tdata), .i_tlast(i1_tlast), .i_tvalid(i1_tvalid), .i_tready(i1_tready),
       .o_tdata(o1_tdata), .o_tlast(o1_tlast), .o_tvalid(o1_tvalid), .o_tready(o1_tready)
       );

   chdr_16sc_to_32f
     #(.BASE(89)) convert_16sc_to_32f
      
       (.clk(clk), .reset(reset),.set_data(0), .set_stb(0), .set_addr(0),
       .i_tdata(i2_tdata), .i_tlast(i2_tlast), .i_tvalid(i2_tvalid), .i_tready(i2_tready),
       .o_tdata(o2_tdata), .o_tlast(o2_tlast), .o_tvalid(o2_tvalid), .o_tready(o2_tready)
       );

   chdr_16sc_to_8sc #(.BASE(89)) convert_16sc_to_8sc
     (.clk(clk), .reset(reset),.set_data(0), .set_stb(0), .set_addr(0),
      .i_tdata(i3_tdata), .i_tlast(i3_tlast), .i_tvalid(i3_tvalid), .i_tready(i3_tready),
      .o_tdata(o3_tdata), .o_tlast(o3_tlast), .o_tvalid(o3_tvalid), .o_tready(o3_tready)
      );
   

   //------------------------------------------------------------------
   // Ingress and Egress muxing
   //------------------------------------------------------------------
   //assign {o_tdata, o_tlast, o_tvalid, i_tready} = {i_tdata, i_tlast, i_tvalid, o_tready};
   ///*
   axi_demux4 #(.ACTIVE_CHAN(4'b1111), .WIDTH(64), .BUFFER(1)) demux_pack_chain
     (.clk(clk), .reset(reset), .clear(1'b0),
      .header(), .dest(demux_dst),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o0_tdata(i0_tdata), .o0_tlast(i0_tlast), .o0_tvalid(i0_tvalid), .o0_tready(i0_tready),
      .o1_tdata(i1_tdata), .o1_tlast(i1_tlast), .o1_tvalid(i1_tvalid), .o1_tready(i1_tready),
      .o2_tdata(i2_tdata), .o2_tlast(i2_tlast), .o2_tvalid(i2_tvalid), .o2_tready(i2_tready),
      .o3_tdata(i3_tdata), .o3_tlast(i3_tlast), .o3_tvalid(i3_tvalid), .o3_tready(i3_tready));

   axi_mux4 #(.PRIO(1), .WIDTH(64), .BUFFER(1)) mux_pack_chain
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i0_tdata(o0_tdata), .i0_tlast(o0_tlast), .i0_tvalid(o0_tvalid), .i0_tready(o0_tready),
      .i1_tdata(o1_tdata), .i1_tlast(o1_tlast), .i1_tvalid(o1_tvalid), .i1_tready(o1_tready),
      .i2_tdata(o2_tdata), .i2_tlast(o2_tlast), .i2_tvalid(o2_tvalid), .i2_tready(o2_tready),
      .i3_tdata(o3_tdata), .i3_tlast(o3_tlast), .i3_tvalid(o3_tvalid), .i3_tready(o3_tready),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));
   //*/

endmodule //chdr_16sc_to_xxxx_chain
