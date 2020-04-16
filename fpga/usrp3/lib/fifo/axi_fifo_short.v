//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description: 32 word FIFO with AXI4-Stream interface.
//
// NOTE: This module uses the SRLC32E primitive explicitly and as such can
// only be used with Xilinx technology.
//

module axi_fifo_short #(
  parameter WIDTH = 32
) (
  input              clk,
  input              reset,
  input              clear,
  input  [WIDTH-1:0] i_tdata,
  input              i_tvalid,
  output             i_tready,
  output [WIDTH-1:0] o_tdata,
  output             o_tvalid,
  input              o_tready,

  output reg [5:0] space,
  output reg [5:0] occupied
);

  reg  full  = 1'b0, empty = 1'b1;
  wire write = i_tvalid & i_tready;
  wire read  = o_tready & o_tvalid;

  assign i_tready = ~full;
  assign o_tvalid = ~empty;
  
  reg [4:0] a;
  genvar i; 
  
  generate
    for (i=0;i<WIDTH;i=i+1) begin : gen_srlc32e
      SRLC32E srlc32e(
        .Q(o_tdata[i]), .Q31(),
        .A(a),
        .CE(write),.CLK(clk),.D(i_tdata[i])
      );
    end
  endgenerate
  
  always @(posedge clk) begin
    if(reset) begin
       a <= 0;
       empty <= 1;
       full <= 0;
    end
    else if(clear) begin
      a <= 0;
      empty <= 1;
      full<= 0;
    end
    else if(read & ~write) begin
      full <= 0;
      if(a==0)
        empty <= 1;
      else
        a <= a - 1;
    end
    else if(write & ~read) begin
      empty <= 0;
      if(~empty)
        a <= a + 1;
      if(a == 30)
        full <= 1;
    end
  end

  // NOTE: Will fail if you write into a full FIFO or read from an empty one

  always @(posedge clk) begin
    if(reset)
      space <= 6'd32;
    else if(clear)
      space <= 6'd32;
    else if(read & ~write)
      space <= space + 6'd1;
    else if(write & ~read)
      space <= space - 6'd1;
  end
  
  always @(posedge clk) begin
    if(reset)
      occupied <= 6'd0;
    else if(clear)
      occupied <= 6'd0;
    else if(read & ~write)
      occupied <= occupied - 6'd1;
    else if(write & ~read)
      occupied <= occupied + 6'd1;
  end
    
endmodule // axi_fifo_short
