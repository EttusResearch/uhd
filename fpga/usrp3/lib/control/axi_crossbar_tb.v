//
// Copyright 2012 Ettus Research LLC
//

`timescale  1 ps / 1 ps

module axi_crossbar_tb;


   localparam STREAM_WIDTH = 64;
   
   // Currently support simulations upto 8x8 configurations
   localparam MAX_NUM_INPUTS = 8;
   localparam MAX_NUM_OUTPUTS = 8;
   
   wire [(MAX_NUM_INPUTS*STREAM_WIDTH)-1:0] i_tdata;
   wire [STREAM_WIDTH-1:0] 		i_tdata_array [0:MAX_NUM_INPUTS-1];   
   wire [MAX_NUM_INPUTS-1:0] 		i_tvalid;
   wire [MAX_NUM_INPUTS-1:0] 		i_tready;
   wire [MAX_NUM_INPUTS-1:0] 		i_tlast;
   wire [MAX_NUM_INPUTS-1:0] 		pkt_present;
   
   reg [STREAM_WIDTH-1:0] 		data_in [0:MAX_NUM_INPUTS-1];   
   reg [MAX_NUM_INPUTS-1:0] 		valid_in;
   wire [MAX_NUM_INPUTS-1:0] 		ready_in;
   reg [MAX_NUM_INPUTS-1:0] 		last_in;
   
   wire [(MAX_NUM_OUTPUTS*STREAM_WIDTH)-1:0] o_tdata;
   wire [STREAM_WIDTH-1:0] 		 o_tdata_array [0:MAX_NUM_OUTPUTS-1];   
   wire [MAX_NUM_OUTPUTS-1:0] 		 o_tvalid;
   wire [MAX_NUM_OUTPUTS-1:0] 		 o_tready;
   wire [MAX_NUM_OUTPUTS-1:0] 		 o_tlast;
   

   wire [STREAM_WIDTH-1:0] 		 data_out [0:MAX_NUM_OUTPUTS-1];   
   wire [MAX_NUM_OUTPUTS-1:0] 		 valid_out;
   reg [MAX_NUM_OUTPUTS-1:0] 		 ready_out; 
   wire [MAX_NUM_OUTPUTS-1:0] 		 last_out;
		 
   
   genvar 				 m;

   reg 					 clk;
   reg 					 reset;
   reg 					 clear;
   reg 					 set_stb;
   reg [15:0] 				 set_addr;
   reg [31:0] 				 set_data;
   
  // reg 					 reset;

   //
   // Simulation specific testbench is included here
   //
`include "task_library.v"
`include "simulation_script.v"

   
   //
   // Define Clocks
   //
   initial begin
      clk = 1'b1;
   end
   
   // 125MHz clock
   always #4000 clk = ~clk;

   //
   // Good starting state
   //
     initial begin
	reset <= 0;
	clear <= 0;
	set_stb <= 0;
	set_addr <= 0;
	set_data <= 0;
/* -----\/----- EXCLUDED -----\/-----
	data_in[0] <= 0;
	valid_in[0] <= 0;
	last_in[0] <= 0;
	
	data_in[1] <= 0;
	valid_in[1] <= 0;
	last_in[1] <= 0;
 -----/\----- EXCLUDED -----/\----- */
	
	
     end

  

   //
   // AXI Crossbar instance
   //
   localparam SR_AWIDTH = 16;
   localparam SR_XB_LOCAL = 512;
   
   wire [7:0] local_addr;

   setting_reg #(.my_addr(SR_XB_LOCAL), .awidth(SR_AWIDTH), .width(8)) sr_local_addr
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(local_addr),.changed());
   
   axi_crossbar
     #(
       .FIFO_WIDTH(STREAM_WIDTH),   // AXI4-STREAM data bus width
       .DST_WIDTH(16),    // Width of DST field we are routing on.
       .NUM_INPUTS(NUM_INPUTS),    // number of input AXI4-STREAM buses
       .NUM_OUTPUTS(NUM_OUTPUTS)    // number of output AXI4-STREAM buses
       ) axi_crossbar_i
       (
	.clk(clk),
	.reset(reset),
	.clear(clear),
	.local_addr(local_addr),
	// Inputs
	.i_tdata(i_tdata[(NUM_INPUTS*STREAM_WIDTH)-1:0]),
	.i_tvalid(i_tvalid[NUM_INPUTS-1:0]),
	.i_tlast(i_tlast[NUM_INPUTS-1:0]),
	.i_tready(i_tready[NUM_INPUTS-1:0]),
	.pkt_present(pkt_present[NUM_INPUTS-1:0]),
	 // Settings bus
	.set_stb(set_stb),
	.set_addr(set_addr),
	.set_data(set_data),
	// Output
	.o_tdata(o_tdata[(NUM_OUTPUTS*STREAM_WIDTH)-1:0]),
	.o_tvalid(o_tvalid[NUM_OUTPUTS-1:0]),
	.o_tlast(o_tlast[NUM_OUTPUTS-1:0]),
	.o_tready(o_tready[NUM_OUTPUTS-1:0]),
	// Readback Bus
	.rb_rd_stb(1'b0),
	.rb_addr(0),
	.rb_data()
	);

   //
   // Input FIFOs
   //
   generate
      for (m=0;m<NUM_INPUTS;m=m+1)
	begin: input_fifos

	   assign i_tdata[(STREAM_WIDTH*m)+STREAM_WIDTH-1:STREAM_WIDTH*m] = i_tdata_array[m];
	   
	   axi_fifo_short
	     #(.WIDTH(STREAM_WIDTH+1)) axi_fifo_short_in
	       (
		.clk(clk), 
		.reset(reset), 
		.clear(clear),
		.o_tdata({i_tlast[m],i_tdata_array[m]}),
		.o_tvalid(i_tvalid[m]),
		.o_tready(i_tready[m]),
		.i_tdata({last_in[m],data_in[m]}),
		.i_tvalid(valid_in[m]),
		.i_tready(ready_in[m]),
		.space(),
		.occupied()
		);

	   monitor_axi_fifo
	     #(
	       .COUNT_BITS(8)
	       )  monitor_axi_fifo_in
	       (
		.clk(clk),
		.reset(reset),
		.clear(clear),
		// Monitored FIFO signals
		.i_tvalid(valid_in[m]),
		.i_tready(ready_in[m]),
		.i_tlast(last_in[m]),
		.o_tvalid(i_tvalid[m]),
		.o_tready(i_tready[m]),
		.o_tlast(i_tlast[m]),
		// FIFO status output
		.pkt_present(pkt_present[m]), // Flags any whole packets present
		.pkt_count()
		);
	   
	end
   endgenerate
   
	   
   //
   // Output FIFO's
   //
   generate
      for (m=0;m<NUM_OUTPUTS;m=m+1)
	begin: output_fifos

	   assign o_tdata_array[m] = o_tdata[(STREAM_WIDTH*m)+STREAM_WIDTH-1:STREAM_WIDTH*m];	   
   
	   axi_fifo_short 
	     #(.WIDTH(STREAM_WIDTH+1)) axi_fifo_short_out
	       (
		.clk(clk), 
		.reset(reset), 
		.clear(clear),
		.i_tdata({o_tlast[m],o_tdata_array[m]}),
		.i_tvalid(o_tvalid[m]),
		.i_tready(o_tready[m]),
		.o_tdata({last_out[m],data_out[m]}),
		.o_tvalid(valid_out[m]),
		.o_tready(ready_out[m]),
		.space(),
		.occupied()
		);
	end
   endgenerate // block: output_fifos

endmodule // axi_crossbar_tb
