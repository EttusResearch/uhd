//
// Copyright Ettus Research, 2014
// Copyright 2014 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// no logic, just wires

module axi_bit_reduce
  #(parameter WIDTH_IN=48,
    parameter WIDTH_OUT=25,
    parameter DROP_TOP=6,
    parameter VECTOR_WIDTH=1)   // vector_width = 2 for complex, 1 for real
   (input [VECTOR_WIDTH*WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [VECTOR_WIDTH*WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   genvar 				i;
   generate
      for(i=0; i<VECTOR_WIDTH; i=i+1)
	assign o_tdata[(i+1)*WIDTH_OUT-1:i*WIDTH_OUT] = i_tdata[(i+1)*WIDTH_IN-DROP_TOP-1:i*WIDTH_IN+(WIDTH_IN-WIDTH_OUT)-DROP_TOP];
   endgenerate

   assign o_tlast = i_tlast;
   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready;

endmodule // axi_bit_sel
