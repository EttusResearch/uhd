//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module cic_interpolate #(
  parameter WIDTH = 16,
  parameter N = 4,
  parameter MAX_RATE = 128
)(
  input clk,
  input reset,
  input rate_stb,
  input [$clog2(MAX_RATE+1)-1:0] rate, // +1 due to $clog2() rounding
  input  strobe_in,
  output reg strobe_out,
  input [WIDTH-1:0] signal_in,
  output reg [WIDTH-1:0] signal_out
);

  wire [WIDTH+$clog2(MAX_RATE**(N-1))-1:0] signal_in_ext;
  reg  [WIDTH+$clog2(MAX_RATE**(N-1))-1:0] integrator [0:N-1];
  reg  [WIDTH+$clog2(MAX_RATE**(N-1))-1:0] differentiator [0:N-1];
  reg  [WIDTH+$clog2(MAX_RATE**(N-1))-1:0] pipeline [0:N-1];
  reg  [WIDTH+$clog2(MAX_RATE**(N-1))-1:0] sampler;

  reg  [N-1:0] strobe_diff;
  reg  [N-1:0] strobe_integ;
  reg          strobe_sampler;

  integer i;

  sign_extend #(WIDTH,WIDTH+$clog2(MAX_RATE**(N-1))) ext_input (.in(signal_in),.out(signal_in_ext));

  // Differentiate
  always @(posedge clk) begin
    if (reset) begin
      strobe_diff         <= 'd0;
      for (i = 0; i < N; i = i + 1) begin
        differentiator[i] <= 0;
        pipeline[i]       <= 0;
      end
    end else begin
      strobe_diff         <= {strobe_diff[N-2:0], strobe_in};
      if (strobe_in) begin
        differentiator[0] <= signal_in_ext;
        pipeline[0]       <= signal_in_ext - differentiator[0];
      end
      for (i = 1; i < N; i = i + 1) begin
        if (strobe_diff[i-1]) begin
          differentiator[i] <= pipeline[i-1];
          pipeline[i]       <= pipeline[i-1] - differentiator[i];
        end
      end
    end
  end

  // Strober
  reg [$clog2(MAX_RATE+1)-1:0] counter;
  wire strobe_out_int;

  always @(posedge clk) begin
    if (reset | rate_stb) begin
      counter <= rate;
    end else if (strobe_diff[N-1]) begin
      counter <= rate - 1;
    end else begin
      if (counter == 0) begin
        counter <= rate;
      end else if (counter < rate) begin
        counter <= counter - 1;
      end
    end
  end

  assign strobe_out_int = (counter < rate) & ~rate_stb;

  // Integrate
  always @(posedge clk) begin
    if (reset) begin
      strobe_sampler    <= 1'b0;
      strobe_integ      <= 'd0;
      for (i = 0; i < N; i = i + 1) begin
        integrator[i]   <= 0;
      end
    end else begin
      strobe_sampler      <= strobe_diff[N-1];
      if (strobe_diff[N-1]) begin
        sampler           <= pipeline[N-1];
      end
      strobe_integ        <= {strobe_integ[N-2:0],strobe_out_int};
      if (strobe_sampler) begin
        integrator[0] <= integrator[0] + sampler;
      end
      for (i = 1; i < N; i = i + 1) begin
        if (strobe_integ[i-1]) begin
          integrator[i] <= integrator[i] + integrator[i-1];
        end
      end
    end
  end

  genvar l;
  wire [WIDTH-1:0] signal_out_shifted[0:MAX_RATE];
  wire signal_out_shifted_strobe[0:MAX_RATE];
  generate
    for (l = 0; l <= MAX_RATE; l = l + 1) begin
      axi_round #(
        .WIDTH_IN((l == 0 || l == 1) ? WIDTH : $clog2(l**(N-1))+WIDTH),
        .WIDTH_OUT(WIDTH))
      axi_round (
        .clk(clk), .reset(reset),
        .i_tdata((l == 0 || l == 1) ? integrator[N-1][WIDTH-1:0] : integrator[N-1][$clog2(l**(N-1))+WIDTH-1:0]),
        .i_tlast(1'b0), .i_tvalid(strobe_integ[N-1]), .i_tready(),
        .o_tdata(signal_out_shifted[l]), .o_tlast(), .o_tvalid(signal_out_shifted_strobe[l]), .o_tready(1'b1));
    end
  endgenerate

  // Output register
  always @(posedge clk) begin
    if (reset) begin
      strobe_out <= 1'b0;
      signal_out <= 'd0;
    end else begin
      strobe_out <= signal_out_shifted_strobe[0]; // Any of the strobes will work here
      signal_out <= signal_out_shifted[rate];
    end
  end

endmodule
