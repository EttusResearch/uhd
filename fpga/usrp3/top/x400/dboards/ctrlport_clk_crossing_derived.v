//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_clk_crossing_derived
//
// Description:
//     Performs a simplified clk crossing for a ctrlport interface.
//     i_clk must be derived from o_clk by an integer multiplier and
//     originate from the same PLL. This ensures the clock crossing
//     can be achieved by using simple registers, as STA will be able
//     to meet setup and hold requirements on them.
//

`default_nettype none

module ctrlport_clk_crossing_derived (
  // Clocks
  input  wire i_clk,
  input  wire o_clk,

  // Request (domain: i_clk)
  input  wire        i_ctrlport_rst,
  input  wire        i_ctrlport_req_wr,
  input  wire        i_ctrlport_req_rd,
  input  wire [19:0] i_ctrlport_req_addr,
  input  wire [31:0] i_ctrlport_req_data,

  // Response (domain: i_clk)
  output wire        i_ctrlport_resp_ack,
  output reg  [ 1:0] i_ctrlport_resp_status,
  output reg  [31:0] i_ctrlport_resp_data,

  // Request (domain: o_clk)
  output reg         o_ctrlport_rst,
  output reg         o_ctrlport_req_wr,
  output reg         o_ctrlport_req_rd,
  output reg  [19:0] o_ctrlport_req_addr,
  output reg  [31:0] o_ctrlport_req_data,

  // Response (domain: o_clk)
  input  wire        o_ctrlport_resp_ack,
  input  wire [ 1:0] o_ctrlport_resp_status,
  input  wire [31:0] o_ctrlport_resp_data

);

  // holding read and write flags for multiple i_clk cycles
  reg         ctrlport_req_wr_hold = 1'b0;
  reg         ctrlport_req_rd_hold = 1'b0;

  reg         ctrlport_req_rd_fall      = 1'b0;
  reg         ctrlport_req_wr_fall      = 1'b0;
  reg  [31:0] ctrlport_resp_data_fall   = 32'b0;
  reg  [ 1:0] ctrlport_resp_status_fall = 2'b0;
  reg         ctrlport_resp_ack_fall    = 1'b0;

  // Retime signals to falling edge of i_clk. By sampling on the falling edge of
  // i_clk, we provide (nominally) half a i_clk period of hold, while
  // reducing setup time by half. The late arrival of i_clk adds back some
  // of the lost setup margin.
  always @(negedge i_clk) begin
    ctrlport_req_rd_fall      <= o_ctrlport_req_rd;
    ctrlport_req_wr_fall      <= o_ctrlport_req_wr;
    ctrlport_resp_ack_fall    <= o_ctrlport_resp_ack;
    ctrlport_resp_status_fall <= o_ctrlport_resp_status;
    ctrlport_resp_data_fall   <= o_ctrlport_resp_data;
  end

  always @(posedge i_clk) begin
    if (ctrlport_req_wr_fall) begin
      ctrlport_req_wr_hold <= 1'b0;
    end else if (i_ctrlport_req_wr) begin
      ctrlport_req_wr_hold <= 1'b1;
    end
    if (ctrlport_req_rd_fall) begin
      ctrlport_req_rd_hold <= 1'b0;
    end else if (i_ctrlport_req_rd) begin
      ctrlport_req_rd_hold <= 1'b1;
    end

    // capture request address and data
    if (i_ctrlport_req_wr || i_ctrlport_req_rd) begin
      o_ctrlport_req_addr <= i_ctrlport_req_addr;
      o_ctrlport_req_data <= i_ctrlport_req_data;
    end
  end

  // capture extended flags in o_clk domain
  always @(posedge o_clk) begin
    o_ctrlport_req_wr <= ctrlport_req_wr_hold;
    o_ctrlport_req_rd <= ctrlport_req_rd_hold;
  end

  // search for rising edge in response
  reg [1:0] ctrlport_resp_ack_reg = 2'b0;
  always @(posedge i_clk) begin
    ctrlport_resp_ack_reg = {ctrlport_resp_ack_reg[0], ctrlport_resp_ack_fall};
  end
  assign i_ctrlport_resp_ack = ctrlport_resp_ack_reg[0] & ~ctrlport_resp_ack_reg[1];

  // capture response data
  always @(posedge i_clk) begin
    if (ctrlport_resp_ack_fall) begin
      i_ctrlport_resp_status <= ctrlport_resp_status_fall;
      i_ctrlport_resp_data   <= ctrlport_resp_data_fall;
    end
  end

  // transfer reset
  reg ctrlport_rst_hold = 1'b0;
  reg ctrlport_rst_fall = 1'b0;
  always @(posedge i_clk) begin
    if (i_ctrlport_rst) begin
      ctrlport_rst_hold <= 1'b1;
    end else if (ctrlport_rst_fall) begin
      ctrlport_rst_hold <= 1'b0;
    end
  end
  always @(posedge o_clk) begin
    o_ctrlport_rst <= ctrlport_rst_hold;
  end
  always @(negedge i_clk) begin
    ctrlport_rst_fall <= o_ctrlport_rst;
  end

endmodule

`default_nettype wire
