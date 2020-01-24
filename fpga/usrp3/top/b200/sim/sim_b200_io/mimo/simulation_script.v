
   initial
     begin
	reset <= 1;
	mimo <= 0;
	repeat(10) @(posedge rx_clk);
	reset <= 0;
	
	repeat(10) @(posedge rx_clk);
	
	mimo_burst(20);
	repeat(10) @(posedge rx_clk);
	$finish;
     end
