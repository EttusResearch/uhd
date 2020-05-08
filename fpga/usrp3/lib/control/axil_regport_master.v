//
// Copyright 2016-2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// An AXI4-Lite read/write register port adapter
//
// Converts memory mapped flow controlled AXI4-Lite transactions into a much
// simpler non flow controlled write and read register bus.
//
// WRITE Transaction:
// - Transaction completes in one cycle
// - Valid, Strobe, Address and Data asserted in same cycle
//                      __    __    __    __
//   clk             __|  |__|  |__|  |__|  |__
//                            _____
//   reg_wr_req      ________|     |___________
//                            _____
//   reg_wr_keep     XXXXXXXX|_____|XXXXXXXXXXX
//                            _____
//   reg_wr_addr     XXXXXXXX|_____|XXXXXXXXXXX
//                            _____
//   reg_wr_data     XXXXXXXX|_____|XXXXXXXXXXX
//;
// READ Transaction:
// - Transaction request completes in one cycle, with valid and address assertion
// - Transaction response must complete in at least one cycle with resp and data
//   - resp must be asserted between 1 and pow(2, TIMEOUT) cycles otherwise the read will timeout
//                      __    __    __    __    __
//   clk             __|  |__|  |__|  |__|  |__|  |__
//                            _____
//   reg_rd_req      ________|     |_________________
//                            _____
//   reg_rd_addr     XXXXXXXX|_____|XXXXXXXXXXXXXXXXX
//                                        _____
//   reg_rd_resp     ____________________|     |_____
//                                        _____
//   reg_rd_data     XXXXXXXXXXXXXXXXXXXX|_____|XXXXX


