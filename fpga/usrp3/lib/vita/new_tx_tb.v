`timescale 1ns/1ps

module new_tx_tb();
`ifdef ISIM
`else //iverilog implied.
   xlnx_glbl glbl (.GSR(),.GTS());
`endif
   

   localparam SR_TX_DSP        = 8;
   localparam SR_TX_RESPONDER  = 16;
   localparam SR_TX_CTRL       = 24;
      
   localparam SR_CYCLES =    SR_TX_RESPONDER + 0;
   localparam SR_PACKETS =   SR_TX_RESPONDER + 1;
   
   localparam SR_PHASE_INC =    SR_TX_DSP + 0;
   localparam SR_SCALE_FACTOR = SR_TX_DSP + 1;
   localparam SR_INTERP =       SR_TX_DSP + 2;

   localparam SR_ERROR_POLICY = SR_TX_CTRL + 0;
   

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("new_tx_tb.vcd");
   initial $dumpvars(0,new_tx_tb);
   wire run, strobe;
   
   initial
     begin
	#1000 reset = 0;
	#30000;
	$finish;
     end
   
   reg [63:0]  tdata;
   reg 	       tlast;
   reg 	       tvalid = 1'b0;
   wire        tready;

   wire [63:0] i_tdata;
   wire        i_tlast, i_tvalid, i_tready;
   
   reg [7:0]   set_addr;
   reg [31:0]  set_data;
   reg 	       set_stb = 1'b0;
   
   reg [63:0]  vita_time;
   wire [31:0] sample;
   
   wire [175:0] sample_tdata;
   wire 	sample_tready, sample_tvalid;
   
   wire [11:0] 	seqnum;
   wire [63:0] 	error_code;
   wire [31:0] 	sid;
   
   reg [31:0] 	samp0, samp1;

   reg [11:0] 	seqno;

   wire 	ack_or_error, packet_consumed;
   
   
   //
   // Task Libaray
   //
   task write_setting_bus;
      input [7:0] address;
      input [31:0] data;

      begin

	 @(negedge clk);
	 set_stb = 1'b0;
	 set_addr = 8'h0;
	 set_data = 32'h0;
	 @(negedge clk);
	 set_stb = 1'b1;
	 set_addr = address;
	 set_data = data;
	 @(negedge clk);
	 set_stb = 1'b0;
	 set_addr = 8'h0;
	 set_data = 32'h0;

      end
   endtask // write_setting_bus


   task send_ramp;
      input [31:0] burst_count;
      input [31:0] len;
      input [31:0] sid;

      reg [31:0] data;

      begin
	 seqno = 0;
	 data = 0;
	 send_packet(len, data, 0, seqno, (burst_count==1), 0, sid);
	 seqno = seqno + 1;
	 data <= data + len;

	 if(burst_count > 2)
	   repeat (burst_count - 2)
	     begin
		send_packet(len, data, 64'h0, seqno, 0, 0, sid);
		seqno = seqno + 1;
		data <= data + len;
	     end
	 if(burst_count > 1)
	   send_packet(len, data, 64'h0, seqno, 1, 0, sid);
      end
   endtask // send_ramp


   task send_dc;
      input [31:0] burst_count;
      input [31:0] len;
      input [31:0] sid;

      reg [31:0] data;

      begin
	 seqno = 0;
	 data = 1 << 14;
	 send_packet(len, data, 0, seqno, (burst_count==1), 0, sid);
	 seqno = seqno + 1;


	 if(burst_count > 2)
	   repeat (burst_count - 2)
	     begin
		send_packet(len, data, 64'h0, seqno, 0, 0, sid);
		seqno = seqno + 1;

	     end
	 if(burst_count > 1)
	   send_packet(len, data, 64'h0, seqno, 1, 0, sid);
      end
   endtask // send_ramp

   task send_burst;
      input [31:0] burst_count;
      input [31:0] len;
      input [31:0] start_data;
      input [63:0] send_time;
      input [11:0] start_seqnum;
      input 	   send_at;
      input [31:0] sid;
      
      begin
	 seqno = start_seqnum;
	 send_packet(len, {seqno,start_data[15:0]}, send_time, seqno, (burst_count==1), send_at, sid);
	 seqno = seqno + 1;
	 
	 if(burst_count > 2)
	   repeat (burst_count - 2)
	     begin
		send_packet(len, {seqno,start_data[15:0]}, 64'h0, seqno, 0, 0, sid);
		seqno = seqno + 1;
	     end
	 if(burst_count > 1)
	   send_packet(len, {seqno,start_data[15:0]}, 64'h0, seqno, 1, 0, sid);
      end
   endtask // send_burst

   task send_burst_with_seqid_error;
      input [31:0] burst_count;
      input [31:0] len;
      input [31:0] start_data;
      input [63:0] send_time;
      input [11:0] start_seqnum;
      input 	   send_at;
      input [31:0] sid;

     
      begin
	 seqno = start_seqnum;
	 send_packet(len, {seqno,start_data[15:0]}, send_time, seqno, (burst_count==1), send_at, sid);
	 seqno = seqno + 1;
	 
	 if(burst_count > 2)
	   repeat (burst_count - 2)
	     begin
		// Add a SeqID error in the middle of the packet burst
		if (seqno == (start_seqnum + burst_count/2))
		  seqno = seqno + 1;
		send_packet(len, {seqno,start_data[15:0]}, 64'h0, seqno, 0, 0, sid);
		seqno = seqno + 1;
	     end
	 if(burst_count > 1)
	   send_packet(len, {seqno,start_data[15:0]}, 64'h0, seqno, 1, 0, sid);
      end
   endtask // send_burst
   
   task send_packet;
      input [31:0] len;
      input [31:0] start_data;
      input [63:0] send_time;
      input [11:0] pkt_seqnum;
      input 	   eob;
      input 	   send_at;
      input [31:0] sid;
      
      begin
	 // Send a packet
	 samp0 <= start_data;
	 samp1 <= start_data + 1;
	 @(posedge clk);

	 tlast <= 0;
	 tdata <= { 1'b0, 1'b0 /*trl*/, send_at, eob, pkt_seqnum, len[15:0]+16'd2+send_at+send_at, sid };
	 tvalid <= 1;
	 @(posedge clk)
	 if(send_at)
	   begin
	      tdata <= send_time;
	      @(posedge clk);
	   end

	 repeat (len[31:1]+len[0]-1)
	   begin
	      tdata <= {samp0,samp1};
	      samp0 <= samp0 + 2;
	      samp1 <= samp1 + 2;
	      @(posedge clk);
	   end
	 
	 tdata <= {samp0,samp1};
	 tlast <= 1'b1;
	 @(posedge clk);
	 tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet

`ifdef SIM_SCRIPT
   // Load simulation script from local directory
`include "simulation_script.v"
   
`else
   initial
     begin
	tvalid <= 1'b0;
	while(reset)
	  @(posedge clk);
	write_setting_bus(SR_ERROR_POLICY,32'h4);
	write_setting_bus(SR_PACKETS,32'h8000_0002);

	write_setting_bus(SR_INTERP,32'h1);
	
	send_burst(2/*count*/,5/*len*/,32'hA000_0000/*start*/,64'h100/*time*/,12'h000/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);
	//send_burst(3/*count*/,6/*len*/,32'hB000_0000/*start*/,64'h0/*time*/,12'h004/*seqnum*/,0/*sendat*/, 32'hDEADBEEF/*sid*/);

	//Intra burst seq_id error
	send_burst_with_seqid_error(8/*count*/,10/*len*/,32'hC000_0000/*start*/,64'h200/*time*/,12'h002/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

	
	// Inter burst sequence error
	send_burst(2/*count*/,10/*len*/,32'hC000_0000/*start*/,64'h300/*time*/,12'h015/*seqnum*/,1/*sendat*/, 32'hDEADBEEF/*sid*/);

	// Single Packet burst, timed
	//send_packet(3/*count*/,32'hA000_0000/*data*/,64'h100/*time*/,1/*SEQ*/,1/*EOP*/,1/*eob*/,1/*timed*/,0/*odd*/);

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
	//send_packet(3/*count*/,32'hD000_0000/*data*/,64'h0/*time*/,4/*SEQ*/,1/*EOP*/,1/*eob*/,1/*timed*/,1/*odd*/);

     end
`endif // !`ifdef SIM_SCRIPT
   
   always @(posedge clk)
     if(reset)
       vita_time <= 0;
     else
       vita_time <= vita_time + 1;
   
   axi_fifo #(.WIDTH(65)) axi_fifo_short
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata({tlast,tdata}), .i_tvalid(tvalid), .i_tready(tready),
      .o_tdata({i_tlast,i_tdata}), .o_tvalid(i_tvalid), .o_tready(i_tready));

   new_tx_deframer new_tx_deframer
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .sample_tdata(sample_tdata), .sample_tvalid(sample_tvalid), .sample_tready(sample_tready));
   
   new_tx_control #(.BASE(SR_TX_CTRL)) new_tx_control
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),

      .vita_time(vita_time),
      .ack_or_error(ack_or_error), .packet_consumed(packet_consumed),
      .seqnum(seqnum), .error_code(error_code), .sid(sid),

      .sample_tdata(sample_tdata), .sample_tvalid(sample_tvalid), .sample_tready(sample_tready),

      .sample(sample), .run(run), .strobe(strobe),
      .debug()
      );

   wire [63:0] o_tdata;
   wire        o_tlast, o_tvalid, o_tready;
   assign o_tready = 1;
   
   tx_responder #(.BASE(SR_TX_RESPONDER)) tx_responder
     (.clk(clk), .reset(reset), .clear(1'b0),
      .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .ack_or_error(ack_or_error), .packet_consumed(packet_consumed),
      .seqnum(seqnum), .error_code(error_code), .sid(sid),
      .vita_time(vita_time),
      .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));
   
   always @(posedge clk)
     if(o_tvalid & o_tready)
       $display("\t\t\t\t\tRESP %x\t%x",o_tdata,o_tlast);
   
   always @(posedge clk)
     if(~reset)
       begin
	  if(strobe & run)
	    $display("%x\t%x", vita_time, sample);
	  if(strobe & ~run) $display("Spurious Strobe at time %x",vita_time);
	  if(packet_consumed) $display("CONSUMED %x", seqnum);
	  if(ack_or_error)
	    if(error_code[63:32] == 1)
	      $display("ACK -- SEQNUM %x", error_code[31:0]);
	    else
	      $display("ERROR -- SEQNUM %x ERRCODE %x", error_code[31:0],error_code[63:32]);
       end
   
   wire [23:0] tx_fe_i, tx_fe_q;

   duc_chain #(.BASE(SR_TX_DSP), .DSPNO(0), .WIDTH(24)) duc_chain
     (.clk(clk), .rst(reset), .clr(1'b0),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .tx_fe_i(tx_fe_i),.tx_fe_q(tx_fe_q),
      .sample(sample), .run(run), .strobe(strobe),
      .debug() );
   
endmodule // new_tx_tb
