
module gpmc_to_fifo
  (input EM_CLK, input [15:0] EM_D, input [1:0] EM_NBE,
   input EM_NCS, input EM_NWE,

   input fifo_clk, input fifo_rst,
   output reg [17:0] data_o, output reg src_rdy_o, input dst_rdy_i,

   input [15:0] frame_len, input [15:0] fifo_space, output fifo_ready);

   reg [10:0] counter;
   // Synchronize the async control signals
   reg [1:0] 	cs_del, we_del;
   always @(posedge fifo_clk)
     if(fifo_rst)
       begin
	  cs_del <= 2'b11;
	  we_del <= 2'b11;
       end
     else
       begin
	  cs_del <= { cs_del[0], EM_NCS };
	  we_del <= { we_del[0], EM_NWE };
       end

   wire do_write = (~cs_del[0] & (we_del == 2'b10));
   wire first_write = (counter == 0);
   wire last_write = ((counter+1) == frame_len);

   always @(posedge fifo_clk)
     if(do_write)
       begin
	  data_o[15:0] <= EM_D;
	  data_o[16] <= first_write;
	  data_o[17] <= last_write;
	  //  no byte writes data_o[18] <= |EM_NBE;  // mark half full if either is not enabled  FIXME
       end

   always @(posedge fifo_clk)
     if(fifo_rst)
       src_rdy_o <= 0;
     else if(do_write)
       src_rdy_o <= 1;
     else
       src_rdy_o <= 0;    // Assume it was taken

   always @(posedge fifo_clk)
     if(fifo_rst)
       counter <= 0;
     else if(do_write)
       if(last_write)
	 counter <= 0;
       else
	 counter <= counter + 1;

   assign fifo_ready = first_write & (fifo_space > frame_len);
   
endmodule // gpmc_to_fifo
