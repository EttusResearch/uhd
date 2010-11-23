
// Assumes a GPMC cycle with GPMC clock, as in the timing diagrams
//   If a packet bigger or smaller than we are told is sent, behavior is undefined.
//   If dst_rdy_i is low when we get data, behavior is undefined and we signal bus error.
//   If there is a bus error, we should be reset

module gpmc_to_fifo_sync
  (input arst,
   input EM_CLK, input [15:0] EM_D, input [1:0] EM_NBE,
   input EM_NCS, input EM_NWE,
   output reg [17:0] data_o, output reg src_rdy_o, input dst_rdy_i,
   input [15:0] frame_len, input [15:0] fifo_space, output fifo_ready, 
   output reg bus_error);
   
   reg [10:0] 	counter;
   wire 	first_write = (counter == 0);
   wire 	last_write = ((counter+1) == frame_len);
   wire 	do_write = ~EM_NCS & ~EM_NWE;
   
   always @(posedge EM_CLK or posedge arst)
     if(arst)
       data_o <= 0;
     else if(do_write)
       begin
	  data_o[15:0] <= EM_D;
	  data_o[16] <= first_write;
	  data_o[17] <= last_write;
	  //  no byte writes data_o[18] <= |EM_NBE;  // mark half full if either is not enabled  FIXME
       end

   always @(posedge EM_CLK or posedge arst)
     if(arst)
       src_rdy_o <= 0;
     else if(do_write & ~bus_error)  // Don't put junk in if there is a bus error
       src_rdy_o <= 1;
     else
       src_rdy_o <= 0;    // Assume it was taken, ignore dst_rdy_i

   always @(posedge EM_CLK or posedge arst)
     if(arst)
       counter <= 0;
     else if(do_write)
       if(last_write)
	 counter <= 0;
       else
	 counter <= counter + 1;

   assign fifo_ready = first_write & (fifo_space > frame_len);
   
   always @(posedge EM_CLK or posedge arst)
     if(arst)
       bus_error <= 0;
     else if(src_rdy_o & ~dst_rdy_i)
       bus_error <= 1;
   // must be reset to make the error go away

endmodule // gpmc_to_fifo_sync
