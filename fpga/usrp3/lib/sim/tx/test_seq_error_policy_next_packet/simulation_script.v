 initial
     begin
	tvalid <= 1'b0;
	while(reset)
	  @(posedge clk);
	write_setting_bus(SR_ERROR_POLICY,32'h2);
	write_setting_bus(SR_PACKETS,32'h8000_0002);

	write_setting_bus(SR_INTERP,32'h1);

	// Burst of len 2
	send_burst(2/*count*/,5/*len*/,32'hA000_0000/*start*/,64'h100/*time*/,12'h000/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

	// Burst of 2
	send_burst(2/*count*/,10/*len*/,32'hC000_0000/*start*/,64'h200/*time*/,12'h002/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

	// Inter burst sequence error
	send_burst(2/*count*/,10/*len*/,32'hC000_0000/*start*/,64'h300/*time*/,12'h015/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

	// Burst of 2
	send_burst(2/*count*/,10/*len*/,32'hC000_0000/*start*/,64'h400/*time*/,12'h0017/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

     end // initial begin
   