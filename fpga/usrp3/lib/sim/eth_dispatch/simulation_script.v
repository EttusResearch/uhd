

initial $dumpfile("eth_dispatch_tb.vcd");
initial $dumpvars(0,eth_dispatch_tb);

   reg [15:0] x;

   localparam MAC=48'h010203040506;
   localparam IP=(192<<24)|(168<<16)|2;
   localparam PORT0=60000;
   localparam PORT1=60001;
   
   
   initial
     begin
	@(posedge clk);
	reset <= 1;
	repeat (5) @(posedge clk);
	@(posedge clk);
	reset <= 0;
	@(posedge clk);
	// Set my MAC address
	write_setting_bus(0,MAC&32'hFFFFFFFF);   
	write_setting_bus(1,MAC>>32);
	// Set my IP Address
	write_setting_bus(2,IP);
	// Set UDP ports for ViTA traffic
	write_setting_bus(3,PORT1<<16|PORT0);
	@(posedge clk);
	enqueue_vita_pkt(MAC,IP,PORT0,10,0,{16'h0,8'hf,8'h5});
	enqueue_vita_pkt(MAC,IP,16'h1234,10,0,{16'h0,8'h5,8'hc});
	enqueue_vita_pkt(48'h223344556677,32'h02030405,16'h1234,10,0,{16'h0,8'hd,8'h7});
	enqueue_arp_req(48'h112233445566,32'h09080706,MAC,IP);
	
/* -----\/----- EXCLUDED -----\/-----
	// 2x2 Switch so only mask one bit of SID for route dest.
	// Each slave must have a unique address, logic doesn't check for this.
	//
	// Network Addr 0 & 1 go to Slave 0.
	write_setting_bus(0,0);   // 0.X goes to Port 0
	write_setting_bus(1,0);   // 1.X goes to Port 0
	// Local Addr = 2
	write_setting_bus(512,2);
	// Host Addr 0 & 2 go to Slave 0...
	write_setting_bus(256,0); // 2.0 goes to Port 0
	write_setting_bus(258,0); // 2.2 goes to Port 0
	// ...Host Addr 1 & 3 go to Slave 1...
	write_setting_bus(257,1); // 2.1 goes to Port 1
	write_setting_bus(259,1); // 2.3 goes to Port 1
	//
	@(posedge clk);
	fork
	   begin
	      // Master0, addr 0.0 to Slave0
	      enqueue_vita_pkt(0,10,0,{16'h0,8'h0,8'h0});
	      // Master0, addr 2.0 to Slave0
	      enqueue_vita_pkt(0,11,'h12345678,{16'h0,8'h2,8'h0});
	      // Master0, addr 2.3 to Slave1
	      enqueue_vita_pkt(0,14,'h45678901,{16'h0,8'h2,8'h3});
	      // Master0, addr 2.2 to Slave0
	      enqueue_vita_pkt(0,11,'h67890123,{16'h0,8'h2,8'h2});
	   end
	   begin
	      // Master1, addr 1.0 to Slave0
	      enqueue_vita_pkt(1,12,'h23456789,{16'h0,8'h1,8'h0});
	      // Master1, addr 2.1 to Slave1
	      enqueue_vita_pkt(1,13,'h34567890,{16'h0,8'h2,8'h1});
	      // Master1, addr 2.3 to Slave1
	      enqueue_vita_pkt(1,14,'h56789012,{16'h0,8'h2,8'h3});
	   end
	join
 -----/\----- EXCLUDED -----/\----- */
	
	repeat (1000) @(posedge clk);
	$finish;
	
     end // initial begin
   
