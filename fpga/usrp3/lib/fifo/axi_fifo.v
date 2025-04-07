//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo
//
// Description:
//
//   General FIFO block
//
// Parameters:
//
//   WIDTH : Width of tdata in bits
//   SIZE  : Log base 2 of the depth of the FIFO. In other words, the size of
//           the FIFO is 2**SIZE elements deep. The implementation changes
//           depending on the FIFO size, as described below.
//
//   SIZE  < 0 : No FIFO (data passes through without any buffer)
//   SIZE == 0 : Uses a single stage flop (axi_fifo_flop)
//   SIZE == 1 : Uses a two-stage flop (axi_fifo_flop2). Best choice for single
//               stage pipelining. Breaks combinatorial paths on the AXI stream
//               data / control lines at the cost of additional registers. Maps
//               to SLICELs (i.e. does not use distributed RAM).
//   SIZE <= 5 : Uses SRL32 to efficient maps a 32 deep FIFO to SLICEMs
//               (axi_fifo_short). Not recommended for pipelining as most
//               devices have twice as many SLICELs as SLICEMs.
//   SIZE  > 5 : Uses BRAM FIFO (axi_fifo_bram)
//

`default_nettype none


module axi_fifo #(
  parameter WIDTH = 32,
  parameter SIZE  = 5
) (
  input  wire             clk,
  input  wire             reset,
  input  wire             clear,

  input  wire [WIDTH-1:0] i_tdata,
  input  wire             i_tvalid,
  output wire             i_tready,

  output wire [WIDTH-1:0] o_tdata,
  output wire             o_tvalid,
  input  wire             o_tready,

  output wire [     15:0] space,
  output wire [     15:0] occupied
);

   generate
     if (SIZE < 0) begin : gen_no_fifo
      assign o_tdata  = i_tdata;
      assign o_tvalid = i_tvalid;
      assign i_tready = o_tready;
      assign space    = 16'h0;
      assign occupied = 16'h0;
     end
     else if (SIZE == 0) begin : gen_fifo_flop
        axi_fifo_flop #(
          .WIDTH(WIDTH)
        ) fifo_flop (
          .clk     (clk),
          .reset   (reset),
          .clear   (clear),
          .i_tdata (i_tdata),
          .i_tvalid(i_tvalid),
          .i_tready(i_tready),
          .o_tdata (o_tdata),
          .o_tvalid(o_tvalid),
          .o_tready(o_tready),
          .space   (space[0]),
          .occupied(occupied[0])
        );
        assign space[15:1] = 15'd0;
        assign occupied[15:1] = 15'd0;
     end
     else if (SIZE == 1) begin : gen_fifo_flop2
        axi_fifo_flop2 #(
          .WIDTH(WIDTH)
        ) fifo_flop2 (
          .clk     (clk),
          .reset   (reset),
          .clear   (clear),
          .i_tdata (i_tdata),
          .i_tvalid(i_tvalid),
          .i_tready(i_tready),
          .o_tdata (o_tdata),
          .o_tvalid(o_tvalid),
          .o_tready(o_tready),
          .space   (space[1:0]),
          .occupied(occupied[1:0])
        );
        assign space[15:2] = 14'd0;
        assign occupied[15:2] = 14'd0;
     end
     else if (SIZE <= 5) begin : gen_fifo_short
        axi_fifo_short #(
          .WIDTH(WIDTH)
        ) fifo_short (
          .clk     (clk),
          .reset   (reset),
          .clear   (clear),
          .i_tdata (i_tdata),
          .i_tvalid(i_tvalid),
          .i_tready(i_tready),
          .o_tdata (o_tdata),
          .o_tvalid(o_tvalid),
          .o_tready(o_tready),
          .space   (space[5:0]),
          .occupied(occupied[5:0])
        );
        assign space[15:6] = 10'd0;
        assign occupied[15:6] = 10'd0;
     end
     else begin : gen_fifo_bram
        axi_fifo_bram #(
          .WIDTH(WIDTH),
          .SIZE (SIZE)
        ) fifo_bram (
          .clk     (clk),
          .reset   (reset),
          .clear   (clear),
          .i_tdata (i_tdata),
          .i_tvalid(i_tvalid),
          .i_tready(i_tready),
          .o_tdata (o_tdata),
          .o_tvalid(o_tvalid),
          .o_tready(o_tready),
          .space   (space),
          .occupied(occupied)
        );
     end
   endgenerate

endmodule // axi_fifo


`default_nettype wire
