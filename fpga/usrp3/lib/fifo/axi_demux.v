//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_demux
//
// Description:
//
//   Implements a 1-to-SIZE demultiplexer for AXI-Stream. Switching output
//   paths happens only between packets. A bubble cycle is added between each
//   packet to allow the switching. The output to use for the next packet is
//   selected using the dest input.
//
//   The header output contains a copy of i_tdata and can be used when the
//   first word of the packet contains information that should be used to
//   determine the destination using combinational logic.
//

`default_nettype none


module axi_demux #(
  parameter WIDTH          = 64,
  parameter SIZE           = 4,
  parameter PRE_FIFO_SIZE  = 0,
  parameter POST_FIFO_SIZE = 0
) (
  input  wire                    clk,
  input  wire                    reset,
  input  wire                    clear,

  output wire [       WIDTH-1:0] header,
  input  wire [$clog2(SIZE)-1:0] dest,

  input  wire [       WIDTH-1:0] i_tdata,
  input  wire                    i_tlast,
  input  wire                    i_tvalid,
  output wire                    i_tready,

  output wire [(WIDTH*SIZE)-1:0] o_tdata,
  output wire [        SIZE-1:0] o_tlast,
  output wire [        SIZE-1:0] o_tvalid,
  input  wire [        SIZE-1:0] o_tready
);

  //---------------------------------------------------------------------------
  // Optional Input FIFO
  //---------------------------------------------------------------------------

  wire             i_tlast_int;
  wire             i_tready_int;
  wire             i_tvalid_int;
  wire [WIDTH-1:0] i_tdata_int;

  generate
    if (PRE_FIFO_SIZE == 0) begin : gen_no_pre_fifo
      assign i_tlast_int = i_tlast;
      assign i_tdata_int = i_tdata;
      assign i_tvalid_int = i_tvalid;
      assign i_tready = i_tready_int;
    end else begin : gen_pre_fifo
      axi_fifo #(
        .WIDTH(WIDTH+1),
        .SIZE(PRE_FIFO_SIZE)
      ) axi_fifo_i (
        .clk     (clk),
        .reset   (reset),
        .clear   (clear),
        .i_tdata ({i_tlast,i_tdata}),
        .i_tvalid(i_tvalid),
        .i_tready(i_tready),
        .o_tdata ({i_tlast_int,i_tdata_int}),
        .o_tvalid(i_tvalid_int),
        .o_tready(i_tready_int),
        .space   (),
        .occupied()
      );
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Demultiplexer Logic
  //---------------------------------------------------------------------------

  assign header = i_tdata_int;

  // The state vector indicates which output is selected
  reg [SIZE-1:0] st = {SIZE{1'b0}};

  always @(posedge clk) begin
    if(reset | clear) begin
      st <= {SIZE{1'b0}};
    end else begin
      // If state is 0, that means no output path is selected and we are free
      // to select the next destination.
      if(st == 0) begin
        if(i_tvalid_int) begin
          st[dest] <= 1'b1;
        end
      end else begin
        // At the end of the packet, clear the state
        if(i_tready_int & i_tvalid_int & i_tlast_int) begin
          st <= {SIZE{1'b0}};
        end
      end
    end
  end

  //---------------------------------------------------------------------------
  // Optional Output FIFO(s)
  //---------------------------------------------------------------------------

  genvar n;
  generate
    if (POST_FIFO_SIZE == 0) begin : gen_no_post_fifo
      assign o_tdata      = {SIZE{i_tdata_int}};
      assign o_tlast      = {SIZE{i_tlast_int}};
      assign o_tvalid     = {SIZE{i_tvalid_int}} & st;
      assign i_tready_int = |(o_tready & st);
    end else begin : gen_post_fifo
      wire [SIZE-1:0] i_tready_fifo;
      wire [SIZE-1:0] i_tvalid_fifo;

      // Only read tready from and drive tvalid to the currently selected
      // output.
      assign i_tready_int  = |(i_tready_fifo & st);
      assign i_tvalid_fifo = {SIZE{i_tvalid_int}} & st;

      for (n = 0; n < SIZE; n = n + 1) begin : gen_post_axi_fifo
        axi_fifo #(
          .WIDTH(WIDTH+1),
          .SIZE (POST_FIFO_SIZE)
        ) axi_fifo_i (
          .clk     (clk),
          .reset   (reset),
          .clear   (clear),
          .i_tdata ({i_tlast_int, i_tdata_int}),
          .i_tvalid(i_tvalid_fifo[n]),
          .i_tready(i_tready_fifo[n]),
          .o_tdata ({o_tlast[n], o_tdata[WIDTH*(n+1)-1:WIDTH*n]}),
          .o_tvalid(o_tvalid[n]),
          .o_tready(o_tready[n]),
          .space   (),
          .occupied()
        );
      end
    end
  endgenerate

 endmodule // axi_demux


`default_nettype wire
