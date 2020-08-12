//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// AXI-Stream multiplexer with select line
//

module axi_mux_select #(
  parameter WIDTH = 32,
  parameter PRE_FIFO_SIZE = 0,
  parameter POST_FIFO_SIZE = 0,
  parameter SWITCH_ON_LAST = 0, // Wait until tlast is asserted before updating
  parameter SIZE = 4)
(
  input clk, input reset, input clear,
  input [$clog2(SIZE)-1:0] select,
  input [SIZE*WIDTH-1:0] i_tdata, input [SIZE-1:0] i_tlast, input [SIZE-1:0] i_tvalid, output [SIZE-1:0] i_tready,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire [WIDTH*SIZE-1:0] i_tdata_int;
  wire [WIDTH-1:0] i_tdata_arr[0:SIZE-1];
  wire [SIZE-1:0] i_tlast_int, i_tvalid_int, i_tready_int;

  wire [WIDTH-1:0] o_tdata_int;
  wire o_tlast_int, o_tvalid_int, o_tready_int;

  genvar n;
  generate
    if (PRE_FIFO_SIZE == 0) begin : gen_no_pre_fifo
      assign i_tdata_int  = i_tdata;
      assign i_tlast_int  = i_tlast;
      assign i_tvalid_int = i_tvalid;
      assign i_tready     = i_tready_int;
    end else begin : gen_pre_fifo
      for (n = 0; n < SIZE; n = n + 1) begin
        axi_fifo #(.WIDTH(WIDTH+1), .SIZE(PRE_FIFO_SIZE)) axi_fifo (
          .clk(clk), .reset(reset), .clear(clear),
          .i_tdata({i_tlast[n],i_tdata[WIDTH*(n+1)-1:WIDTH*n]}), .i_tvalid(i_tvalid[n]), .i_tready(i_tready[n]),
          .o_tdata({i_tlast_int[n],i_tdata_int[WIDTH*(n+1)-1:WIDTH*n]}), .o_tvalid(i_tvalid_int[n]), .o_tready(i_tready_int[n]),
          .space(), .occupied());
      end
    end
  endgenerate

  // Make arrays for easier muxing
  genvar i;
  generate
    for (i = 0; i < SIZE; i = i + 1) begin : gen_muxing
      assign i_tdata_arr[i] = i_tdata_int[WIDTH*(i+1)-1:WIDTH*i];
    end
  endgenerate

  // Switch select line either immediately or if we're not in the middle of a
  // packet.
  reg [$clog2(SIZE)-1:0] select_hold = 0;

  generate
    if (SWITCH_ON_LAST) begin : gen_switch_on_last
      reg  in_packet_reg = 1'b0;
      wire in_packet;
      wire end_of_packet;
      wire enable_switch;

      // Create a signal to indicate if we're in the middle of a packet
      assign in_packet = in_packet_reg || o_tvalid_int;

      // Create a signal to indicate if this is the last transfer of the packet
      assign end_of_packet = o_tlast_int & o_tvalid_int & o_tready_int;

      // Create a signal that indicates when it's OK to switch the mux select.
      // We can switch if we're not in the middle of outputting a packet, or if
      // we're on the last transfer of a packet.
      assign enable_switch = !in_packet || end_of_packet;

      always @(posedge clk) begin
        if (reset | clear) begin
          select_hold   <= 0;
          in_packet_reg <= 1'b0;
        end else begin
          // Use in_packet_reg to indicate if we're in a packet. But this
          // register is delayed by a clock cycle, so we need the in_pakcet
          // signal above to add the first clock cycle of a packet.
          if (end_of_packet) begin
            in_packet_reg <= 1'b0;
          end else if (o_tvalid_int) begin
            in_packet_reg <= 1'b1;
          end

          if (enable_switch) begin
            select_hold <= select;
          end
        end
      end
    end else begin : gen_no_switch_on_last
      always @(*) begin
        select_hold <= select;
      end
    end
  endgenerate

  // Mux
  assign o_tdata_int  = i_tdata_arr[select_hold];
  assign o_tlast_int  = i_tlast_int[select_hold];
  assign o_tvalid_int = i_tvalid_int[select_hold];
  assign i_tready_int = (1'b1 << select_hold) & {SIZE{o_tready_int}};

  generate
    if(POST_FIFO_SIZE == 0) begin : gen_no_post_fifo
      assign o_tdata = o_tdata_int;
      assign o_tlast = o_tlast_int;
      assign o_tvalid = o_tvalid_int;
      assign o_tready_int = o_tready;
    end else begin : gen_post_fifo
      axi_fifo #(.WIDTH(WIDTH+1),.SIZE(POST_FIFO_SIZE)) axi_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({o_tlast_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
        .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied());
    end
  endgenerate

endmodule