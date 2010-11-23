
// Assumes an asynchronous GPMC cycle
//   If a packet bigger or smaller than we are told is sent, behavior is undefined.
//   If dst_rdy_i is low when we get data, behavior is undefined and we signal bus error.
//   If there is a bus error, we should be reset

module fifo_to_gpmc_async
  (input clk, input reset, input clear,
   input [17:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [15:0] EM_D, input EM_NCS, input EM_NOE,
   input [15:0] frame_len);

   // Synchronize the async control signals
   reg [2:0] 	cs_del, oe_del;
   reg [15:0] 	counter;
   
   always @(posedge clk)
     if(reset)
       begin
	  cs_del <= 3'b11;
	  oe_del <= 3'b11;
       end
     else
       begin
	  cs_del <= { cs_del[1:0], EM_NCS };
	  oe_del <= { oe_del[1:0], EM_NOE };
       end

   wire do_read = ( (~cs_del[1] | ~cs_del[2]) & (oe_del[1:0] == 2'b01));  // change output on trailing edge
   wire first_read = (counter == 0);
   wire last_read = ((counter+1) == frame_len);

   assign EM_D = data_i[15:0];

   assign dst_rdy_o = do_read;

endmodule // fifo_to_gpmc_async
