//
// Copyright 2012-2013 Ettus Research LLC
//


module axi_fifo_tb();
   
   reg clk, reset;
   reg read_flag, write_flag;
   
   reg error;
   reg [7:0] i_tdata, o_tdata_ref;
   wire [7:0] o_tdata;
   reg 	      i_tvalid, o_tready;
   wire       o_tvalid, i_tready;
   wire [15:0] space, occupied;

   always
     #100 clk = ~clk;

   initial clk = 0;
   
 
   axi_fifo 
     #(
       .WIDTH(8),
       .SIZE(8)
       )
       dut
	 (.clk(clk),
	  .reset(reset),
	  .clear(1'b0),
	  .i_tdata(i_tdata),
	  .i_tvalid(i_tvalid),
	  .i_tready(i_tready),
	  .o_tdata(o_tdata),
	  .o_tvalid(o_tvalid),
	  .o_tready(o_tready),    
	  .space(space),
	  .occupied(occupied)
	  );


   task write;
      begin
	 write_flag <= 1;
	 i_tvalid <= 1'b1;	
	 #1; 
	 while (i_tready != 1'b1)
	   @(posedge clk);
	 #1;
	 @(posedge clk);
	 write_flag <= 0;
	 i_tvalid <= 1'b0;
	 i_tdata <= i_tdata + 8'h1;
      end
   endtask // write

   task read;
      begin
	 read_flag <= 1;
	 o_tready <= 1'b1;
	 #1;
	 while (o_tvalid != 1'b1)
	    @(posedge clk);
	 #1;
	 @(posedge clk);
	 read_flag <= 0;
	 o_tready <= 1'b0;
	 if (o_tdata_ref != o_tdata) begin
	    $display("ERROR: Expected %d, got %d, at time %d",o_tdata_ref,o_tdata,$time);
	    error <= 1'b1;
	 end else  
	   error <= 1'b0;
	 o_tdata_ref = o_tdata_ref + 8'h1;
      end
   endtask // read

   initial
     begin
	reset <= 1'b0;
	error <= 1'b0;
	i_tdata <= 8'b00;
	o_tdata_ref <= 8'b00;
	i_tvalid <= 1'b0;
	o_tready <= 1'b0;
	read_flag <= 0;
	write_flag <= 0;
	
	repeat(10) @(posedge clk);
	reset <= 1'b1;
   	repeat(10) @(posedge clk);
	reset <= 1'b0;
	@(posedge clk);
	@(negedge clk);

	// FIFO Should be empty now, check avail space
	if (space != 16'd256)
	  begin $display("ERROR: FIFO is empty, space should read 256 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd0)
	  begin $display("ERROR: FIFO is empty, occupied should read 0 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b1)
	  begin $display("ERROR: FIFO is empty, o_tvalid should be 0 at time %d",$time); error <= 1; end
	@(posedge clk);
	// Push 1 item onto FIFO, check fullness updates accordingly
	write();
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd255)
	  begin $display("ERROR: FIFO space should read 255 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd1)
	  begin $display("ERROR: FIFO occupied should read 1 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b0)
	  begin $display("ERROR: FIFO is not empty, o_tvalid should be 1 at time %d",$time); error <= 1; end
	// Pop FIFO once, check it goes back empty OK.
	@(posedge clk);	
	read();
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd256)
	  begin $display("ERROR: FIFO is empty, space should read 256 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd0)
	  begin $display("ERROR: FIFO is empty, occupied should read 0 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b1)
	  begin $display("ERROR: FIFO is empty, o_tvalid should be 0 at time %d",$time); error <= 1; end
	// Push FIFO 255 times and see if it goes full incorrectly
	repeat(255) begin
	   @(posedge clk);
	   write();
	end
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd1)
	  begin $display("ERROR: FIFO is nearly full, space should read 1 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd255)
	  begin $display("ERROR: FIFO is nearly full, occupied should read 255 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b0)
	  begin $display("ERROR: FIFO is nearly full, o_tvalid should be 1 at time %d",$time); error <= 1; end
	if (i_tready == 1'b0)
	  begin $display("ERROR: FIFO is nearly full, i_tready should be 1 at time %d",$time); error <= 1; end
	// Push FIFO one more time, now it should be full
	@(posedge clk);	
	write();
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd0)
	  begin $display("ERROR: FIFO is full, space should read 0 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd256)
	  begin $display("ERROR: FIFO is full, occupied should read 256 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b0)
	  begin $display("ERROR: FIFO is full, o_tvalid should be 1 at time %d",$time); error <= 1; end
	if (i_tready == 1'b1)
	  begin $display("ERROR: FIFO is full, i_tready should be 0 at time %d",$time); error <= 1; end
	// POP FIFO once, check it went nonfull.
	@(posedge clk);	
	read();
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd1)
	  begin $display("ERROR: FIFO is nearly full, space should read 1 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd255)
	  begin $display("ERROR: FIFO is nearly full, occupied should read 255 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b0)
	  begin $display("ERROR: FIFO is nearly full, o_tvalid should be 1 at time %d",$time); error <= 1; end
	if (i_tready == 1'b0)
	  begin $display("ERROR: FIFO is nearly full, i_tready should be 1 at time %d",$time); error <= 1; end
	// Take FIFO to empty state
	repeat(255) begin
	   @(posedge clk);
	   read();
	end
	@(posedge clk);
	@(negedge clk);
	if (space != 16'd256)
	  begin $display("ERROR: FIFO is empty, space should read 256 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd0)
	  begin $display("ERROR: FIFO is empty, occupied should read 0 not %d at time %d",occupied,$time); error <= 1; end
	if (o_tvalid == 1'b1)
	  begin $display("ERROR: FIFO is empty, o_tvalid should be 0 at time %d",$time); error <= 1; end
	// Push 1 item onto FIFO
	@(posedge clk);
	write();
	@(posedge clk);
	// Now write twice as fast as we read, and write 256 times, which should leave, 129 elements in FIFO.
	fork
	   repeat(256) begin
	      write();
	      @(posedge clk);
	   end
	   repeat(128) begin
	      read();
	      @(posedge clk);
	      @(posedge clk);
	   end
	join
	@(posedge clk);
	if (space != 16'd127)
	  begin $display("ERROR: FIFO space should read 127 not %d at time %d",space,$time); error <= 1; end
	if (occupied != 16'd129)
	  begin $display("ERROR: FIFO occupied should read 129 not %d at time %d",occupied,$time); error <= 1; end
	

	
	//
	// END
	//
	repeat(10) @(posedge clk);
	$finish;
     end // initial begin
   
endmodule // axi_fifo_tb
