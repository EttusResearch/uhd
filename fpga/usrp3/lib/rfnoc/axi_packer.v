//
// Copyright 2015 Ettus Research
//
// Increases AXI stream bit width by concatenating inputs across multiple clock cycles.
// Note: WIDTH_IN must be a multiple of WIDTH_OUT

module axi_packer #(
  parameter WIDTH_IN          = 8,   // Input bit width
  parameter WIDTH_OUT         = 32,  // Output bit width
  parameter REVERSE           = 0)   // 0: Fill LSB to MSB, 1: Fill MSB to LSB
(
  input clk, input reset, input clear,
  input [WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  localparam M = WIDTH_OUT/WIDTH_IN;

  reg packed_tlast, packed_tvalid;
  reg [WIDTH_OUT-1:0] packed_tdata;
  reg [$clog2(M)-1:0] cnt;
  reg i_tlast_hold;
  integer i;
  always @(posedge clk) begin
    if (reset | clear) begin
      i_tlast_hold      <= 1'b0;
      cnt               <= 'd0;
      packed_tdata      <= 'd0;
      packed_tvalid     <= 1'b0;
      packed_tlast      <= 1'b0;
    end else begin
      packed_tvalid     <= 1'b0;
      packed_tlast      <= 1'b0;
      if (i_tvalid & i_tready) begin
        if (i_tlast) begin
          i_tlast_hold  <= 1'b1;
        end
        if (cnt > M-1) begin
          cnt           <= 'd0;
          packed_tlast  <= i_tlast_hold;
          packed_tvalid <= 1'b1;
        end else begin
          cnt           <= cnt + 1;
        end
        if (REVERSE) begin
          packed_tdata[WIDTH_OUT-1:WIDTH_OUT-WIDTH_IN] <= i_tdata;
          for (i = 0; i < M-1; i = i + 1) begin
            packed_tdata[WIDTH_OUT-(i+1)*WIDTH_IN-1 -: WIDTH_IN] <= packed_tdata[WIDTH_OUT-i*WIDTH_IN-1 -: WIDTH_IN];
          end
        end else begin
          packed_tdata[WIDTH_IN-1:0] <= i_tdata;
          for (i = 0; i < M-1; i = i + 1) begin
            packed_tdata[(i+2)*WIDTH_IN-1 -: WIDTH_IN] <= packed_tdata[(i+1)*WIDTH_IN-1 -: WIDTH_IN];
          end
        end
      end
    end
  end

  axi_fifo_flop #(.WIDTH(WIDTH_OUT+1)) axi_fifo_flop_pack (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({packed_tlast,packed_tdata}), .i_tvalid(packed_tvalid), .i_tready(i_tready),
    .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .space(), .occupied());

endmodule

