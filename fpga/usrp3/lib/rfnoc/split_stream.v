//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module split_stream
  #(parameter WIDTH=16,
    parameter ACTIVE_MASK=4'b1111)
   (input clk, input reset, input clear,  // These are not used in plain split_stream
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o0_tdata, output o0_tlast, output o0_tvalid, input o0_tready,
    output [WIDTH-1:0] o1_tdata, output o1_tlast, output o1_tvalid, input o1_tready,
    output [WIDTH-1:0] o2_tdata, output o2_tlast, output o2_tvalid, input o2_tready,
    output [WIDTH-1:0] o3_tdata, output o3_tlast, output o3_tvalid, input o3_tready);

   assign { o0_tlast, o0_tdata } = { i_tlast, i_tdata };
   assign { o1_tlast, o1_tdata } = { i_tlast, i_tdata };
   assign { o2_tlast, o2_tdata } = { i_tlast, i_tdata };
   assign { o3_tlast, o3_tdata } = { i_tlast, i_tdata };

   // NOTE -- this violates the AXI spec because tvalids are dependent on treadys.
   //   It will be ok most of the time, but muxes and demuxes will need a fifo in 
   //   the middle to avoid deadlock
   
   assign i_tready = ~|(~{o3_tready,o2_tready,o1_tready,o0_tready} & ACTIVE_MASK);
   assign { o3_tvalid, o2_tvalid, o1_tvalid, o0_tvalid } = {4{i_tready & i_tvalid}};
   
endmodule // split_stream
