//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Test Virtual FIFO's by streaming modulo 2^32 counter (replicated in upper
// and lower 32bits). Test result by tracking count on receive and using 
// sticky flag for error indication.
// Also provide signal from MSB of 32bit count to blink LED.
//

module axi_test_vfifo
  #(parameter PACKET_SIZE = 128)
   (
    input aclk,
    input aresetn,
    input enable,
    // AXI Stream Out
    output reg out_axis_tvalid,
    input out_axis_tready,
    output [63 : 0] out_axis_tdata,
    output reg [7 : 0] out_axis_tstrb,
    output reg [7 : 0] out_axis_tkeep,
    output reg out_axis_tlast,
    output reg [0 : 0] out_axis_tid,
    output reg [0 : 0] out_axis_tdest,
    input vfifo_full,
    // AXI Stream In
    input in_axis_tvalid,
    output reg in_axis_tready,
    input [63 : 0] in_axis_tdata,
    input [7 : 0] in_axis_tstrb,
    input [7 : 0] in_axis_tkeep,
    input in_axis_tlast,
    input [0 : 0] in_axis_tid,
    input [0 : 0] in_axis_tdest,
    // Flags
    output reg flag_error,
    output heartbeat_in,
    output heartbeat_out,
    output [31:0] expected_count
    );
   

   reg [31:0] 	  out_count;
   reg [31:0] 	  in_count;
   reg [63:0] 	  in_axis_tdata_reg;
   reg 		  in_data_valid;
   


   //
   // Output 
   //
   always @(posedge aclk)
     if (!aresetn) begin
	out_count <= 0;
	out_axis_tvalid <= 0;
	out_axis_tid <= 0; // Don't care.
	out_axis_tdest <= 0; // Only use port 0 of VFIFO.
	out_axis_tstrb <= 0;  // Unused in VFIFO
	out_axis_tkeep <= 8'hFF; // Always use every byte of data
	out_axis_tlast <= 1'b0;
     end else if (enable) begin
	if (~vfifo_full) begin
	   // Always ready to output new count value.
	   out_axis_tvalid <= 1;
	   if (out_axis_tready) 
	     out_count <= out_count + 1;
	   // Assert TLAST every PACKET_SIZE beats.
	   if (out_count[15:0] == PACKET_SIZE)
	     out_axis_tlast <= 1'b1;
	   else
	     out_axis_tlast <= 1'b0;
	end else begin
	   out_axis_tvalid <= 0;
	end
     end else begin
	out_axis_tlast <= 1'b0;	
	out_axis_tvalid <= 0;
     end

   assign out_axis_tdata = {out_count,out_count};

   assign heartbeat_out = out_count[28];
   

   //
   // Input (Ignore TLAST signal)
   //
   always @(posedge aclk)
     if (!aresetn) begin
	in_axis_tready <= 0;
	in_axis_tdata_reg <= 0;
	in_data_valid <= 0;
	
     end else if (enable) begin
	in_axis_tready <= 1;
	in_axis_tdata_reg <= in_axis_tdata;
	if (in_axis_tvalid) 
	  in_data_valid <= 1;
	else
	  in_data_valid <= 0;
     end else begin
	in_data_valid <= 0;
	in_axis_tready <= 0;
     end // else: !if(enable)
   
   	  
   assign heartbeat_in = in_count[28];
   
   //
   // Input Checker
   //
   always @(posedge aclk)
     if (!aresetn) begin
	in_count <= 0;
	flag_error <= 0;
     end else if (enable) begin
	if (in_data_valid) begin
	   
	   if ((in_axis_tdata_reg[63:32] != in_count) || (in_axis_tdata_reg[31:0] != in_count))
	     begin
		flag_error <= 1;
		in_count <= in_axis_tdata_reg[63:32] + 1;
	     end
	   else
	     begin
		flag_error <= 0;
		in_count <= in_count + 1;
	     end
	   
	end
     end

   assign expected_count = in_count;
   

endmodule // axi_test_vfifo


   
   
