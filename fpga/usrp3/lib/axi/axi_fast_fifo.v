//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Ultra fast critical path FIFO.
// Only 2 entrys but no combinatorial feed through paths
//


module axi_fast_fifo
  #(parameter WIDTH=64)
  (
   input clk,
   input reset,
   input clear,
   //
   input [WIDTH-1:0] i_tdata,
   input i_tvalid,
   output reg i_tready,
   //
   output [WIDTH-1:0] o_tdata,
   output reg o_tvalid,
   input  o_tready
   );

   reg [WIDTH-1:0] data_reg1, data_reg2;

   reg [1:0] 	   state;
   
   localparam EMPTY = 0;
   localparam HALF = 1;
   localparam FULL = 2;
   
   always @(posedge clk) 
     if (reset | clear) begin
	state <= EMPTY;
	data_reg1 <= 0;
	data_reg2 <= 0;
	o_tvalid <= 1'b0;
	i_tready <= 1'b0;
	
     end else begin
	case (state)
	  // Nothing in either register.
	  // Upstream can always push data to us.
	  // Downstream has nothing to take from us.
	  EMPTY: begin
	     if (i_tvalid) begin
		data_reg1 <= i_tdata;
		state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end else begin
		state <= EMPTY;
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
	     end
	  end
	  // First Register Full.
	  // Upstream can always push data to us.
	  // Downstream can always read from us.
	  HALF: begin
	     if (i_tvalid && o_tready) begin
		data_reg1 <= i_tdata;
		state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end else if (i_tvalid) begin
		data_reg1 <= i_tdata;
		data_reg2 <= data_reg1;
		state <= FULL;
		i_tready <= 1'b0;
		o_tvalid <= 1'b1;
	     end else if (o_tready) begin
		state <= EMPTY;
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
	     end else begin
		state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end
	  end // case: HALF
	  // Both Registers Full.
	  // Upstream can not push to us in this state.
	  // Downstream can always read from us.
	  FULL: begin
	     if (o_tready) begin
		state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end
	     else begin
		state <= FULL;
		i_tready <= 1'b0;
		o_tvalid <= 1'b1;
	     end
	  end
	endcase // case(state)
     end // else: !if(reset | clear)

   assign o_tdata = (state == FULL) ? data_reg2 : data_reg1;
   

endmodule // axi_fast_fifo
