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


module axi_fast_extract_tlast
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
   output o_tlast,
   output reg o_tvalid,
   input  o_tready
   );

   reg [WIDTH:0] data_reg1, data_reg2;

   reg [1:0] 	   fifo_state;
   
   localparam EMPTY = 0;
   localparam HALF = 1;
   localparam FULL = 2;

   reg [1:0]  extract_state;
   
   localparam IDLE = 0;
   localparam EXTRACT1 = 1;
   localparam EXTRACT2 = 2;
   localparam EXTRACT3 = 3;

   
   always @(posedge clk) 
     if (reset | clear) begin
	fifo_state <= EMPTY;
     end else begin
	case (fifo_state)
	  // Nothing in either register.
	  // Upstream can always push data to us.
	  // Downstream has nothing to take from us.
	  EMPTY: begin
	     if ((extract_state == IDLE) && (i_tdata == 64'hDEADBEEFFEEDCAFE) && i_tvalid) begin
		// Embeded escpae code received.
		extract_state <= EXTRACT1;
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
		fifo_state <= EMPTY;
	     end else if ((extract_state == EXTRACT1) && i_tvalid) begin
		// Now work out if its a genuine embeded tlast or emulation.
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
		fifo_state <= EMPTY;
		if (i_tdata[31:0] == 'h1) begin
		   extract_state <= EXTRACT2;
		end else begin
		   extract_state <= EXTRACT3;
		end
	     end else if ((extract_state == EXTRACT2) && i_tvalid) begin
		// Extract tlast.
		data_reg1 <= {1'b1,i_tdata};
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
		fifo_state <= HALF;
		extract_state <= IDLE;
	     end else if (i_tvalid) begin
		// Get here both for normal data and for EXTRACT3 emulation data.
		data_reg1 <= {1'b0,i_tdata};
		fifo_state <= HALF;
		extract_state <= IDLE;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end else begin
		// Nothing to do.
		fifo_state <= EMPTY;
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
	     end
	  end
	  // First Register Full.
	  // Upstream can always push data to us.
	  // Downstream can always read from us.
	  HALF: begin
	     if ((extract_state == IDLE) && (i_tdata == 64'hDEADBEEFFEEDCAFE) && i_tvalid) begin
		// Embeded escpae code received.
		extract_state <= EXTRACT1;
		if (o_tready) begin
		   // If meanwhile we get read then go empty...
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b0;
		   fifo_state <= EMPTY;
		end else begin
		   // ...else stay half full.
		   fifo_state <= HALF;
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b1;
		end
	     end else if ((extract_state == EXTRACT1) && i_tvalid) begin
		// Now work out if its a genuine embeded tlast or emulation.
		if (i_tdata[31:0] == 'h1) begin
		   extract_state <= EXTRACT2;
		end else begin
		   extract_state <= EXTRACT3;
		end
		if (o_tready) begin
		   // If meanwhile we get read then go empty...
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b0;
		   fifo_state <= EMPTY;
		end else begin
		   // ...else stay half full.
		   fifo_state <= HALF;
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b1;
		end		
	     end else if ((extract_state == EXTRACT2) && i_tvalid) begin
		// Extract tlast.
		data_reg1 <= {1'b1,i_tdata};
		extract_state <= IDLE;
		if (o_tready) begin
		   // We get read and writen same cycle...
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b1;
		   fifo_state <= HALF;
		end else begin
		   // ...or we get written and go full.
		   data_reg2 <= data_reg1;
		   i_tready <= 1'b0;
		   o_tvalid <= 1'b1;
		   fifo_state <= FULL;
		end	
	     end else if (i_tvalid) begin
		// Get here both for normal data and for EXTRACT3 emulation data.
		data_reg1 <= {1'b0,i_tdata};
		extract_state <= IDLE;
		if (o_tready) begin
		   // We get read and writen same cycle...
		   fifo_state <= HALF;
		   i_tready <= 1'b1;
		   o_tvalid <= 1'b1;
		end else begin
		   // ...or we get written and go full.
		   data_reg2 <= data_reg1;
		   i_tready <= 1'b0;
		   o_tvalid <= 1'b1;
		   fifo_state <= FULL;
		end	
	     end else if (o_tready) begin // if (i_tvalid)
		// Only getting read this cycle so go empty
		fifo_state <= EMPTY;
		i_tready <= 1'b1;
		o_tvalid <= 1'b0;
	     end else begin
		// Absolutley nothing happens, everything stays the same.
		fifo_state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end
	  end // case: HALF
	  // Both Registers Full.
	  // Upstream can not push to us in this fifo_state.
	  // Downstream can always read from us.
	  FULL: begin
	     if (o_tready) begin
		fifo_state <= HALF;
		i_tready <= 1'b1;
		o_tvalid <= 1'b1;
	     end
	     else begin
		fifo_state <= FULL;
		i_tready <= 1'b0;
		o_tvalid <= 1'b1;
	     end
	  end
	endcase // case(fifo_state)
     end // else: !if(reset | clear)
   
   assign {o_tlast,o_tdata} = (fifo_state == FULL) ? data_reg2 : data_reg1;
   

endmodule // axi_fast_extract_tlast
