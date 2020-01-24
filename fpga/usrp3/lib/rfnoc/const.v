//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module const
  #(parameter WIDTH=32)
   (input clk, input reset,
    input [WIDTH-1:0] config_tdata, input config_tlast, input config_tvalid, output config_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   reg [WIDTH-1:0]     const_val;

   always @(posedge clk)
     if(reset)
       const_val <= 0;
     else
       if(config_tvalid & config_tready)
	 const_val <= config_tdata;

   assign config_tready = 1'b1;
   // FIXME do we want to sync constant change to tlasts?
   
   assign o_tdata = const_val;
   assign o_tlast = 1'b0; // FIXME do we want something else here?
   assign o_tvalid = 1'b1; // caution -- will fill up a fifo
   
endmodule // const
