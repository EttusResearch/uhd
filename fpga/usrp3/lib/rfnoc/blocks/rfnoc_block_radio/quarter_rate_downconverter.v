//
// Copyright 2018 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// mixer with 90 degree angles, i.e., multiplying the input signal with 1, i, -1, -i:

// Let S(t) = I(t) + i*Q(t) be the input signal based on inputs i_in and q_in
// Multiplying with (1,i,-1,-i) then becomes:
// S(t) * 1  =  I(t) + i*Q(t)
// S(t) * i  = -Q(t) + i*I(t)
// S(t) * -1 = -I(t) - i*Q(t)
// S(t) * -i =  Q(t) - i*I(t)

// To control the direction of rotation, the dirctn input is used
// When set to 0, the phase is increased with pi/2 every sample, i.e., rotating counter clock wise
// When set to 1, the phase is increased with -pi/2 every sample, i.e., rotating clock wise

// the input is the concatenation of the i and q signal: {i_in, q_in}

module quarter_rate_downconverter #(
  parameter WIDTH=24
)(
  input clk,
  input reset,
  input phase_sync,

  input [2*WIDTH-1:0] i_tdata,
  input i_tlast,
  input i_tvalid,
  output i_tready,

  output [2*WIDTH-1:0] o_tdata,
  output o_tlast,
  output o_tvalid,
  input o_tready,

  input dirctn
);

  // temporary signals for i and q after rotation
  reg [WIDTH-1:0] tmp_i = {WIDTH{1'b0}};
  reg [WIDTH-1:0] tmp_q = {WIDTH{1'b0}};

  localparam [WIDTH-1:0] MAX_NEG_VAL = -2**(WIDTH-1);
  localparam [WIDTH-1:0] MAX_POS_VAL = 2**(WIDTH-1)-1;

  function [WIDTH-1:0] invert_sig(
    input [WIDTH-1:0] x
  );
    begin
      invert_sig = x == MAX_NEG_VAL ? MAX_POS_VAL : -x;
    end
  endfunction

  // State machine types and reg
  localparam S0=0, S1=1, S2=2, S3=3;
  reg[1:0] cur_state;

  // split input into i and q signal
  wire[WIDTH-1:0] i_in, q_in;
  assign i_in = i_tdata[2*WIDTH-1:WIDTH];
  assign q_in = i_tdata[WIDTH-1:0];

  // The state machine doing the rotations among states
  always @(posedge clk) begin
    if(reset || phase_sync) begin
       cur_state <= S0;
    end else begin
      case (cur_state)
        S0: begin
            if(i_tvalid == 1'b1 && i_tready == 1'b1)
              if(dirctn == 1'b0)
                cur_state <= S1;
              else
                cur_state <= S3;
            else
              cur_state <= S0;
          end
        S1: begin
            if(i_tvalid == 1'b1 && i_tready == 1'b1)
              if(dirctn == 1'b0)
                cur_state <= S2;
              else
                cur_state <= S0;
            else
              cur_state <= S1;
          end
        S2: begin
            if(i_tvalid == 1'b1 && i_tready == 1'b1)
              if(dirctn == 1'b0)
                cur_state <= S3;
              else
                cur_state <= S1;
            else
              cur_state <= S2;
          end
        S3: begin
            if(i_tvalid == 1'b1 && i_tready == 1'b1)
              if(dirctn == 1'b0)
                cur_state <= S0;
              else
                cur_state <= S2;
            else
              cur_state <= S3;
          end
      endcase
    end
  end

  // Multiplication of input IQ signal with (1,i,-1,-i):
  always @(*) begin
    case (cur_state)
      S0: begin
          // S(t) * 1 = I(t) + iQ(t):
          tmp_i = i_in;
          tmp_q = q_in;
        end
      S1: begin
          // S(t) * i = -Q(t) + iI(t):
          tmp_i = invert_sig(q_in);
          tmp_q =            i_in;
        end
      S2: begin
          // S(t) * -1 = -I(t) - iQ(t):
          tmp_i = invert_sig(i_in);
          tmp_q = invert_sig(q_in);
        end
      S3: begin
          // S(t) * -i = Q(t) - iI(t):
          tmp_i =            q_in;
          tmp_q = invert_sig(i_in);
        end
      default: begin
          tmp_i = i_in;
          tmp_q = q_in;
        end
    endcase
  end

  // Flop for valid and ready signals and shortening of comb. paths.
  axi_fifo #(.WIDTH(2*WIDTH + 1), .SIZE(1)) flop (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata({i_tlast, tmp_i, tmp_q}), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .occupied(), .space());

endmodule // quarter_rate_downconverter
