
`include "../serial_settings_tasks.v"

   initial 
     begin
	clk <= 1'b0;
	reset <= 1'b0;
	scl_r <= 1'b1;
	sda_r <= 1'b1;
     end
   
   always
     #5 clk <= ~clk;
   
   initial
     begin

	
	@(negedge clk);
	reset <= 1'b1;
	repeat(10) @(negedge clk);
	reset <= 1'b0;
	repeat(10) @(negedge clk);

	serial_settings_transaction(8'h0,32'h01b2);

	serial_settings_transaction(8'h3, 32'h5);
	
	serial_settings_transaction(8'h3,32'hA);
	
	serial_settings_transaction(8'h3,32'hF);
	
	
	repeat(10000) @(negedge clk);	 
	@(negedge clk);
	@(negedge clk);	
	@(negedge clk);

	$finish;
	
     end // initial begin

	 