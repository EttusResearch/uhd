`timescale 1ns/1ps

module new_tx_control_tb();

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("new_tx_control_tb.vcd");
   initial $dumpvars(0,new_tx_control_tb);

   initial
     begin
	#1000 reset = 0;
	#30000;
	$finish;
     end
   
   reg [143:0]  tdata;
   reg 		tlast;
   wire 	tlast_int;
   reg 		tvalid = 1'b0;
   wire 	tready;
   
   reg [7:0] 	set_addr;
   reg [31:0] 	set_data;
   reg 		set_stb = 1'b0;
   
   reg [31:0] 	samp0, samp1;
   
   task send_packet;
      input [31:0] count;
      input [31:0] start_data;
      input [63:0] send_time;
      input [11:0] pkt_seqnum;
      input 	   eop;
      input 	   eob;
      input 	   send_at;
      input 	   odd;
      
      begin
	 // Send a packet
	 samp0 <= start_data;
	 samp1 <= start_data + 1;
	 @(posedge clk);
	 repeat (count-1)
	   begin
	      tdata <= { 1'b0,send_at,1'b0,1'b0,1'b0,pkt_seqnum,send_time,samp0,samp1 };
	      tvalid <= 1;
	      samp0 <= samp0 + 2;
	      samp1 <= samp1 + 2;
	      @(posedge clk);
	   end
	 
	 tdata <= { odd,send_at,1'b0,eob,eop,pkt_seqnum,send_time,samp0,samp1 };
	 @(posedge clk);
	 
	 tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet
   
   initial
     begin
	tvalid <= 1'b0;
	while(reset)
	  @(posedge clk);
	set_addr <= 8'd0;
	set_data <= 32'd2;
	set_stb <= 1'b1;
	@(posedge clk);
	set_stb <= 1'b0;
	
	// Single Packet burst, timed
	send_packet(3/*count*/,32'hA000_0000/*data*/,64'h100/*time*/,1/*SEQ*/,1/*EOP*/,1/*eob*/,1/*timed*/,0/*odd*/);

	// 2 packet burst, timed
	//send_packet(3/*count*/,32'hB000_0000/*data*/,64'h200/*time*/,2/*SEQ*/,1/*EOP*/,0/*eob*/,1/*timed*/,0/*odd*/);
	//send_packet(3/*count*/,32'hC000_0000/*data*/,64'h0/*time*/,3/*SEQ*/,1/*EOP*/,1/*eob*/,0/*timed*/,0/*odd*/);
	
	// single odd packet
	//send_packet(3/*count*/,32'h0A00_0000/*data*/,64'h300/*time*/,4/*SEQ*/,1/*EOP*/,1/*eob*/,1/*timed*/,1/*odd*/);

	// 2 packet burst, timed, odd
	//send_packet(3/*count*/,32'hD000_0000/*data*/,64'h400/*time*/,5/*SEQ*/,1/*EOP*/,0/*eob*/,1/*timed*/,1/*odd*/);
	//send_packet(3/*count*/,32'hE000_0000/*data*/,64'd0/*time*/,6/*SEQ*/,1/*EOP*/,1/*eob*/,0/*timed*/,1/*odd*/);

	// 2 packet burst, untimed, no eob set
	//send_packet(3/*count*/,32'hF000_0000/*data*/,64'd0/*time*/,7/*SEQ*/,1/*EOP*/,0/*eob*/,0/*timed*/,0/*odd*/);
	//send_packet(3/*count*/,32'h9000_0000/*data*/,64'd0/*time*/,8/*SEQ*/,1/*EOP*/,0/*eob*/,0/*timed*/,0/*odd*/);
	
	// single packet late
	send_packet(3/*count*/,32'hD000_0000/*data*/,64'h0/*time*/,4/*SEQ*/,1/*EOP*/,1/*eob*/,1/*timed*/,1/*odd*/);

     end

   reg [63:0] vita_time;
   wire [31:0] sample;
   wire [143:0] sample_tdata;
   wire 	sample_tready, sample_tvalid;
   wire [11:0] 	seqnum;
   wire [31:0] 	error_code;
   
   always @(posedge clk)
     if(reset)
       vita_time <= 0;
     else
       vita_time <= vita_time + 1;
   
   axi_fifo #(.WIDTH(144)) axi_fifo_short
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(tdata), .i_tvalid(tvalid), .i_tready(tready),
      .o_tdata(sample_tdata), .o_tvalid(sample_tvalid), .o_tready(sample_tready));

   new_tx_control new_tx_control
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),

      .vita_time(vita_time),
      .error(error), .ack(ack), .packet_consumed(consumed), .seqnum(seqnum), .error_code(error_code),

      .sample_tdata(sample_tdata), .sample_tvalid(sample_tvalid), .sample_tready(sample_tready),

      .sample(sample), .run(run), .strobe(strobe),
      .debug()
      );

   assign strobe = run;
   
   always @(posedge clk)
     begin
	if(strobe)
	  $display("%x\t%x", vita_time, sample);
	if(consumed) $display("CONSUMED %x", seqnum);
	if(ack) $display("ACK %x", seqnum);
	if(error) $display("ERROR %x\t%x", seqnum,error_code);
     end
   
endmodule // new_tx_control_tb
