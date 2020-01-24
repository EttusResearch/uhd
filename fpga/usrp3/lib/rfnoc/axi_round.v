//
// Copyright 2016 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_round
  #(parameter WIDTH_IN=17,
    parameter WIDTH_OUT=16,
    parameter round_to_zero=0,       // original behavior
    parameter round_to_nearest=1,    // lowest noise
    parameter trunc=0,               // round to negative infinity
    parameter FIFOSIZE=0)  // leave at 0 for a normal single flop
   (input clk, input reset,
    input [WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

  wire [WIDTH_OUT-1:0] out;

  generate
    if (WIDTH_IN == WIDTH_OUT) begin
      assign o_tdata = i_tdata;
      assign o_tlast = i_tlast;
      assign o_tvalid = i_tvalid;
      assign i_tready = o_tready;
    end else begin
      wire round_corr,round_corr_trunc,round_corr_rtz,round_corr_nearest,round_corr_nearest_safe;
      wire [WIDTH_IN-WIDTH_OUT-1:0] err;

      assign round_corr_trunc = 0;
      assign round_corr_rtz = (i_tdata[WIDTH_IN-1] & |i_tdata[WIDTH_IN-WIDTH_OUT-1:0]);
      assign round_corr_nearest = i_tdata[WIDTH_IN-WIDTH_OUT-1];

      assign round_corr_nearest_safe =  (WIDTH_IN-WIDTH_OUT > 1) ? 
                                        ((~i_tdata[WIDTH_IN-1] & (&i_tdata[WIDTH_IN-2:WIDTH_IN-WIDTH_OUT])) ? 1'b0 : round_corr_nearest) :
                                        round_corr_nearest;

      assign round_corr = round_to_nearest ? round_corr_nearest_safe :
                          trunc ? round_corr_trunc : 
                          round_to_zero ? round_corr_rtz :
                          0;  // default to trunc

      assign out = i_tdata[WIDTH_IN-1:WIDTH_IN-WIDTH_OUT] + round_corr;

      assign err = i_tdata - {out,{(WIDTH_IN-WIDTH_OUT){1'b0}}};

      axi_fifo #(.WIDTH(WIDTH_OUT+1), .SIZE(FIFOSIZE)) flop
        (.clk(clk), .reset(reset), .clear(1'b0),
         .i_tdata({i_tlast, out}), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
         .occupied(), .space());

   end
  endgenerate

endmodule // axi_round
