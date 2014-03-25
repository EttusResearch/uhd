module axi_dram_fifo_tb;



   reg clk;                    // Global AXI clock
   reg reset;                  // Global reset, active high.
   reg clear;
   wire aresetn;               // Global AXI reset, active low.
   //
   // AXI Write address channel
   //
   wire [0 : 0] axi_awid;     // Write address ID. This signal is the identification tag for the write address signals
   wire [31 : 0] axi_awaddr;  // Write address. The write address gives the address of the first transfer in a write burst
   wire [7 : 0] axi_awlen;    // Burst length. The burst length gives the exact number of transfers in a burst.
   wire [2 : 0] axi_awsize;   // Burst size. This signal indicates the size of each transfer in the burst. 
   wire [1 : 0] axi_awburst;  // Burst type. The burst type and the size information, determine how the address is calculated
   wire [0 : 0] axi_awlock;   // Lock type. Provides additional information about the atomic characteristics of the transfer.
   wire [3 : 0] axi_awcache;  // Memory type. This signal indicates how transactions are required to progress
   wire [2 : 0] axi_awprot;   // Protection type. This signal indicates the privilege and security level of the transaction
   wire [3 : 0] axi_awqos;    // Quality of Service, QoS. The QoS identifier sent for each write transaction
   wire [3 : 0] axi_awregion; // Region identifier. Permits a single physical interface on a slave to be re-used.
   wire [0 : 0] axi_awuser;   // User signal. Optional User-defined signal in the write address channel.
   wire axi_awvalid;          // Write address valid. This signal indicates that the channel is signaling valid write addr
   wire axi_awready;          // Write address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Write data channel.
   //
   wire [63 : 0] axi_wdata;   // Write data
   wire [7 : 0] axi_wstrb;    // Write strobes. This signal indicates which byte lanes hold valid data.
   wire axi_wlast;            // Write last. This signal indicates the last transfer in a write burst
   wire [0 : 0] axi_wuser;    // User signal. Optional User-defined signal in the write data channel.
   wire axi_wvalid;           // Write valid. This signal indicates that valid write data and strobes are available. 
   wire axi_wready;           // Write ready. This signal indicates that the slave can accept the write data.
   //
   // AXI Write response channel signals
   //
   wire [0 : 0] axi_bid;       // Response ID tag. This signal is the ID tag of the write response. 
   wire [1 : 0] axi_bresp;     // Write response. This signal indicates the status of the write transaction.
   wire [0 : 0] axi_buser;     // User signal. Optional User-defined signal in the write response channel.
   wire axi_bvalid;            // Write response valid. This signal indicates that the channel is signaling a valid response
   wire axi_bready;            // Response ready. This signal indicates that the master can accept a write response
   //
   // AXI Read address channel
   //
   wire [0 : 0] axi_arid;     // Read address ID. This signal is the identification tag for the read address group of signals
   wire [31 : 0] axi_araddr;  // Read address. The read address gives the address of the first transfer in a read burst
   wire [7 : 0] axi_arlen;    // Burst length. This signal indicates the exact number of transfers in a burst.
   wire [2 : 0] axi_arsize;   // Burst size. This signal indicates the size of each transfer in the burst.
   wire [1 : 0] axi_arburst;  // Burst type. The burst type and the size information determine how the address for each transfer
   wire [0 : 0] axi_arlock;   // Lock type. This signal provides additional information about the atomic characteristics
   wire [3 : 0] axi_arcache;  // Memory type. This signal indicates how transactions are required to progress 
   wire [2 : 0] axi_arprot;   // Protection type. This signal indicates the privilege and security level of the transaction
   wire [3 : 0] axi_arqos;    // Quality of Service, QoS. QoS identifier sent for each read transaction.
   wire [3 : 0] axi_arregion; // Region identifier. Permits a single physical interface on a slave to be re-used
   wire [0 : 0] axi_aruser;   // User signal. Optional User-defined signal in the read address channel.
   wire axi_arvalid;          // Read address valid. This signal indicates that the channel is signaling valid read addr
   wire axi_arready;          // Read address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Read data channel
   //
   wire [0 : 0] axi_rid;       // Read ID tag. This signal is the identification tag for the read data group of signals
   wire [63 : 0] axi_rdata;    // Read data.
   wire [1 : 0] axi_rresp;     // Read response. This signal indicates the status of the read transfer
   wire axi_rlast;             // Read last. This signal indicates the last transfer in a read burst.
   wire [0 : 0] axi_ruser;     // User signal. Optional User-defined signal in the read data channel.
   wire axi_rvalid;            // Read valid. This signal indicates that the channel is signaling the required read data. 
   wire axi_rready;            // Read ready. This signal indicates that the master can accept the read data and response
 
   //
   // CHDR friendly AXI stream input
   //
   wire [63:0] i_tdata;
   wire        i_tlast;
   wire        i_tvalid;
   wire       i_tready;
   //
   // CHDR friendly AXI Stream output
   //
   wire [63:0] o_tdata;
   wire        o_tlast;
   wire        o_tvalid;
   wire        o_tready;

   //
   // These registers optionaly used
   // to drive nets through procedural assignments in test bench.
   // These drivers default to tri-stated.
   //
   
   reg [63:0] i_tdata_r;
   reg        i_tlast_r;
   reg 	      i_tvalid_r;
   reg 	      o_tready_r;

   assign i_tdata = i_tdata_r;
   assign i_tlast = i_tlast_r;
   assign i_tvalid = i_tvalid_r;
   assign o_tready = o_tready_r;

   initial 
     begin
	i_tdata_r <= 64'hzzzz_zzzz_zzzz_zzzz;
	i_tlast_r <= 1'bz;
	i_tvalid_r <= 1'bz;
	o_tready_r <= 1'bz;
     end
      
   
   
   axi_dram_fifo  
     #(.SIZE(13))
       axi_dram_fifo_i1
	 (
	  .bus_clk(clk), // input s_aclk
	  .bus_reset(reset), // input s_aresetn
	  .clear(clear),
	  .dram_clk(clk), // input s_aclk
	  .dram_reset(reset), // input s_aresetn
	  // Write control
	  .m_axi_awid(axi_awid), // input [0 : 0] s_axi_awid
	  .m_axi_awaddr(axi_awaddr), // input [31 : 0] s_axi_awaddr
	  .m_axi_awlen(axi_awlen), // input [7 : 0] s_axi_awlen
	  .m_axi_awsize(axi_awsize), // input [2 : 0] s_axi_awsize
	  .m_axi_awburst(axi_awburst), // input [1 : 0] s_axi_awburst
	  .m_axi_awvalid(axi_awvalid), // input s_axi_awvalid
	  .m_axi_awready(axi_awready), // output s_axi_awready
	  .m_axi_awlock(),
	  .m_axi_awcache(),
	  .m_axi_awprot(),
	  .m_axi_awqos(),
	  .m_axi_awregion(),
	  .m_axi_awuser(),
	  // Write Data
	  .m_axi_wdata(axi_wdata), // input [63 : 0] s_axi_wdata
	  .m_axi_wstrb(axi_wstrb), // input [7 : 0] s_axi_wstrb
	  .m_axi_wlast(axi_wlast), // input s_axi_wlast
	  .m_axi_wvalid(axi_wvalid), // input s_axi_wvalid
	  .m_axi_wready(axi_wready), // output s_axi_wready
	  .m_axi_wuser(),
	  // Write Response
	  .m_axi_bid(axi_bid), // output [0 : 0] s_axi_bid
	  .m_axi_bresp(axi_bresp), // output [1 : 0] s_axi_bresp
	  .m_axi_bvalid(axi_bvalid), // output s_axi_bvalid
	  .m_axi_bready(axi_bready), // input s_axi_bready
	  .m_axi_buser(),
	  // Read Control
	  .m_axi_arid(axi_arid), // input [0 : 0] s_axi_arid
	  .m_axi_araddr(axi_araddr), // input [31 : 0] s_axi_araddr
	  .m_axi_arlen(axi_arlen), // input [7 : 0] s_axi_arlen
	  .m_axi_arsize(axi_arsize), // input [2 : 0] s_axi_arsize
	  .m_axi_arburst(axi_arburst), // input [1 : 0] s_axi_arburst
	  .m_axi_arvalid(axi_arvalid), // input s_axi_arvalid
	  .m_axi_arready(axi_arready), // output s_axi_arready
	  .m_axi_arlock(),
	  .m_axi_arcache(),
	  .m_axi_arprot(),
	  .m_axi_arqos(),
	  .m_axi_arregion(),
	  .m_axi_aruser(),
	  // Read Data
	  .m_axi_rid(axi_rid), // output [0 : 0] s_axi_rid
	  .m_axi_rdata(axi_rdata), // output [63 : 0] s_axi_rdata
	  .m_axi_rresp(axi_rresp), // output [1 : 0] s_axi_rresp
	  .m_axi_rlast(axi_rlast), // output s_axi_rlast
	  .m_axi_rvalid(axi_rvalid), // output s_axi_rvalid
	  .m_axi_rready(axi_rready), // input s_axi_rready
	  .m_axi_ruser(),
	  // CHDR in
	  .i_tdata(i_tdata),
	  .i_tlast(i_tlast),
	  .i_tvalid(i_tvalid),
	  .i_tready(i_tready),
	  // CHDR out
	  .o_tdata(o_tdata),
	  .o_tlast(o_tlast),
	  .o_tvalid(o_tvalid),
	  .o_tready(o_tready),
	  //
	  .supress_threshold(16'h0),
	  .supress_enable(1'b0)      
	  );


   axi4_bram_1kx64 axi4_bram_1kx64_i1
     (
      .s_aclk(clk), // input s_aclk
      .s_aresetn(aresetn), // input s_aresetn
      .s_axi_awid(axi_awid), // input [0 : 0] s_axi_awid
      .s_axi_awaddr(axi_awaddr), // input [31 : 0] s_axi_awaddr
      .s_axi_awlen(axi_awlen), // input [7 : 0] s_axi_awlen
      .s_axi_awsize(axi_awsize), // input [2 : 0] s_axi_awsize
      .s_axi_awburst(axi_awburst), // input [1 : 0] s_axi_awburst
      .s_axi_awvalid(axi_awvalid), // input s_axi_awvalid
      .s_axi_awready(axi_awready), // output s_axi_awready
      .s_axi_wdata(axi_wdata), // input [63 : 0] s_axi_wdata
      .s_axi_wstrb(axi_wstrb), // input [7 : 0] s_axi_wstrb
      .s_axi_wlast(axi_wlast), // input s_axi_wlast
      .s_axi_wvalid(axi_wvalid), // input s_axi_wvalid
      .s_axi_wready(axi_wready), // output s_axi_wready
      .s_axi_bid(axi_bid), // output [0 : 0] s_axi_bid
      .s_axi_bresp(axi_bresp), // output [1 : 0] s_axi_bresp
      .s_axi_bvalid(axi_bvalid), // output s_axi_bvalid
      .s_axi_bready(axi_bready), // input s_axi_bready
      .s_axi_arid(axi_arid), // input [0 : 0] s_axi_arid
      .s_axi_araddr(axi_araddr), // input [31 : 0] s_axi_araddr
      .s_axi_arlen(axi_arlen), // input [7 : 0] s_axi_arlen
      .s_axi_arsize(axi_arsize), // input [2 : 0] s_axi_arsize
      .s_axi_arburst(axi_arburst), // input [1 : 0] s_axi_arburst
      .s_axi_arvalid(axi_arvalid), // input s_axi_arvalid
      .s_axi_arready(axi_arready), // output s_axi_arready
      .s_axi_rid(axi_rid), // output [0 : 0] s_axi_rid
      .s_axi_rdata(axi_rdata), // output [63 : 0] s_axi_rdata
      .s_axi_rresp(axi_rresp), // output [1 : 0] s_axi_rresp
      .s_axi_rlast(axi_rlast), // output s_axi_rlast
      .s_axi_rvalid(axi_rvalid), // output s_axi_rvalid
      .s_axi_rready(axi_rready) // input s_axi_rready
      );


   //
   //
   //


   task send_ramp;
      input [31:0] burst_count;
      input [31:0] len;
      input [31:0] sid;

      reg [31:0] data;
      reg [11:0] seqno;
      
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
      reg [11:0] seqno;

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

	 i_tlast_r <= 0;
	 i_tdata_r <= { 1'b0, 1'b0 /*trl*/, send_at, eob, pkt_seqnum, len[15:0]+16'd2+send_at+send_at, sid };
	 i_tvalid_r <= 1;
	 @(posedge clk)
	 if(send_at)
	   begin
	      i_tdata_r <= send_time;
	      @(posedge clk);
	   end

	 repeat (len[31:1]+len[0]-1)
	   begin
	      i_tdata_r <= {samp0,samp1};
	      samp0 <= samp0 + 2;
	      samp1 <= samp1 + 2;
	      @(posedge clk);
	   end
	 
	 i_tdata_r <= {samp0,samp1};
	 i_tlast_r <= 1'b1;
	 @(posedge clk);
	 i_tvalid_r <= 0;	 
	 @(posedge clk);
      end
   endtask // send_packet

   task send_raw_packet;
      input [31:0] len;

      reg [63:0] data;

      begin
	 data = 0;
	 @(posedge clk);
	 repeat (len-1) begin
	    i_tlast_r <= 0;
	    i_tdata_r <= data;
	    i_tvalid_r <= 1;
	    @(posedge clk);
	    while (~i_tready) @(posedge clk);
	    data = data + 1;
	 end
	 i_tlast_r <= 1;
	 i_tdata_r <= data;
	 i_tvalid_r <= 1;
	 @(posedge clk);
	 while (~i_tready) @(posedge clk);
	 i_tvalid_r <= 0;
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
	    o_tready_r <= 1;
	    @(posedge clk);
	    while (~o_tvalid) @(posedge clk);
	    //$display("Data = %d, o_tdata = %d, o_tlast = %d",data,o_tdata,o_tlast);
	    
	    fail = fail || (data !== o_tdata);
	    fail = fail || ~(o_tlast === 0);
	    data = data + 1;
	    
	 end
	 o_tready_r <= 1;
	 @(posedge clk);
	 while (~o_tvalid) @(posedge clk);
	 //$display("Data = %d, o_tdata = %d, o_tlast = %d",data,o_tdata,o_tlast);
	 fail = fail || (data !== o_tdata);
	 fail = fail || ~(o_tlast === 1);
	 o_tready_r <= 0;
	 @(posedge clk);
	 if (fail) $display("receive_raw_packet size %d failed",len);
	 
      end
   endtask // receive_raw_packet
   
      

   assign aresetn = ~reset;
   
   //
   // Bring in a simulation script here
   //
   `include "simulation_script.v"

endmodule // axi_dram_fifo_tb
