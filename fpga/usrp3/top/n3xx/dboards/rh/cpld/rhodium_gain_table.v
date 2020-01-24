///////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rhodium_gain_table
// Description:
//   Simple dual port memory for use as gain table
//   Implements a 128 x 16 bit dual-port RAM for storing 10-bit gain values.
//   Write and read domains are independent. Data takes 1 cycle to become valid
//   on the output of the RAM once written.
//////////////////////////////////////////////////////////////////////

`default_nettype none

module rhodium_gain_table
(
  input wire        wr_clk,
  input wire        wr_en,
  input wire  [6:0] wr_addr,
  input wire  [9:0] wr_data,

  // Read data for wr_addr (read-first/read-before-write): One cycle latency
  output wire [9:0] wr_data_prev,

  input wire        rd_clk,
  input wire  [6:0] rd_addr,
  output wire [9:0] rd_data // Read data for rd_addr: One cycle latency
);

reg [15:0] gain_table[127:0];
reg [15:0] wr_data_prev_r;
reg [15:0] rd_data_r;

assign wr_data_prev = wr_data_prev_r[15:6];
assign rd_data      = rd_data_r[15:6];

always @ (posedge wr_clk)
begin
  if (wr_en)
    gain_table[wr_addr] <= {wr_data, 6'b0};
  wr_data_prev_r <= gain_table[wr_addr];
end

always @ (posedge rd_clk)
begin
  rd_data_r <= gain_table[rd_addr];
end


endmodule // rhodium_gain_table
`default_nettype wire

