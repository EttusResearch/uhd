// Simulate a 4x4 switch configuration
localparam NUM_INPUTS = 4;
localparam NUM_OUTPUTS = 4;

//initial $dumpfile("axi_crossbar_tb.vcd");
//initial $dumpvars(0,axi_crossbar_tb);

reg [15:0] x;
reg [31:0] seq_i0, seq_i1, seq_i2, seq_i3, seq_o0, seq_o1, seq_o2, seq_o3;
reg sync_flag0, sync_flag1;


/////////////////////////////////////////////
//
// Control and input data thread.
//
/////////////////////////////////////////////
initial
  begin
     // Flags to synchronise test bench threads
     sync_flag0 <= 0;
     sync_flag1 <= 0;

     @(posedge clk);
     reset <= 1;
     repeat (5) @(posedge clk);
     @(posedge clk);
     reset <= 0;
     @(posedge clk);
     // 2x2 Switch so only mask one bit of SID for route dest.
     // Each slave must have a unique address, logic doesn't check for this.
     //
     // Local Addr = 2
     write_setting_bus(512,2);
     // Network Addr 0 & 1 go to Slave 0.
     write_setting_bus(0,0);   // 0.X goes to Port 0
     write_setting_bus(1,0);   // 1.X goes to Port 0
     // Host Addr 0 goes to Slave 0...
     write_setting_bus(256,0); // 2.0 goes to Port 0
     // ...Host Addr 1 goes to Slave 1...
     write_setting_bus(257,1); // 2.1 goes to Port 1
     // ...Host Addr 2 goes to Slave 2...
     write_setting_bus(258,2); // 2.2 goes to Port 2
     // ...Host Addr 3 goes to Slave 3...
     write_setting_bus(259,3); // 2.3 goes to Port 3

     //
     @(posedge clk);
     fork
	begin
	   // Master0 Sender Thread.
	   //
	   // addr 2.3 to Slave3
	   for (seq_i0 = 0; seq_i0 < 10; seq_i0=seq_i0 + 1)
	     enqueue_chdr_pkt_count(0,seq_i0,32+seq_i0,1,'h12345678+seq_i0*100,0,0,`SID(0,0,2,3));

	   while (sync_flag0 !== 1'b1)
	     @(posedge clk);

	   //
	   // addr 2.0 to Slave0
	   for (seq_i0 = 30; seq_i0 < 40; seq_i0=seq_i0 + 1)
	     enqueue_chdr_pkt_count(0,seq_i0,32+seq_i0,1,'h45678901+seq_i0*100,0,0,`SID(0,0,2,0));

	end
	begin
	   // Master1 Sender Thread.
	   //
	   // addr 2.2 to Slave2
	   for (seq_i1 = 10; seq_i1 < 20; seq_i1=seq_i1 + 1)
	     enqueue_chdr_pkt_count(1,seq_i1,32+seq_i1,1,'h23456789+seq_i1*100,0,0,`SID(0,0,2,2));


	   while (sync_flag1 !== 1'b1)
	     @(posedge clk);

	   //
	   // addr 2.1 to Slave1
	   for (seq_i1 = 20; seq_i1 < 30; seq_i1=seq_i1 + 1)
	     enqueue_chdr_pkt_count(1,seq_i1,32+seq_i1,1,'h34567890+seq_i1*100,0,0,`SID(0,0,2,1));
	end
	begin
	   // Master2 Sender Thread.
	   //
	   // addr 2.1 to Slave1
	   for (seq_i2 = 20; seq_i2 < 30; seq_i2=seq_i2 + 1)
	     enqueue_chdr_pkt_count(2,seq_i2,32+seq_i2,1,'h34567890+seq_i2*100,0,0,`SID(0,0,2,1));

	   //
	   // addr 2.2 to Slave2
	   for (seq_i2 = 10; seq_i2 < 20; seq_i2=seq_i2 + 1)
	     enqueue_chdr_pkt_count(2,seq_i2,32+seq_i2,1,'h23456789+seq_i2*100,0,0,`SID(0,0,2,2));
	end
	begin
	   // Master3 Sender Thread.
	   //
	   // addr 2.0 to Slave0
	   for (seq_i3 = 30; seq_i3 < 40; seq_i3=seq_i3 + 1)
	     enqueue_chdr_pkt_count(3,seq_i3,32+seq_i3,1,'h45678901+seq_i3*100,0,0,`SID(0,0,2,0));

	   //
	   // addr 2.3 to Slave3
	   for (seq_i3 = 0; seq_i3 < 10; seq_i3=seq_i3 + 1)
	     enqueue_chdr_pkt_count(3,seq_i3,32+seq_i3,1,'h12345678+seq_i3*100,0,0,`SID(0,0,2,3));
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
	      // addr 2.0 to Slave0
	      for (seq_o0 = 30; seq_o0 < 40; seq_o0=seq_o0 + 1)
		dequeue_chdr_pkt_count(0,seq_o0,32+seq_o0,1,'h45678901+seq_o0*100,0,0,`SID(0,0,2,0));

	      sync_flag0 <= 1'b1;

	      //
	      // addr 2.0 to Slave0
	      for (seq_o0 = 30; seq_o0 < 40; seq_o0=seq_o0 + 1)
		enqueue_chdr_pkt_count(0,seq_o0,32+seq_o0,1,'h45678901+seq_o0*100,0,0,`SID(0,0,2,0));
	   end

	   begin
	      // Slave1 Recevier thread.
	      //
	      // addr 2.1 to Slave1
	      for (seq_o1 = 20; seq_o1 < 30; seq_o1=seq_o1 + 1)
		dequeue_chdr_pkt_count(1,seq_o1,32+seq_o1,1,'h34567890+seq_o1*100,0,0,`SID(0,0,2,1));

	      sync_flag1 <= 1'b1;

	      //
	      // addr 2.1 to Slave1
	      for (seq_o1 = 20; seq_o1 < 30; seq_o1=seq_o1 + 1)
		enqueue_chdr_pkt_count(1,seq_o1,32+seq_o1,1,'h34567890+seq_o1*100,0,0,`SID(0,0,2,1));
	   end

	   begin
	      // Slave2 Recevier thread.
	      //
	      // addr 2.2 to Slave2
	      for (seq_o2 = 10; seq_o2 < 20; seq_o2=seq_o2 + 1)
		dequeue_chdr_pkt_count(2,seq_o2,32+seq_o2,1,'h23456789+seq_o2*100,0,0,`SID(0,0,2,2));
	      //
	      // addr 2.2 to Slave2
	      for (seq_o2 = 10; seq_o2 < 20; seq_o2=seq_o2 + 1)
		enqueue_chdr_pkt_count(2,seq_o2,32+seq_o2,1,'h23456789+seq_o2*100,0,0,`SID(0,0,2,2));
	   end

	   begin
	      // Slave3 Recevier thread.
	      //
	      // addr 2.3 to Slave3
	      for (seq_o3 = 0; seq_o3 < 10; seq_o3=seq_o3 + 1)
		dequeue_chdr_pkt_count(3,seq_o3,32+seq_o3,1,'h12345678+seq_o3*100,0,0,`SID(0,0,2,3));
	      //
	      // addr 2.3 to Slave3
	      for (seq_o3 = 0; seq_o3 < 10; seq_o3=seq_o3 + 1)
		enqueue_chdr_pkt_count(3,seq_o3,32+seq_o3,1,'h12345678+seq_o3*100,0,0,`SID(0,0,2,3));
	   end

	join

	repeat (1000) @(posedge clk);
	$finish;
     end // initial begin
