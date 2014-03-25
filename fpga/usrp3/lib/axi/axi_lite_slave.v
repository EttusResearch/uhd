module axi_lite_slave
  (
   input aclk,                    // Global AXI clock
   input aresetn,                 // Global AXI reset, active low.
   //
   // AXI  Write address channel
   //
   input [31 : 0] m_axi_awaddr,  // Write address. The write address gives the address of the first transfer in a write burst 
   input [2 : 0] m_axi_awprot,   // Protection type. This signal indicates the privilege and security level of the transaction
   input m_axi_awvalid,          // Write address valid. This signal indicates that the channel is signaling valid write addr
   output m_axi_awready,         // Write address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Write data channel.
   //
   input [31 : 0] m_axi_wdata,   // Write data
   input [3 : 0] m_axi_wstrb,    // Write strobes. This signal indicates which byte lanes hold valid data.
   input m_axi_wvalid,           // Write valid. This signal indicates that valid write data and strobes are available. 
   output m_axi_wready,          // Write ready. This signal indicates that the slave can accept the write data.
   //
   // AXI Write response channel signals
   //
   output [1 : 0] m_axi_bresp,   // Write response. This signal indicates the status of the write transaction.
   output m_axi_bvalid,          // Write response valid. This signal indicates that the channel is signaling a valid response
   input m_axi_bready,           // Response ready. This signal indicates that the master can accept a write response
   //
   // AXI Read address channel
   //
   input [31 : 0] m_axi_araddr,  // Read address. The read address gives the address of the first transfer in a read burst
   input [2 : 0] m_axi_arprot,   // Protection type. This signal indicates the privilege and security level of the transaction
   input m_axi_arvalid,          // Read address valid. This signal indicates that the channel is signaling valid read addr
   output m_axi_arready,         // Read address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Read data channel
   //
   output [31 : 0] m_axi_rdata,  // Read data.
   output [1 : 0] m_axi_rresp,   // Read response. This signal indicates the status of the read transfer
   output m_axi_rvalid,          // Read valid. This signal indicates that the channel is signaling the required read data. 
   input m_axi_rready,           // Read ready. This signal indicates that the master can accept the read data and response
   //
   //
   //
   )