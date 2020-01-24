//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module cic_decimate #(
  parameter WIDTH = 16,
  parameter N = 4,
  parameter MAX_RATE = 256
)(
  input clk,
  input reset,
  input rate_stb,
  input [$clog2(MAX_RATE+1)-1:0] rate, // +1 due to $clog2() rounding
  input strobe_in,
  output reg strobe_out,
  input last_in,
  output reg last_out,
  input [WIDTH-1:0] signal_in,
  output reg [WIDTH-1:0] signal_out
);

  wire [WIDTH+(N*$clog2(MAX_RATE+1))-1:0] signal_in_ext;
  reg  [WIDTH+(N*$clog2(MAX_RATE+1))-1:0] integrator [0:N-1];
  reg  [WIDTH+(N*$clog2(MAX_RATE+1))-1:0] differentiator [0:N-1];
  reg  [WIDTH+(N*$clog2(MAX_RATE+1))-1:0] pipeline [0:N-1];
  reg  [WIDTH+(N*$clog2(MAX_RATE+1))-1:0] sampler;
  reg  [N-1:0] last_integ;
  reg          last_integ_hold;
  reg  [N-1:0] last_diff;
  reg          last_sampler;
  reg  [N-1:0] strobe_integ;
  reg          strobe_sampler;
  reg  [N-1:0] strobe_diff;

  integer i;

  sign_extend #(WIDTH,WIDTH+(N*$clog2(MAX_RATE+1))) ext_input (.in(signal_in),.out(signal_in_ext));

  // Integrate
  always @(posedge clk) begin
    if (reset) begin
      last_integ        <= 0;
      last_integ_hold   <= 0;
      for (i = 0; i < N; i = i + 1) begin
        integrator[i]   <= 0;
        strobe_integ[i] <= 0;
      end
    end else begin
      strobe_integ      <= {strobe_integ[N-2:0],strobe_in};
      if (strobe_in) begin
        last_integ[0]   <= last_in;
        integrator[0]   <= integrator[0] + signal_in_ext;
      end
      for (i = 1; i < N; i = i + 1) begin
        if (strobe_integ[i-1]) begin
          last_integ[i] <= last_integ[i-1];
          integrator[i] <= integrator[i] + integrator[i-1];
        end
      end
      if (last_integ[N-1] & ~strobe_sampler) begin
        last_integ_hold <= 1'b1;
      end else if (strobe_sampler) begin
        last_integ_hold <= 1'b0;
      end
    end
  end

  // Sampler strobe
  reg [$clog2(MAX_RATE+1)-1:0] counter;
  always @(posedge clk) begin
    if (reset) begin
      counter        <= rate;
      strobe_sampler <= 1'b0;
      last_sampler   <= 1'b0;
      sampler        <= 'd0;
    end else begin
      strobe_sampler <= 1'b0;
      last_sampler   <= 1'b0;
      if (rate_stb) begin
        counter          <= rate;
      end else if (strobe_integ[N-1]) begin
        if (counter <= 1) begin
          counter        <= rate;
          strobe_sampler <= 1'b1;
          last_sampler   <= last_integ[N-1] | last_integ_hold;
          sampler        <= integrator[N-1];
        end else begin
          counter        <= counter - 1;
        end
      end
    end
  end

  // Differentiate
  always @(posedge clk) begin
    if (reset) begin
      last_diff          <= 0;
      for (i = 0; i < N; i = i + 1) begin
        pipeline[i]       <= 0;
        differentiator[i] <= 0;
        strobe_diff       <= 0;
      end
    end else begin
      strobe_diff         <= {strobe_diff[N-2:0], strobe_sampler};
      if (strobe_sampler) begin
        last_diff[0]      <= last_sampler;
        differentiator[0] <= sampler;
        pipeline[0]       <= sampler - differentiator[0];
      end
      for (i = 1; i < N; i = i + 1) begin
        if (strobe_diff[i-1]) begin
          last_diff[i]      <= last_diff[i-1];
          differentiator[i] <= pipeline[i-1];
          pipeline[i]       <= pipeline[i-1] - differentiator[i];
        end
      end
    end
  end

  genvar l;
  wire [WIDTH-1:0] signal_out_shifted[0:MAX_RATE];
  generate
    for (l = 1; l <= MAX_RATE; l = l + 1) begin
      // N*log2(rate), $clog2(rate) = ceil(log2(rate)) which rounds to nearest shift without overflow
      assign signal_out_shifted[l] = pipeline[N-1][$clog2(l**N)+WIDTH-1:$clog2(l**N)];
    end
  endgenerate
  assign signal_out_shifted[0] = pipeline[N-1][WIDTH-1:0];

  // Output register
  always @(posedge clk) begin
    if (reset) begin
      last_out   <= 1'b0;
      strobe_out <= 1'b0;
      signal_out <= 'd0;
    end else begin
      strobe_out <= strobe_diff[N-1];
      if (strobe_diff[N-1]) begin
        last_out   <= last_diff[N-1];
        signal_out <= signal_out_shifted[rate];
      end
    end
  end

endmodule
