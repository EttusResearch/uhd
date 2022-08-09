//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_mux
//
// Description:
//
//   Takes arbitrary number of AXI streams and merges them to into a single
//   output channel. Bubble cycles are inserted after each packet.
//
// Parameters:
//
//   PRIO           : Controls the arbitration scheme.
//                    0 - Round-robin
//                    1 - Priority (lower number ports get priority)
//   WIDTH          : Width of each AXI-Stream (width of TDATA).
//   PRE_FIFO_SIZE  : Log2 of the input buffer FIFO. Set to 0 for no FIFO.
//   POST_FIFO_SIZE : Log2 of the output buffer FIFO. Set to 0 for no FIFO.
//   SIZE           : Number of input ports to the multiplexer.
//

`default_nettype none


module axi_mux #(
  parameter PRIO           = 0,
  parameter WIDTH          = 64,
  parameter PRE_FIFO_SIZE  = 0,
  parameter POST_FIFO_SIZE = 0,
  parameter SIZE           = 4
) (
  input  wire                  clk,
  input  wire                  reset,
  input  wire                  clear,

  // Input streams
  input  wire [WIDTH*SIZE-1:0] i_tdata,
  input  wire [      SIZE-1:0] i_tlast,
  input  wire [      SIZE-1:0] i_tvalid,
  output wire [      SIZE-1:0] i_tready,

  // Single output stream
  output wire [     WIDTH-1:0] o_tdata,
  output wire                  o_tlast,
  output wire                  o_tvalid,
  input  wire                  o_tready
);

  wire [WIDTH*SIZE-1:0] i_tdata_int;
  wire [      SIZE-1:0] i_tlast_int;
  wire [      SIZE-1:0] i_tvalid_int;
  wire [      SIZE-1:0] i_tready_int;

  wire [WIDTH-1:0] o_tdata_int;
  wire             o_tlast_int;
  wire             o_tvalid_int;
  wire             o_tready_int;

  reg [$clog2(SIZE)-1:0] st_port;
  reg                    st_active;

  //---------------------------------------------------------------------------
  // Input FIFO
  //---------------------------------------------------------------------------

  genvar n;
  generate
    if (PRE_FIFO_SIZE == 0) begin : gen_no_pre_fifo
      assign i_tdata_int  = i_tdata;
      assign i_tlast_int  = i_tlast;
      assign i_tvalid_int = i_tvalid;
      assign i_tready     = i_tready_int;
    end else begin : gen_pre_fifo
      for (n = 0; n < SIZE; n = n + 1) begin
        axi_fifo #(
          .WIDTH(WIDTH+1      ),
          .SIZE (PRE_FIFO_SIZE)
        ) axi_fifo (
          .clk     (clk                                                ),
          .reset   (reset                                              ),
          .clear   (clear                                              ),
          .i_tdata ({i_tlast[n],i_tdata[WIDTH*(n+1)-1:WIDTH*n]}        ),
          .i_tvalid(i_tvalid[n]                                        ),
          .i_tready(i_tready[n]                                        ),
          .o_tdata ({i_tlast_int[n],i_tdata_int[WIDTH*(n+1)-1:WIDTH*n]}),
          .o_tvalid(i_tvalid_int[n]                                    ),
          .o_tready(i_tready_int[n]                                    ),
          .space   (                                                   ),
          .occupied(                                                   )
        );
      end
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Multiplexer Logic
  //---------------------------------------------------------------------------

  always @(posedge clk) begin
    if (reset) begin
        st_port <= 0;
        st_active <= 1'b0;
    end else begin
      if (st_active) begin
        if (o_tlast_int & o_tvalid_int & o_tready_int) begin
          st_active <= 1'b0;
          if ((PRIO != 0) | (st_port == (SIZE-1))) begin
            st_port <= 0;
          end else begin
            st_port <= st_port + 1;
          end
        end
      end else begin
        if (i_tvalid_int[st_port]) begin
          st_active <= 1'b1;
        end else begin
          if (st_port == (SIZE-1)) begin
            st_port <= 0;
          end else begin
            st_port <= st_port + 1;
          end
        end
      end
    end
  end

  genvar i;
  generate
    for (i=0; i<SIZE; i=i+1) begin : gen_tready
      assign i_tready_int[i] = st_active & o_tready_int & (st_port == i);
    end
  endgenerate

  assign o_tvalid_int = st_active & i_tvalid_int[st_port];
  assign o_tlast_int  = i_tlast_int[st_port];

  genvar j;
  generate
    for (j=0; j<WIDTH; j=j+1) begin : gen_tdata
     assign o_tdata_int[j] = i_tdata_int[st_port*WIDTH+j];
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------

  generate
    if (POST_FIFO_SIZE == 0) begin
      assign o_tdata      = o_tdata_int;
      assign o_tlast      = o_tlast_int;
      assign o_tvalid     = o_tvalid_int;
      assign o_tready_int = o_tready;
    end else begin
      axi_fifo #(
        .WIDTH(WIDTH+1       ),
        .SIZE (POST_FIFO_SIZE)
      ) axi_fifo (
        .clk     (clk                      ),
        .reset   (reset                    ),
        .clear   (clear                    ),
        .i_tdata ({o_tlast_int,o_tdata_int}),
        .i_tvalid(o_tvalid_int             ),
        .i_tready(o_tready_int             ),
        .o_tdata ({o_tlast,o_tdata}        ),
        .o_tvalid(o_tvalid                 ),
        .o_tready(o_tready                 ),
        .space   (                         ),
        .occupied(                         )
      );
    end
  endgenerate

endmodule


`default_nettype wire
