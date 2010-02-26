
module bsm
  (input clk, input reset, input clear,
   input write_done,
   input read_done,
   output readable,
   output writeable);

   reg 	  state;
   localparam ST_WRITEABLE = 0;
   localparam ST_READABLE = 1;
   
   always @(posedge clk)
     if(reset | clear)
       state <= ST_WRITEABLE;
     else
       case(state)
	 ST_WRITEABLE :
	   if(write_done)
	     state <= ST_READABLE;
	 ST_READABLE :
	   if(read_done)
	     state <= ST_WRITEABLE;
       endcase // case (state)

   assign readable = (state == ST_READABLE);
   assign writeable = (state == ST_WRITEABLE);
   
endmodule // bsm

module dbsm
  (input clk, input reset, input clear,
   output reg read_sel, output read_ready, input read_done,
   output reg write_sel, output write_ready, input write_done);

   localparam NUM_BUFS = 2;

   wire       [NUM_BUFS-1:0] readable, writeable, read_done_buf, write_done_buf;
   
   // Two of these buffer state machines
   genvar     i;
   for(i=0;i<NUM_BUFS;i=i+1)
     generate
	bsm bsm(.clk(clk), .reset(reset), .clear(clear), 
		.write_done((write_sel == i) & write_done), 
		.read_done((read_sel == i) & read_done), 
		.readable(readable[i]), .writeable(writeable[i]));
     endgenerate
   
   reg 	 full;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  write_sel <= 0;
	  full <= 0;
       end
     else
       if(write_done)
	 if(writeable[write_sel]==(NUM_BUFS-1))
	   begin
	      write_sel <= 0;
	      if(read_sel == 0)
		full <= 1;
	   end
	 else
	   begin
	      write_sel <= write_sel + 1;
	      if(read_sel == write_sel + 1)
		full <= 1;
	   end // else: !if(writeable[write_sel]==(NUM_BUFS-1))
       else if(read_done)
	 full <= 0;

   always @(posedge clk)
     if(reset | clear)
       read_sel <= 0;
     else
       if(read_done)
	 if(readable[read_sel]==(NUM_BUFS-1))
	   read_sel <= 0;
	 else
	   read_sel <= read_sel + 1;
          
   assign write_ready = writeable[write_sel];
   assign read_ready = readable[read_sel];

endmodule // dbsm
