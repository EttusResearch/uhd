//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Settings register with AXI stream output.
//
// Parameters / common use cases:
// USE_ADDR_LAST & ADDR_LAST  User wants additional address that when written to asserts tlast.
//                            Useful for the last word in a packet.
// USE_FIFO & FIFO_SIZE       Downstream block can throttle and a FIFO is needed to handle that case.
// STROBE_LAST                User always wants to assert tlast on writes. More efficient than USE_ADDR_LAST
//                            since only one address is used instead of two.
// REPEATS                    Keep tvalid asserted after initial write.
// STROBE_LAST & REPEATS      tlast is asserted on the initial write then deasserted for repeating output. 
// MSB_ALIGN                  Left justify data versus right justify.

module axi_setting_reg #(
  parameter ADDR = 0,
  parameter USE_ADDR_LAST = 0,
  parameter ADDR_LAST = ADDR+1,
  parameter AWIDTH = 8,
  parameter WIDTH = 32,
  parameter USE_FIFO = 0,
  parameter FIFO_SIZE = 5,
  parameter DATA_AT_RESET = 0,
  parameter VALID_AT_RESET = 0,
  parameter LAST_AT_RESET = 0,
  parameter STROBE_LAST = 0,
  parameter REPEATS = 0,
  parameter MSB_ALIGN = 0
)
(
  input clk, input reset, output reg error_stb,
  input set_stb, input [AWIDTH-1:0] set_addr, input [31:0] set_data,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  reg init;

  reg [WIDTH-1:0] o_tdata_int;
  reg o_tlast_int, o_tvalid_int;
  wire o_tready_int;

  always @(posedge clk) begin
    if (reset) begin
      o_tdata_int <= DATA_AT_RESET;
      o_tvalid_int <= VALID_AT_RESET;
      o_tlast_int <= LAST_AT_RESET;
      init <= 1'b0;
      error_stb <= 1'b0;
    end else begin
      error_stb <= 1'b0;
      if (o_tvalid_int & o_tready_int) begin
        // Deassert tvalid / tlast only if not repeating the output
        if (REPEATS == 0) begin
          o_tvalid_int <= 1'b0;
        end
        if ((REPEATS == 0) | (STROBE_LAST == 1)) begin
          o_tlast_int <= 1'b0;
        end
      end
      if (set_stb & ((ADDR[AWIDTH-1:0] == set_addr) | (USE_ADDR_LAST & (ADDR_LAST[AWIDTH-1:0] == set_addr)))) begin
        init <= 1'b1;
        o_tdata_int <= (MSB_ALIGN == 0) ? set_data[WIDTH-1:0] : set_data[31:32-WIDTH];
        o_tvalid_int <= 1'b1;
        if (set_stb & (STROBE_LAST | (USE_ADDR_LAST & (ADDR_LAST[AWIDTH-1:0] == set_addr)))) begin
          o_tlast_int <= 1'b1;
        end else begin
          o_tlast_int <= 1'b0;
        end
        if (~o_tready_int) begin
          error_stb <= 1'b1;
        end
      end
    end
  end

  generate
    if (USE_FIFO) begin
      axi_fifo #(
        .WIDTH(WIDTH+1), .SIZE(FIFO_SIZE))
      axi_fifo (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({o_tlast_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
        .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied());
    end else begin
      assign o_tdata = o_tdata_int;
      assign o_tlast = o_tlast_int;
      assign o_tvalid = o_tvalid_int;
      assign o_tready_int = o_tready;
    end
  endgenerate

endmodule
