
// Copyright 2012 Ettus Research LLC
// axi_demux -- takes one AXI stream, sends to one of 4 output channels
//   Choice of output channel is by external logic based on first line of packet ("header" port)
//   If compressed vita data, this line contains vita header and streamid.

module axi_demux4
  #(parameter ACTIVE_CHAN = 4'b1111,  // ACTIVE_CHAN is a map of connected outputs
    parameter WIDTH = 64,
    parameter BUFFER=0)
   (input clk, input reset, input clear,
    output [WIDTH-1:0] header, input [1:0] dest,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o0_tdata, output o0_tlast, output o0_tvalid, input o0_tready,
    output [WIDTH-1:0] o1_tdata, output o1_tlast, output o1_tvalid, input o1_tready,
    output [WIDTH-1:0] o2_tdata, output o2_tlast, output o2_tvalid, input o2_tready,
    output [WIDTH-1:0] o3_tdata, output o3_tlast, output o3_tvalid, input o3_tready);

   wire [WIDTH-1:0]    i_tdata_int;
   wire 	       i_tlast_int, i_tvalid_int, i_tready_int;

   generate
      if(BUFFER == 0)
	begin
	   assign i_tdata_int = i_tdata;
	   assign i_tlast_int = i_tlast;
	   assign i_tvalid_int = i_tvalid;
	   assign i_tready = i_tready_int;
	end
      else
	axi_fifo_short #(.WIDTH(WIDTH+1)) axi_fifo_short
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
	   .o_tdata({i_tlast_int,i_tdata_int}), .o_tvalid(i_tvalid_int), .o_tready(i_tready_int),
	   .space(), .occupied());
   endgenerate

   reg [3:0] 	  dm_state;
   localparam DM_IDLE = 4'b0000;
   localparam DM_0    = 4'b0001;
   localparam DM_1    = 4'b0010;
   localparam DM_2    = 4'b0100;
   localparam DM_3    = 4'b1000;

   assign header = i_tdata_int;
   
   always @(posedge clk)
     if(reset | clear)
       dm_state <= DM_IDLE;
     else
       case (dm_state)
	 DM_IDLE :
	   if(i_tvalid_int)
	     case(dest)
	       2'b00 : dm_state <= DM_0;
	       2'b01 : dm_state <= DM_1;
	       2'b10 : dm_state <= DM_2;
	       2'b11 : dm_state <= DM_3;
	     endcase // case (i_tdata[1:0])
	 
	 DM_0, DM_1, DM_2, DM_3 :
	   if(i_tvalid_int & i_tready_int & i_tlast_int)
	     dm_state <= DM_IDLE;
	 
	 default :
	   dm_state <= DM_IDLE;
       endcase // case (dm_state)

   assign {o3_tvalid, o2_tvalid, o1_tvalid, o0_tvalid} = dm_state & {4{i_tvalid_int}};
   assign i_tready_int = |(dm_state & ({o3_tready, o2_tready, o1_tready, o0_tready} | ~ACTIVE_CHAN));
      
   assign {o0_tlast, o0_tdata} = {i_tlast_int, i_tdata_int};
   assign {o1_tlast, o1_tdata} = {i_tlast_int, i_tdata_int};
   assign {o2_tlast, o2_tdata} = {i_tlast_int, i_tdata_int};
   assign {o3_tlast, o3_tdata} = {i_tlast_int, i_tdata_int};
   
endmodule // axi_demux4
