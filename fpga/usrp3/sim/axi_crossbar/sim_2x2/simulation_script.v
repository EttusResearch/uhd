// Simulate a 2x2 switch configuration
localparam NUM_INPUTS = 2;
localparam NUM_OUTPUTS = 2;

//initial $dumpfile("axi_crossbar_tb.vcd");
//initial $dumpvars(0,axi_crossbar_tb);

reg [15:0] x;
reg [31:0] seq_i0, seq_i1, seq_o0, seq_o1;


/////////////////////////////////////////////
//
// Control and input data thread.
//
/////////////////////////////////////////////
initial
  begin
     @(posedge clk);
     reset <= 1;
     repeat (5) @(posedge clk);
     @(posedge clk);
     reset <= 0;
     @(posedge clk);
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
/* -----\/----- EXCLUDED -----\/-----
     @(posedge clk);
     fork
	begin
	   // input_port,size,tsf,sid
	   //
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
     //
     @(posedge clk);
     fork
	begin
	   // Master0 Sender Thread.
	   //
	   // Master0, addr 0.0 to Slave0
	   for (seq_i0 = 0; seq_i0 < 10; seq_i0=seq_i0 + 1)
	     enqueue_chdr_pkt_count(0,seq_i0,32+seq_i0,0,0,0,0,`SID(0,0,0,0));
	   // Master1, addr 1.0 to Slave0
	   for (seq_i0 = 20; seq_i0 < 30; seq_i0=seq_i0 + 1)
	     enqueue_chdr_pkt_count(0,seq_i0,32+seq_i0,0,0,0,0,`SID(0,0,1,0));
	end

	begin
	   // Master1 Sender Thread.
	   //
	   // Master1, addr 2.1 to Slave1
	   for (seq_i1 = 10; seq_i1 < 20; seq_i1=seq_i1 + 1)
	     enqueue_chdr_pkt_count(1,seq_i1,32+seq_i1,1,'h12345678+seq_i1*100,0,0,`SID(0,0,2,1));
	   // Master0, addr 2.3 to Slave1
	   for (seq_i1 = 30; seq_i1 < 40; seq_i1=seq_i1 + 1)
	     enqueue_chdr_pkt_count(1,seq_i1,32+seq_i1,1,'h23456789+seq_i1*100,0,0,`SID(0,0,2,3));
	end
     join

     repeat (1000) @(posedge clk);


  end // initial begin


   /////////////////////////////////////////////
   //
   // Control and input data thread.
   //
   /////////////////////////////////////////////
   initial
     begin
	// Wait for reset to go high
	while (reset!==1'b1)
	  @(posedge clk);
	// Wait for reset to go low
	while (reset!==1'b0)
	  @(posedge clk);
	// Fork concurrent output checkers for each egress port.
	fork
	   begin
	      // Slave0 Recevier thread.
	      //
	      // Master0, addr 0.0 to Slave0
	      for (seq_o0 = 0; seq_o0 < 10; seq_o0=seq_o0 + 1)
		dequeue_chdr_pkt_count(0,seq_o0,32+seq_o0,0,0,0,0,`SID(0,0,0,0));
	      // Master1, addr 1.0 to Slave0
	      for (seq_o0 = 20; seq_o0 < 30; seq_o0=seq_o0 + 1)
		dequeue_chdr_pkt_count(0,seq_o0,32+seq_o0,0,0,0,0,`SID(0,0,1,0));
	   end
	   begin
	      // Slave1 Receiver thread.
	      //
	      // Master1, addr 2.1 to Slave1
	      for (seq_o1 = 10; seq_o1 < 20; seq_o1=seq_o1 + 1)
		dequeue_chdr_pkt_count(1,seq_o1,32+seq_o1,1,'h12345678+seq_o1*100,0,0,`SID(0,0,2,1));
	      // Master0, addr 2.3 to Slave1
	      for (seq_o1 = 30; seq_o1 < 40; seq_o1=seq_o1 + 1)
		dequeue_chdr_pkt_count(1,seq_o1,32+seq_o1,1,'h23456789+seq_o1*100,0,0,`SID(0,0,2,3));
	   end
	join

	repeat (1000) @(posedge clk);
	$finish;
     end // initial begin
