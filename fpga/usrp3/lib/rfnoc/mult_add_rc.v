
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
// Complex times real.  Complex number is on port B (18I, 18Q), real is on A (25 bits)

module mult_add_rc
  #(parameter WIDTH_REAL=25,
    parameter WIDTH_CPLX=18,
    parameter WIDTH_P=48,
    parameter DROP_TOP_P=0,
    parameter LATENCY=3,
    parameter CASCADE_IN=0,
    parameter CASCADE_OUT=0)
   (input clk, input reset,
    input [WIDTH_REAL-1:0] real_tdata, input real_tlast, input real_tvalid, output real_tready,
    input [2*WIDTH_CPLX-1:0] cplx_tdata, input cplx_tlast, input cplx_tvalid, output cplx_tready,
    input [2*WIDTH_P-1:0] c_tdata, input c_tlast, input c_tvalid, output c_tready,
    output [2*WIDTH_P-1:0] p_tdata, output p_tlast, output p_tvalid, input p_tready);

   // NOTE -- we cheat here and share ready/valid.  This works because we can guarantee both
   //           paths will match

   generate
      if(WIDTH_REAL > WIDTH_CPLX)
	begin
	   mult_add #(.WIDTH_A(WIDTH_REAL), .WIDTH_B(WIDTH_CPLX), .WIDTH_P(WIDTH_P), .DROP_TOP_P(DROP_TOP_P),
		      .LATENCY(LATENCY), .CASCADE_IN(CASCADE_IN), .CASCADE_OUT(CASCADE_OUT)) mult_add_i
	     (.clk(clk), .reset(reset),
	      .a_tdata(real_tdata), .a_tlast(real_tlast), .a_tvalid(real_tvalid), .a_tready(real_tready),
	      .b_tdata(cplx_tdata[2*WIDTH_CPLX-1:WIDTH_CPLX]), .b_tlast(cplx_tlast), .b_tvalid(cplx_tvalid), .b_tready(cplx_tready),
	      .c_tdata(c_tdata[2*WIDTH_P-1:WIDTH_P]), .c_tlast(c_tlast), .c_tvalid(c_tvalid), .c_tready(c_tready),
	      .p_tdata(p_tdata[2*WIDTH_P-1:WIDTH_P]), .p_tlast(p_tlast), .p_tvalid(p_tvalid), .p_tready(p_tready));
           
	   mult_add #(.WIDTH_A(WIDTH_REAL), .WIDTH_B(WIDTH_CPLX), .WIDTH_P(WIDTH_P), .DROP_TOP_P(DROP_TOP_P),
		      .LATENCY(LATENCY), .CASCADE_IN(CASCADE_IN), .CASCADE_OUT(CASCADE_OUT)) mult_add_q
	     (.clk(clk), .reset(reset),
	      .a_tdata(real_tdata), .a_tlast(real_tlast), .a_tvalid(real_tvalid), .a_tready(),
	      .b_tdata(cplx_tdata[WIDTH_CPLX-1:0]), .b_tlast(cplx_tlast), .b_tvalid(cplx_tvalid), .b_tready(),
	      .c_tdata(c_tdata[WIDTH_P-1:0]), .c_tlast(c_tlast), .c_tvalid(c_tvalid), .c_tready(),
	      .p_tdata(p_tdata[WIDTH_P-1:0]), .p_tlast(), .p_tvalid(), .p_tready(p_tready));
        end // if (WIDTH_REAL > WIDTH_CPLX)
      else
	begin
	   mult_add #(.WIDTH_A(WIDTH_CPLX), .WIDTH_B(WIDTH_REAL), .WIDTH_P(WIDTH_P), .DROP_TOP_P(DROP_TOP_P),
		      .LATENCY(LATENCY), .CASCADE_IN(CASCADE_IN), .CASCADE_OUT(CASCADE_OUT)) mult_add_i
	     (.clk(clk), .reset(reset),
	      .a_tdata(cplx_tdata[2*WIDTH_CPLX-1:WIDTH_CPLX]), .a_tlast(cplx_tlast), .a_tvalid(cplx_tvalid), .a_tready(cplx_tready),
	      .b_tdata(real_tdata), .b_tlast(real_tlast), .b_tvalid(real_tvalid), .b_tready(real_tready),
	      .c_tdata(c_tdata[2*WIDTH_P-1:WIDTH_P]), .c_tlast(c_tlast), .c_tvalid(c_tvalid), .c_tready(c_tready),
	      .p_tdata(p_tdata[2*WIDTH_P-1:WIDTH_P]), .p_tlast(p_tlast), .p_tvalid(p_tvalid), .p_tready(p_tready));
           
	   mult_add #(.WIDTH_A(WIDTH_CPLX), .WIDTH_B(WIDTH_REAL), .WIDTH_P(WIDTH_P), .DROP_TOP_P(DROP_TOP_P),
		      .LATENCY(LATENCY), .CASCADE_IN(CASCADE_IN), .CASCADE_OUT(CASCADE_OUT)) mult_add_q
	     (.clk(clk), .reset(reset),
	      .a_tdata(cplx_tdata[WIDTH_CPLX-1:0]), .a_tlast(cplx_tlast), .a_tvalid(cplx_tvalid), .a_tready(),
	      .b_tdata(real_tdata), .b_tlast(real_tlast), .b_tvalid(real_tvalid), .b_tready(),
	      .c_tdata(c_tdata[WIDTH_P-1:0]), .c_tlast(c_tlast), .c_tvalid(c_tvalid), .c_tready(),
	      .p_tdata(p_tdata[WIDTH_P-1:0]), .p_tlast(), .p_tvalid(), .p_tready(p_tready));
        end // else: !if(WIDTH_REAL > WIDTH_CPLX)
   endgenerate
	
endmodule // mult
