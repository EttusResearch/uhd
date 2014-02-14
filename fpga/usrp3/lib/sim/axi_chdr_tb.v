
   //
   // CHDR friendly AXI stream input
   //
   reg [63:0] i_tdata;
   reg 	      i_tlast;
   reg 	      i_tvalid;
   wire       i_tready;
   //
   // CHDR friendly AXI Stream output
   //
   wire [63:0] o_tdata;
   wire        o_tlast;
   wire        o_tvalid;
   reg 	       o_tready;


//
// This task sends a burst of CHDR packets with populated headers.
// The burst payload contains a ramp of incrementing amplitude strating at 0.
//
   task send_ramp;
      input [31:0] burst_count;    // Number of CHDR packets in burst.
      input [31:0] len;            // Length of each CHDR packet in 32bit words.
      input [63:0] send_time;      // Optional 64 VITA time for first packet of burst.
      input [11:0] start_seqnum;   // Seeds initial seqnum of this burst.
      input 	   send_at;        // Set this to include VITA time on first poacket in burst.
      input [31:0] sid;            // SID value for all CHDR packets in burst

      reg [31:0] data;
      reg [11:0] seqno;
      
      begin
	 seqno = start_seqnum;
	 data = 0;
	 send_packet(len, data, send_time, seqno, (burst_count==1), send_at, sid);
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



//
// This task sends a burst of CHDR packets with populated headers
// Each packets payload is an incrementing count re-starting at the start value.
//
   task send_burst;
      input [31:0] burst_count;    // Number of CHDR packets in burst.
      input [31:0] len;            // Length of each CHDR packet in 32bit words.
      input [31:0] start_data;     // Seed initial sample magnitude.
      input [63:0] send_time;      // Optional 64 VITA time for first packet of burst.
      input [11:0] start_seqnum;   // Seeds initial seqnum of this burst.
      input 	   send_at;        // Set this to include VITA time on first packet in burst.
      input [31:0] sid;            // SID value for all CHDR packets in burst
   
      reg [11:0] seqno;

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

//
// Sends a single CHDR packet. Has valid CHDR headers and incrementing sample payload.
// Alter this with care, many other tasks depend on this task.
//
   task send_packet;
      input [31:0] len;
      input [31:0] start_data;
      input [63:0] send_time;
      input [11:0] pkt_seqnum;
      input 	   eob;
      input 	   send_at;
      input [31:0] sid;

      reg [31:0] samp0, samp1;

      
      begin
	 // Send a packet
	 samp0 <= start_data;
	 samp1 <= start_data + 1;
	 @(posedge clk);

	 i_tlast <= 0;
	 i_tdata <= { 1'b0, 1'b0 /*trl*/, send_at, eob, pkt_seqnum, len[15:0]+16'd2+send_at+send_at, sid };
	 i_tvalid <= 1;
	 @(posedge clk)
	 if(send_at)
	   begin
	      i_tdata <= send_time;
	      @(posedge clk);
	   end

	 repeat (len[31:1]+len[0]-1)
	   begin
	      i_tdata <= {samp0,samp1};
	      samp0 <= samp0 + 2;
	      samp1 <= samp1 + 2;
	      @(posedge clk);
	   end
	 
	 i_tdata <= {samp0,samp1};
	 i_tlast <= 1'b1;
	 @(posedge clk);
	 i_tvalid <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet

//
// These 2 tasks stuff an incrementing count and then check for a match 
// on Egress to test CHDR blocks for transparaent data pass through.
// CHDR fields are not inteligently populated by these tasks.
// 
   task send_raw_packet;
      input [31:0] len;

      reg [63:0] data;

      begin
	 data = 0;
	 @(posedge clk);
	 repeat (len-1) begin
	    i_tlast <= 0;
	    i_tdata <= data;
	    i_tvalid <= 1;
	    @(posedge clk);
	    while (~i_tready) @(posedge clk);
	    data = data + 1;
	 end
	 i_tlast <= 1;
	 i_tdata <= data;
	 i_tvalid <= 1;
	 @(posedge clk);
	 while (~i_tready) @(posedge clk);
	 i_tvalid <= 0;
	 @(posedge clk);
      end
   endtask // send_raw_packet
   
   task receive_raw_packet;
      input [31:0] len;
      output fail;
      reg [63:0] data;

      begin
	 data = 0;
	 fail = 0;
	 
	 @(posedge clk);
	 repeat (len-1) begin
	    o_tready = 1;
	    @(posedge clk);
	    while (~o_tvalid) @(posedge clk);
	    //$display("Data = %d, o_tdata = %d, o_tlast = %d",data,o_tdata,o_tlast);
	    
	    fail = fail || (data != o_tdata);
	    fail = fail || (o_tlast == 1);
	    data = data + 1;
	    
	 end
	 o_tready = 1;
	 @(posedge clk);
	 while (~o_tvalid) @(posedge clk);
	 //$display("Data = %d, o_tdata = %d, o_tlast = %d",data,o_tdata,o_tlast);
	 fail = fail || (data != o_tdata);
	 fail = fail || (o_tlast == 0);
	 o_tready = 0;
	 @(posedge clk);
	 if (fail) $display("receive_raw_packet size %d failed",len);
	 
      end
   endtask // receive_raw_packet
   
      