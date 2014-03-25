
// Copyright 2014 Ettus Research LLC


module axi_chdr_header_trigger
  #(
    parameter WIDTH=64,
    parameter SID=0
   )
    (input clk, input reset, input clear,
     input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, input i_tready,
     output trigger
     );

   
   reg 	  state;
   localparam IDLE = 0;
   localparam RUN  = 1;
 

   always @(posedge clk)
     if(reset | clear)
       state <= IDLE;
     else
       case (state)
	 IDLE :
	   if(i_tvalid && i_tready)
	     state <= RUN;

	 RUN :
	   if(i_tready && i_tvalid && i_tlast)	    
	     state <= IDLE;

	 default :
	   state <= IDLE;
       endcase // case (state)

   assign     trigger =  i_tvalid && i_tready && (state == IDLE) && (i_tdata[15:0] != SID);

endmodule // axi_chdr_header_trigger
