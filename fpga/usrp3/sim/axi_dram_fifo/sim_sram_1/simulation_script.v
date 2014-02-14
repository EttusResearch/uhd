reg [31:0] count_rx, count_tx;
reg status;
reg fail;


//
// Use task library
//
`define USE_TASKS

   initial 
     begin
	clk <= 1'b0;
	reset <= 1'b0;
	clear <= 1'b0;
	i_tdata_r <= 0;
	i_tlast_r <= 0;
	i_tvalid_r <= 0;
	o_tready_r <= 0;
     end
   
   always
     #5 clk <= ~clk;
   
   initial
     begin
	count_tx = 2;
	count_rx = 2;
	status = 0;
	
	
	@(negedge clk);
	reset <= 1'b1;
	repeat(10) @(negedge clk);
	reset <= 1'b0;
	repeat(10) @(negedge clk);

	// Send 40 packets.
	repeat(40) begin
	   send_raw_packet(count_tx);
	   repeat(2) @(posedge clk);
	   count_tx = count_tx + 1;
	   @(posedge clk);
	end
	repeat(100) @(posedge clk);
	

	// Recieve 40 packets
	repeat(40) begin
	   receive_raw_packet(count_rx,fail);
	   status = status || fail;
	   repeat(2) @(posedge clk);
	   count_rx = count_rx + 1;
	   @(posedge clk);
	end
	repeat(100) @(posedge clk);

	count_tx = 2;
	count_rx = 2;
	
	// Send 40 packets.
	repeat(40) begin
	   send_raw_packet(count_tx);
	   repeat(2) @(posedge clk);
	   count_tx = count_tx + 1;
	   @(posedge clk);
	end
	repeat(100) @(posedge clk);
	// Now fork so send and receive run concurrently
	fork
	   begin
	      // Send 40 packets.
	      repeat(40) begin
		 send_raw_packet(count_tx);
		 repeat(2) @(posedge clk);
		 count_tx = count_tx + 1;
		 @(posedge clk);
	      end
	   end
	   begin
	      // Recieve 80 packets
	      repeat(80) begin
		 receive_raw_packet(count_rx,status);
		 status = status || fail;
		 repeat(2) @(posedge clk);
		 count_rx = count_rx + 1;
		 @(posedge clk);
		 if (status !== 0) begin
		    repeat(100) @(posedge clk);
		    $display("FAILED.");
		    $finish;	
		 end
	      end
	   end
	join
	// Now single threaded agian.
	repeat(100) @(posedge clk);

	$display;
	// Should not be able to get to here with FAIL status but check anyhow
	if (status != 0)
	  $display("FAILED.");
	else
	  $display("PASSED.");
	
	@(posedge clk);
	$finish;
	
     end
   
   //initial
   //  o_tready = 1;
   
