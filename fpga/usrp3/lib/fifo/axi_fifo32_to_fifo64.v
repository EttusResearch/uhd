//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_fifo32_to_fifo64
  (input clk, input reset, input clear,
   input [31:0] i_tdata, input [1:0] i_tuser, input i_tlast, input i_tvalid, output i_tready,
   output [63:0] o_tdata, output [2:0] o_tuser, output o_tlast, output o_tvalid, input o_tready
   );

   reg [31:0] 	 holding;

   reg 		 state;

   always @(posedge clk)
     if(reset | clear)
       state <= 0;
     else
       if(i_tvalid & i_tready)
       case(state)
	 0 : if(~i_tlast) state <= 1'b1;
    	 1 : state <= 1'b0;
	 default : state <= 1'b0;
       endcase // case (state)

   always @(posedge clk)
     if(i_tvalid & i_tready)
       holding <= i_tdata;
   
   assign i_tready = (state == 0 && !i_tlast)? 1'b1 : o_tready;
   assign o_tvalid = (state == 0 && !i_tlast)? 1'b0 : i_tvalid;
   
   assign o_tdata = (state == 0) ? {i_tdata, 32'h0} : { holding, i_tdata };
   assign o_tlast = i_tlast;

   wire [2:0] 	 occ_in = (i_tuser == 0) ? 3'd4 : {1'b0, i_tuser};
   wire [2:0] 	 occ_out = (state == 0) ? occ_in : (occ_in + 3'd4);
   
   assign o_tuser = ~o_tlast ? 3'd0 : occ_out;
   	 
endmodule // axi_fifo32_to_fifo64
