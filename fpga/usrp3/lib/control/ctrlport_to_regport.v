//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_regport
//
// Description:
//
//   This is a bridge that converts a CtrlPort request to a RegPort request. No
//   address translation or filtering is performed. In the case where the
//   REG_AWIDTH and/or REG_DWIDTH parameters don't match the widths used by
//   CtrlPort, the address and data are simply resized according to Verilog
//   rules.
//
//   Because RegPort doesn't acknowledge writes, this block will acknowledge
//   every write request. The designer must ensure that all requests are for
//   the address space behind this bridge or else they may get an unintended
//   write acknowledgment from this block.
//
// Parameters:
//
//   REG_AWIDTH : Width of the RegPort address bus
//   REG_DWIDTH : Width of the RegPort data bugs


module ctrlport_to_regport #(
  parameter REG_AWIDTH = 20,
  parameter REG_DWIDTH = 32
) (
  input clk,
  input rst,

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack   = 1'b0,
  output reg  [31:0] s_ctrlport_resp_data  = 'bX,

  output reg                   reg_wr_req   = 1'b0,
  output reg  [REG_AWIDTH-1:0] reg_wr_addr  = 'bX,
  output reg  [REG_DWIDTH-1:0] reg_wr_data  = 'bX,
  output reg                   reg_rd_req   = 1'b0,
  output reg  [REG_AWIDTH-1:0] reg_rd_addr  = 'bX,
  input  wire                  reg_rd_resp,
  input  wire [REG_DWIDTH-1:0] reg_rd_data
);

  always @(posedge clk) begin
    if (rst) begin
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_data <= 'bx;
      reg_wr_req           <= 1'b0;
      reg_wr_addr          <= 'bX;
      reg_wr_data          <= 'bX;
      reg_rd_req           <= 1'b0;
      reg_rd_addr          <= 'bX;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack <= 1'b0;
      reg_wr_req          <= 1'b0;
      reg_rd_req          <= 1'b0;

      // Translate write requests
      if (s_ctrlport_req_wr) begin
        reg_wr_req  <= 1'b1;
        reg_wr_addr <= s_ctrlport_req_addr;
        reg_wr_data <= s_ctrlport_req_data;

        // RegPort has no write acknowledge, so we acknowledge every write
        s_ctrlport_resp_ack <= 1'b1;
      end

      // Translate read requests
      if (s_ctrlport_req_rd) begin
        reg_rd_req  <= 1'b1;
        reg_rd_addr <= s_ctrlport_req_addr;
      end

      // Translate read responses
      if (reg_rd_resp) begin
        s_ctrlport_resp_ack  <= 1'b1;
        s_ctrlport_resp_data <= reg_rd_data;
      end

    end
  end

endmodule
