//
// Copyright 2015 Ettus Research LLC
//

`timescale 1ns/1ps

module axis_dram_fifo_single
#(
  parameter USE_SRAM_MEMORY = 0,
  parameter USE_BD_INTERCON = 0,
  parameter SR_BASE = 0
) (
  input         bus_clk,
  input         bus_rst,
  input         sys_clk,
  input         sys_rst,

  input [63:0]  i_tdata,
  input         i_tlast,
  input         i_tvalid,
  output        i_tready,

  output [63:0] o_tdata,
  output        o_tlast,
  output        o_tvalid,
  input         o_tready,

  input         set_stb,
  input [7:0]   set_addr,
  input [31:0]  set_data,
  output [31:0] rb_data,

  input [63:0]  forced_bit_err,

  output        init_calib_complete
);

  // Misc declarations
  axi4_rd_t #(.DWIDTH(64), .AWIDTH(32), .IDWIDTH(1)) dma_axi_rd(.clk(sys_clk));
  axi4_wr_t #(.DWIDTH(64), .AWIDTH(32), .IDWIDTH(1)) dma_axi_wr(.clk(sys_clk));
  axi4_rd_t #(.DWIDTH(256), .AWIDTH(32), .IDWIDTH(1)) mig_axi_rd(.clk(sys_clk));
  axi4_wr_t #(.DWIDTH(256), .AWIDTH(32), .IDWIDTH(1)) mig_axi_wr(.clk(sys_clk));

  wire [31:0]   ddr3_dq;      // Data pins. Input for Reads; Output for Writes.
  wire [3:0]    ddr3_dqs_n;   // Data Strobes. Input for Reads; Output for Writes.
  wire [3:0]    ddr3_dqs_p;
  wire [14:0]   ddr3_addr;    // Address
  wire [2:0]    ddr3_ba;      // Bank Address
  wire          ddr3_ras_n;   // Row Address Strobe.
  wire          ddr3_cas_n;   // Column address select
  wire          ddr3_we_n;    // Write Enable
  wire          ddr3_reset_n; // SDRAM reset pin.
  wire [0:0]    ddr3_ck_p;    // Differential clock
  wire [0:0]    ddr3_ck_n;
  wire [0:0]    ddr3_cke;     // Clock Enable
  wire [0:0]    ddr3_cs_n;    // Chip Select
  wire [3:0]    ddr3_dm;      // Data Mask [3] = UDM.U26; [2] = LDM.U26; ...
  wire [0:0]    ddr3_odt;     // On-Die termination enable.

  wire          ddr3_axi_clk;       // 1/4 DDR external clock rate (250MHz)
  wire          ddr3_axi_rst;       // Synchronized to ddr_sys_clk
  reg           ddr3_axi_rst_reg_n; // Synchronized to ddr_sys_clk
  wire          ddr3_axi_clk_x2;

  always @(posedge ddr3_axi_clk)
    ddr3_axi_rst_reg_n <= ~ddr3_axi_rst;

  axi_dma_fifo #(
    .DEFAULT_BASE(30'h00010000),
    .DEFAULT_MASK(30'hFFFF0000),
    .DEFAULT_TIMEOUT(280),
    .SR_BASE(SR_BASE),
    .EXT_BIST(1),
    .SIMULATION(1)
  ) axi_dma_fifo_i0 (
    //
    // Clocks and reset
    .bus_clk                    (bus_clk),
    .bus_reset                  (bus_rst),
    .dram_clk                   (ddr3_axi_clk_x2),
    .dram_reset                 (ddr3_axi_rst),
    //
    // AXI Write address channel
    .m_axi_awid                 (dma_axi_wr.addr.id),
    .m_axi_awaddr               (dma_axi_wr.addr.addr),
    .m_axi_awlen                (dma_axi_wr.addr.len),
    .m_axi_awsize               (dma_axi_wr.addr.size),
    .m_axi_awburst              (dma_axi_wr.addr.burst),
    .m_axi_awlock               (dma_axi_wr.addr.lock),
    .m_axi_awcache              (dma_axi_wr.addr.cache),
    .m_axi_awprot               (dma_axi_wr.addr.prot),
    .m_axi_awqos                (dma_axi_wr.addr.qos),
    .m_axi_awregion             (dma_axi_wr.addr.region),
    .m_axi_awuser               (dma_axi_wr.addr.user),
    .m_axi_awvalid              (dma_axi_wr.addr.valid),
    .m_axi_awready              (dma_axi_wr.addr.ready),
    //
    // AXI Write data channel.
    .m_axi_wdata                (dma_axi_wr.data.data),
    .m_axi_wstrb                (dma_axi_wr.data.strb),
    .m_axi_wlast                (dma_axi_wr.data.last),
    .m_axi_wuser                (dma_axi_wr.data.user),
    .m_axi_wvalid               (dma_axi_wr.data.valid),
    .m_axi_wready               (dma_axi_wr.data.ready),
    //
    // AXI Write response channel signals
    .m_axi_bid                  (dma_axi_wr.resp.id),
    .m_axi_bresp                (dma_axi_wr.resp.resp),
    .m_axi_buser                (dma_axi_wr.resp.user),
    .m_axi_bvalid               (dma_axi_wr.resp.valid),
    .m_axi_bready               (dma_axi_wr.resp.ready),
    //
    // AXI Read address channel
    .m_axi_arid                 (dma_axi_rd.addr.id),
    .m_axi_araddr               (dma_axi_rd.addr.addr),
    .m_axi_arlen                (dma_axi_rd.addr.len),
    .m_axi_arsize               (dma_axi_rd.addr.size),
    .m_axi_arburst              (dma_axi_rd.addr.burst),
    .m_axi_arlock               (dma_axi_rd.addr.lock),
    .m_axi_arcache              (dma_axi_rd.addr.cache),
    .m_axi_arprot               (dma_axi_rd.addr.prot),
    .m_axi_arqos                (dma_axi_rd.addr.qos),
    .m_axi_arregion             (dma_axi_rd.addr.region),
    .m_axi_aruser               (dma_axi_rd.addr.user),
    .m_axi_arvalid              (dma_axi_rd.addr.valid),
    .m_axi_arready              (dma_axi_rd.addr.ready),
    //
    // AXI Read data channel
    .m_axi_rid                  (dma_axi_rd.data.id),
    .m_axi_rdata                (dma_axi_rd.data.data),
    .m_axi_rresp                (dma_axi_rd.data.resp),
    .m_axi_rlast                (dma_axi_rd.data.last),
    .m_axi_ruser                (dma_axi_rd.data.user),
    .m_axi_rvalid               (dma_axi_rd.data.valid),
    .m_axi_rready               (dma_axi_rd.data.ready),
    //
    // CHDR friendly AXI stream input
    .i_tdata                    (i_tdata),
    .i_tlast                    (i_tlast),
    .i_tvalid                   (i_tvalid),
    .i_tready                   (i_tready),
    //
    // CHDR friendly AXI Stream output
    .o_tdata                    (o_tdata),
    .o_tlast                    (o_tlast),
    .o_tvalid                   (o_tvalid),
    .o_tready                   (o_tready),
    //
    // Settings
    .set_stb                    (set_stb),
    .set_addr                   (set_addr),
    .set_data                   (set_data),
    .rb_data                    (rb_data),

    .debug()
  );

  generate if (USE_SRAM_MEMORY) begin
    assign init_calib_complete  = 1;
    assign ddr3_axi_clk         = bus_clk;
    assign ddr3_axi_clk_x2      = bus_clk;
    assign ddr3_axi_rst         = bus_rst;

    axi4_dualport_sram axi4_dualport_sram_i1 (
      .s_aclk         (ddr3_axi_clk_x2), // input s_aclk
      .s_aresetn      (~ddr3_axi_rst), // input s_aresetn
      .s_axi_awid     (dma_axi_wr.addr.id), // input [0 : 0] s_axi_awid
      .s_axi_awaddr   (dma_axi_wr.addr.addr), // input [31 : 0] s_axi_awaddr
      .s_axi_awlen    (dma_axi_wr.addr.len), // input [7 : 0] s_axi_awlen
      .s_axi_awsize   (dma_axi_wr.addr.size), // input [2 : 0] s_axi_awsize
      .s_axi_awburst  (dma_axi_wr.addr.burst), // input [1 : 0] s_axi_awburst
      .s_axi_awvalid  (dma_axi_wr.addr.valid), // input s_axi_awvalid
      .s_axi_awready  (dma_axi_wr.addr.ready), // output s_axi_awready
      .s_axi_wdata    (dma_axi_wr.data.data ^ forced_bit_err), // input [63 : 0] s_axi_wdata
      .s_axi_wstrb    (dma_axi_wr.data.strb), // input [7 : 0] s_axi_wstrb
      .s_axi_wlast    (dma_axi_wr.data.last), // input s_axi_wlast
      .s_axi_wvalid   (dma_axi_wr.data.valid), // input s_axi_wvalid
      .s_axi_wready   (dma_axi_wr.data.ready), // output s_axi_wready
      .s_axi_bid      (dma_axi_wr.resp.id), // output [0 : 0] s_axi_bid
      .s_axi_bresp    (dma_axi_wr.resp.resp), // output [1 : 0] s_axi_bresp
      .s_axi_bvalid   (dma_axi_wr.resp.valid), // output s_axi_bvalid
      .s_axi_bready   (dma_axi_wr.resp.ready), // input s_axi_bready
      .s_axi_arid     (dma_axi_rd.addr.id), // input [0 : 0] s_axi_arid
      .s_axi_araddr   (dma_axi_rd.addr.addr), // input [31 : 0] s_axi_araddr
      .s_axi_arlen    (dma_axi_rd.addr.len), // input [7 : 0] s_axi_arlen
      .s_axi_arsize   (dma_axi_rd.addr.size), // input [2 : 0] s_axi_arsize
      .s_axi_arburst  (dma_axi_rd.addr.burst), // input [1 : 0] s_axi_arburst
      .s_axi_arvalid  (dma_axi_rd.addr.valid), // input s_axi_arvalid
      .s_axi_arready  (dma_axi_rd.addr.ready), // output s_axi_arready
      .s_axi_rid      (dma_axi_rd.data.id), // output [0 : 0] s_axi_rid
      .s_axi_rdata    (dma_axi_rd.data.data), // output [63 : 0] s_axi_rdata
      .s_axi_rresp    (dma_axi_rd.data.resp), // output [1 : 0] s_axi_rresp
      .s_axi_rlast    (dma_axi_rd.data.last), // output s_axi_rlast
      .s_axi_rvalid   (dma_axi_rd.data.valid), // output s_axi_rvalid
      .s_axi_rready   (dma_axi_rd.data.ready) // input s_axi_rready
    );

  end else begin  //generate if (USE_SRAM_MEMORY) begin
    //---------------------------------------------------
    // We use an interconnect to connect to FIFOs.
    //---------------------------------------------------
    if (USE_BD_INTERCON) begin
    // Vivado Block Diagram interconnect.
    axi_intercon_2x64_128_bd_wrapper axi_intercon_2x64_128_i (
      .S00_AXI_ACLK                         (ddr3_axi_clk_x2), // input S00_AXI_ACLK
      .S00_AXI_ARESETN                      (~ddr3_axi_rst), // input S00_AXI_ARESETN
      .S00_AXI_AWID                         (dma_axi_wr.addr.id), // input [0 : 0] S00_AXI_AWID
      .S00_AXI_AWADDR                       (dma_axi_wr.addr.addr), // input [31 : 0] S00_AXI_AWADDR
      .S00_AXI_AWLEN                        (dma_axi_wr.addr.len), // input [7 : 0] S00_AXI_AWLEN
      .S00_AXI_AWSIZE                       (dma_axi_wr.addr.size), // input [2 : 0] S00_AXI_AWSIZE
      .S00_AXI_AWBURST                      (dma_axi_wr.addr.burst), // input [1 : 0] S00_AXI_AWBURST
      .S00_AXI_AWLOCK                       (dma_axi_wr.addr.lock), // input S00_AXI_AWLOCK
      .S00_AXI_AWCACHE                      (dma_axi_wr.addr.cache), // input [3 : 0] S00_AXI_AWCACHE
      .S00_AXI_AWPROT                       (dma_axi_wr.addr.prot), // input [2 : 0] S00_AXI_AWPROT
      .S00_AXI_AWQOS                        (dma_axi_wr.addr.qos), // input [3 : 0] S00_AXI_AWQOS
      .S00_AXI_AWVALID                      (dma_axi_wr.addr.valid), // input S00_AXI_AWVALID
      .S00_AXI_AWREADY                      (dma_axi_wr.addr.ready), // output S00_AXI_AWREADY
      .S00_AXI_WDATA                        (dma_axi_wr.data.data ^ forced_bit_err), // input [63 : 0] S00_AXI_WDATA
      .S00_AXI_WSTRB                        (dma_axi_wr.data.strb), // input [7 : 0] S00_AXI_WSTRB
      .S00_AXI_WLAST                        (dma_axi_wr.data.last), // input S00_AXI_WLAST
      .S00_AXI_WVALID                       (dma_axi_wr.data.valid), // input S00_AXI_WVALID
      .S00_AXI_WREADY                       (dma_axi_wr.data.ready), // output S00_AXI_WREADY
      .S00_AXI_BID                          (dma_axi_wr.resp.id), // output [0 : 0] S00_AXI_BID
      .S00_AXI_BRESP                        (dma_axi_wr.resp.resp), // output [1 : 0] S00_AXI_BRESP
      .S00_AXI_BVALID                       (dma_axi_wr.resp.valid), // output S00_AXI_BVALID
      .S00_AXI_BREADY                       (dma_axi_wr.resp.ready), // input S00_AXI_BREADY
      .S00_AXI_ARID                         (dma_axi_rd.addr.id), // input [0 : 0] S00_AXI_ARID
      .S00_AXI_ARADDR                       (dma_axi_rd.addr.addr), // input [31 : 0] S00_AXI_ARADDR
      .S00_AXI_ARLEN                        (dma_axi_rd.addr.len), // input [7 : 0] S00_AXI_ARLEN
      .S00_AXI_ARSIZE                       (dma_axi_rd.addr.size), // input [2 : 0] S00_AXI_ARSIZE
      .S00_AXI_ARBURST                      (dma_axi_rd.addr.burst), // input [1 : 0] S00_AXI_ARBURST
      .S00_AXI_ARLOCK                       (dma_axi_rd.addr.lock), // input S00_AXI_ARLOCK
      .S00_AXI_ARCACHE                      (dma_axi_rd.addr.cache), // input [3 : 0] S00_AXI_ARCACHE
      .S00_AXI_ARPROT                       (dma_axi_rd.addr.prot), // input [2 : 0] S00_AXI_ARPROT
      .S00_AXI_ARQOS                        (dma_axi_rd.addr.qos), // input [3 : 0] S00_AXI_ARQOS
      .S00_AXI_ARVALID                      (dma_axi_rd.addr.valid), // input S00_AXI_ARVALID
      .S00_AXI_ARREADY                      (dma_axi_rd.addr.ready), // output S00_AXI_ARREADY
      .S00_AXI_RID                          (dma_axi_rd.data.id), // output [0 : 0] S00_AXI_RID
      .S00_AXI_RDATA                        (dma_axi_rd.data.data), // output [63 : 0] S00_AXI_RDATA
      .S00_AXI_RRESP                        (dma_axi_rd.data.resp), // output [1 : 0] S00_AXI_RRESP
      .S00_AXI_RLAST                        (dma_axi_rd.data.last), // output S00_AXI_RLAST
      .S00_AXI_RVALID                       (dma_axi_rd.data.valid), // output S00_AXI_RVALID
      .S00_AXI_RREADY                       (dma_axi_rd.data.ready), // input S00_AXI_RREADY
      //
      //.S01_AXI_ARESET_OUT_N                 (), // output S01_AXI_ARESET_OUT_N
      .S01_AXI_ACLK                         (ddr3_axi_clk_x2), // input S01_AXI_ACLK
      .S01_AXI_ARESETN                      (~ddr3_axi_rst), // input S01_AXI_ARESETN
      .S01_AXI_AWID                         (0), // input [0 : 0] S01_AXI_AWID
      .S01_AXI_AWADDR                       (0), // input [31 : 0] S01_AXI_AWADDR
      .S01_AXI_AWLEN                        (0), // input [7 : 0] S01_AXI_AWLEN
      .S01_AXI_AWSIZE                       (0), // input [2 : 0] S01_AXI_AWSIZE
      .S01_AXI_AWBURST                      (0), // input [1 : 0] S01_AXI_AWBURST
      .S01_AXI_AWLOCK                       (0), // input S01_AXI_AWLOCK
      .S01_AXI_AWCACHE                      (0), // input [3 : 0] S01_AXI_AWCACHE
      .S01_AXI_AWPROT                       (0), // input [2 : 0] S01_AXI_AWPROT
      .S01_AXI_AWQOS                        (0), // input [3 : 0] S01_AXI_AWQOS
      .S01_AXI_AWVALID                      (0), // input S01_AXI_AWVALID
      .S01_AXI_AWREADY                      (), // output S01_AXI_AWREADY
      .S01_AXI_WDATA                        (0), // input [63 : 0] S01_AXI_WDATA
      .S01_AXI_WSTRB                        (0), // input [7 : 0] S01_AXI_WSTRB
      .S01_AXI_WLAST                        (0), // input S01_AXI_WLAST
      .S01_AXI_WVALID                       (0), // input S01_AXI_WVALID
      .S01_AXI_WREADY                       (), // output S01_AXI_WREADY
      .S01_AXI_BID                          (), // output [0 : 0] S01_AXI_BID
      .S01_AXI_BRESP                        (), // output [1 : 0] S01_AXI_BRESP
      .S01_AXI_BVALID                       (), // output S01_AXI_BVALID
      .S01_AXI_BREADY                       (0), // input S01_AXI_BREADY
      .S01_AXI_ARID                         (0), // input [0 : 0] S01_AXI_ARID
      .S01_AXI_ARADDR                       (0), // input [31 : 0] S01_AXI_ARADDR
      .S01_AXI_ARLEN                        (0), // input [7 : 0] S01_AXI_ARLEN
      .S01_AXI_ARSIZE                       (0), // input [2 : 0] S01_AXI_ARSIZE
      .S01_AXI_ARBURST                      (0), // input [1 : 0] S01_AXI_ARBURST
      .S01_AXI_ARLOCK                       (0), // input S01_AXI_ARLOCK
      .S01_AXI_ARCACHE                      (0), // input [3 : 0] S01_AXI_ARCACHE
      .S01_AXI_ARPROT                       (0), // input [2 : 0] S01_AXI_ARPROT
      .S01_AXI_ARQOS                        (0), // input [3 : 0] S01_AXI_ARQOS
      .S01_AXI_ARVALID                      (0), // input S01_AXI_ARVALID
      .S01_AXI_ARREADY                      (), // output S01_AXI_ARREADY
      .S01_AXI_RID                          (), // output [0 : 0] S01_AXI_RID
      .S01_AXI_RDATA                        (), // output [63 : 0] S01_AXI_RDATA
      .S01_AXI_RRESP                        (), // output [1 : 0] S01_AXI_RRESP
      .S01_AXI_RLAST                        (), // output S01_AXI_RLAST
      .S01_AXI_RVALID                       (), // output S01_AXI_RVALID
      .S01_AXI_RREADY                       (0), // input S01_AXI_RREADY
      //
      //.M00_AXI_ARESET_OUT_N                 (), // output M00_AXI_ARESET_OUT_N
      .M00_AXI_ACLK                         (ddr3_axi_clk), // input M00_AXI_ACLK
      .M00_AXI_ARESETN                      (~ddr3_axi_rst), // input M00_AXI_ARESETN
      .M00_AXI_AWID                         (mig_axi_wr.addr.id), // output [3 : 0] M00_AXI_AWID
      .M00_AXI_AWADDR                       (mig_axi_wr.addr.addr), // output [31 : 0] M00_AXI_AWADDR
      .M00_AXI_AWLEN                        (mig_axi_wr.addr.len), // output [7 : 0] M00_AXI_AWLEN
      .M00_AXI_AWSIZE                       (mig_axi_wr.addr.size), // output [2 : 0] M00_AXI_AWSIZE
      .M00_AXI_AWBURST                      (mig_axi_wr.addr.burst), // output [1 : 0] M00_AXI_AWBURST
      .M00_AXI_AWLOCK                       (mig_axi_wr.addr.lock), // output M00_AXI_AWLOCK
      .M00_AXI_AWCACHE                      (mig_axi_wr.addr.cache), // output [3 : 0] M00_AXI_AWCACHE
      .M00_AXI_AWPROT                       (mig_axi_wr.addr.prot), // output [2 : 0] M00_AXI_AWPROT
      .M00_AXI_AWQOS                        (mig_axi_wr.addr.qos), // output [3 : 0] M00_AXI_AWQOS
      .M00_AXI_AWVALID                      (mig_axi_wr.addr.valid), // output M00_AXI_AWVALID
      .M00_AXI_AWREADY                      (mig_axi_wr.addr.ready), // input M00_AXI_AWREADY
      .M00_AXI_WDATA                        (mig_axi_wr.data.data), // output [127 : 0] M00_AXI_WDATA
      .M00_AXI_WSTRB                        (mig_axi_wr.data.strb), // output [15 : 0] M00_AXI_WSTRB
      .M00_AXI_WLAST                        (mig_axi_wr.data.last), // output M00_AXI_WLAST
      .M00_AXI_WVALID                       (mig_axi_wr.data.valid), // output M00_AXI_WVALID
      .M00_AXI_WREADY                       (mig_axi_wr.data.ready), // input M00_AXI_WREADY
      .M00_AXI_BID                          (mig_axi_wr.resp.id), // input [3 : 0] M00_AXI_BID
      .M00_AXI_BRESP                        (mig_axi_wr.resp.resp), // input [1 : 0] M00_AXI_BRESP
      .M00_AXI_BVALID                       (mig_axi_wr.resp.valid), // input M00_AXI_BVALID
      .M00_AXI_BREADY                       (mig_axi_wr.resp.ready), // output M00_AXI_BREADY
      .M00_AXI_ARID                         (mig_axi_rd.addr.id), // output [3 : 0] M00_AXI_ARID
      .M00_AXI_ARADDR                       (mig_axi_rd.addr.addr), // output [31 : 0] M00_AXI_ARADDR
      .M00_AXI_ARLEN                        (mig_axi_rd.addr.len), // output [7 : 0] M00_AXI_ARLEN
      .M00_AXI_ARSIZE                       (mig_axi_rd.addr.size), // output [2 : 0] M00_AXI_ARSIZE
      .M00_AXI_ARBURST                      (mig_axi_rd.addr.burst), // output [1 : 0] M00_AXI_ARBURST
      .M00_AXI_ARLOCK                       (mig_axi_rd.addr.lock), // output M00_AXI_ARLOCK
      .M00_AXI_ARCACHE                      (mig_axi_rd.addr.cache), // output [3 : 0] M00_AXI_ARCACHE
      .M00_AXI_ARPROT                       (mig_axi_rd.addr.prot), // output [2 : 0] M00_AXI_ARPROT
      .M00_AXI_ARQOS                        (mig_axi_rd.addr.qos), // output [3 : 0] M00_AXI_ARQOS
      .M00_AXI_ARVALID                      (mig_axi_rd.addr.valid), // output M00_AXI_ARVALID
      .M00_AXI_ARREADY                      (mig_axi_rd.addr.ready), // input M00_AXI_ARREADY
      .M00_AXI_RID                          (mig_axi_rd.data.id), // input [3 : 0] M00_AXI_RID
      .M00_AXI_RDATA                        (mig_axi_rd.data.data), // input [127 : 0] M00_AXI_RDATA
      .M00_AXI_RRESP                        (mig_axi_rd.data.resp), // input [1 : 0] M00_AXI_RRESP
      .M00_AXI_RLAST                        (mig_axi_rd.data.last), // input M00_AXI_RLAST
      .M00_AXI_RVALID                       (mig_axi_rd.data.valid), // input M00_AXI_RVALID
      .M00_AXI_RREADY                       (mig_axi_rd.data.ready) // output M00_AXI_RREADY
    );
    end else begin
    //Original IP interconnect
    axi_intercon_2x64_128 axi_intercon_2x64_128_i (
      .INTERCONNECT_ACLK(ddr3_axi_clk_x2), // input INTERCONNECT_ACLK
      .INTERCONNECT_ARESETN(~ddr3_axi_rst), // input INTERCONNECT_ARESETN
      //
      .S00_AXI_ARESET_OUT_N                 (), // output S00_AXI_ARESET_OUT_N
      .S00_AXI_ACLK                         (ddr3_axi_clk_x2), // input S00_AXI_ACLK
      .S00_AXI_AWID                         (dma_axi_wr.addr.id), // input [0 : 0] S00_AXI_AWID
      .S00_AXI_AWADDR                       (dma_axi_wr.addr.addr), // input [31 : 0] S00_AXI_AWADDR
      .S00_AXI_AWLEN                        (dma_axi_wr.addr.len), // input [7 : 0] S00_AXI_AWLEN
      .S00_AXI_AWSIZE                       (dma_axi_wr.addr.size), // input [2 : 0] S00_AXI_AWSIZE
      .S00_AXI_AWBURST                      (dma_axi_wr.addr.burst), // input [1 : 0] S00_AXI_AWBURST
      .S00_AXI_AWLOCK                       (dma_axi_wr.addr.lock), // input S00_AXI_AWLOCK
      .S00_AXI_AWCACHE                      (dma_axi_wr.addr.cache), // input [3 : 0] S00_AXI_AWCACHE
      .S00_AXI_AWPROT                       (dma_axi_wr.addr.prot), // input [2 : 0] S00_AXI_AWPROT
      .S00_AXI_AWQOS                        (dma_axi_wr.addr.qos), // input [3 : 0] S00_AXI_AWQOS
      .S00_AXI_AWVALID                      (dma_axi_wr.addr.valid), // input S00_AXI_AWVALID
      .S00_AXI_AWREADY                      (dma_axi_wr.addr.ready), // output S00_AXI_AWREADY
      .S00_AXI_WDATA                        (dma_axi_wr.data.data ^ forced_bit_err), // input [63 : 0] S00_AXI_WDATA
      .S00_AXI_WSTRB                        (dma_axi_wr.data.strb), // input [7 : 0] S00_AXI_WSTRB
      .S00_AXI_WLAST                        (dma_axi_wr.data.last), // input S00_AXI_WLAST
      .S00_AXI_WVALID                       (dma_axi_wr.data.valid), // input S00_AXI_WVALID
      .S00_AXI_WREADY                       (dma_axi_wr.data.ready), // output S00_AXI_WREADY
      .S00_AXI_BID                          (dma_axi_wr.resp.id), // output [0 : 0] S00_AXI_BID
      .S00_AXI_BRESP                        (dma_axi_wr.resp.resp), // output [1 : 0] S00_AXI_BRESP
      .S00_AXI_BVALID                       (dma_axi_wr.resp.valid), // output S00_AXI_BVALID
      .S00_AXI_BREADY                       (dma_axi_wr.resp.ready), // input S00_AXI_BREADY
      .S00_AXI_ARID                         (dma_axi_rd.addr.id), // input [0 : 0] S00_AXI_ARID
      .S00_AXI_ARADDR                       (dma_axi_rd.addr.addr), // input [31 : 0] S00_AXI_ARADDR
      .S00_AXI_ARLEN                        (dma_axi_rd.addr.len), // input [7 : 0] S00_AXI_ARLEN
      .S00_AXI_ARSIZE                       (dma_axi_rd.addr.size), // input [2 : 0] S00_AXI_ARSIZE
      .S00_AXI_ARBURST                      (dma_axi_rd.addr.burst), // input [1 : 0] S00_AXI_ARBURST
      .S00_AXI_ARLOCK                       (dma_axi_rd.addr.lock), // input S00_AXI_ARLOCK
      .S00_AXI_ARCACHE                      (dma_axi_rd.addr.cache), // input [3 : 0] S00_AXI_ARCACHE
      .S00_AXI_ARPROT                       (dma_axi_rd.addr.prot), // input [2 : 0] S00_AXI_ARPROT
      .S00_AXI_ARQOS                        (dma_axi_rd.addr.qos), // input [3 : 0] S00_AXI_ARQOS
      .S00_AXI_ARVALID                      (dma_axi_rd.addr.valid), // input S00_AXI_ARVALID
      .S00_AXI_ARREADY                      (dma_axi_rd.addr.ready), // output S00_AXI_ARREADY
      .S00_AXI_RID                          (dma_axi_rd.data.id), // output [0 : 0] S00_AXI_RID
      .S00_AXI_RDATA                        (dma_axi_rd.data.data), // output [63 : 0] S00_AXI_RDATA
      .S00_AXI_RRESP                        (dma_axi_rd.data.resp), // output [1 : 0] S00_AXI_RRESP
      .S00_AXI_RLAST                        (dma_axi_rd.data.last), // output S00_AXI_RLAST
      .S00_AXI_RVALID                       (dma_axi_rd.data.valid), // output S00_AXI_RVALID
      .S00_AXI_RREADY                       (dma_axi_rd.data.ready), // input S00_AXI_RREADY
      //
      .S01_AXI_ARESET_OUT_N                 (), // output S01_AXI_ARESET_OUT_N
      .S01_AXI_ACLK                         (ddr3_axi_clk_x2), // input S01_AXI_ACLK
      .S01_AXI_AWID                         (0), // input [0 : 0] S01_AXI_AWID
      .S01_AXI_AWADDR                       (0), // input [31 : 0] S01_AXI_AWADDR
      .S01_AXI_AWLEN                        (0), // input [7 : 0] S01_AXI_AWLEN
      .S01_AXI_AWSIZE                       (0), // input [2 : 0] S01_AXI_AWSIZE
      .S01_AXI_AWBURST                      (0), // input [1 : 0] S01_AXI_AWBURST
      .S01_AXI_AWLOCK                       (0), // input S01_AXI_AWLOCK
      .S01_AXI_AWCACHE                      (0), // input [3 : 0] S01_AXI_AWCACHE
      .S01_AXI_AWPROT                       (0), // input [2 : 0] S01_AXI_AWPROT
      .S01_AXI_AWQOS                        (0), // input [3 : 0] S01_AXI_AWQOS
      .S01_AXI_AWVALID                      (0), // input S01_AXI_AWVALID
      .S01_AXI_AWREADY                      (), // output S01_AXI_AWREADY
      .S01_AXI_WDATA                        (0), // input [63 : 0] S01_AXI_WDATA
      .S01_AXI_WSTRB                        (0), // input [7 : 0] S01_AXI_WSTRB
      .S01_AXI_WLAST                        (0), // input S01_AXI_WLAST
      .S01_AXI_WVALID                       (0), // input S01_AXI_WVALID
      .S01_AXI_WREADY                       (), // output S01_AXI_WREADY
      .S01_AXI_BID                          (), // output [0 : 0] S01_AXI_BID
      .S01_AXI_BRESP                        (), // output [1 : 0] S01_AXI_BRESP
      .S01_AXI_BVALID                       (), // output S01_AXI_BVALID
      .S01_AXI_BREADY                       (0), // input S01_AXI_BREADY
      .S01_AXI_ARID                         (0), // input [0 : 0] S01_AXI_ARID
      .S01_AXI_ARADDR                       (0), // input [31 : 0] S01_AXI_ARADDR
      .S01_AXI_ARLEN                        (0), // input [7 : 0] S01_AXI_ARLEN
      .S01_AXI_ARSIZE                       (0), // input [2 : 0] S01_AXI_ARSIZE
      .S01_AXI_ARBURST                      (0), // input [1 : 0] S01_AXI_ARBURST
      .S01_AXI_ARLOCK                       (0), // input S01_AXI_ARLOCK
      .S01_AXI_ARCACHE                      (0), // input [3 : 0] S01_AXI_ARCACHE
      .S01_AXI_ARPROT                       (0), // input [2 : 0] S01_AXI_ARPROT
      .S01_AXI_ARQOS                        (0), // input [3 : 0] S01_AXI_ARQOS
      .S01_AXI_ARVALID                      (0), // input S01_AXI_ARVALID
      .S01_AXI_ARREADY                      (), // output S01_AXI_ARREADY
      .S01_AXI_RID                          (), // output [0 : 0] S01_AXI_RID
      .S01_AXI_RDATA                        (), // output [63 : 0] S01_AXI_RDATA
      .S01_AXI_RRESP                        (), // output [1 : 0] S01_AXI_RRESP
      .S01_AXI_RLAST                        (), // output S01_AXI_RLAST
      .S01_AXI_RVALID                       (), // output S01_AXI_RVALID
      .S01_AXI_RREADY                       (0), // input S01_AXI_RREADY
      //
      .M00_AXI_ARESET_OUT_N                 (), // output M00_AXI_ARESET_OUT_N
      .M00_AXI_ACLK                         (ddr3_axi_clk), // input M00_AXI_ACLK
      .M00_AXI_AWID                         (mig_axi_wr.addr.id), // output [3 : 0] M00_AXI_AWID
      .M00_AXI_AWADDR                       (mig_axi_wr.addr.addr), // output [31 : 0] M00_AXI_AWADDR
      .M00_AXI_AWLEN                        (mig_axi_wr.addr.len), // output [7 : 0] M00_AXI_AWLEN
      .M00_AXI_AWSIZE                       (mig_axi_wr.addr.size), // output [2 : 0] M00_AXI_AWSIZE
      .M00_AXI_AWBURST                      (mig_axi_wr.addr.burst), // output [1 : 0] M00_AXI_AWBURST
      .M00_AXI_AWLOCK                       (mig_axi_wr.addr.lock), // output M00_AXI_AWLOCK
      .M00_AXI_AWCACHE                      (mig_axi_wr.addr.cache), // output [3 : 0] M00_AXI_AWCACHE
      .M00_AXI_AWPROT                       (mig_axi_wr.addr.prot), // output [2 : 0] M00_AXI_AWPROT
      .M00_AXI_AWQOS                        (mig_axi_wr.addr.qos), // output [3 : 0] M00_AXI_AWQOS
      .M00_AXI_AWVALID                      (mig_axi_wr.addr.valid), // output M00_AXI_AWVALID
      .M00_AXI_AWREADY                      (mig_axi_wr.addr.ready), // input M00_AXI_AWREADY
      .M00_AXI_WDATA                        (mig_axi_wr.data.data), // output [127 : 0] M00_AXI_WDATA
      .M00_AXI_WSTRB                        (mig_axi_wr.data.strb), // output [15 : 0] M00_AXI_WSTRB
      .M00_AXI_WLAST                        (mig_axi_wr.data.last), // output M00_AXI_WLAST
      .M00_AXI_WVALID                       (mig_axi_wr.data.valid), // output M00_AXI_WVALID
      .M00_AXI_WREADY                       (mig_axi_wr.data.ready), // input M00_AXI_WREADY
      .M00_AXI_BID                          (mig_axi_wr.resp.id), // input [3 : 0] M00_AXI_BID
      .M00_AXI_BRESP                        (mig_axi_wr.resp.resp), // input [1 : 0] M00_AXI_BRESP
      .M00_AXI_BVALID                       (mig_axi_wr.resp.valid), // input M00_AXI_BVALID
      .M00_AXI_BREADY                       (mig_axi_wr.resp.ready), // output M00_AXI_BREADY
      .M00_AXI_ARID                         (mig_axi_rd.addr.id), // output [3 : 0] M00_AXI_ARID
      .M00_AXI_ARADDR                       (mig_axi_rd.addr.addr), // output [31 : 0] M00_AXI_ARADDR
      .M00_AXI_ARLEN                        (mig_axi_rd.addr.len), // output [7 : 0] M00_AXI_ARLEN
      .M00_AXI_ARSIZE                       (mig_axi_rd.addr.size), // output [2 : 0] M00_AXI_ARSIZE
      .M00_AXI_ARBURST                      (mig_axi_rd.addr.burst), // output [1 : 0] M00_AXI_ARBURST
      .M00_AXI_ARLOCK                       (mig_axi_rd.addr.lock), // output M00_AXI_ARLOCK
      .M00_AXI_ARCACHE                      (mig_axi_rd.addr.cache), // output [3 : 0] M00_AXI_ARCACHE
      .M00_AXI_ARPROT                       (mig_axi_rd.addr.prot), // output [2 : 0] M00_AXI_ARPROT
      .M00_AXI_ARQOS                        (mig_axi_rd.addr.qos), // output [3 : 0] M00_AXI_ARQOS
      .M00_AXI_ARVALID                      (mig_axi_rd.addr.valid), // output M00_AXI_ARVALID
      .M00_AXI_ARREADY                      (mig_axi_rd.addr.ready), // input M00_AXI_ARREADY
      .M00_AXI_RID                          (mig_axi_rd.data.id), // input [3 : 0] M00_AXI_RID
      .M00_AXI_RDATA                        (mig_axi_rd.data.data), // input [127 : 0] M00_AXI_RDATA
      .M00_AXI_RRESP                        (mig_axi_rd.data.resp), // input [1 : 0] M00_AXI_RRESP
      .M00_AXI_RLAST                        (mig_axi_rd.data.last), // input M00_AXI_RLAST
      .M00_AXI_RVALID                       (mig_axi_rd.data.valid), // input M00_AXI_RVALID
      .M00_AXI_RREADY                       (mig_axi_rd.data.ready) // output M00_AXI_RREADY
    );
    end

    //---------------------------------------------------
    // MIG
    //---------------------------------------------------
    wire ddr3_idelay_refclk;

    ddr3_32bit ddr_mig_i (
      // Memory interface ports
      .ddr3_addr                      (ddr3_addr),
      .ddr3_ba                        (ddr3_ba),
      .ddr3_cas_n                     (ddr3_cas_n),
      .ddr3_ck_n                      (ddr3_ck_n),
      .ddr3_ck_p                      (ddr3_ck_p),
      .ddr3_cke                       (ddr3_cke),
      .ddr3_ras_n                     (ddr3_ras_n),
      .ddr3_reset_n                   (ddr3_reset_n),
      .ddr3_we_n                      (ddr3_we_n),
      .ddr3_dq                        (ddr3_dq),
      .ddr3_dqs_n                     (ddr3_dqs_n),
      .ddr3_dqs_p                     (ddr3_dqs_p),
      .init_calib_complete            (init_calib_complete),

      .ddr3_cs_n                      (ddr3_cs_n),
      .ddr3_dm                        (ddr3_dm),
      .ddr3_odt                       (ddr3_odt),
      // Application interface ports
      .ui_clk                         (ddr3_axi_clk),    // 150MHz clock out
      .ui_addn_clk_0                  (ddr3_axi_clk_x2), // 300MHz clock out
      .ui_addn_clk_1                  (ddr3_idelay_refclk),
      .ui_addn_clk_2                  (),
      .ui_addn_clk_3                  (),
      .ui_addn_clk_4                  (),
      .clk_ref_i                      (ddr3_idelay_refclk),
      .ui_clk_sync_rst                (ddr3_axi_rst),  // Active high Reset signal synchronised to 150MHz
      .aresetn                        (ddr3_axi_rst_reg_n),
      .app_sr_req                     (1'b0),
      .app_sr_active                  (),
      .app_ref_req                    (1'b0),
      .app_ref_ack                    (),
      .app_zq_req                     (1'b0),
      .app_zq_ack                     (),
      .device_temp_i                  (12'd0),
      // Slave Interface Write Address Ports
      .s_axi_awid                     (mig_axi_wr.addr.id),
      .s_axi_awaddr                   (mig_axi_wr.addr.addr),
      .s_axi_awlen                    (mig_axi_wr.addr.len),
      .s_axi_awsize                   (mig_axi_wr.addr.size),
      .s_axi_awburst                  (mig_axi_wr.addr.burst),
      .s_axi_awlock                   (mig_axi_wr.addr.lock),
      .s_axi_awcache                  (mig_axi_wr.addr.cache),
      .s_axi_awprot                   (mig_axi_wr.addr.prot),
      .s_axi_awqos                    (mig_axi_wr.addr.qos),
      .s_axi_awvalid                  (mig_axi_wr.addr.valid),
      .s_axi_awready                  (mig_axi_wr.addr.ready),
      // Slave Interface Write Data Ports
      .s_axi_wdata                    (mig_axi_wr.data.data),
      .s_axi_wstrb                    (mig_axi_wr.data.strb),
      .s_axi_wlast                    (mig_axi_wr.data.last),
      .s_axi_wvalid                   (mig_axi_wr.data.valid),
      .s_axi_wready                   (mig_axi_wr.data.ready),
      // Slave Interface Write Response Ports
      .s_axi_bid                      (mig_axi_wr.resp.id),
      .s_axi_bresp                    (mig_axi_wr.resp.resp),
      .s_axi_bvalid                   (mig_axi_wr.resp.valid),
      .s_axi_bready                   (mig_axi_wr.resp.ready),
      // Slave Interface Read Address Ports
      .s_axi_arid                     (mig_axi_rd.addr.id),
      .s_axi_araddr                   (mig_axi_rd.addr.addr),
      .s_axi_arlen                    (mig_axi_rd.addr.len),
      .s_axi_arsize                   (mig_axi_rd.addr.size),
      .s_axi_arburst                  (mig_axi_rd.addr.burst),
      .s_axi_arlock                   (mig_axi_rd.addr.lock),
      .s_axi_arcache                  (mig_axi_rd.addr.cache),
      .s_axi_arprot                   (mig_axi_rd.addr.prot),
      .s_axi_arqos                    (mig_axi_rd.addr.qos),
      .s_axi_arvalid                  (mig_axi_rd.addr.valid),
      .s_axi_arready                  (mig_axi_rd.addr.ready),
      // Slave Interface Read Data Ports
      .s_axi_rid                      (mig_axi_rd.data.id),
      .s_axi_rdata                    (mig_axi_rd.data.data),
      .s_axi_rresp                    (mig_axi_rd.data.resp),
      .s_axi_rlast                    (mig_axi_rd.data.last),
      .s_axi_rvalid                   (mig_axi_rd.data.valid),
      .s_axi_rready                   (mig_axi_rd.data.ready),
      // System Clock Ports
      .sys_clk_i                      (sys_clk),  // From external 100MHz source.
      .sys_rst                        (sys_rst)
    );

    //---------------------------------------------------
    // DDR3 SDRAM Models
    //---------------------------------------------------
    ddr3_model #(
      .DEBUG(0)   //Disable verbose prints
    ) sdram_i0 (
      .rst_n    (ddr3_reset_n),
      .ck       (ddr3_ck_p),
      .ck_n     (ddr3_ck_n),
      .cke      (ddr3_cke),
      .cs_n     (ddr3_cs_n),
      .ras_n    (ddr3_ras_n),
      .cas_n    (ddr3_cas_n),
      .we_n     (ddr3_we_n),
      .dm_tdqs  (ddr3_dm[1:0]),
      .ba       (ddr3_ba),
      .addr     (ddr3_addr),
      .dq       (ddr3_dq[15:0]),
      .dqs      (ddr3_dqs_p[1:0]),
      .dqs_n    (ddr3_dqs_n[1:0]),
      .tdqs_n   (), // Unused on x16
      .odt      (ddr3_odt)
    );

    ddr3_model #(
      .DEBUG(0)   //Disable verbose prints
    ) sdram_i1 (
      .rst_n    (ddr3_reset_n),
      .ck       (ddr3_ck_p),
      .ck_n     (ddr3_ck_n),
      .cke      (ddr3_cke),
      .cs_n     (ddr3_cs_n),
      .ras_n    (ddr3_ras_n),
      .cas_n    (ddr3_cas_n),
      .we_n     (ddr3_we_n),
      .dm_tdqs  (ddr3_dm[3:2]),
      .ba       (ddr3_ba),
      .addr     (ddr3_addr),
      .dq       (ddr3_dq[31:16]),
      .dqs      (ddr3_dqs_p[3:2]),
      .dqs_n    (ddr3_dqs_n[3:2]),
      .tdqs_n   (), // Unused on x16
      .odt      (ddr3_odt)
    );
  end endgenerate

endmodule
