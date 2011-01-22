
// Parameter LE tells us if we are little-endian.  
// Little-endian means send lower 16 bits first.
// Default is big endian (network order), send upper bits first.

module fifo19_to_fifo36
  #(parameter LE=0)
   (input clk, input reset, input clear,
    input [18:0] f19_datain,
    input f19_src_rdy_i,
    output f19_dst_rdy_o,

    output [35:0] f36_dataout,
    output f36_src_rdy_o,
    input f36_dst_rdy_i,
    output [31:0] debug
    );

   reg 		  f36_sof, f36_eof;
   reg [1:0] 	  f36_occ;
   
   reg [1:0] 	  state;
   reg [15:0] 	  dat0, dat1;

   wire 	  f19_sof  = f19_datain[16];
   wire 	  f19_eof  = f19_datain[17];
   wire 	  f19_occ  = f19_datain[18];

   wire 	  xfer_out = f36_src_rdy_o & f36_dst_rdy_i;

   always @(posedge clk)
     if(f19_src_rdy_i & ((state==0)|xfer_out))
       f36_sof 	<= f19_sof;

   always @(posedge clk)
     if(f19_src_rdy_i & ((state != 2)|xfer_out))
       f36_eof 	<= f19_eof;

   always @(posedge clk)
     if(reset)
       begin
	  state 	<= 0;
	  f36_occ <= 0;
       end
     else
       if(f19_src_rdy_i)
	 case(state)
	   0 : 
	     begin
		dat0 <= f19_datain;
		if(f19_eof)
		  begin
		     state <= 2;
		     f36_occ <= f19_occ ? 2'b01 : 2'b10;
		  end
		else
		  state <= 1;
	     end
	   1 : 
	     begin
		dat1 <= f19_datain;
		state <= 2;
		if(f19_eof)
		  f36_occ <= f19_occ ? 2'b11 : 2'b00;
	     end
	   2 : 
	     if(xfer_out)
	       begin
		  dat0 <= f19_datain;
		  if(f19_eof) // remain in state 2 if we are at eof
		    f36_occ <= f19_occ ? 2'b01 : 2'b10;
		  else
		    state 	   <= 1;
	       end
	 endcase // case(state)
       else
	 if(xfer_out)
	   begin
	      state 	   <= 0;
	      f36_occ <= 0;
	   end
   
   assign    f19_dst_rdy_o  = xfer_out | (state != 2);
   assign    f36_dataout    = LE ? {f36_occ,f36_eof,f36_sof,dat1,dat0} :
			      {f36_occ,f36_eof,f36_sof,dat0,dat1};
   assign    f36_src_rdy_o  = (state == 2);

   assign    debug = state;
   
endmodule // fifo19_to_fifo36
