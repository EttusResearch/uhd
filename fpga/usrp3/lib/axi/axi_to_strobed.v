//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Convert AXI Stream to a strobed interface.
// Note: Not especially useful if simply wanting to set
//       

module axi_to_strobed #(
  parameter WIDTH = 32,
  parameter FIFO_SIZE = 1,
  parameter MIN_RATE = 256
)(
  input clk, input reset, input clear,
  input [$clog2(MIN_RATE):0] out_rate,  // Number of clock cycles between strobes
  input ready,
  output error,                           // Output strobe but no data
  input [WIDTH-1:0] i_tdata, input i_tvalid, input i_tlast, output i_tready,
  output out_stb, output out_last, output [WIDTH-1:0] out_data
);

  reg strobe;
  wire valid;
  reg [$clog2(MIN_RATE):0] counter = 1;
  always @(posedge clk) begin
    if (reset | clear) begin
      strobe    <= 1'b0;
      counter   <= 1;
    end else if (ready) begin
      if (counter >= out_rate) begin
        strobe  <= 1'b1;
        counter <= 1;
      end else begin
        strobe  <= 1'b0;
        counter <= counter + 1'b1;
      end
    end else begin
      strobe <= 1'b0;
    end
  end

  axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({out_last,out_data}), .o_tvalid(valid), .o_tready(strobe),
    .space(), .occupied());

  assign out_stb = valid & strobe;
  assign error = ~valid & strobe;
endmodule