
// Mux packets from multiple FIFO interfaces onto a single one.
//  Can alternate or give priority to one port (port 0)
//  In prio mode, port 1 will never get access if port 0 is always busy

module fifo36_mux
  #(parameter prio = 0)
   (input clk, input reset, input clear,
    input [35:0] data0_i, input src0_rdy_i, output dst0_rdy_o,
    input [35:0] data1_i, input src1_rdy_i, output dst1_rdy_o,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   localparam MUX_IDLE0 = 0;
   localparam MUX_DATA0 = 1;
   localparam MUX_IDLE1 = 2;
   localparam MUX_DATA1 = 3;
   
   reg [1:0] 	  state;

   wire 	  eof0 = data0_i[33];
   wire 	  eof1 = data1_i[33];
   
   always @(posedge clk)
     if(reset | clear)
       state <= MUX_IDLE0;
     else
       case(state)
	 MUX_IDLE0 :
	   if(src0_rdy_i)
	     state <= MUX_DATA0;
	   else if(src1_rdy_i)
	     state <= MUX_DATA1;

	 MUX_DATA0 :
	   if(src0_rdy_i & dst_rdy_i & eof0)
	     state <= prio ? MUX_IDLE0 : MUX_IDLE1;

	 MUX_IDLE1 :
	   if(src1_rdy_i)
	     state <= MUX_DATA1;
	   else if(src0_rdy_i)
	     state <= MUX_DATA0;
	   
	 MUX_DATA1 :
	   if(src1_rdy_i & dst_rdy_i & eof1)
	     state <= MUX_IDLE0;
	 
	 default :
	   state <= MUX_IDLE0;
       endcase // case (state)

   assign dst0_rdy_o = (state==MUX_DATA0) ? dst_rdy_i : 0;
   assign dst1_rdy_o = (state==MUX_DATA1) ? dst_rdy_i : 0;
   assign src_rdy_o = (state==MUX_DATA0) ? src0_rdy_i : (state==MUX_DATA1) ? src1_rdy_i : 0;
   assign data_o = (state==MUX_DATA0) ? data0_i : data1_i;
   
endmodule // fifo36_demux
