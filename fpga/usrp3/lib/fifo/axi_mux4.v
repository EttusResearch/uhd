
// Copyright 2012 Ettus Research LLC
// axi_mux -- takes 4 64-bit AXI stream, merges them to 1 output channel
// Round-robin if PRIO=0, priority if PRIO=1 (lower number ports get priority)
// Bubble cycles are inserted after each packet in PRIO mode, or on wraparound in Round Robin mode

module axi_mux4
  #(parameter PRIO=0,
    parameter WIDTH=64,
    parameter BUFFER=0)
   (input clk, input reset, input clear,
    input [WIDTH-1:0] i0_tdata, input i0_tlast, input i0_tvalid, output i0_tready,
    input [WIDTH-1:0] i1_tdata, input i1_tlast, input i1_tvalid, output i1_tready,
    input [WIDTH-1:0] i2_tdata, input i2_tlast, input i2_tvalid, output i2_tready,
    input [WIDTH-1:0] i3_tdata, input i3_tlast, input i3_tvalid, output i3_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire [WIDTH-1:0]    o_tdata_int;
   wire 	       o_tlast_int, o_tvalid_int, o_tready_int;
   
   reg [3:0] 	  mx_state;
   localparam MX_IDLE = 4'b0000;
   localparam MX_0    = 4'b0001;
   localparam MX_1    = 4'b0010;
   localparam MX_2    = 4'b0100;
   localparam MX_3    = 4'b1000;

   always @(posedge clk)
     if(reset | clear)
       mx_state <= MX_IDLE;
     else
       case (mx_state)
	 MX_IDLE :
	   if(i0_tvalid)
	     mx_state <= MX_0;
	   else if(i1_tvalid)
	     mx_state <= MX_1;
	   else if(i2_tvalid)
	     mx_state <= MX_2;
	   else if(i3_tvalid)
	     mx_state <= MX_3;

	 MX_0 :
	   if(o_tready_int & o_tvalid_int & o_tlast_int)
	     if(PRIO)
	       mx_state <= MX_IDLE;
	     else if(i1_tvalid)
	       mx_state <= MX_1;
	     else if(i2_tvalid)
	       mx_state <= MX_2;
	     else if(i3_tvalid)
	       mx_state <= MX_3;
	     else
	       mx_state <= MX_IDLE;
	 	 
	 MX_1 :
	   if(o_tready_int & o_tvalid_int & o_tlast_int)
	     if(PRIO)
	       mx_state <= MX_IDLE;
	     else if(i2_tvalid)
	       mx_state <= MX_2;
	     else if(i3_tvalid)
	       mx_state <= MX_3;
	     else
	       mx_state <= MX_IDLE;

	 MX_2 :
	   if(o_tready_int & o_tvalid_int & o_tlast_int)
	     if(PRIO)
	       mx_state <= MX_IDLE;
	     else if(i3_tvalid)
	       mx_state <= MX_3;
	     else
	       mx_state <= MX_IDLE;
	 	 
	 MX_3 :
	   if(o_tready_int & o_tvalid_int & o_tlast_int)
	     if(PRIO)
	       mx_state <= MX_IDLE;
	     else
	       mx_state <= MX_IDLE;
	 	 
	 default :
	   mx_state <= MX_IDLE;
       endcase // case (mx_state)

   assign {i3_tready, i2_tready, i1_tready, i0_tready} = mx_state & {4{o_tready_int}};

   assign o_tvalid_int = |(mx_state & ({i3_tvalid, i2_tvalid, i1_tvalid, i0_tvalid}));
         
   assign {o_tlast_int, o_tdata_int} = mx_state[3] ? {i3_tlast, i3_tdata} :
				       mx_state[2] ? {i2_tlast, i2_tdata} :
				       mx_state[1] ? {i1_tlast, i1_tdata} :
				       {i0_tlast, i0_tdata};

   generate
      if(BUFFER == 0)
	begin
	   assign o_tdata = o_tdata_int;
	   assign o_tlast = o_tlast_int;
	   assign o_tvalid = o_tvalid_int;
	   assign o_tready_int = o_tready;
	end
      else
	axi_fifo_short #(.WIDTH(WIDTH+1)) axi_fifo_short
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({o_tlast_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
	   .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
	   .space(), .occupied());
   endgenerate
   
endmodule // axi__mux4
