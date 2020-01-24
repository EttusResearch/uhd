/////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: regport_to_settingsbus
// Description:
//   Converts regport write bus to the a setting bus
//   ADDRESSING: Set to "WORD" in case of settings bus. The settings bus
//   uses word addressing and hence the address needs to be shifted by
//   to convert to set_addr.
//
/////////////////////////////////////////////////////////////////////

module regport_to_settingsbus #(
  parameter BASE   = 14'h0,
  parameter END_ADDR = 14'h3FFF,
  parameter DWIDTH = 32,
  parameter AWIDTH = 14,
  parameter SR_AWIDTH = 12,
  // Dealign for settings bus by shifting by 2
  parameter ADDRESSING = "WORD",
  parameter SHIFT = $clog2(DWIDTH/8)
)(
  input reset,
  input clk,
  input reg_wr_req,
  input [AWIDTH-1:0] reg_wr_addr,
  input [DWIDTH-1:0] reg_wr_data,

  output reg set_stb,
  output reg [SR_AWIDTH-1:0] set_addr,
  output reg [DWIDTH-1:0] set_data
);

  wire set_stb_int;
  wire [DWIDTH-1:0] set_data_int;
  wire [SR_AWIDTH-1:0] set_addr_base;
  wire [SR_AWIDTH-1:0] set_addr_int;

  // Strobe asserted only when address is between BASE and END ADDR
  assign set_stb_int = reg_wr_req && (reg_wr_addr >= BASE) && (reg_wr_addr <= END_ADDR);
  assign set_addr_base = reg_wr_addr - BASE;
  // Shift by 2 in case of setting bus
  assign set_addr_int = (ADDRESSING == "WORD") ? {{SHIFT{1'b0}}, set_addr_base[SR_AWIDTH-1:SHIFT]}
                                               : set_addr_base[SR_AWIDTH-1:0];
  assign set_data_int = reg_wr_data;

  // Adding a pipeline stage
  always @(posedge clk) begin
    if (reset) begin
      set_stb  <= 'b0;
      set_addr <= 'h0;
      set_data <= 'h0;
    end else begin
      set_stb  <= set_stb_int;
      set_addr <= set_addr_int;
      set_data <= set_data_int;
    end
  end

endmodule // regport_to_settingsbus
