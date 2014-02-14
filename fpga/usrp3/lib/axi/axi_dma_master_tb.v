module axi_dma_master_tb;



   wire aclk;                   // Global AXI clock
   wire aresetn;                // Global AXI reset, active low.
   //
   // AXI Write address channel
   //
   wire [0 : 0] m_axi_awid;     // Write address ID. This signal is the identification tag for the write address signals
   wire [31 : 0] m_axi_awaddr;  // Write address. The write address gives the address of the first transfer in a write burst
   wire [7 : 0] m_axi_awlen;    // Burst length. The burst length gives the exact number of transfers in a burst.
   wire [2 : 0] m_axi_awsize;   // Burst size. This signal indicates the size of each transfer in the burst. 
   wire [1 : 0] m_axi_awburst;  // Burst type. The burst type and the size information, determine how the address is calculated
   wire [0 : 0] m_axi_awlock;   // Lock type. Provides additional information about the atomic characteristics of the transfer.
   wire [3 : 0] m_axi_awcache;  // Memory type. This signal indicates how transactions are required to progress
   wire [2 : 0] m_axi_awprot;   // Protection type. This signal indicates the privilege and security level of the transaction
   wire [3 : 0] m_axi_awqos;    // Quality of Service, QoS. The QoS identifier sent for each write transaction
   wire [3 : 0] m_axi_awregion; // Region identifier. Permits a single physical interface on a slave to be re-used.
   wire [0 : 0] m_axi_awuser;   // User signal. Optional User-defined signal in the write address channel.
   wire m_axi_awvalid;          // Write address valid. This signal indicates that the channel is signaling valid write addr
   wire m_axi_awready;          // Write address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Write data channel.
   //
   wire [63 : 0] m_axi_wdata;   // Write data
   wire [7 : 0] m_axi_wstrb;    // Write strobes. This signal indicates which byte lanes hold valid data.
   wire m_axi_wlast;            // Write last. This signal indicates the last transfer in a write burst
   wire [0 : 0] m_axi_wuser;    // User signal. Optional User-defined signal in the write data channel.
   wire m_axi_wvalid;           // Write valid. This signal indicates that valid write data and strobes are available. 
   wire m_axi_wready;           // Write ready. This signal indicates that the slave can accept the write data.
   //
   // AXI Write response channel signals
   //
   wire [0 : 0] m_axi_bid;       // Response ID tag. This signal is the ID tag of the write response. 
   wire [1 : 0] m_axi_bresp;     // Write response. This signal indicates the status of the write transaction.
   wire [0 : 0] m_axi_buser;     // User signal. Optional User-defined signal in the write response channel.
   wire m_axi_bvalid;            // Write response valid. This signal indicates that the channel is signaling a valid response
   wire m_axi_bready;            // Response ready. This signal indicates that the master can accept a write response
   //
   // AXI Read address channel
   //
   wire [0 : 0] m_axi_arid;     // Read address ID. This signal is the identification tag for the read address group of signals
   wire [31 : 0] m_axi_araddr;  // Read address. The read address gives the address of the first transfer in a read burst
   wire [7 : 0] m_axi_arlen;    // Burst length. This signal indicates the exact number of transfers in a burst.
   wire [2 : 0] m_axi_arsize;   // Burst size. This signal indicates the size of each transfer in the burst.
   wire [1 : 0] m_axi_arburst;  // Burst type. The burst type and the size information determine how the address for each transfer
   wire [0 : 0] m_axi_arlock;   // Lock type. This signal provides additional information about the atomic characteristics
   wire [3 : 0] m_axi_arcache;  // Memory type. This signal indicates how transactions are required to progress 
   wire [2 : 0] m_axi_arprot;   // Protection type. This signal indicates the privilege and security level of the transaction
   wire [3 : 0] m_axi_arqos;    // Quality of Service, QoS. QoS identifier sent for each read transaction.
   wire [3 : 0] m_axi_arregion; // Region identifier. Permits a single physical interface on a slave to be re-used
   wire [0 : 0] m_axi_aruser;   // User signal. Optional User-defined signal in the read address channel.
   wire m_axi_arvalid;          // Read address valid. This signal indicates that the channel is signaling valid read addr
   wire m_axi_arready;          // Read address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Read data channel
   //
   wire [0 : 0] m_axi_rid;       // Read ID tag. This signal is the identification tag for the read data group of signals
   wire [63 : 0] m_axi_rdata;    // Read data.
   wire [1 : 0] m_axi_rresp;     // Read response. This signal indicates the status of the read transfer
   wire m_axi_rlast;             // Read last. This signal indicates the last transfer in a read burst.
   wire [0 : 0] m_axi_ruser;     // User signal. Optional User-defined signal in the read data channel.
   wire m_axi_rvalid;            // Read valid. This signal indicates that the channel is signaling the required read data. 
   wire m_axi_rready;            // Read ready. This signal indicates that the master can accept the read data and response
   //
   // DMA interface for Write transaction
   //
   wire [31:0] write_addr;       // Byte address for start of write transaction (should be 64bit alligned)
   wire [3:0] write_count;       // Count of 64 words to write.
   wire write_ctrl_valid;
   wire write_ctrl_ready;
   wire [63:0] write_data;
   wire write_data_valid;
   wire write_data_ready;
   //
   // DMA interface for Read
   //
   wire [31:0] read_addr;       // Byte address for start of read transaction (should be 64bit alligned)
   wire [3:0] read_count;       // Count of 64 words to read.
   wire read_ctrl_valid;
   wire read_ctrl_ready;
   wire [63:0] read_data;
   wire read_data_valid;
   wire read_data_ready;
   


   axi_dma_master  axi_dma_master_i1
     (
      .aclk(s_aclk), // input s_aclk
      .aresetn(s_aresetn), // input s_aresetn
      //
      .s_axi_awid(s_axi_awid), // input [0 : 0] s_axi_awid
      .s_axi_awaddr(s_axi_awaddr), // input [31 : 0] s_axi_awaddr
      .s_axi_awlen(s_axi_awlen), // input [7 : 0] s_axi_awlen
      .s_axi_awsize(s_axi_awsize), // input [2 : 0] s_axi_awsize
      .s_axi_awburst(s_axi_awburst), // input [1 : 0] s_axi_awburst
      .s_axi_awvalid(s_axi_awvalid), // input s_axi_awvalid
      .s_axi_awready(s_axi_awready), // output s_axi_awready
      //
      .s_axi_wdata(s_axi_wdata), // input [63 : 0] s_axi_wdata
      .s_axi_wstrb(s_axi_wstrb), // input [7 : 0] s_axi_wstrb
      .s_axi_wlast(s_axi_wlast), // input s_axi_wlast
      .s_axi_wvalid(s_axi_wvalid), // input s_axi_wvalid
      .s_axi_wready(s_axi_wready), // output s_axi_wready
      //
      .s_axi_bid(s_axi_bid), // output [0 : 0] s_axi_bid
      .s_axi_bresp(s_axi_bresp), // output [1 : 0] s_axi_bresp
      .s_axi_bvalid(s_axi_bvalid), // output s_axi_bvalid
      .s_axi_bready(s_axi_bready), // input s_axi_bready
      //
      .s_axi_arid(s_axi_arid), // input [0 : 0] s_axi_arid
      .s_axi_araddr(s_axi_araddr), // input [31 : 0] s_axi_araddr
      .s_axi_arlen(s_axi_arlen), // input [7 : 0] s_axi_arlen
      .s_axi_arsize(s_axi_arsize), // input [2 : 0] s_axi_arsize
      .s_axi_arburst(s_axi_arburst), // input [1 : 0] s_axi_arburst
      .s_axi_arvalid(s_axi_arvalid), // input s_axi_arvalid
      .s_axi_arready(s_axi_arready), // output s_axi_arready
      //
      .s_axi_rid(s_axi_rid), // output [0 : 0] s_axi_rid
      .s_axi_rdata(s_axi_rdata), // output [63 : 0] s_axi_rdata
      .s_axi_rresp(s_axi_rresp), // output [1 : 0] s_axi_rresp
      .s_axi_rlast(s_axi_rlast), // output s_axi_rlast
      .s_axi_rvalid(s_axi_rvalid), // output s_axi_rvalid
      .s_axi_rready(s_axi_rready) // input s_axi_rready
      );


   axi4_bram_1kx64 axi4_bram_1kx64_i1
     (
      .s_aclk(s_aclk), // input s_aclk
      .s_aresetn(s_aresetn), // input s_aresetn
      .s_axi_awid(s_axi_awid), // input [0 : 0] s_axi_awid
      .s_axi_awaddr(s_axi_awaddr), // input [31 : 0] s_axi_awaddr
      .s_axi_awlen(s_axi_awlen), // input [7 : 0] s_axi_awlen
      .s_axi_awsize(s_axi_awsize), // input [2 : 0] s_axi_awsize
      .s_axi_awburst(s_axi_awburst), // input [1 : 0] s_axi_awburst
      .s_axi_awvalid(s_axi_awvalid), // input s_axi_awvalid
      .s_axi_awready(s_axi_awready), // output s_axi_awready
      .s_axi_wdata(s_axi_wdata), // input [63 : 0] s_axi_wdata
      .s_axi_wstrb(s_axi_wstrb), // input [7 : 0] s_axi_wstrb
      .s_axi_wlast(s_axi_wlast), // input s_axi_wlast
      .s_axi_wvalid(s_axi_wvalid), // input s_axi_wvalid
      .s_axi_wready(s_axi_wready), // output s_axi_wready
      .s_axi_bid(s_axi_bid), // output [0 : 0] s_axi_bid
      .s_axi_bresp(s_axi_bresp), // output [1 : 0] s_axi_bresp
      .s_axi_bvalid(s_axi_bvalid), // output s_axi_bvalid
      .s_axi_bready(s_axi_bready), // input s_axi_bready
      .s_axi_arid(s_axi_arid), // input [0 : 0] s_axi_arid
      .s_axi_araddr(s_axi_araddr), // input [31 : 0] s_axi_araddr
      .s_axi_arlen(s_axi_arlen), // input [7 : 0] s_axi_arlen
      .s_axi_arsize(s_axi_arsize), // input [2 : 0] s_axi_arsize
      .s_axi_arburst(s_axi_arburst), // input [1 : 0] s_axi_arburst
      .s_axi_arvalid(s_axi_arvalid), // input s_axi_arvalid
      .s_axi_arready(s_axi_arready), // output s_axi_arready
      .s_axi_rid(s_axi_rid), // output [0 : 0] s_axi_rid
      .s_axi_rdata(s_axi_rdata), // output [63 : 0] s_axi_rdata
      .s_axi_rresp(s_axi_rresp), // output [1 : 0] s_axi_rresp
      .s_axi_rlast(s_axi_rlast), // output s_axi_rlast
      .s_axi_rvalid(s_axi_rvalid), // output s_axi_rvalid
      .s_axi_rready(s_axi_rready) // input s_axi_rready
      );

endmodule // axi_dma_master_tb
