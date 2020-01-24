//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module regport_resp_mux #(
  parameter WIDTH      = 32,
  parameter NUM_SLAVES = 2
)(
  input                            clk,
  input                            reset,

  input [NUM_SLAVES-1:0]           sla_rd_resp,
  input [(NUM_SLAVES*WIDTH)-1:0]   sla_rd_data,

  output reg                       mst_rd_resp,
  output reg [WIDTH-1:0]           mst_rd_data
);
  // Treat sla_rd_resp as a one-hot bus.
  // If multiple resp lines are asserted at the same time, then
  // it is a violation of the register port protocol

  wire [NUM_SLAVES-1:0] bit_options[0:WIDTH-1];
  wire [WIDTH-1:0]      data_out;

  genvar i, b;
  generate
     for (b = 0; b < WIDTH; b = b + 1) begin
        for (i = 0; i < NUM_SLAVES; i = i + 1) begin
           assign bit_options[b][i] = sla_rd_data[(i*WIDTH)+b];
        end
        assign data_out[b] = |(bit_options[b] & sla_rd_resp);
     end
  endgenerate

  always @(posedge clk) begin
     mst_rd_data <= data_out;
     if (reset)
        mst_rd_resp <= 1'b0;
     else
        mst_rd_resp <= |(sla_rd_resp);
  end

endmodule
