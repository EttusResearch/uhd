
// Copyright 2014, Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Module to split a complex stream to I and Q outputs.  NOTE -- ONLY works when you can guarantee downstream paths match!

module split_complex
  #(parameter WIDTH=16)
   (input [WIDTH*2-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] oi_tdata, output oi_tlast, output oi_tvalid, input oi_tready,
    output [WIDTH-1:0] oq_tdata, output oq_tlast, output oq_tvalid, input oq_tready,
    output error);

   assign oi_tdata = i_tdata[WIDTH*2-1:WIDTH];
   assign oq_tdata = i_tdata[WIDTH-1:0];

   assign oi_tlast = i_tlast;
   assign oq_tlast = i_tlast;

   assign oi_tvalid = i_tvalid;
   assign oq_tvalid = i_tvalid;

   assign i_tready = oi_tready;

   assign  error = oi_tready ^ oq_tready;
   
endmodule // split_complex

