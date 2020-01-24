//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`include "axi_defs.v"

`define DEBUG if (0)

module axi_dma_master #(
   parameter AWIDTH = 32,
   parameter DWIDTH = 64
) (
   input aclk,                    // Global AXI clock
   input areset,                 // Global AXI reset
   //
   // AXI Write address channel
   //
   output [0 : 0] m_axi_awid,     // Write address ID. This signal is the identification tag for the write address signals
   output reg [AWIDTH-1 : 0] m_axi_awaddr,  // Write address. The write address gives the address of the first transfer in a write burst
   output reg [7 : 0] m_axi_awlen,    // Burst length. The burst length gives the exact number of transfers in a burst.
   output [2 : 0] m_axi_awsize,   // Burst size. This signal indicates the size of each transfer in the burst. 
   output [1 : 0] m_axi_awburst,  // Burst type. The burst type and the size information, determine how the address is calculated
   output [0 : 0] m_axi_awlock,   // Lock type. Provides additional information about the atomic characteristics of the transfer.
   output [3 : 0] m_axi_awcache,  // Memory type. This signal indicates how transactions are required to progress
   output [2 : 0] m_axi_awprot,   // Protection type. This signal indicates the privilege and security level of the transaction
   output [3 : 0] m_axi_awqos,    // Quality of Service, QoS. The QoS identifier sent for each write transaction
   output [3 : 0] m_axi_awregion, // Region identifier. Permits a single physical interface on a slave to be re-used.
   output [0 : 0] m_axi_awuser,   // User signal. Optional User-defined signal in the write address channel.
   output reg m_axi_awvalid,      // Write address valid. This signal indicates that the channel is signaling valid write addr
   input m_axi_awready,           // Write address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Write data channel.
   //
   output [DWIDTH-1 : 0] m_axi_wdata,   // Write data
   output [DWIDTH/8-1 : 0] m_axi_wstrb,    // Write strobes. This signal indicates which byte lanes hold valid data.
   output reg m_axi_wlast,        // Write last. This signal indicates the last transfer in a write burst
   output m_axi_wuser,            // User signal. Optional User-defined signal in the write data channel.
   output m_axi_wvalid,           // Write valid. This signal indicates that valid write data and strobes are available. 
   input m_axi_wready,            // Write ready. This signal indicates that the slave can accept the write data.
   //
   // AXI Write response channel signals
   //
   input [0 : 0] m_axi_bid,       // Response ID tag. This signal is the ID tag of the write response. 
   input [1 : 0] m_axi_bresp,     // Write response. This signal indicates the status of the write transaction.
   input [0 : 0] m_axi_buser,     // User signal. Optional User-defined signal in the write response channel.
   input m_axi_bvalid,            // Write response valid. This signal indicates that the channel is signaling a valid response
   output reg m_axi_bready,       // Response ready. This signal indicates that the master can accept a write response
   //
   // AXI Read address channel
   //
   output [0 : 0] m_axi_arid,     // Read address ID. This signal is the identification tag for the read address group of signals
   output reg [AWIDTH-1 : 0] m_axi_araddr,  // Read address. The read address gives the address of the first transfer in a read burst
   output reg [7 : 0] m_axi_arlen,    // Burst length. This signal indicates the exact number of transfers in a burst.
   output [2 : 0] m_axi_arsize,   // Burst size. This signal indicates the size of each transfer in the burst.
   output [1 : 0] m_axi_arburst,  // Burst type. The burst type and the size information determine how the address for each transfer
   output [0 : 0] m_axi_arlock,   // Lock type. This signal provides additional information about the atomic characteristics
   output [3 : 0] m_axi_arcache,  // Memory type. This signal indicates how transactions are required to progress 
   output [2 : 0] m_axi_arprot,   // Protection type. This signal indicates the privilege and security level of the transaction
   output [3 : 0] m_axi_arqos,    // Quality of Service, QoS. QoS identifier sent for each read transaction.
   output [3 : 0] m_axi_arregion, // Region identifier. Permits a single physical interface on a slave to be re-used
   output [0 : 0] m_axi_aruser,   // User signal. Optional User-defined signal in the read address channel.
   output reg m_axi_arvalid,          // Read address valid. This signal indicates that the channel is signaling valid read addr
   input m_axi_arready,           // Read address ready. This signal indicates that the slave is ready to accept an address
   //
   // AXI Read data channel
   //
   input [0 : 0] m_axi_rid,       // Read ID tag. This signal is the identification tag for the read data group of signals
   input [DWIDTH-1 : 0] m_axi_rdata,    // Read data.
   input [1 : 0] m_axi_rresp,     // Read response. This signal indicates the status of the read transfer
   input m_axi_rlast,             // Read last. This signal indicates the last transfer in a read burst.
   input [0 : 0] m_axi_ruser,     // User signal. Optional User-defined signal in the read data channel.
   input m_axi_rvalid,            // Read valid. This signal indicates that the channel is signaling the required read data. 
   output m_axi_rready,           // Read ready. This signal indicates that the master can accept the read data and response
   //
   // DMA interface for Write transaction
   //
   input [AWIDTH-1:0] write_addr,       // Byte address for start of write transaction (should be 64bit alligned)
   input [7:0] write_count,       // Count of 64bit words to write. (minus one)
   input write_ctrl_valid,
   output reg write_ctrl_ready,
   input [DWIDTH-1:0] write_data,
   input write_data_valid,
   output write_data_ready,
   //
   // DMA interface for Read
   //
   input [AWIDTH-1:0] read_addr,       // Byte address for start of read transaction (should be 64bit alligned)
   input [7:0] read_count,       // Count of 64bit words to read.
   input read_ctrl_valid,
   output reg read_ctrl_ready,
   output [DWIDTH-1:0] read_data,
   output read_data_valid,
   input read_data_ready,
   //
   // Debug Bus
   //
   output [31:0] debug
   
   );
 
   
   localparam AW_IDLE = 0;
   localparam WAIT_AWREADY = 1;
   localparam WAIT_BVALID = 2;
   localparam AW_ERROR = 3;
   
   reg [1:0] write_addr_state;  
   reg [7:0] write_data_count;         // Count write transfers.
   reg 	     enable_data_write;
   
   localparam DW_IDLE = 0;
   localparam DW_RUN = 1;
   localparam DW_LAST = 2;
   
   reg [1:0]  write_data_state;
   
   localparam AR_IDLE = 0;
   localparam WAIT_ARREADY = 1;
   localparam WAIT_READ_DONE = 2;
   localparam AR_ERROR = 3;
   
   reg [1:0]  read_addr_state;

   localparam DR_IDLE = 0;
   localparam DR_RUN = 1;
   localparam DR_WAIT_ERROR = 2;
   localparam DR_ERROR = 3;

   reg [1:0]  read_data_state;
   reg [7:0]  read_data_count;
   reg 	      enable_data_read;

   ///////////////////////////
   // DEBUG
   ///////////////////////////
   assign debug= {24'h0,write_addr_state[1:0],write_data_state[1:0],read_addr_state[1:0],read_data_state[1:0]};
   
   
   //
   //
   //
   
   
   
   
   /////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Write address channel
   //
   /////////////////////////////////////////////////////////////////////////////////
   assign m_axi_awid = 1'b0;
   assign m_axi_awsize = $clog2(DWIDTH/8);
   assign m_axi_awburst = `AXI4_BURST_INCR;
   assign m_axi_awlock = `AXI4_LOCK_NORMAL;
   assign m_axi_awcache = `AXI4_CACHE_ALLOCATE | `AXI4_CACHE_OTHER_ALLOCATE | `AXI4_CACHE_MODIFIABLE | `AXI4_CACHE_BUFFERABLE;
   assign m_axi_awprot = `AXI4_PROT_NON_SECURE;
   assign m_axi_awqos = 4'h0;
   assign m_axi_awregion = 4'h0;
   assign m_axi_awuser = 1'b0;

   
   //
   // AXI Write address state machine
   //
   always @(posedge aclk)
     if (areset) begin
	write_ctrl_ready <= 1'b0;
	write_addr_state <= AW_IDLE;
	m_axi_awaddr <= {AWIDTH{1'b0}};
	m_axi_awlen[7:0] <= 8'h0;
	m_axi_awvalid <= 1'b0;
	m_axi_bready <= 1'b0;
     end else
       case (write_addr_state)
	 //
	 // AW_IDLE
	 // We are ready to accept a new write transaction.
	 //
	 AW_IDLE: begin
	    // Premptively accept new write transaction since we are idle.
	    write_ctrl_ready <= 1'b1;
	    // No need to be waiting for a response while idle.
	    m_axi_bready <= 1'b0;
	    // If we are offered a new transaction then.....
	    if (write_ctrl_valid) begin
	       // Drive all the relevent AXI4 write address channel signals next cycle.
	       m_axi_awaddr <= write_addr;
	       m_axi_awlen[7:0] <= {write_count};
	       m_axi_awvalid <= 1'b1;
	       // If the AXI4 write channel is pre-emptively accepting the transaction...
	       if (m_axi_awready == 1'b1) begin
		  // ...go straight to looking for a transaction response...
		  `DEBUG $display("WRITE TRANSACTION: ADDR: %x  LEN: %x @ time %d",write_addr,write_count,$time);	 		  
		  write_addr_state <= WAIT_BVALID;
		  m_axi_bready <= 1'b1;
	       end else begin
                  // ...otherwise wait to get the transaction accepted.
		  write_addr_state <= WAIT_AWREADY;
	       end	       
	    end
	 end
	 //
	 // WAIT_AWREADY
	 // Waiting for AXI4 slave to accept new write transaction.
	 //
	 WAIT_AWREADY: begin
	    write_ctrl_ready <= 1'b0;
	    // If the AXI4 write channel is accepting the transaction...
	    if (m_axi_awready == 1'b1) begin
	       // ...go to looking for a transaction response...
	       write_addr_state <= WAIT_BVALID;
	       m_axi_awvalid <= 1'b0;
	       m_axi_bready <= 1'b1;
	       `DEBUG $display("WRITE TRANSACTION: ADDR: %x  LEN: %x @ time %d",m_axi_awaddr,m_axi_awlen[7:0],$time);	       
	    end else begin
               // ...otherwise wait to get the trasaction accepted.
	       write_addr_state <= WAIT_AWREADY;
	    end
	 end // case: WAIT_AWREADY
	 //
	 // WAIT_BVALID
	 // Write transaction has been accepted, now waiting for a response to signal it's sucsesful.
	 // Ignoring ID tag for the moment
	 //
	 WAIT_BVALID: begin
	    write_ctrl_ready <= 1'b0;
	    m_axi_awvalid <= 1'b0;
	    // Wait for response channel to signal how write transaction went down....
	    if (m_axi_bvalid == 1'b1) begin
	       if ((m_axi_bresp == `AXI4_RESP_OKAY) ||  (m_axi_bresp == `AXI4_RESP_EXOKAY)) begin
		  // ....it went well, we are ready to start something new.
		  write_addr_state <= AW_IDLE;
		  m_axi_bready <= 1'b0;
		  write_ctrl_ready <= 1'b1; // Ready to run again as soon as we hit idle.
	       end else if ((m_axi_bresp == `AXI4_RESP_SLVERR) || (m_axi_bresp == `AXI4_RESP_DECERR)) begin
		  // ....things got ugly, retreat to an error stat and wait for intervention.
		  write_addr_state <= AW_ERROR;
		  m_axi_bready <= 1'b0;
	       end	       
	    end else begin
	       write_addr_state <= WAIT_BVALID;
	       m_axi_bready <= 1'b1;
	    end
	 end // case: WAIT_BVALID
	 //
	 // AW_ERROR
	 // Something bad happened, going to need external intervention to restore a safe state.
	 //
	 AW_ERROR: begin
	    write_ctrl_ready <= 1'b0;
	    write_addr_state <= AW_ERROR;
	    m_axi_awaddr <= {AWIDTH{1'b0}};
	    m_axi_awlen[7:0] <= 8'h0;
	    m_axi_awvalid <= 1'b0;
	    m_axi_bready <= 1'b0;
	 end
       endcase // case(write_addr_state)

   /////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Write data channel
   //
   /////////////////////////////////////////////////////////////////////////////////
   assign m_axi_wstrb = {DWIDTH/8{1'b1}};
   assign m_axi_wuser = 1'b0;

   //
   // AXI Write data state machine
   //
   always @(posedge aclk)
     if (areset) begin
	write_data_state <= AW_IDLE;
	write_data_count <= 1;
	enable_data_write <= 1'b0;
	m_axi_wlast <= 1'b0;
	
     end else
       case (write_data_state)
	 //
	 // DW_IDLE
	 // Sit in this state until presented with the control details of a new write transaction.
	 //
	 DW_IDLE: begin
	    write_data_count <= 1;
	    m_axi_wlast <= 1'b0;
	    
	    if (write_ctrl_valid && write_ctrl_ready) begin	       
	       enable_data_write <= 1'b1;
	       if (write_count[7:0] == 8'h0) begin
		  // Single transfer transaction
		  write_data_state <= DW_LAST;
		  m_axi_wlast <= 1'b1;
	       end else begin
		  write_data_state <= DW_RUN;
	       end
	    end else begin
	       write_data_state <= DW_IDLE;
	    end
	 end
	 //
	 // DW_RUN
	 //
	 DW_RUN : begin
	    enable_data_write <= 1'b1;
	    m_axi_wlast <= 1'b0;
	    
	    if (write_data_valid && m_axi_wready) begin
	      // Single write transfer
	       write_data_count <= write_data_count + 1;
	    
	       if (write_data_count == m_axi_awlen[7:0]) begin
		  write_data_state <= DW_LAST;
		  m_axi_wlast <= 1'b1;
	       end else begin
		  write_data_state <= DW_RUN;
	       end
	    end else begin
	       write_data_state <= DW_RUN;
	    end	    
	 end
	 //
	 // DW_LAST
	 //
	 DW_LAST: begin
	    if (write_data_valid && m_axi_wready) begin
	       enable_data_write <= 1'b0;
	       write_data_state <= DW_IDLE;
	       m_axi_wlast <= 1'b0;
	    end else begin
	       enable_data_write <= 1'b1;
	       write_data_state <= DW_LAST;
	       m_axi_wlast <= 1'b1;
	    end
	 end // case: DW_LAST
	 //
	 default:
	   write_data_state <= DW_IDLE;
	 
       endcase // case(write_data_state)
   
	       
   assign m_axi_wdata = write_data;
   assign m_axi_wvalid = enable_data_write && write_data_valid;
   assign write_data_ready = enable_data_write && m_axi_wready;

   /////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Read address channel
   //
   /////////////////////////////////////////////////////////////////////////////////
   assign m_axi_arid = 1'b0;
   assign m_axi_arsize = $clog2(DWIDTH/8);
   assign m_axi_arburst = `AXI4_BURST_INCR;
   assign m_axi_arlock = `AXI4_LOCK_NORMAL;
   assign m_axi_arcache = `AXI4_CACHE_ALLOCATE | `AXI4_CACHE_OTHER_ALLOCATE | `AXI4_CACHE_MODIFIABLE | `AXI4_CACHE_BUFFERABLE;
   assign m_axi_arprot = `AXI4_PROT_NON_SECURE;
   assign m_axi_arqos = 4'h0;
   assign m_axi_arregion = 4'h0;
   assign m_axi_aruser = 1'b0;


   //
   // AXI Read address state machine
   //
   always @(posedge aclk)
     if (areset) begin
	read_ctrl_ready <= 1'b0;
	read_addr_state <= AR_IDLE;
	m_axi_araddr <= {AWIDTH{1'b0}};
	m_axi_arlen[7:0] <= 8'h0;
	m_axi_arvalid <= 1'b0;
     end else
       case (read_addr_state)
	 //
	 // AR_IDLE
	 // We are ready to accept a new read transaction.
	 //
	 AR_IDLE: begin
	    // Premptively accept new read transaction since we are idle.
	    read_ctrl_ready <= 1'b1;
	    // If we are offered a new transaction then.....
	    if (read_ctrl_valid) begin
	       // Drive all the relevent AXI4 read address channel signals next cycle.
	       m_axi_araddr <= read_addr;
	       m_axi_arlen[7:0] <= {read_count};
	       m_axi_arvalid <= 1'b1;
	       // If the AXI4 read channel is pre-emptively accepting the transaction...
	       if (m_axi_arready == 1'b1) begin
		  // ...go straight to looking for the transaction to complete
		   `DEBUG $display("READ TRANSACTION: ADDR: %x  LEN: %x @ time %d",read_addr,read_count,$time);
		  read_addr_state <= WAIT_READ_DONE;
	       end else begin
                  // ...otherwise wait to get the transaction accepted.
		  read_addr_state <= WAIT_ARREADY;
	       end	       
	    end
	 end
	 //
	 // WAIT_ARREADY
	 // Waiting for AXI4 slave to accept new read transaction.
	 //
	 WAIT_ARREADY: begin
	    read_ctrl_ready <= 1'b0;
	    // If the AXI4 read channel is accepting the transaction...
	    if (m_axi_arready == 1'b1) begin
	       // ...go to looking for the transaction to complete...
	       read_addr_state <= WAIT_READ_DONE;
	       m_axi_arvalid <= 1'b0;
	        `DEBUG $display("READ TRANSACTION: ADDR: %x  LEN: %x @ time %d",m_axi_araddr,m_axi_arlen[7:0],$time);	       
	    end else begin
               // ...otherwise wait to get the trasaction accepted.
	       read_addr_state <= WAIT_ARREADY;
	    end
	 end // case: WAIT_ARREADY
	 //
	 // WAIT_READ_DONE
	 // Read transaction has been accepted, now waiting for the data transfer to complete
	 // Ignoring ID tag for the moment
	 //
	 WAIT_READ_DONE: begin
	    read_ctrl_ready <= 1'b0;
	    m_axi_arvalid <= 1'b0;
	    // Wait for read transaction to complete
	    if (read_data_state == DR_IDLE) begin
		 // ....it went well, we are ready to start something new.
	       read_addr_state <= AR_IDLE;
	       read_ctrl_ready <= 1'b1; // Ready to run again as soon as we hit idle.
	    end else if (read_data_state == DR_ERROR) begin
		 // ....things got ugly, retreat to an error stat and wait for intervention.
	       read_addr_state <= AR_ERROR;
	    end else begin
	       read_addr_state <= WAIT_READ_DONE;
	    end
	 end // case: WAIT_BVALID
	 //
	 // AR_ERROR
	 // Something bad happened, going to need external intervention to restore a safe state.
	 //
	 AR_ERROR: begin
	    read_ctrl_ready <= 1'b0;
	    read_addr_state <= AR_ERROR;
	    m_axi_araddr <= {AWIDTH{1'b0}};
	    m_axi_arlen[7:0] <= 8'h0;
	    m_axi_arvalid <= 1'b0;
	 end
       endcase // case(read_addr_state)

   /////////////////////////////////////////////////////////////////////////////////
   //
   // AXI Read data channel
   //
   /////////////////////////////////////////////////////////////////////////////////
  

   //
   // AXI Read data state machine
   //
   always @(posedge aclk)
     if (areset) begin
	read_data_state <= AR_IDLE;
	read_data_count <= 0;
	enable_data_read <= 1'b0;
	
     end else
       case (read_data_state)
	 //
	 // DR_IDLE
	 // Sit in this state until presented with the control details of a new read transaction.
	 //
	 DR_IDLE: begin
	    read_data_count <= 0;
	    
	    if (read_ctrl_valid && read_ctrl_ready) begin	       
	       enable_data_read <= 1'b1;
	       read_data_state <= DR_RUN;			   
	    end else begin
	       read_data_state <= DR_IDLE;
	    end
	 end
	 //
	 // DR_RUN
	 // Sit here counting read transfers. If any have error's shift to error state.
	 //
	 DR_RUN : begin
	    enable_data_read <= 1'b1;
	     
	    if (read_data_ready && m_axi_rvalid) begin
	      // Single read transfer
	       read_data_count <= read_data_count + 1;
	       if ((m_axi_rresp == `AXI4_RESP_SLVERR) ||  (m_axi_rresp == `AXI4_RESP_DECERR)) begin
		  if (m_axi_rlast) begin
		     read_data_state <= DR_ERROR;
		  end else begin
		     read_data_state <= DR_WAIT_ERROR;
		  end
	       end else if (m_axi_rlast) begin // Implicitly good response signalled this transfer.
		  if (read_data_count == m_axi_arlen[7:0]) begin
		     read_data_state <= DR_IDLE;
		  end else begin
		     read_data_state <= DR_ERROR;
		  end
	       end else begin
		  read_data_state <= DR_RUN;
	       end
	    end else begin
	       read_data_state <= DR_RUN;
	    end
	 end
	 //
	 // DR_WAIT_ERROR
	 // Something bad happened, wait for last signalled in this burst
	 //
	 DR_WAIT_ERROR: begin
	    if (read_data_ready && m_axi_rvalid && m_axi_rlast) begin
	       enable_data_read <= 1'b0;
	       read_data_state <= DR_ERROR;
	    end else begin
	       enable_data_read <= 1'b1;
	       read_data_state <= DR_WAIT_ERROR;
	    end
	 end // case: DR_WAIT_ERROR
	 //
	 // DR_ERROR
	 // Something bad happened, going to need external intervention to restore a safe state.
	 //
	 DR_ERROR: begin
	    enable_data_read <= 1'b0;
	    read_data_state <= DR_ERROR;
	 end // case: DR_ERROR
	 

       endcase // case(read_data_state)
   
	       
   assign read_data = m_axi_rdata;
   assign m_axi_rready = enable_data_read && read_data_ready;
   assign read_data_valid = enable_data_read && m_axi_rvalid;
			   
endmodule // axi_dma_master

	      
	    
	 
   
	      