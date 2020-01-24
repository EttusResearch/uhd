//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Note: n == 0 lets everything through.
// Warning: Sample / packet counts reset when n is changed, caution if changing during operation!

module keep_one_in_n #(
  parameter KEEP_FIRST=0,  // 0: Drop n-1 words then keep last word, 1: Keep 1st word then drop n-1
  parameter WIDTH=16,
  parameter MAX_N=65535
)(
  input clk, input reset,
  input vector_mode,
  input [$clog2(MAX_N+1)-1:0] n,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  reg [$clog2(MAX_N+1)-1:0] sample_cnt, pkt_cnt, n_reg;
  reg n_changed;

  always @(posedge clk) begin
    if (reset) begin
       n_reg      <= 1;
       n_changed  <= 1'b0;
    end else begin
      n_reg       <= n;
      if (n_reg != n) begin
        n_changed <= 1'b1;
      end else begin
        n_changed <= 1'b0;
      end
    end
  end

  wire on_last_sample  = ( (sample_cnt >= n_reg) | (n_reg == 0) );
  wire on_first_sample = ( (sample_cnt == 1)     | (n_reg == 0) );
  wire on_last_pkt     = ( (pkt_cnt >= n_reg)    | (n_reg == 0) );
  wire on_first_pkt    = ( (pkt_cnt == 1)        | (n_reg == 0) );

  always @(posedge clk) begin
    if (reset | n_changed) begin
       sample_cnt <= 1;
       pkt_cnt    <= 1;
    end else begin
      if (i_tvalid & i_tready) begin
        if (on_last_sample) begin
          sample_cnt <= 1;
        end else begin
          sample_cnt <= sample_cnt + 1'd1;
        end
      end
      if (i_tvalid & i_tready & i_tlast) begin
        if (on_last_pkt) begin
          pkt_cnt <= 1;
        end else begin
          pkt_cnt <= pkt_cnt + 1'd1;
        end
      end
    end
  end

  assign i_tready = o_tready | (vector_mode ? (KEEP_FIRST ? ~on_first_pkt    : ~on_last_pkt) :
                                              (KEEP_FIRST ? ~on_first_sample : ~on_last_sample));
  assign o_tvalid = i_tvalid & (vector_mode ? (KEEP_FIRST ?  on_first_pkt    :  on_last_pkt) :
                                              (KEEP_FIRST ?  on_first_sample :  on_last_sample));
  assign o_tdata  = i_tdata;
  assign o_tlast  = i_tlast  & (vector_mode ? 1'b1 : on_last_pkt);

endmodule // keep_one_in_n_vec
