

module ram_to_fifo
  (input clk, input reset,
   input [10:0] read_length,  // From the dbsm (?)
   output read_en, output reg [8:0] read_addr, input [31:0] read_data, input read_ready, output read_done,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   // read_length/2 = number of 32 bit lines, numbered 0 through read_length/2-1
   wire [8:0] 	 last_line = (read_length[10:1]-1); 

   reg 		 read_phase, sop;

   assign read_en = (read_phase == 0) | dst_rdy_i;
   assign src_rdy_o = (read_phase == 1);
   
   always @(posedge clk)
     if(reset)
       begin
	  read_addr <= 0;
	  read_phase <= 0;
	  sop <= 1;
       end
     else
       if(read_phase == 0)
	 begin
	    read_addr <= read_ready;
	    read_phase <= read_ready;
	 end
       else if(dst_rdy_i)
	 begin
	    sop <= 0;
	    if(read_addr == last_line)
	      begin
		 read_addr <= 0;
		 read_phase <= 0;
	      end
	    else
	      read_addr <= read_addr + 1;
	 end
   
   assign read_done = (read_phase == 1) & (read_addr == last_line) & dst_rdy_i;
   wire eop = (read_addr == last_line);
   assign data_o = { 2'b00, eop, sop, read_data };
   
endmodule // ram_to_fifo