module axil_regport_master #(
  parameter DWIDTH  = 32,    // Width of the AXI4-Lite data bus (must be 32 or 64)
  parameter AWIDTH  = 32,    // Width of the address bus
  parameter WRBASE  = 32'h0, // Write address base
  parameter RDBASE  = 32'h0, // Read address base
  parameter TIMEOUT = 10     // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
)(
  // Clock and reset
  input                      s_axi_aclk,
  input                      s_axi_aresetn,
  input                      reg_clk,
  // AXI4-Lite: Write address port (domain: s_axi_aclk)
  input      [AWIDTH-1:0]    s_axi_awaddr,
  input                      s_axi_awvalid,
  output reg                 s_axi_awready,
  // AXI4-Lite: Write data port (domain: s_axi_aclk)
  input      [DWIDTH-1:0]    s_axi_wdata,
  input      [DWIDTH/8-1:0]  s_axi_wstrb,
  input                      s_axi_wvalid,
  output reg                 s_axi_wready,
  // AXI4-Lite: Write response port (domain: s_axi_aclk)
  output reg [1:0]           s_axi_bresp,
  output reg                 s_axi_bvalid,
  input                      s_axi_bready,
  // AXI4-Lite: Read address port (domain: s_axi_aclk)
  input      [AWIDTH-1:0]    s_axi_araddr,
  input                      s_axi_arvalid,
  output reg                 s_axi_arready,
  // AXI4-Lite: Read data port (domain: s_axi_aclk)
  output reg [DWIDTH-1:0]    s_axi_rdata,
  output reg [1:0]           s_axi_rresp,
  output reg                 s_axi_rvalid,
  input                      s_axi_rready,
  // Register port: Write port (domain: reg_clk)
  output                     reg_wr_req,
  output     [AWIDTH-1:0]    reg_wr_addr,
  output     [DWIDTH-1:0]    reg_wr_data,
  output     [DWIDTH/8-1:0]  reg_wr_keep,
  // Register port: Read port (domain: reg_clk)
  output                     reg_rd_req,
  output     [AWIDTH-1:0]    reg_rd_addr,
  input                      reg_rd_resp,
  input      [DWIDTH-1:0]    reg_rd_data
);

  localparam ADDR_LSB = $clog2(DWIDTH/8); // Do not modify

  //----------------------------------------------------------
  // Write state machine
  //----------------------------------------------------------
  reg [AWIDTH-1:0]  wr_addr_cache;
  wire              wr_fifo_valid, wr_fifo_ready;
  wire [AWIDTH-1:0] wr_addr_rel = (s_axi_awaddr - WRBASE);

  // Generate s_axi_awready and latch write address
  always @(posedge s_axi_aclk) begin
     if (!s_axi_aresetn) begin
        s_axi_awready <= 1'b0;
        wr_addr_cache <= {AWIDTH{1'b0}};
     end else begin
        if (~s_axi_awready && s_axi_awvalid && s_axi_wvalid && wr_fifo_ready) begin
           s_axi_awready <= 1'b1;
           wr_addr_cache <= {wr_addr_rel[AWIDTH-1:ADDR_LSB], {ADDR_LSB{1'b0}}};
        end else begin
           s_axi_awready <= 1'b0;
        end
     end
  end

  // Generate s_axi_wready
  always @(posedge s_axi_aclk) begin
     if (!s_axi_aresetn) begin
        s_axi_wready <= 1'b0;
     end else begin
        if (~s_axi_wready && s_axi_wvalid && s_axi_awvalid && wr_fifo_ready) begin
           s_axi_wready <= 1'b1;
        end else begin
           s_axi_wready <= 1'b0;
        end
     end
  end

  // Generate write response
  assign wr_fifo_valid = s_axi_awready && s_axi_awvalid && s_axi_wready && s_axi_wvalid;

  reg [4:0]  unacked_writes; //sized big enough for SRL fifo (32 deep)
  always @(posedge s_axi_aclk) begin
     if (!s_axi_aresetn) begin
        s_axi_bvalid   <= 1'b0;
        s_axi_bresp    <= 2'b0;
        unacked_writes <= 0;
     end else begin
        s_axi_bresp <= 2'b0; // 'OKAY' response
        if (wr_fifo_valid && wr_fifo_ready) begin
           unacked_writes <= unacked_writes+1;
        end
        if (unacked_writes > 0) begin
           // indicates a valid write response is available
           s_axi_bvalid <= 1'b1;
        end
        if (s_axi_bready && s_axi_bvalid) begin
           if (wr_fifo_valid && wr_fifo_ready)
             unacked_writes <= unacked_writes;
           else
             unacked_writes <= unacked_writes-1;
           s_axi_bvalid <= 1'b0;
        end
     end
  end

   axi_fifo_2clk #( .WIDTH(DWIDTH/8 + AWIDTH + DWIDTH), .SIZE(0) ) wr_fifo_2clk_i (
      .reset(~s_axi_aresetn), .i_aclk(s_axi_aclk),
      .i_tdata({s_axi_wstrb, wr_addr_cache, s_axi_wdata}),
      .i_tvalid(wr_fifo_valid), .i_tready(wr_fifo_ready),
      .o_aclk(reg_clk),
      .o_tdata({reg_wr_keep, reg_wr_addr, reg_wr_data}),
      .o_tvalid(reg_wr_req), .o_tready(1'b1)
   );

   //----------------------------------------------------------
   // Read state machine
   //----------------------------------------------------------
   reg [TIMEOUT-1:0] read_pending_ctr = {TIMEOUT{1'b0}};
   wire              read_timed_out = (read_pending_ctr == {{(TIMEOUT-1){1'b0}}, 1'b1});
   wire [AWIDTH-1:0] rd_addr_rel = (s_axi_araddr - RDBASE);

   wire              rdreq_fifo_ready, rdresp_fifo_valid;
   wire [DWIDTH-1:0] rdresp_fifo_data;

   // Generate s_axi_arready and latch read address
   reg [4:0]  unacked_reads; //sized big enough for SRL fifo (32 deep)   
   always @(posedge s_axi_aclk) begin
      if (!s_axi_aresetn) begin
         s_axi_arready <= 1'b0;
         read_pending_ctr <= {TIMEOUT{1'b0}};
         unacked_reads <= 0;
      end else begin
         if (unacked_reads > 0) begin
            if (read_pending_ctr > 0) begin
               read_pending_ctr <= read_pending_ctr-1;
            end
         end else begin
            read_pending_ctr <= {TIMEOUT{1'b0}};
         end

         if (~s_axi_arready && s_axi_arvalid && rdreq_fifo_ready) begin
            s_axi_arready <= 1'b1;
            read_pending_ctr <= {TIMEOUT{1'b1}};
            unacked_reads <= unacked_reads+1;            
         end else begin
            s_axi_arready <= 1'b0;
         end
         
         if (s_axi_rvalid && s_axi_rready) begin
            if (~s_axi_arready && s_axi_arvalid && rdreq_fifo_ready) begin
               unacked_reads <= unacked_reads;
            end else begin
               unacked_reads <= unacked_reads-1;
            end
         end         
      end
   end

   // Perform read transaction
   always @(posedge s_axi_aclk) begin
      if (!s_axi_aresetn) begin
         s_axi_rvalid <= 1'b0;
         s_axi_rresp <= 2'b00;
         s_axi_rdata <= 0;
      end else begin
         if (unacked_reads > 0 && rdresp_fifo_valid && ~s_axi_rvalid) begin
            // Valid read data is available at the read data bus
            s_axi_rvalid <= 1'b1;
            s_axi_rresp <= 2'b00; // 'OKAY' response
            s_axi_rdata <= rdresp_fifo_data;
         end else if (unacked_reads > 0 && read_timed_out && ~s_axi_rvalid) begin
            // Read timed out. Assert error.
            s_axi_rvalid <= 1'b1;
            s_axi_rresp <= 2'b10; // 'SLVERR' response
            s_axi_rdata <= {DWIDTH{1'b1}};
         end else if (s_axi_rvalid && s_axi_rready) begin
             // Read data is accepted by the master
            s_axi_rvalid <= 1'b0;
         end
      end
   end

   axi_fifo_2clk #( .WIDTH(AWIDTH), .SIZE(0) ) readreq_fifo_2clk_i (
      .reset(~s_axi_aresetn), .i_aclk(s_axi_aclk),
      .i_tdata({rd_addr_rel[AWIDTH-1:ADDR_LSB], {ADDR_LSB{1'b0}}}),
      .i_tvalid(s_axi_arready && s_axi_arvalid), .i_tready(rdreq_fifo_ready),
      .o_aclk(reg_clk),
      .o_tdata(reg_rd_addr),
      .o_tvalid(reg_rd_req), .o_tready(1'b1)
   );

   axi_fifo_2clk #( .WIDTH(DWIDTH), .SIZE(0) ) rdresp_fifo_2clk_i (
      .reset(~s_axi_aresetn), .i_aclk(reg_clk),
      .i_tdata(reg_rd_data),
      .i_tvalid(reg_rd_resp), .i_tready(/* lossy */),
      .o_aclk(s_axi_aclk),
      .o_tdata(rdresp_fifo_data),
      .o_tvalid(rdresp_fifo_valid), .o_tready((unacked_reads==0) || (s_axi_rvalid && s_axi_rready && (s_axi_rresp == 2'b00)))
   );

endmodule
