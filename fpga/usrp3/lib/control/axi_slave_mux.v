//
// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`ifndef LOG2
`define LOG2(N) (\
                 N < 2    ? 0 : \
                 N < 4    ? 1 : \
                 N < 8    ? 2 : \
                 N < 16   ? 3 : \
                 N < 32   ? 4 : \
                 N < 64   ? 5 : \
                 N < 128  ? 6 : \
                 N < 256  ? 7 : \
                 N < 512  ? 8 : \
                 N < 1024 ? 9 : 10)
`endif

module axi_slave_mux
  #(
    parameter FIFO_WIDTH = 64, // AXI4-STREAM data bus width
    parameter DST_WIDTH = 16,  // Width of DST field we are routing on.
    parameter NUM_INPUTS = 2   // number of input AXI buses
    )
    (
     input 				 clk,
     input 				 reset,
     input 				 clear,
     // Inputs
     input [(FIFO_WIDTH*NUM_INPUTS)-1:0] i_tdata,
     input [NUM_INPUTS-1:0] 		 i_tvalid,
     input [NUM_INPUTS-1:0] 		 i_tlast,
     output [NUM_INPUTS-1:0] 		 i_tready,
     // Forwarding Flags
     input [NUM_INPUTS-1:0] 		 forward_valid,
     output reg [NUM_INPUTS-1:0] 	 forward_ack,
     // Output
     output [FIFO_WIDTH-1:0] 		 o_tdata,
     output 				 o_tvalid,
     output 				 o_tlast,
     input 				 o_tready
     );

   wire [FIFO_WIDTH-1:0] i_tdata_array [0:NUM_INPUTS-1];
   
   reg [`LOG2(NUM_INPUTS):0] select;
   reg 			     enable;


   reg 			     state;

   localparam CHECK_THIS_INPUT = 0;
   localparam WAIT_LAST = 1;
   

   always @(posedge clk)
     if (reset | clear) begin
	state <= CHECK_THIS_INPUT;
	select <= 0;
	enable <= 0;
	forward_ack <= 0;	
     end else begin
	case(state)
	  // Is the currently selected input addressing this slave with a ready packet?
	  CHECK_THIS_INPUT:  begin
	     if (forward_valid[select]) begin
		enable <= 1;
		forward_ack[select] <= 1;
		state <= WAIT_LAST;
	     end else if (select == NUM_INPUTS - 1 ) begin
		select <= 0;
	     end else begin
		select <= select + 1;
	     end
	  end
	  // Assert ACK immediately to forwarding logic and then wait for end of packet.
	  WAIT_LAST: begin
	    
	     if (i_tlast[select] && i_tvalid[select] && o_tready) begin
		if (select == NUM_INPUTS - 1 ) begin
		   select <= 0;
		end else begin
		   select <= select + 1;
		end
		state <= CHECK_THIS_INPUT;
		forward_ack <= 0;
		enable <= 0;
	     end else begin
		forward_ack[select] <= 1;
		enable <= 1;
	     end
	  end
	endcase // case(state)
     end
	
   //
   // Combinatorial mux
   //
   genvar m;
     
   generate
      for (m = 0; m < NUM_INPUTS; m = m + 1) begin: form_buses
	 assign i_tdata_array[m] = i_tdata[(m*FIFO_WIDTH)+FIFO_WIDTH-1:m*FIFO_WIDTH];
      end
   endgenerate

   assign 	o_tdata = i_tdata_array[select];
   assign 	o_tvalid = enable && i_tvalid[select];
   assign 	o_tlast = enable && i_tlast[select];
 //  assign 	i_tready = {NUM_INPUTS{o_tready}} & (enable << select);

   generate
      for (m = 0; m < NUM_INPUTS; m = m + 1) begin: form_ready
	 assign i_tready[m] = o_tready && enable && (select == m);
      end
   endgenerate

    
endmodule // axi_slave_mux
