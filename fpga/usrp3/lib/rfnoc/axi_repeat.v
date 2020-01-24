//
// Copyright 2015 Ettus Research
//
// Output always valid (except in reset) and repeats last valid i_tdata & i_tlast value

module axi_repeat
#(
  parameter WIDTH = 16) 
(
  input clk, input reset,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output reg [WIDTH-1:0] o_tdata, output reg o_tlast, output reg o_tvalid, input o_tready
);

  assign i_tready = 1'b1;

  always @(posedge clk) begin
    if (reset) begin
      o_tdata  <= 'd0;
      o_tlast  <= 'd0;
      o_tvalid <= 'd0;
    end else begin
      if (i_tvalid) begin
        o_tvalid <= 1'b1;
        o_tlast  <= i_tlast;
        o_tdata  <= o_tdata;
      end
    end
  end

endmodule