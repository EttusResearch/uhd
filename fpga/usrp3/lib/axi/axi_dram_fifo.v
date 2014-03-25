
//
// There are various obligations put on this code not present in regular BRAM based FIFO's
//
// 1) Bursts are way more efficient, use local small FIFO's to interact with DRAM
// 2) Never cross a 4KByte address boundry within a single transaction, this is an AXI4 rule.
// 3) 2^SIZE must be greater than 4KB so that the 4KByte page protection also deals with FIFO wrap corner case.
//
module axi_dram_fifo
  // NOTE: SIZE is log2 of size of FIFO buffer in bytes. i.e 13 for 8KBytes which is 1kx64
  #(parameter BASE=0, SIZE=16, TIMEOUT=64)
    (
     input bus_clk,                  
     input bus_reset,         
     input clear,
     input dram_clk,
     input dram_reset,
     //
     // AXI Write address channel
     //
     output [0 : 0] m_axi_awid,     // Write address ID. This signal is the identification tag for the write address signals
     output [31 : 0] m_axi_awaddr,  // Write address. The write address gives the address of the first transfer in a write burst
     output [7 : 0] m_axi_awlen,    // Burst length. The burst length gives the exact number of transfers in a burst.
     output [2 : 0] m_axi_awsize,   // Burst size. This signal indicates the size of each transfer in the burst. 
     output [1 : 0] m_axi_awburst,  // Burst type. The burst type and the size information, determine how the address is calculated
     output [0 : 0] m_axi_awlock,   // Lock type. Provides additional information about the atomic characteristics of the transfer.
     output [3 : 0] m_axi_awcache,  // Memory type. This signal indicates how transactions are required to progress
     output [2 : 0] m_axi_awprot,   // Protection type. This signal indicates the privilege and security level of the transaction
     output [3 : 0] m_axi_awqos,    // Quality of Service, QoS. The QoS identifier sent for each write transaction
     output [3 : 0] m_axi_awregion, // Region identifier. Permits a single physical interface on a slave to be re-used.
     output [0 : 0] m_axi_awuser,   // User signal. Optional User-defined signal in the write address channel.
     output m_axi_awvalid,      // Write address valid. This signal indicates that the channel is signaling valid write addr
     input m_axi_awready,           // Write address ready. This signal indicates that the slave is ready to accept an address
     //
     // AXI Write data channel.
     //
     output [63 : 0] m_axi_wdata,   // Write data
     output [7 : 0] m_axi_wstrb,    // Write strobes. This signal indicates which byte lanes hold valid data.
     output m_axi_wlast,        // Write last. This signal indicates the last transfer in a write burst
     output [0 : 0] m_axi_wuser,    // User signal. Optional User-defined signal in the write data channel.
     output m_axi_wvalid,           // Write valid. This signal indicates that valid write data and strobes are available. 
     input m_axi_wready,            // Write ready. This signal indicates that the slave can accept the write data.
     //
     // AXI Write response channel signals
     //
     input [0 : 0] m_axi_bid,       // Response ID tag. This signal is the ID tag of the write response. 
     input [1 : 0] m_axi_bresp,     // Write response. This signal indicates the status of the write transaction.
     input [0 : 0] m_axi_buser,     // User signal. Optional User-defined signal in the write response channel.
     input m_axi_bvalid,            // Write response valid. This signal indicates that the channel is signaling a valid response
     output m_axi_bready,       // Response ready. This signal indicates that the master can accept a write response
     //
     // AXI Read address channel
     //
     output [0 : 0] m_axi_arid,     // Read address ID. This signal is the identification tag for the read address group of signals
     output [31 : 0] m_axi_araddr,  // Read address. The read address gives the address of the first transfer in a read burst
     output [7 : 0] m_axi_arlen,    // Burst length. This signal indicates the exact number of transfers in a burst.
     output [2 : 0] m_axi_arsize,   // Burst size. This signal indicates the size of each transfer in the burst.
     output [1 : 0] m_axi_arburst,  // Burst type. The burst type and the size information determine how the address for each transfer
     output [0 : 0] m_axi_arlock,   // Lock type. This signal provides additional information about the atomic characteristics
     output [3 : 0] m_axi_arcache,  // Memory type. This signal indicates how transactions are required to progress 
     output [2 : 0] m_axi_arprot,   // Protection type. This signal indicates the privilege and security level of the transaction
     output [3 : 0] m_axi_arqos,    // Quality of Service, QoS. QoS identifier sent for each read transaction.
     output [3 : 0] m_axi_arregion, // Region identifier. Permits a single physical interface on a slave to be re-used
     output [0 : 0] m_axi_aruser,   // User signal. Optional User-defined signal in the read address channel.
     output m_axi_arvalid,          // Read address valid. This signal indicates that the channel is signaling valid read addr
     input m_axi_arready,           // Read address ready. This signal indicates that the slave is ready to accept an address
     //
     // AXI Read data channel
     //
     input [0 : 0] m_axi_rid,       // Read ID tag. This signal is the identification tag for the read data group of signals
     input [63 : 0] m_axi_rdata,    // Read data.
     input [1 : 0] m_axi_rresp,     // Read response. This signal indicates the status of the read transfer
     input m_axi_rlast,             // Read last. This signal indicates the last transfer in a read burst.
     input [0 : 0] m_axi_ruser,     // User signal. Optional User-defined signal in the read data channel.
     input m_axi_rvalid,            // Read valid. This signal indicates that the channel is signaling the required read data. 
     output m_axi_rready,           // Read ready. This signal indicates that the master can accept the read data and response
     //
     // CHDR friendly AXI stream input
     //
     input [63:0] i_tdata,
     input i_tlast,
     input i_tvalid,
     output i_tready,
     //
     // CHDR friendly AXI Stream output
     //
     output [63:0] o_tdata,
     output o_tlast,
     output o_tvalid,
     input o_tready,
     //
     //
     //
     input [15:0] supress_threshold,
     input supress_enable,
     //
     // Debug Bus
     //
     output [197:0] debug
     );
 
   //
   // We are only solving for width 64bits here, since it's our standard CHDR quanta
   //
   localparam WIDTH=64;
   
   //
   // Input side declarations
   //
   localparam INPUT_IDLE = 0;
   localparam INPUT1 = 1;
   localparam INPUT2 = 2;
   localparam INPUT3 = 3;   
   localparam INPUT4 = 4;
   localparam INPUT5 = 5;
   localparam INPUT6 = 6;
	     
   reg [2:0]  input_state;
   reg 	      input_timeout_triggered;
   reg 	      input_timeout_reset;
   reg [8:0]  input_timeout_count;
   reg [31:0] write_addr;
   reg 	      write_ctrl_valid;
   wire       write_ctrl_ready;
   reg [7:0]  write_count;
   reg 	      update_write;
   wire [63:0] write_data;
   wire        write_data_valid;
   wire        write_data_ready;
   
   //
   // Output side declarations
   //
   localparam OUTPUT_IDLE = 0;
   localparam OUTPUT1 = 1;
   localparam OUTPUT2 = 2;
   localparam OUTPUT3 = 3;   
   localparam OUTPUT4 = 4;
   localparam OUTPUT5 = 5;
   localparam OUTPUT6 = 6;
	     
   reg [2:0]  output_state;
   reg 	      output_timeout_triggered;
   reg 	      output_timeout_reset;
   reg [8:0]  output_timeout_count;
   reg [31:0] read_addr;
   reg 	      read_ctrl_valid;
   wire       read_ctrl_ready;
   reg [7:0]  read_count; 
   reg 	      update_read;
   wire [63:0] read_data;
   wire        read_data_valid;
   wire        read_data_ready;
   
   // Track main FIFO active size.
   reg [SIZE-3:0] space, occupied;
   wire [11:0] 	  input_page_boundry, output_page_boundry;
   

   //
   // Buffer input in FIFO's. Embeded tlast signal using ESCape code.
   //
   wire [WIDTH-1:0] i_tdata_i0;
   wire 	    i_tvalid_i0, i_tready_i0, i_tlast_i0;
 
   wire [WIDTH-1:0] i_tdata_i1;
   wire 	    i_tvalid_i1, i_tready_i1;

   wire [WIDTH-1:0] i_tdata_i2;
   wire 	    i_tvalid_i2, i_tready_i2;
   
   wire [WIDTH-1:0] i_tdata_input;
   wire 	    i_tvalid_input, i_tready_input;
   wire [15:0] 	    space_input, occupied_input;
   reg [15:0] 	    space_input_reg;
   reg 		    supress_reads;
   
   ///////////////////////////
   // DEBUG
   ///////////////////////////
   wire [31:0] 	    debug_axi_dma_master;
   
   //assign debug = {18'h0, input_state[2:0], output_state[2:0], debug_axi_dma_master[7:0]};
    
   ///////////////////////////////////////////////////////////////////////////////
   
   wire 	    write_in, read_in, empty_in, full_in;
   assign 	    i_tready = ~full_in;
   assign 	    write_in = i_tvalid & i_tready;
   assign 	    i_tvalid_i0 = ~empty_in;
   assign 	    read_in = i_tvalid_i0 & i_tready_i0;
   wire [6:0] 	    discard_i0;
   
   fifo_short_2clk fifo_short_2clk_i0
     (.rst(bus_reset),
      .wr_clk(bus_clk),
      .din({7'h0,i_tlast,i_tdata}), // input [71 : 0] din
      .wr_en(write_in), // input wr_en
      .full(full_in), // output full
      .wr_data_count(), // output [9 : 0] wr_data_count
	       
      .rd_clk(dram_clk), // input rd_clk
      .dout({discard_i0,i_tlast_i0,i_tdata_i0}), // output [71 : 0] dout
      .rd_en(read_in), // input rd_en
      .empty(empty_in), // output empty
      .rd_data_count()  // output [9 : 0] rd_data_count
      );

   axi_embed_tlast axi_embed_tlast_i
     (
      .clk(dram_clk),
      .reset(dram_reset),
      .clear(clear),
      //
      .i_tdata(i_tdata_i0),
      .i_tlast(i_tlast_i0),
      .i_tvalid(i_tvalid_i0),
      .i_tready(i_tready_i0),
      //
      .o_tdata(i_tdata_i1),
      .o_tvalid(i_tvalid_i1),
      .o_tready(i_tready_i1)
      );

   
   axi_fast_fifo #(.WIDTH(WIDTH)) fast_fifo_i0
     (
      .clk(dram_clk), 
      .reset(dram_reset), 
      .clear(clear),
      //
      .i_tdata(i_tdata_i1), 
      .i_tvalid(i_tvalid_i1), 
      .i_tready(i_tready_i1),
      //
      .o_tdata(i_tdata_i2), 
      .o_tvalid(i_tvalid_i2), 
      .o_tready(i_tready_i2)
      );
 
   axi_fifo #(.WIDTH(WIDTH),.SIZE(12)) fifo_i1
     (
      .clk(dram_clk), 
      .reset(dram_reset), 
      .clear(clear),
      //
      .i_tdata(i_tdata_i2), 
      .i_tvalid(i_tvalid_i2), 
      .i_tready(i_tready_i2),
      //
      .o_tdata(i_tdata_input), 
      .o_tvalid(i_tvalid_input), 
      .o_tready(i_tready_input),
      //
      .space(space_input), 
      .occupied(occupied_input)
      );

   //
   // Monitor occupied_input to deduce when DRAM FIFO is running short of bandwidth and there is a danger of backpressure
   // passing upstream of the DRAM FIFO.
   // In this situation supress read requests to the DRAM FIFO so that more bandwidth is available to writes.
   //

   
   always @(posedge dram_clk) 
     begin
	space_input_reg <= space_input;
	if ((space_input_reg < supress_threshold[15:0])  && supress_enable)
	  supress_reads <= 1'b1;
	else 
	  supress_reads <= 1'b0;
     end

   //
   // Buffer output in 32entry FIFO's. Extract embeded tlast signal.
   //
   wire [WIDTH-1:0] o_tdata_output;
   wire 	    o_tvalid_output, o_tready_output;
   wire [15:0] 	    space_output, occupied_output;

   wire [WIDTH-1:0] o_tdata_i0;
   wire 	    o_tvalid_i0, o_tready_i0;
   
   wire [WIDTH-1:0] o_tdata_i1;
   wire 	    o_tvalid_i1, o_tready_i1, o_tlast_i1;
   
   wire [WIDTH-1:0] o_tdata_i2;
   wire 	    o_tvalid_i2, o_tready_i2, o_tlast_i2;
   
   wire [WIDTH-1:0] o_tdata_i3;
   wire 	    o_tvalid_i3, o_tready_i3, o_tlast_i3;

  wire 	    checksum_error;
   
   
   axi_fifo #(.WIDTH(WIDTH),.SIZE(9)) fifo_i2
     (
      .clk(dram_clk), 
      .reset(dram_reset), 
      .clear(clear),
      //
      .i_tdata(o_tdata_output), 
      .i_tvalid(o_tvalid_output), 
      .i_tready(o_tready_output),
      //
      .o_tdata(o_tdata_i0), 
      .o_tvalid(o_tvalid_i0), 
      .o_tready(o_tready_i0),
      //
      .space(space_output), 
      .occupied(occupied_output)
      );

   // Place FLops straight after SRAM read access for timing.
   axi_fast_fifo #(.WIDTH(WIDTH)) fast_fifo_i1
     (
      .clk(dram_clk), 
      .reset(dram_reset), 
      .clear(clear),
      //
      .i_tdata(o_tdata_i0), 
      .i_tvalid(o_tvalid_i0), 
      .i_tready(o_tready_i0),
      //
      .o_tdata(o_tdata_i1), 
      .o_tvalid(o_tvalid_i1), 
      .o_tready(o_tready_i1 && ~supress_reads)
      );

   // More pipeline flops to meet timing 
   axi_fast_fifo #(.WIDTH(WIDTH)) fast_fifo_i2
     (
      .clk(dram_clk), 
      .reset(dram_reset), 
      .clear(clear),
      //
      .i_tdata(o_tdata_i1), 
      .i_tvalid(o_tvalid_i1 && ~supress_reads), 
      .i_tready(o_tready_i1),
      //
      .o_tdata(o_tdata_i2), 
      .o_tvalid(o_tvalid_i2), 
      .o_tready(o_tready_i2)
      );
   
    axi_fast_extract_tlast axi_fast_extract_tlast_i0
     (
      .clk(dram_clk),
      .reset(dram_reset),
      .clear(clear),
      //
      .i_tdata(o_tdata_i2),
      .i_tvalid(o_tvalid_i2),
      .i_tready(o_tready_i2),
      //
      .o_tdata(o_tdata_i3),
      .o_tlast(o_tlast_i3),
      .o_tvalid(o_tvalid_i3),
      .o_tready(o_tready_i3)
      //
 //     .checksum_error_reg(checksum_error)
      );

    
   wire 	    write_out, read_out, empty_out, full_out;
   assign 	    o_tready_i3 = ~full_out;
   assign 	    write_out = o_tvalid_i3 & o_tready_i3;
   assign 	    o_tvalid = ~empty_out;
   assign 	    read_out = o_tvalid & o_tready;
   wire [6:0] 	    discard_i1;
   
   fifo_short_2clk fifo_short_2clk_i1
     (
      .rst(bus_reset),
      .wr_clk(dram_clk),
      .din({7'h0,o_tlast_i3,o_tdata_i3}), // input [71 : 0] din
      .wr_en(write_out), // input wr_en
      .full(full_out), // output full
      .wr_data_count(), // output [9 : 0] wr_data_count
	       
      .rd_clk(bus_clk), // input rd_clk
      .dout({discard_i1,o_tlast,o_tdata}), // output [71 : 0] dout
      .rd_en(read_out), // input rd_en
      .empty(empty_out), // output empty
      .rd_data_count()  // output [9 : 0] rd_data_count
      );

   //
   // Simple input timeout counter for now.
   // Timeout count only increments when there is some data waiting to be written.
   //
   always @(posedge dram_clk)
     if (dram_reset | clear) begin
	input_timeout_count <= 0;
	input_timeout_triggered <= 1'b0;
     end else if (input_timeout_reset) begin
	input_timeout_count <= 0;
	input_timeout_triggered <= 1'b0;
     end else if (input_timeout_count == TIMEOUT) begin
	input_timeout_triggered <= 1'b1;
     end else if (input_state == INPUT_IDLE) begin
	input_timeout_count <= input_timeout_count + (occupied_input != 0);
     end
	
   		      
   //
   // Wait for 16 entries in input FIFO to trigger DRAM write burst.
   // Timeout can also trigger burst so fragments of data are not left to rot in the input FIFO.
   // Also if enough data is present in the input FIFO to complete a burst upto the edge
   // of a 4KByte page then immediately start the burst.
   //
   always @(posedge dram_clk)
     if (dram_reset | clear) begin
	input_state <= INPUT_IDLE;
	write_addr[31:SIZE] <= BASE >> SIZE;
	write_addr[SIZE-1:0] <= 0;
	input_timeout_reset <= 1'b0;
	write_ctrl_valid <= 1'b0;
	write_count <= 8'd0; 
	update_write <= 1'b0;
     end else
       case (input_state)
	 //
	 // INPUT_IDLE.
	 // To start an input transfer to DRAM need:
	 // 1) Space in the DRAM FIFO 
	 // and either
	 // 2) 256 entrys in the input FIFO
	 // or
	 // 3) Timeout waiting for more data.
	 //
	 INPUT_IDLE: begin
	    write_ctrl_valid <= 1'b0;
	    update_write <= 1'b0;
	    if (space > 255) begin // Space in the DRAM FIFO
	       if (occupied_input > 255) begin  // 256 or more entrys in input FIFO
		  input_state <= INPUT1;
		  input_timeout_reset <= 1'b1;
	       end else if (input_timeout_triggered) begin // input FIFO timeout waiting for new data.
		  input_state <= INPUT2;
		  input_timeout_reset <= 1'b1;
	       end else begin
	       	  input_timeout_reset <= 1'b0;
		  input_state <= INPUT_IDLE;
	       end
	    end else begin
	       input_timeout_reset <= 1'b0;
	       input_state <= INPUT_IDLE;
	    end
	 end
	 //
	 // INPUT1.
	 // Caused by input FIFO reaching 256 entries.
	 // Request write burst of lesser of:
	 // 1) Entrys until page boundry crossed
	 // 2) 256.
	 //
	 INPUT1: begin
	    write_count <= (input_page_boundry < 255) ? input_page_boundry[7:0] : 8'd255;
	    write_ctrl_valid <= 1'b1;
	    if (write_ctrl_ready)
	      input_state <= INPUT4; // Pre-emptive ACK
	    else
	      input_state <= INPUT3; // Wait for ACK
	 end
	 //
	 // INPUT2.
	 // Caused by timeout of input FIFO. (occupied_input was implicitly less than 256 last cycle)
	 // Request write burst of lesser of:
	 // 1) Entries until page boundry crossed
	 // 2) Entries in input FIFO
	 //
	 INPUT2: begin
	    write_count <= (input_page_boundry < ({3'h0,occupied_input[8:0]} - 12'd1)) ? input_page_boundry[7:0] : (occupied_input[8:0] - 7'd1);
	    write_ctrl_valid <= 1'b1;
	    if (write_ctrl_ready)
	      input_state <= INPUT4; // Pre-emptive ACK
	    else
	      input_state <= INPUT3; // Wait for ACK
	 end
	 //
	 // INPUT3.
	 // Wait in this state for AXI4_DMA engine to accept transaction.
	 //
	 INPUT3: begin
	    if (write_ctrl_ready) begin
	       write_ctrl_valid <= 1'b0;
	       input_state <= INPUT4; // ACK
	    end else begin
	       write_ctrl_valid <= 1'b1;
	       input_state <= INPUT3; // Wait for ACK
	    end
	 end
	 //
	 // INPUT4.
	 // Wait here until write_ctrl_ready_deasserts.
	 // This is important as the next time it asserts we know that a write response was receieved.
	 INPUT4: begin
	    write_ctrl_valid <= 1'b0;
	    if (!write_ctrl_ready)
	      input_state <= INPUT5; // Move on
	    else
	      input_state <= INPUT4; // Wait for deassert
	 end	 
	 //
	 // INPUT5.
	 // Transaction has been accepted by AXI4 DMA engine. Now we wait for the re-assertion
	 // of write_ctrl_ready which signals that the AXI4 DMA engine has receieved a response
	 // for the whole write transaction and we assume that this means it is commited to DRAM.
	 // We are now free to update write_addr pointer and go back to idle state.
	 // 
	 INPUT5: begin
	    write_ctrl_valid <= 1'b0;
	    if (write_ctrl_ready) begin
	       write_addr[SIZE-1:0] <= write_addr[SIZE-1:0] + ((write_count + 1) << 3);
	       input_state <= INPUT6;
	       update_write <= 1'b1;	       
	    end else begin
	       input_state <= INPUT5;
	    end
	 end
	 //
	 // INPUT6:
	 // Need to let space update before looking if there's more to do.
	 //
	 INPUT6: begin
	    input_state <= INPUT_IDLE;
	    update_write <= 1'b0;
	 end
	 // Ass covering.
	 default: input_state <= INPUT_IDLE;
	 
       endcase // case(input_state)
   

   //
   // Simple output timeout counter for now
   //
   always @(posedge dram_clk)
     if (dram_reset | clear) begin
	output_timeout_count <= 0;
	output_timeout_triggered <= 1'b0;
     end else if (output_timeout_reset) begin
	output_timeout_count <= 0;
	output_timeout_triggered <= 1'b0;
     end else if (output_timeout_count == TIMEOUT) begin
	output_timeout_triggered <= 1'b1;
     end else if (output_state == OUTPUT_IDLE) begin
	output_timeout_count <= output_timeout_count + (occupied != 0 );
     end


   //
   // Wait for 64 entries in main FIFO to trigger DRAM read burst.
   // Timeout can also trigger burst so fragments of data are not left to rot in the main FIFO.
   // Also if enough data is present in the main FIFO to complete a burst upto the edge
   // of a 4KByte page then immediately start the burst.
   //
   always @(posedge dram_clk)
     if (dram_reset | clear) begin
	output_state <= OUTPUT_IDLE;
	read_addr[31:SIZE] <= BASE >> SIZE;
	read_addr[SIZE-1:0] <= 0;
	output_timeout_reset <= 1'b0;
	read_ctrl_valid <= 1'b0;
	read_count <= 8'd0;
	update_read <= 1'b0;
     end else
       case (output_state)
	 //
	 // OUTPUT_IDLE.
	 // To start an output tranfer from DRAM
	 // 1) Space in the small output FIFO 
	 // and either
	 // 2) 256 entrys in the DRAM FIFO
	 // or
	 // 3) Timeout waiting for more data.
	 //
	 OUTPUT_IDLE: begin
	    read_ctrl_valid <= 1'b0;
	    update_read <= 1'b0;
	    if (space_output > 255) begin // Space in the output FIFO.
	      if (occupied > 255) begin // 64 or more entrys in main FIFO
		 output_state <= OUTPUT1;
		 output_timeout_reset <= 1'b1;
	      end else if (output_timeout_triggered) begin // output FIFO timeout waiting for new data.
		 output_state <= OUTPUT2;
		 output_timeout_reset <= 1'b1;
	      end else begin
		 output_timeout_reset <= 1'b0;
		 output_state <= OUTPUT_IDLE;
	      end
	    end else begin
	       output_timeout_reset <= 1'b0;
	       output_state <= OUTPUT_IDLE;
	    end
	 end // case: OUTPUT_IDLE
	 //
	 // OUTPUT1.
	 // Caused by main FIFO reaching 256 entries.
	 // Request read burst of lesser of lesser of:
	 // 1) Entrys until page boundry crossed
	 // 2) 256.
	 //
	 OUTPUT1: begin
	    read_count <= (output_page_boundry < 255) ? output_page_boundry : 8'd255;
	    read_ctrl_valid <= 1'b1;
	    if (read_ctrl_ready)
	      output_state <= OUTPUT4; // Pre-emptive ACK
	    else
	      output_state <= OUTPUT3; // Wait for ACK
	 end
	 //
	 // OUTPUT2.
	 // Caused by timeout of main FIFO
	 // Request read burst of lesser of:
	 // 1) Entries until page boundry crossed
	 // 2) Entries in main FIFO
	 //
	 OUTPUT2: begin
	    read_count <= (output_page_boundry < (occupied - 1)) ? output_page_boundry : (occupied - 1);
	    read_ctrl_valid <= 1'b1;
	    if (read_ctrl_ready)
	      output_state <= OUTPUT4; // Pre-emptive ACK
	    else
	      output_state <= OUTPUT3; // Wait for ACK
	 end
	 //
	 // OUTPUT3.
	 // Wait in this state for AXI4_DMA engine to accept transaction.
	 //
	 OUTPUT3: begin
	    if (read_ctrl_ready) begin
	       read_ctrl_valid <= 1'b0;
	       output_state <= OUTPUT4; // ACK
	    end else begin
	       read_ctrl_valid <= 1'b1;
	       output_state <= OUTPUT3; // Wait for ACK
	    end
	 end
	 //
	 // OUTPUT4.
	 // Wait here unitl read_ctrl_ready_deasserts.
	 // This is important as the next time it asserts we know that a read response was receieved.
	 OUTPUT4: begin
	    read_ctrl_valid <= 1'b0;
	    if (!read_ctrl_ready)
	      output_state <= OUTPUT5; // Move on
	    else
	      output_state <= OUTPUT4; // Wait for deassert
	 end	 
	 //
	 // OUTPUT5.
	 // Transaction has been accepted by AXI4 DMA engine. Now we wait for the re-assertion
	 // of read_ctrl_ready which signals that the AXI4 DMA engine has receieved a last signal and good response
	 // for the whole read transaction.
	 // We are now free to update read_addr pointer and go back to idle state.
	 // 
	 OUTPUT5: begin
	    read_ctrl_valid <= 1'b0;
	    if (read_ctrl_ready) begin
	       read_addr[SIZE-1:0] <= read_addr[SIZE-1:0] + ((read_count + 1) << 3);
	       output_state <= OUTPUT6;
	       update_read <= 1'b1;
	       
	    end else begin
	       output_state <= OUTPUT5;
	    end
	 end // case: OUTPUT5
	 //
	 // OUTPUT6.
	 // Need to get occupied value updated before checking if there's more to do.
	 //
	 OUTPUT6: begin
	    update_read <= 1'b0;
	    output_state <= OUTPUT_IDLE;
	 end
	 // Ass covering.
	 default: output_state <= OUTPUT_IDLE;
	 
       endcase // case(output_state)

   //
   // Calculate number of entries remaining until next 4KB page boundry is crossed minus 1.
   // Note, units of calculation are 64bit wide words. Address is always 64bit alligned.
   //
   assign input_page_boundry = {write_addr[31:12],9'h1ff} - write_addr[31:3];   
   assign output_page_boundry = {read_addr[31:12],9'h1ff} - read_addr[31:3];
   
   //
   // Count number of used entries in main DRAM FIFO.
   // Note that this is expressed in units of 64bit wide words.
   //
   always @(posedge dram_clk)
     if (dram_reset | clear)
       occupied <= 0;
     else
       occupied <= occupied + (update_write ? write_count + 1 : 0) - (update_read ? read_count + 1 : 0);
   
   always @(posedge dram_clk)
     if (dram_reset | clear)
       space <= (1 << SIZE-3) - 'd64; // Subtract 64 from space to make allowance for read/write reordering in DRAM controller confuing pointer math.
     else
       space <= space - (update_write ? write_count + 1 : 0) + (update_read ? read_count + 1 : 0);

   //
   // Instamce of axi_dma_master
   //

   
   axi_dma_master axi_dma_master_i
     (
      .aclk(dram_clk), // input aclk
      .areset(dram_reset | clear), // input aresetn
      // Write control
      .m_axi_awid(m_axi_awid), // input [0 : 0] m_axi_awid
      .m_axi_awaddr(m_axi_awaddr), // input [31 : 0] m_axi_awaddr
      .m_axi_awlen(m_axi_awlen), // input [7 : 0] m_axi_awlen
      .m_axi_awsize(m_axi_awsize), // input [2 : 0] m_axi_awsize
      .m_axi_awburst(m_axi_awburst), // input [1 : 0] m_axi_awburst
      .m_axi_awvalid(m_axi_awvalid), // input m_axi_awvalid
      .m_axi_awready(m_axi_awready), // output m_axi_awready
      .m_axi_awlock(m_axi_awlock),
      .m_axi_awcache(m_axi_awcache),
      .m_axi_awprot(m_axi_awprot),
      .m_axi_awqos(m_axi_awqos),
      .m_axi_awregion(m_axi_awregion),
      .m_axi_awuser(m_axi_awuser),
      // Write Data
      .m_axi_wdata(m_axi_wdata), // input [63 : 0] m_axi_wdata
      .m_axi_wstrb(m_axi_wstrb), // input [7 : 0] m_axi_wstrb
      .m_axi_wlast(m_axi_wlast), // input m_axi_wlast
      .m_axi_wvalid(m_axi_wvalid), // input m_axi_wvalid
      .m_axi_wready(m_axi_wready), // output m_axi_wready
      .m_axi_wuser(),
      // Write Response
      .m_axi_bid(m_axi_bid), // output [0 : 0] m_axi_bid
      .m_axi_bresp(m_axi_bresp), // output [1 : 0] m_axi_bresp
      .m_axi_bvalid(m_axi_bvalid), // output m_axi_bvalid
      .m_axi_bready(m_axi_bready), // input m_axi_bready
      .m_axi_buser(),
      // Read Control
      .m_axi_arid(m_axi_arid), // input [0 : 0] m_axi_arid
      .m_axi_araddr(m_axi_araddr), // input [31 : 0] m_axi_araddr
      .m_axi_arlen(m_axi_arlen), // input [7 : 0] m_axi_arlen
      .m_axi_arsize(m_axi_arsize), // input [2 : 0] m_axi_arsize
      .m_axi_arburst(m_axi_arburst), // input [1 : 0] m_axi_arburst
      .m_axi_arvalid(m_axi_arvalid), // input m_axi_arvalid
      .m_axi_arready(m_axi_arready), // output m_axi_arready
      .m_axi_arlock(m_axi_arlock),
      .m_axi_arcache(m_axi_arcache),
      .m_axi_arprot(m_axi_arprot),
      .m_axi_arqos(m_axi_arqos),
      .m_axi_arregion(m_axi_arregion),
      .m_axi_aruser(m_axi_aruser),
      // Read Data
      .m_axi_rid(m_axi_rid), // output [0 : 0] m_axi_rid
      .m_axi_rdata(m_axi_rdata), // output [63 : 0] m_axi_rdata
      .m_axi_rresp(m_axi_rresp), // output [1 : 0] m_axi_rresp
      .m_axi_rlast(m_axi_rlast), // output m_axi_rlast
      .m_axi_rvalid(m_axi_rvalid), // output m_axi_rvalid
      .m_axi_rready(m_axi_rready), // input m_axi_rready
      .m_axi_ruser(),
      //
      // DMA interface for Write transaction
      //
      .write_addr(write_addr),       // Byte address for start of write transaction (should be 64bit alligned)
      .write_count(write_count),       // Count of 64bit words to write.
      .write_ctrl_valid(write_ctrl_valid),
      .write_ctrl_ready(write_ctrl_ready),
      .write_data(i_tdata_input),
      .write_data_valid(i_tvalid_input),
      .write_data_ready(i_tready_input),
      //
      // DMA interface for Read
      //
      .read_addr(read_addr),       // Byte address for start of read transaction (should be 64bit alligned)
      .read_count(read_count),       // Count of 64bit words to read.
      .read_ctrl_valid(read_ctrl_valid),
      .read_ctrl_ready(read_ctrl_ready),
      .read_data(o_tdata_output),
      .read_data_valid(o_tvalid_output),
      .read_data_ready(o_tready_output),
      //
      // Debug
      //
      .debug(debug_axi_dma_master)
      );

   //
   // Debug
   //
   assign debug = { checksum_error, 
		    /*debug_axi_dma_master[7:0]*/
		    input_timeout_triggered, // 195
		    input_state[2:0], // 194-192
		    output_timeout_triggered, // 191
		    output_state[2:0],    // 190-188
		    space_output[15:0],   // 187-172
		    occupied[21:0],       // 171-150
		    occupied_input[15:0], // 149-134

   		    i_tvalid_i0,     // 133
		    i_tready_i0,     // 132
		    i_tlast_i0,      // 131
		    i_tdata_i0[63:0],// 130-67
		    o_tvalid_i1,     // 66
		    o_tready_i1,     // 65
		    o_tlast_i1,      // 64
		    o_tdata_i1[63:0] // 63-0
		    };

   
 endmodule // axi_dram_fifo

