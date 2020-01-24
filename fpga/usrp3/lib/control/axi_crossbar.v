/////////////////////////////////////////////////////////////////////
//
// Copyright 2012 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_crossbar
// Description:
//   - Control Registers
//   - CAM to setup routing between RFNoC blocks
//
/////////////////////////////////////////////////////////////////////

module axi_crossbar
  #(
    parameter BASE        = 0,  // settings bus base address
    parameter FIFO_WIDTH  = 64, // AXI4-STREAM data bus width
    parameter DST_WIDTH   = 16, // Width of DST field we are routing on.
    parameter NUM_INPUTS  = 2,  // number of input AXI4-STREAM buses
    parameter NUM_OUTPUTS = 2   // number of output AXI4-STREAM buses
    )
    (
     input 				   clk,
     input 				   reset,
     input 				   clear,
     input [7:0] 		 local_addr,
     // Inputs
     input [(FIFO_WIDTH*NUM_INPUTS)-1:0]   i_tdata,
     input [NUM_INPUTS-1:0] 		   i_tvalid,
     input [NUM_INPUTS-1:0] 		   i_tlast,
     output [NUM_INPUTS-1:0] 		   i_tready,
     input [NUM_INPUTS-1:0] 		   pkt_present,
     // Setting Bus
     input 				   set_stb,
     input [15:0] 			   set_addr,
     input [31:0] 			   set_data,
     // Output
     output [(FIFO_WIDTH*NUM_OUTPUTS)-1:0] o_tdata,
     output [NUM_OUTPUTS-1:0] 		   o_tvalid,
     output [NUM_OUTPUTS-1:0] 		   o_tlast,
     input [NUM_OUTPUTS-1:0] 		   o_tready,
     // readback bus
     input                        rb_rd_stb,
     input [$clog2(NUM_OUTPUTS)+$clog2(NUM_INPUTS)-1:0] rb_addr,
     output reg [31:0]            rb_data
     );

   genvar m,n;

   wire [(NUM_INPUTS*NUM_OUTPUTS)-1:0] forward_valid_in;
   wire [(NUM_INPUTS*NUM_OUTPUTS)-1:0] forward_ack_in;
   wire [(NUM_INPUTS*NUM_OUTPUTS)-1:0] forward_valid_out;
   wire [(NUM_INPUTS*NUM_OUTPUTS)-1:0] forward_ack_out;

   wire [NUM_INPUTS-1:0] i_tready_slave [0:NUM_OUTPUTS-1];

   //
   // Instantiate an axi_slave_mux for every slave/output of the Crossbar switch.
   // Each axi_slave_mux contains logic to maux and resolve arbitration
   // for this particular slave/output.
   //

   generate
      for (m = 0; m < NUM_OUTPUTS; m = m + 1) begin: instantiate_slave_mux

	 wire [NUM_INPUTS-1:0] i_tready_tmp;

	 axi_slave_mux
	   #(
	     .FIFO_WIDTH(FIFO_WIDTH),     // AXI4-STREAM data bus width
	     .DST_WIDTH(DST_WIDTH),  // Width of DST field we are routing on.
	     .NUM_INPUTS(NUM_INPUTS)  // number of input AXI buses
	     ) 	axi_slave_mux_i
	     (
	      .clk(clk),
	      .reset(reset),
	      .clear(clear),
	      // Inputs
	      .i_tdata(i_tdata),
	      .i_tvalid(i_tvalid),
	      .i_tlast(i_tlast),
	      .i_tready(i_tready_tmp),
	      // Forwarding flags (One from each Input/Master)
	      .forward_valid(forward_valid_in[(m+1)*NUM_INPUTS-1:m*NUM_INPUTS]),
	      .forward_ack(forward_ack_out[(m+1)*NUM_INPUTS-1:m*NUM_INPUTS]),
	      // Output
	      .o_tdata(o_tdata[(m*FIFO_WIDTH)+FIFO_WIDTH-1:m*FIFO_WIDTH]),
	      .o_tvalid(o_tvalid[m]),
	      .o_tlast(o_tlast[m]),
	      .o_tready(o_tready[m])
	      );

	 if (m==0)
	       assign i_tready_slave[0] = i_tready_tmp;
	 else
	      assign i_tready_slave[m] = i_tready_tmp | i_tready_slave[m-1] ;

      end // block: instantiate_slave_mux
   endgenerate

   assign 	  i_tready = i_tready_slave[NUM_OUTPUTS-1];

   //
   // Permute the forwarding flag buses
   //

   generate
      for (m = 0; m < NUM_OUTPUTS; m = m + 1) begin: permute_outer
	 for (n = 0; n < NUM_INPUTS; n = n + 1) begin: permute_inner
	    assign forward_valid_in[n*NUM_OUTPUTS+m] = forward_valid_out[n+m*NUM_INPUTS];
	    assign forward_ack_in[n+m*NUM_INPUTS] = forward_ack_out[n*NUM_OUTPUTS+m];
	 end
      end

   endgenerate


   //
   // Instantiate an axi_forwarding_cam for every Input/Master of the Crossbar switch.
   // Each contains a TCAM like lookup that allocates an egress port.
   //

   wire [31:0] 	   rb_data_mux[0:NUM_INPUTS-1];

   generate
      for (m = 0; m < NUM_INPUTS; m = m + 1) begin: instantiate_cam
	 axi_forwarding_cam
	   #(
	     .BASE(BASE),
	     .WIDTH(FIFO_WIDTH),  // Bit width of FIFO word.
	     .NUM_OUTPUTS(NUM_OUTPUTS)
	     ) axi_forwarding_cam_i
	     (
	      .clk(clk),
	      .reset(reset),
	      .clear(clear),
	      // Monitored FIFO signals
	      .o_tdata(i_tdata[(m*FIFO_WIDTH)+FIFO_WIDTH-1:m*FIFO_WIDTH]),
	      .o_tvalid(i_tvalid[m]),
	      .o_tready(i_tready[m]),
	      .o_tlast(i_tlast[m]),
	      .pkt_present(pkt_present[m]),
	      // Configuration
	      .local_addr(local_addr),
	      // Setting Bus
	      .set_stb(set_stb),
	      .set_addr(set_addr),
	      .set_data(set_data),
	      // Header signals
	      .forward_valid(forward_valid_out[(m+1)*NUM_OUTPUTS-1:m*NUM_OUTPUTS]),
	      .forward_ack(forward_ack_in[(m+1)*NUM_OUTPUTS-1:m*NUM_OUTPUTS]),
	      // Readback bus
	      .rb_rd_stb(rb_rd_stb && (rb_addr[$clog2(NUM_OUTPUTS)+$clog2(NUM_INPUTS)-1:$clog2(NUM_OUTPUTS)] == m)),
	      .rb_addr(rb_addr[$clog2(NUM_OUTPUTS)-1:0]),
	      .rb_data(rb_data_mux[m])
	      );
      end // block: instantiate_fifo_header
   endgenerate

   // Pipeline readback data to alleviate timing issues
   always @(posedge clk) rb_data <= rb_data_mux[rb_addr[$clog2(NUM_OUTPUTS)+$clog2(NUM_INPUTS)-1:$clog2(NUM_OUTPUTS)]];


endmodule // axi_crossbar
