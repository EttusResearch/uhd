wire fail;
wire done;
reg start;
reg [15:0] control;



axi_chdr_test_pattern axi_chdr_test_pattern_i
  (
   .clk(clk),
   .reset(reset),
  
   //
   // CHDR friendly AXI stream input
   //
   .i_tdata(i_tdata),
   .i_tlast(i_tlast),
   .i_tvalid(i_tvalid),
   .i_tready(i_tready),
   //
   // CHDR friendly AXI Stream output
   //
   .o_tdata(o_tdata),
   .o_tlast(o_tlast),
   .o_tvalid(o_tvalid),
   .o_tready(o_tready),
   //
   // Test flags
   //
   .start(start),
   .fail(fail),
   .done(done),
   .control(control)
   );

   
   always
     #5 clk <= ~clk;
   
   initial
     begin
	clk <= 1'b0;
	reset <= 1'b0;
	clear <= 1'b0;
	start <= 1'b0;
	control <= 16'h0101;
	
	
	@(negedge clk);
	reset <= 1'b1;
	repeat(10) @(negedge clk);
	reset <= 1'b0;
	repeat(10) @(negedge clk);
	// Now activate BIST
	start <= 1'b1;
	
	// Wait until simulation is done.
	while(!done)
	  @(negedge clk);
	
	$display;
	
	if (fail)
	  $display("FAILED.");
	else
	  $display("Done 1st pass.");
	
	@(posedge clk);
	start <= 1'b0;
	repeat(10) @(negedge clk);
	// Now activate BIST
	start <= 1'b1;
	
	// Wait until simulation is done.
	while(!done)
	  @(negedge clk);
	
	$display;
	
	if (fail)
	  $display("FAILED.");
	else
	  $display("PASSED.");
	
	$finish;
	
     end
   
   //initial
   //  o_tready = 1;
   
