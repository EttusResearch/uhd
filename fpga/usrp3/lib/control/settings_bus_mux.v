//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Mux multiple settings buses

module settings_bus_mux #(
  parameter PRIO=0, // 0 = Round robin, 1 = Lower ports get priority (see axi_mux)
  parameter AWIDTH=8,
  parameter DWIDTH=32,
  parameter FIFO_SIZE=1,
  parameter NUM_BUSES=2)
(
  input clk, input reset, input clear,
  input [NUM_BUSES-1:0] in_set_stb, input [NUM_BUSES*AWIDTH-1:0] in_set_addr, input [NUM_BUSES*DWIDTH-1:0] in_set_data,
  output out_set_stb, output [AWIDTH-1:0] out_set_addr, output [DWIDTH-1:0] out_set_data, input ready
);

  wire [NUM_BUSES*(AWIDTH+DWIDTH)-1:0] i_tdata;
  generate
    if(NUM_BUSES <= 1) begin
      assign out_set_stb = in_set_stb;
      assign out_set_addr = in_set_addr;
      assign out_set_data = in_set_data;
    end else begin
      genvar i;
        for (i = 0; i < NUM_BUSES; i = i + 1) begin
          assign i_tdata[(i+1)*(AWIDTH+DWIDTH)-1:i*(AWIDTH+DWIDTH)] = {in_set_addr[(i+1)*AWIDTH-1:i*AWIDTH],in_set_data[(i+1)*DWIDTH-1:i*DWIDTH]};
        end
      axi_mux #(
        .PRIO(PRIO),
        .WIDTH(AWIDTH+DWIDTH),
        .PRE_FIFO_SIZE($clog2(NUM_BUSES)),
        .POST_FIFO_SIZE(FIFO_SIZE),
        .SIZE(NUM_BUSES))
      axi_mux (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata(i_tdata), .i_tlast({NUM_BUSES{1'b1}}), .i_tvalid(in_set_stb), .i_tready(),
        .o_tdata({out_set_addr,out_set_data}), .o_tlast(), .o_tvalid(out_set_stb), .o_tready(ready));
    end
  endgenerate
endmodule