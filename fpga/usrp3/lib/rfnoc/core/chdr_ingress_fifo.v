//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module chdr_ingress_fifo #(
  parameter WIDTH  = 64,
  parameter SIZE   = 12,
  parameter DEVICE = "7SERIES"
) (
  input              clk,
  input              reset,
  input              clear,

  input [WIDTH-1:0]  i_tdata,
  input              i_tlast,
  input              i_tvalid,
  output             i_tready,

  output [WIDTH-1:0] o_tdata,
  output             o_tlast,
  output             o_tvalid,
  input              o_tready
);

  localparam SIZE_THRESHOLD = (
    (DEVICE == "7SERIES")   ? 14 : (
    (DEVICE == "VIRTEX6")   ? 14 : (
    (DEVICE == "SPARTAN6")  ? 12 : (
    12
  ))));

  wire [WIDTH-1:0] i_tdata_pre;
  wire             i_tlast_pre, i_tvalid_pre, i_tready_pre;
  
  // SRL based FIFO to break timing paths to BRAM resources
  axi_fifo_flop2 #(.WIDTH(WIDTH+1)) pre_fifo (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({i_tlast, i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({i_tlast_pre, i_tdata_pre}), .o_tvalid(i_tvalid_pre), .o_tready(i_tready_pre),
    .space(), .occupied()
  );

  generate 
    if (SIZE <= SIZE_THRESHOLD) begin
      wire [WIDTH-1:0] o_tdata_int;
      wire             o_tlast_int, o_tvalid_int, o_tready_int;
      // Instantiate a single axi_fifo if size is not larger than threshold
      axi_fifo #(.WIDTH(WIDTH+1), .SIZE(SIZE)) main_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({i_tlast_pre, i_tdata_pre}), .i_tvalid(i_tvalid_pre), .i_tready(i_tready_pre),
        .o_tdata({o_tlast_int, o_tdata_int}), .o_tvalid(o_tvalid_int), .o_tready(o_tready_int),
        .space(), .occupied()
      );
      axi_fifo_flop2 #(.WIDTH(WIDTH+1)) fifo_flop2 (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({o_tlast_int, o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
        .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied()
      );
    end else begin
      // Instantiate a cascade of axi_fifos if size is larger than threshold
      localparam CDEPTH = 2**(SIZE - SIZE_THRESHOLD); //Cascade Depth
      wire [WIDTH-1:0] c_tdata[CDEPTH:0], int_tdata[CDEPTH-1:0];
      wire             c_tlast[CDEPTH:0], c_tvalid[CDEPTH:0], c_tready[CDEPTH:0];
      wire             int_tlast[CDEPTH-1:0], int_tvalid[CDEPTH-1:0], int_tready[CDEPTH-1:0];
      
      //Connect input to first cascade state
      assign {c_tdata[0], c_tlast[0], c_tvalid[0]} = {i_tdata_pre, i_tlast_pre, i_tvalid_pre};
      assign i_tready_pre = c_tready[0];
      //Connect output to last cascade state
      assign {o_tdata, o_tlast, o_tvalid} = {c_tdata[CDEPTH], c_tlast[CDEPTH], c_tvalid[CDEPTH]};
      assign c_tready[CDEPTH] = o_tready;

      genvar i;
      for (i=0; i<CDEPTH; i=i+1) begin: fifo_stages
        axi_fifo #(.WIDTH(WIDTH+1), .SIZE(SIZE_THRESHOLD)) main_fifo (
          .clk(clk), .reset(reset), .clear(clear),
          .i_tdata({c_tlast[i], c_tdata[i]}), .i_tvalid(c_tvalid[i]), .i_tready(c_tready[i]),
          .o_tdata({int_tlast[i], int_tdata[i]}), .o_tvalid(int_tvalid[i]), .o_tready(int_tready[i]),
          .space(), .occupied()
        );
        axi_fifo_flop2 #(.WIDTH(WIDTH+1)) fifo_flop2 (
          .clk(clk), .reset(reset), .clear(clear),
          .i_tdata({int_tlast[i], int_tdata[i]}), .i_tvalid(int_tvalid[i]), .i_tready(int_tready[i]),
          .o_tdata({c_tlast[i+1], c_tdata[i+1]}), .o_tvalid(c_tvalid[i+1]), .o_tready(c_tready[i+1]),
          .space(), .occupied()
        );
      end
    end
  endgenerate

endmodule // axi_fifo_large
