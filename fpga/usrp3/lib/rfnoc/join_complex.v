
// Copyright 2014, Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Module to join a complex stream to I and Q outputs.  NOTE -- ONLY works when you can guarantee upstream paths match!

module join_complex
  #(parameter WIDTH=16)
   (input [WIDTH-1:0] ii_tdata, input ii_tlast, input ii_tvalid, output ii_tready,
    input [WIDTH-1:0] iq_tdata, input iq_tlast, input iq_tvalid, output iq_tready,
    output [WIDTH*2-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,
    output error);

   assign o_tdata = {ii_tdata,iq_tdata};

   assign o_tlast = ii_tlast;

   assign o_tvalid = ii_tvalid;

   assign ii_tready = o_tready;
   assign iq_tready = o_tready;

   assign  error = (ii_tlast ^ iq_tlast) | (ii_tvalid ^ iq_tvalid);
      
endmodule // join_complex
