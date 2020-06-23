//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_axi_ram_fifo_tb
//
// Description:  Testbench for rfnoc_block_axi_ram_fifo
//


module rfnoc_block_axi_ram_fifo_tb #(
  parameter int CHDR_W        = 64,
  parameter int NUM_PORTS     = 2,
  parameter int MEM_DATA_W    = 64,
  parameter int MEM_ADDR_W    = 13,
  parameter int FIFO_ADDR_W   = 12,
  parameter int IN_FIFO_SIZE  = 9,
  parameter int OUT_FIFO_SIZE = 9,
  parameter bit OVERFLOW      = 1,
  parameter bit BIST          = 1
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;

  `include "axi_ram_fifo_regs.vh"


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Simulation parameters
  localparam int  USE_RANDOM     = 1;        // Use random simulation data (not sequential)
  localparam real CHDR_CLK_PER   = 5.333333; // CHDR clock rate
  localparam real CTRL_CLK_PER   = 10.0;     // CTRL clock rate
  localparam real MEM_CLK_PER    = 3.333333; // Memory clock rate
  localparam int  SPP            = 256;      // Samples per packet
  localparam int  PKT_SIZE_BYTES = SPP*4;    // Bytes per packet
  localparam int  STALL_PROB     = 25;       // BFM stall probability

  // Block configuration
  localparam int NOC_ID        = 'hF1F0_0000;
  localparam int THIS_PORTID   = 'h123;
  localparam int MTU           = 10;
  localparam int NUM_HB        = 3;
  localparam int CIC_MAX_DECIM = 255;
  localparam int BURST_TIMEOUT = 64;
  localparam int MEM_CLK_RATE  = int'(1.0e9/MEM_CLK_PER);  // Frequency in Hz
  localparam int AWIDTH        = MEM_ADDR_W+1;

  // Put FIFO 0 at the bottom of the memory and FIFO 1 immediately above it.
  localparam bit [MEM_ADDR_W-1:0] FIFO_ADDR_BASE_0 = 0;
  localparam bit [MEM_ADDR_W-1:0] FIFO_ADDR_BASE_1 = 2**FIFO_ADDR_W;
  localparam bit [MEM_ADDR_W-1:0] FIFO_ADDR_MASK   = 2**FIFO_ADDR_W-1;


  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit mem_clk, mem_rst;

  // Don't start the clocks automatically (AUTOSTART=0), since we expect
  // multiple instances of this testbench to run in sequence. They will be
  // started before the first test.
  sim_clock_gen #(.PERIOD(CHDR_CLK_PER), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CTRL_CLK_PER), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(.PERIOD(MEM_CLK_PER), .AUTOSTART(0))
    mem_clk_gen        (.clk(mem_clk),        .rst(mem_rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W)::chdr_word_t chdr_word_t;

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W) blk_ctrl = 
    new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // AXI Memory Model
  //---------------------------------------------------------------------------

  // AXI Write Address Channel
  wire [         NUM_PORTS*1-1:0] m_axi_awid;
  wire [    NUM_PORTS*AWIDTH-1:0] m_axi_awaddr;
  wire [         NUM_PORTS*8-1:0] m_axi_awlen;
  wire [         NUM_PORTS*3-1:0] m_axi_awsize;
  wire [         NUM_PORTS*2-1:0] m_axi_awburst;
  wire [         NUM_PORTS*1-1:0] m_axi_awlock;   // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_awcache;  // Unused master output
  wire [         NUM_PORTS*3-1:0] m_axi_awprot;   // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_awqos;    // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_awregion; // Unused master output
  wire [         NUM_PORTS*1-1:0] m_axi_awuser;   // Unused master output
  wire [         NUM_PORTS*1-1:0] m_axi_awvalid;
  wire [         NUM_PORTS*1-1:0] m_axi_awready;
  // AXI Write Data Channel
  wire [  NUM_PORTS*MEM_DATA_W-1:0] m_axi_wdata;
  wire [NUM_PORTS*MEM_DATA_W/8-1:0] m_axi_wstrb;
  wire [           NUM_PORTS*1-1:0] m_axi_wlast;
  wire [           NUM_PORTS*1-1:0] m_axi_wuser;  // Unused master output
  wire [           NUM_PORTS*1-1:0] m_axi_wvalid;
  wire [           NUM_PORTS*1-1:0] m_axi_wready;
  // AXI Write Response Channel
  wire [         NUM_PORTS*1-1:0] m_axi_bid;
  wire [         NUM_PORTS*2-1:0] m_axi_bresp;
  wire [         NUM_PORTS*1-1:0] m_axi_buser;  // Unused master input
  wire [         NUM_PORTS*1-1:0] m_axi_bvalid;
  wire [         NUM_PORTS*1-1:0] m_axi_bready;
  // AXI Read Address Channel
  wire [         NUM_PORTS*1-1:0] m_axi_arid;
  wire [    NUM_PORTS*AWIDTH-1:0] m_axi_araddr;
  wire [         NUM_PORTS*8-1:0] m_axi_arlen;
  wire [         NUM_PORTS*3-1:0] m_axi_arsize;
  wire [         NUM_PORTS*2-1:0] m_axi_arburst;
  wire [         NUM_PORTS*1-1:0] m_axi_arlock;   // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_arcache;  // Unused master output
  wire [         NUM_PORTS*3-1:0] m_axi_arprot;   // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_arqos;    // Unused master output
  wire [         NUM_PORTS*4-1:0] m_axi_arregion; // Unused master output
  wire [         NUM_PORTS*1-1:0] m_axi_aruser;   // Unused master output
  wire [         NUM_PORTS*1-1:0] m_axi_arvalid;
  wire [         NUM_PORTS*1-1:0] m_axi_arready;
  // AXI Read Data Channel
  wire [         NUM_PORTS*1-1:0] m_axi_rid;
  wire [NUM_PORTS*MEM_DATA_W-1:0] m_axi_rdata;
  wire [         NUM_PORTS*2-1:0] m_axi_rresp;
  wire [         NUM_PORTS*1-1:0] m_axi_rlast;
  wire [         NUM_PORTS*1-1:0] m_axi_ruser;  // Unused master input
  wire [         NUM_PORTS*1-1:0] m_axi_rvalid;
  wire [         NUM_PORTS*1-1:0] m_axi_rready;

  // Unused master input signals
  assign m_axi_buser = {NUM_PORTS{1'b0}};
  assign m_axi_ruser = {NUM_PORTS{1'b0}};

  for (genvar i = 0; i < NUM_PORTS; i = i+1) begin : gen_sim_axi_ram
    sim_axi_ram #(
      .AWIDTH     (AWIDTH),
      .DWIDTH     (MEM_DATA_W),
      .IDWIDTH    (1),
      .BIG_ENDIAN (0),
      .STALL_PROB (STALL_PROB)
    ) sim_axi_ram_i (
      .s_aclk        (mem_clk),
      .s_aresetn     (~mem_rst),
      .s_axi_awid    (m_axi_awid[i]),
      .s_axi_awaddr  (m_axi_awaddr[i*AWIDTH +: AWIDTH]),
      .s_axi_awlen   (m_axi_awlen[i*8 +: 8]),
      .s_axi_awsize  (m_axi_awsize[i*3 +: 3]),
      .s_axi_awburst (m_axi_awburst[i*2 +: 2]),
      .s_axi_awvalid (m_axi_awvalid[i]),
      .s_axi_awready (m_axi_awready[i]),
      .s_axi_wdata   (m_axi_wdata[i*MEM_DATA_W +: MEM_DATA_W]),
      .s_axi_wstrb   (m_axi_wstrb[i*(MEM_DATA_W/8) +: (MEM_DATA_W/8)]),
      .s_axi_wlast   (m_axi_wlast[i]),
      .s_axi_wvalid  (m_axi_wvalid[i]),
      .s_axi_wready  (m_axi_wready[i]),
      .s_axi_bid     (m_axi_bid[i]),
      .s_axi_bresp   (m_axi_bresp[i*2 +: 2]),
      .s_axi_bvalid  (m_axi_bvalid[i]),
      .s_axi_bready  (m_axi_bready[i]),
      .s_axi_arid    (m_axi_arid[i]),
      .s_axi_araddr  (m_axi_araddr[i*AWIDTH +: AWIDTH]),
      .s_axi_arlen   (m_axi_arlen[i*8 +: 8]),
      .s_axi_arsize  (m_axi_arsize[i*3 +: 3]),
      .s_axi_arburst (m_axi_arburst[i*2 +: 2]),
      .s_axi_arvalid (m_axi_arvalid[i]),
      .s_axi_arready (m_axi_arready[i]),
      .s_axi_rid     (m_axi_rid[i]),
      .s_axi_rdata   (m_axi_rdata[i*MEM_DATA_W +: MEM_DATA_W]),
      .s_axi_rresp   (m_axi_rresp[i*2 +: 2]),
      .s_axi_rlast   (m_axi_rlast[i]),
      .s_axi_rvalid  (m_axi_rvalid[i]),
      .s_axi_rready  (m_axi_rready[i])
    );
  end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tready;

  logic [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];

    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_axi_ram_fifo #(
    .THIS_PORTID    (THIS_PORTID),
    .CHDR_W         (CHDR_W),
    .NUM_PORTS      (NUM_PORTS),
    .MTU            (MTU),
    .MEM_DATA_W     (MEM_DATA_W),
    .MEM_ADDR_W     (MEM_ADDR_W),
    .AWIDTH         (AWIDTH),
    .FIFO_ADDR_BASE ({ FIFO_ADDR_BASE_1, FIFO_ADDR_BASE_0 }),
    .FIFO_ADDR_MASK ({NUM_PORTS{FIFO_ADDR_MASK}}),
    .BURST_TIMEOUT  ({NUM_PORTS{BURST_TIMEOUT}}),
    .IN_FIFO_SIZE   (IN_FIFO_SIZE),
    .OUT_FIFO_SIZE  (OUT_FIFO_SIZE),
    .BIST           (BIST),
    .MEM_CLK_RATE   (MEM_CLK_RATE)
  ) rfnoc_block_axi_ram_fifo_i (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready),
    .mem_clk             (mem_clk),
    .axi_rst             (mem_rst),
    .m_axi_awid          (m_axi_awid),
    .m_axi_awaddr        (m_axi_awaddr),
    .m_axi_awlen         (m_axi_awlen),
    .m_axi_awsize        (m_axi_awsize),
    .m_axi_awburst       (m_axi_awburst),
    .m_axi_awlock        (m_axi_awlock),
    .m_axi_awcache       (m_axi_awcache),
    .m_axi_awprot        (m_axi_awprot),
    .m_axi_awqos         (m_axi_awqos),
    .m_axi_awregion      (m_axi_awregion),
    .m_axi_awuser        (m_axi_awuser),
    .m_axi_awvalid       (m_axi_awvalid),
    .m_axi_awready       (m_axi_awready),
    .m_axi_wdata         (m_axi_wdata),
    .m_axi_wstrb         (m_axi_wstrb),
    .m_axi_wlast         (m_axi_wlast),
    .m_axi_wuser         (m_axi_wuser),
    .m_axi_wvalid        (m_axi_wvalid),
    .m_axi_wready        (m_axi_wready),
    .m_axi_bid           (m_axi_bid),
    .m_axi_bresp         (m_axi_bresp),
    .m_axi_buser         (m_axi_buser),
    .m_axi_bvalid        (m_axi_bvalid),
    .m_axi_bready        (m_axi_bready),
    .m_axi_arid          (m_axi_arid),
    .m_axi_araddr        (m_axi_araddr),
    .m_axi_arlen         (m_axi_arlen),
    .m_axi_arsize        (m_axi_arsize),
    .m_axi_arburst       (m_axi_arburst),
    .m_axi_arlock        (m_axi_arlock),
    .m_axi_arcache       (m_axi_arcache),
    .m_axi_arprot        (m_axi_arprot),
    .m_axi_arqos         (m_axi_arqos),
    .m_axi_arregion      (m_axi_arregion),
    .m_axi_aruser        (m_axi_aruser),
    .m_axi_arvalid       (m_axi_arvalid),
    .m_axi_arready       (m_axi_arready),
    .m_axi_rid           (m_axi_rid),
    .m_axi_rdata         (m_axi_rdata),
    .m_axi_rresp         (m_axi_rresp),
    .m_axi_rlast         (m_axi_rlast),
    .m_axi_ruser         (m_axi_ruser),
    .m_axi_rvalid        (m_axi_rvalid),
    .m_axi_rready        (m_axi_rready)
  );

 
  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  task automatic write_reg(int port, bit [19:0] addr, bit [31:0] value);
    blk_ctrl.reg_write((2**RAM_FIFO_ADDR_W)*port + addr, value);
  endtask : write_reg

  task automatic write_reg_64(int port, bit [19:0] addr, bit [63:0] value);
    blk_ctrl.reg_write((2**RAM_FIFO_ADDR_W)*port + addr + 0, value[31: 0]);
    blk_ctrl.reg_write((2**RAM_FIFO_ADDR_W)*port + addr + 4, value[63:32]);
  endtask : write_reg_64

  task automatic read_reg(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**RAM_FIFO_ADDR_W)*port + addr, value[31: 0]);
  endtask : read_reg

  task automatic read_reg_64(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**RAM_FIFO_ADDR_W)*port + addr + 0, value[31: 0]);
    blk_ctrl.reg_read((2**RAM_FIFO_ADDR_W)*port + addr + 4, value[63:32]);
  endtask : read_reg_64


  // Generate a unique sequence of incrementing numbers
  task automatic gen_test_data(int num_bytes, output chdr_word_t queue[$]);
    int             num_chdr_words;
    chdr_word_t     val64;

    // Calculate the number of chdr_word_t size words
    num_chdr_words = int'($ceil(real'(num_bytes) / ($bits(chdr_word_t) / 8)));

    for (int i = 0; i < num_chdr_words; i++) begin
      if (USE_RANDOM) begin
        val64 = { $urandom(), $urandom() };   // Random data, for more rigorous testing
      end else begin
        val64 = i;                            // Sequential data, for easier debugging
      end
      queue.push_back(val64);
    end
  endtask : gen_test_data


  //---------------------------------------------------------------------------
  // Reset
  //---------------------------------------------------------------------------
  
  task test_reset();
    test.start_test("Wait for Reset", 10us);
    mem_clk_gen.reset();
    blk_ctrl.flush_and_reset();
    wait(!mem_rst);
    test.end_test();
  endtask : test_reset


  //---------------------------------------------------------------------------
  // Check NoC ID and Block Info
  //---------------------------------------------------------------------------
  
  task test_block_info();
    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();
  endtask : test_block_info


  //---------------------------------------------------------------------------
  // Check Unused Signals
  //---------------------------------------------------------------------------

  task test_unused();
    test.start_test("Check unused/static signals");
    for (int port = 0; port < NUM_PORTS; port++) begin
      `ASSERT_ERROR(m_axi_awlock   [port*1 +: 1] == 1'b0, "m_axi_awlock value unexpected");
      `ASSERT_ERROR(m_axi_awcache  [port*4 +: 4] == 4'hF, "m_axi_awcache value unexpected");
      `ASSERT_ERROR(m_axi_awprot   [port*3 +: 3] == 3'h2, "m_axi_awprot value unexpected");
      `ASSERT_ERROR(m_axi_awqos    [port*4 +: 4] == 4'h0, "m_axi_awqos value unexpected");
      `ASSERT_ERROR(m_axi_awregion [port*4 +: 4] == 4'h0, "m_axi_awregion value unexpected");
      `ASSERT_ERROR(m_axi_awuser   [port*1 +: 1] == 1'b0, "m_axi_awuser value unexpected");
      //
      `ASSERT_ERROR(m_axi_wuser    [port*1 +: 1] == 1'b0, "m_axi_wuser value unexpected");
      //
      `ASSERT_ERROR(m_axi_arlock   [port*1 +: 1] == 1'b0, "m_axi_arlock value unexpected");
      `ASSERT_ERROR(m_axi_arcache  [port*4 +: 4] == 4'hF, "m_axi_arcache value unexpected");
      `ASSERT_ERROR(m_axi_arprot   [port*3 +: 3] == 3'h2, "m_axi_arprot value unexpected");
      `ASSERT_ERROR(m_axi_arqos    [port*4 +: 4] == 4'h0, "m_axi_arqos value unexpected");
      `ASSERT_ERROR(m_axi_arregion [port*4 +: 4] == 4'h0, "m_axi_arregion value unexpected");
      `ASSERT_ERROR(m_axi_aruser   [port*1 +: 1] == 1'b0, "m_axi_aruser value unexpected");
    end
    test.end_test();
  endtask : test_unused


  //---------------------------------------------------------------------------
  // Test Registers
  //---------------------------------------------------------------------------

  task test_registers();
    logic [63:0] val64, expected64, temp64;
    logic [31:0] val32, expected32, temp32;

    test.start_test("Test registers", 50us);

    for (int port = 0; port < NUM_PORTS; port++) begin
      //
      // REG_FIFO_INFO
      //
      expected32 = 0;
      expected32[REG_FIFO_MAGIC_POS +: REG_FIFO_MAGIC_W] = 16'hF1F0;
      expected32[REG_FIFO_BIST_PRSNT_POS] = (BIST != 0);
      read_reg(port, REG_FIFO_INFO, val32);
      `ASSERT_ERROR(val32 == expected32, "Initial value for REG_FIFO_INFO is not correct");

      //
      // REG_FIFO_READ_SUPPRESS
      //
      expected32 = 0;
      expected32[REG_FIFO_IN_FIFO_SIZE_POS+:REG_FIFO_IN_FIFO_SIZE_W] = IN_FIFO_SIZE;
      expected32[REG_FIFO_SUPPRESS_THRESH_POS+:REG_FIFO_SUPPRESS_THRESH_W] = 0;
      read_reg(port, REG_FIFO_READ_SUPPRESS, val32);
      `ASSERT_ERROR(val32 == expected32, "Initial value for REG_FIFO_READ_SUPPRESS is not correct");

      temp32 = expected32;
      expected32[REG_FIFO_SUPPRESS_THRESH_POS+:REG_FIFO_SUPPRESS_THRESH_W] = 
        ~val32[REG_FIFO_SUPPRESS_THRESH_POS+:REG_FIFO_SUPPRESS_THRESH_W];
      write_reg(port, REG_FIFO_READ_SUPPRESS, expected32);
      read_reg(port, REG_FIFO_READ_SUPPRESS, val32);
      `ASSERT_ERROR(val32 == expected32, "REG_FIFO_READ_SUPPRESS did not update");

      expected32 = temp32;
      write_reg(port, REG_FIFO_READ_SUPPRESS, expected32);
      read_reg(port, REG_FIFO_READ_SUPPRESS, val32);
      `ASSERT_ERROR(val32 == expected32, "REG_FIFO_READ_SUPPRESS did not reset");

      //
      // REG_FIFO_MEM_SIZE
      //
      expected32 = 0;
      expected32[REG_FIFO_DATA_SIZE_POS +: REG_FIFO_DATA_SIZE_W] = MEM_DATA_W;
      expected32[REG_FIFO_ADDR_SIZE_POS +: REG_FIFO_ADDR_SIZE_W] = MEM_ADDR_W;
      read_reg(port, REG_FIFO_MEM_SIZE, val32);
      `ASSERT_ERROR(val32 == expected32, "Incorrect REG_FIFO_MEM_SIZE value!");

      //
      // REG_FIFO_TIMEOUT
      //
      expected32 = BURST_TIMEOUT;
      read_reg(port, REG_FIFO_TIMEOUT, val32);
      `ASSERT_ERROR(val32 == expected32, "Initial value for REG_FIFO_TIMEOUT is not correct");

      write_reg(port, REG_FIFO_TIMEOUT, {REG_TIMEOUT_W{1'b1}});
      read_reg(port, REG_FIFO_TIMEOUT, val32);
      `ASSERT_ERROR(val32 == {REG_TIMEOUT_W{1'b1}}, "REG_FIFO_TIMEOUT did not update");

      write_reg(port, REG_FIFO_TIMEOUT, expected32);
      read_reg(port, REG_FIFO_TIMEOUT, val32);
      `ASSERT_ERROR(val32 == expected32, "REG_FIFO_TIMEOUT did not reset");

      //
      // REG_FIFO_FULLNESS
      //
      read_reg_64(port, REG_FIFO_FULLNESS_LO, val64);
      `ASSERT_ERROR(val64 == 0, "Incorrect REG_FIFO_FULLNESS value!");

      //
      // REG_FIFO_ADDR_BASE
      //
      expected64 = port * 2**FIFO_ADDR_W;
      read_reg_64(port, REG_FIFO_ADDR_BASE_LO, val64);
      `ASSERT_ERROR(val64 == expected64, "Initial value for REG_FIFO_ADDR_BASE is not correct");

      write_reg_64(port, REG_FIFO_ADDR_BASE_LO, {MEM_ADDR_W{1'b1}});
      read_reg_64(port, REG_FIFO_ADDR_BASE_LO, val64);
      `ASSERT_ERROR(val64 == {MEM_ADDR_W{1'b1}}, "REG_FIFO_ADDR_BASE did not update");

      write_reg_64(port, REG_FIFO_ADDR_BASE_LO, expected64);
      read_reg_64(port, REG_FIFO_ADDR_BASE_LO, val64);
      `ASSERT_ERROR(val64 == expected64, "REG_FIFO_ADDR_BASE did not reset");

      //
      // REG_FIFO_ADDR_MASK
      //
      expected64 = {FIFO_ADDR_W{1'b1}};
      read_reg_64(port, REG_FIFO_ADDR_MASK_LO, val64);
      `ASSERT_ERROR(val64 == expected64, "Initial value for REG_FIFO_ADDR_MASK_LO is not correct");

      // Set to the max value
      write_reg_64(port, REG_FIFO_ADDR_MASK_LO, {MEM_ADDR_W{1'b1}});
      read_reg_64(port, REG_FIFO_ADDR_MASK_LO, val64);
      `ASSERT_ERROR(val64 == {MEM_ADDR_W{1'b1}}, "REG_FIFO_ADDR_MASK_LO did not update");

      // Set to the min value
      write_reg_64(port, REG_FIFO_ADDR_MASK_LO, 0);
      read_reg_64(port, REG_FIFO_ADDR_MASK_LO, val64);
      // Coerce to the minimum allowed value
      temp64 = rfnoc_block_axi_ram_fifo_i.gen_ram_fifos[0].axi_ram_fifo_i.FIFO_ADDR_MASK_MIN;
      `ASSERT_ERROR(val64 == temp64, "REG_FIFO_ADDR_MASK_LO did not update");

      write_reg_64(port, REG_FIFO_ADDR_MASK_LO, expected64);
      read_reg_64(port, REG_FIFO_ADDR_MASK_LO, val64);
      `ASSERT_ERROR(val64 == expected64, "REG_FIFO_ADDR_MASK_LO did not reset");

      //
      // REG_FIFO_PACKET_CNT
      //
      read_reg(port, REG_FIFO_PACKET_CNT, val32);
      `ASSERT_ERROR(val32 == 0, "Incorrect REG_FIFO_PACKET_CNT value!");

      if (BIST) begin
        read_reg(port, REG_BIST_CTRL, val32);
        `ASSERT_ERROR(val32 == 0, "Initial value for REG_BIST_CTRL is not correct");
        read_reg(port, REG_BIST_CLK_RATE, val32);
        `ASSERT_ERROR(val32 == MEM_CLK_RATE, "Initial value for REG_BIST_CLK_RATE is not correct");
        read_reg_64(port, REG_BIST_NUM_BYTES_LO, val64);
        `ASSERT_ERROR(val64 == 0, "Initial value for REG_BIST_NUM_BYTES is not correct");
        read_reg_64(port, REG_BIST_TX_BYTE_COUNT_LO, val64);
        `ASSERT_ERROR(val64 == 0, "Initial value for REG_BIST_TX_BYTE_COUNT is not correct");
        read_reg_64(port, REG_BIST_RX_BYTE_COUNT_LO, val64);
        `ASSERT_ERROR(val64 == 0, "Initial value for REG_BIST_RX_BYTE_COUNT is not correct");
        read_reg_64(port, REG_BIST_ERROR_COUNT_LO, val64);
        `ASSERT_ERROR(val64 == 0, "Initial value for REG_BIST_ERROR_COUNT is not correct");
        read_reg_64(port, REG_BIST_CYCLE_COUNT_LO, val64);
        `ASSERT_ERROR(val64 == 0, "Initial value for REG_BIST_CYCLE_COUNT is not correct");
      end
    end

    test.end_test();
  endtask : test_registers


  //---------------------------------------------------------------------------
  // Basic Test
  //---------------------------------------------------------------------------
  //
  // Push a few packets through each FIFO.
  //
  //---------------------------------------------------------------------------

  task test_basic();
    logic [31:0] val32;

    test.start_test("Basic test", NUM_PORTS*20us);

    for (int port = 0; port < NUM_PORTS; port++) begin
      chdr_word_t test_data[$];
      logic [63:0] val64;
      timeout_t timeout;

      // Generate the test data to send
      gen_test_data(PKT_SIZE_BYTES*3, test_data);

      // Queue up the packets to send
      blk_ctrl.send_packets(port, test_data);

      // Make sure fullness increases
      test.start_timeout(timeout, 4us, 
        $sformatf("Waiting for fullness to increase on port %0d", port));
      forever begin
        read_reg_64(port, REG_FIFO_FULLNESS_LO, val64);
        if (val64 != 0) break;
      end
      test.end_timeout(timeout);

      // Verify the data, one packet at a time
      for (int count = 0; count < test_data.size(); ) begin
        chdr_word_t recv_data[$];
        int data_bytes;
        blk_ctrl.recv(port, recv_data, data_bytes);

        `ASSERT_ERROR(
          data_bytes == PKT_SIZE_BYTES, 
          "Length didn't match expected value"
        );

        for (int i = 0; i < recv_data.size(); i++, count++) begin
          if (recv_data[i] != test_data[count]) begin
            $display("Expected %X, received %X on port %0d", test_data[count], recv_data[i], port);
          end
          `ASSERT_ERROR(
            recv_data[i] == test_data[count], 
            "Received data doesn't match expected value"
          );
        end
      end

      // Make sure the packet count updated
      read_reg(port, REG_FIFO_PACKET_CNT, val32);
      `ASSERT_ERROR(val32 > 0, "REG_FIFO_PACKET_CNT didn't update");
    end

    test.end_test();
  endtask : test_basic


  //---------------------------------------------------------------------------
  // Single Byte Test
  //---------------------------------------------------------------------------

  task test_single_byte();
    test.start_test("Single byte test", 20us);

    for (int port = 0; port < NUM_PORTS; port++) begin
      chdr_word_t test_data[$];
      chdr_word_t recv_data[$];
      int data_bytes;

      gen_test_data(1, test_data);

      blk_ctrl.send(port, test_data, 1);
      blk_ctrl.recv(port, recv_data, data_bytes);

      `ASSERT_ERROR(
        data_bytes == 1 && recv_data.size() == CHDR_W/$bits(chdr_word_t), 
        "Length didn't match expected value"
      );
      `ASSERT_ERROR(
        recv_data[0][7:0] == test_data[0][7:0], 
        "Received data doesn't match expected value"
      );
    end

    test.end_test();
  endtask : test_single_byte


  //---------------------------------------------------------------------------
  // Test Overflow
  //---------------------------------------------------------------------------
  // 
  // Fill the FIFO on both ports to make sure if fills correctly and flow 
  // control works correct at the limits.
  //
  //---------------------------------------------------------------------------

  task test_overflow();
    chdr_word_t test_data[NUM_PORTS][$];
    int num_bytes, num_words;
    bit [NUM_PORTS-1:0] full_bits;
    logic [63:0] val64;
    timeout_t timeout;
    realtime start_time;

    if (!OVERFLOW) return;

    num_bytes = (MEM_DATA_W/8) * (2**IN_FIFO_SIZE + 2**OUT_FIFO_SIZE) + 2**MEM_ADDR_W;
    num_bytes = num_bytes * 3 / 2;
    num_words = num_bytes / (CHDR_W/8);

    test.start_test("Overflow test", 10 * num_words * CHDR_CLK_PER);

    // Stall the output of each FIFO, allow unrestricted input
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, 0);
      blk_ctrl.set_slave_stall_prob(port, 100);
    end

    // Input more packets into each FIFO than they can fit
    for (int port = 0; port < NUM_PORTS; port++) begin
      gen_test_data(num_bytes, test_data[port]);
      blk_ctrl.send_packets(port, test_data[port]);
    end

    // Wait for both inputs to stall
    test.start_timeout(timeout, (4 * num_words + 1000) * CHDR_CLK_PER, 
      $sformatf("Waiting for input to stall"));
    full_bits = 0;
    forever begin
      for (int port = 0; port < NUM_PORTS; port++) begin
        full_bits[port] = ~s_rfnoc_chdr_tready[port];
        if (!full_bits[port]) start_time = $realtime;
      end

      // Break as soon as all FIFOs have been stalled for 1000 clock cycles
      if (full_bits == {NUM_PORTS{1'b1}} && $realtime-start_time > 1000 * CHDR_CLK_PER) break;
      #(CHDR_CLK_PER*100);
    end
    test.end_timeout(timeout);

    // Make sure all the FIFOs filled up
    for (int port = 0; port < NUM_PORTS; port++) begin
      read_reg_64(port, REG_FIFO_FULLNESS_LO, val64);
      // FIFO is full once it comes within 256 words of being full
      `ASSERT_ERROR(val64 >= (2**FIFO_ADDR_W / (MEM_DATA_W/8)) - 256, "FIFO not reading full");
    end

    // Restore the input/output rates
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    end

    // Read out and verify the data
    for (int port = 0; port < NUM_PORTS; port++) begin
      for (int count = 0; count < test_data[port].size(); ) begin
        chdr_word_t recv_data[$];
        int data_bytes;
        int expected_length;
        blk_ctrl.recv(port, recv_data, data_bytes);

        if (count*($bits(chdr_word_t)/8) + PKT_SIZE_BYTES <= num_bytes) begin
          // Should be a full packet
          expected_length = PKT_SIZE_BYTES;
        end else begin
          // Should be a partial packet
          expected_length = num_bytes % PKT_SIZE_BYTES;
        end

        // Check the length
        `ASSERT_ERROR(
          data_bytes == expected_length, 
          "Length didn't match expected value"
        );

        for (int i = 0; i < recv_data.size(); i++, count++) begin
          `ASSERT_ERROR(
            recv_data[i] == test_data[port][count], 
            "Received data doesn't match expected value"
          );
        end
      end
    end

    test.end_test();
  endtask : test_overflow


  //---------------------------------------------------------------------------
  // Test Read Suppression
  //---------------------------------------------------------------------------

  task test_read_suppression();
    chdr_word_t test_data[$];
    logic [31:0] val32, save32;
    
    localparam int port = 0; // Only test one port

    test.start_test("Read suppression test", 1ms);

    // Turn on read suppression with the max threshold to cause it to 
    // suppress everything.
    read_reg(port, REG_FIFO_READ_SUPPRESS, save32);
    val32 = save32;
    val32[REG_FIFO_SUPPRESS_THRESH_POS +: REG_FIFO_SUPPRESS_THRESH_W] = {REG_FIFO_SUPPRESS_THRESH_W{1'b1}};
    write_reg(port, REG_FIFO_READ_SUPPRESS, val32);

    // Disable the input stall check to properly test read suppression. If it
    // were not disabled, it would override the read suppression setting,
    // making it difficult to test.
    force rfnoc_block_axi_ram_fifo_i.gen_ram_fifos[port].axi_ram_fifo_i.input_stalled = 1'b0;

    // Generate enough test data to ensure we should get some output if read
    // suppression weren't being used (1 full input FIFO plus 8 RAM bursts).
    gen_test_data((2**IN_FIFO_SIZE + 256*8)*(MEM_DATA_W/8), test_data);

    // Start sending packets then wait for the input to stall, either because 
    // we've filled the FIFO or we've input everything.
    blk_ctrl.set_master_stall_prob(port, 0);
    blk_ctrl.send_packets(port, test_data);
    wait (s_rfnoc_chdr_tvalid[port] && s_rfnoc_chdr_tready[port]);
    wait (s_rfnoc_chdr_tvalid[port] && !s_rfnoc_chdr_tready[port]);

    // Make sure nothing made it through
    `ASSERT_ERROR(blk_ctrl.num_received(port) == 0, "Read suppression failed");

    // Re-enable the input stall check
    release rfnoc_block_axi_ram_fifo_i.gen_ram_fifos[port].axi_ram_fifo_i.input_stalled;

    // Turn down the threshold 
    val32[REG_FIFO_SUPPRESS_THRESH_POS +: REG_FIFO_SUPPRESS_THRESH_W] = {REG_FIFO_SUPPRESS_THRESH_W{1'b0}};
    write_reg(port, REG_FIFO_READ_SUPPRESS, val32);

    blk_ctrl.set_master_stall_prob(port, STALL_PROB);

    // Verify the data, one packet at a time
    for (int count = 0; count < test_data.size(); ) begin
      chdr_word_t recv_data[$];
      int data_bytes;
      blk_ctrl.recv(port, recv_data, data_bytes);

      for (int i = 0; i < recv_data.size(); i++, count++) begin
        if (recv_data[i] != test_data[count]) begin
          $display("Expected %X, received %X on port %0d", test_data[count], recv_data[i], port);
        end
        `ASSERT_ERROR(
          recv_data[i] == test_data[count], 
          "Received data doesn't match expected value"
        );
      end
    end

    // Restore suppression settings
    write_reg(port, REG_FIFO_READ_SUPPRESS, save32);

    test.end_test();
  endtask : test_read_suppression


  //---------------------------------------------------------------------------
  // Random Tests
  //---------------------------------------------------------------------------
  //
  // Perform a series of random tests with different read/write probabilities 
  // test unexpected conditions.
  //
  //---------------------------------------------------------------------------

  class RandTrans;
    chdr_word_t data[$];
    int         num_bytes;
  endclass;

  task test_random();
    localparam NUM_PACKETS = 256;

    mailbox #(RandTrans) data_queue;
    int port;

    test.start_test("Random test", NUM_PACKETS * 2us);

    data_queue = new();
    port       = 0;         // Just check one port for this test

    // Queue up a bunch of random packets
    begin : data_gen
      RandTrans trans;
      $display("Generating %0d random packets...", NUM_PACKETS);

      for (int packet_count = 0; packet_count < NUM_PACKETS; packet_count++) begin
        trans = new();
        trans.num_bytes = $urandom_range(1, PKT_SIZE_BYTES);
        gen_test_data(trans.num_bytes, trans.data);
        blk_ctrl.send(port, trans.data, trans.num_bytes);
        data_queue.put(trans);
      end
    end

    // Receive and check all the packets
    fork
      begin : stall_update
        // Split the packets into four groups and use different stall 
        // behavior for each. 
        // 
        //   1. Start filling up the FIFO
        //   2. Let it run for a while
        //   3. Start emptying the FIFO
        //   4. Let it run until all the data gets through
        //
        for (int i = 0; i < 4; i++) begin
          case (i)
            0 : begin
              $display("Test fast writer, slow reader");
              blk_ctrl.set_master_stall_prob(port, 10);
              blk_ctrl.set_slave_stall_prob(port, 80);
            end
            1 : begin
              $display("Test matched reader/writer");
              blk_ctrl.set_master_stall_prob(port, STALL_PROB);
              blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
            end
            2 : begin
              $display("Test slow writer, fast reader");
              blk_ctrl.set_master_stall_prob(port, 90);
              blk_ctrl.set_slave_stall_prob(port, 10);
            end
            3 : begin
              $display("Test matched reader/writer");
              blk_ctrl.set_master_stall_prob(port, STALL_PROB);
              blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
            end
          endcase

          // Wait for a quarter of the packets to be accepted by the RAM FIFO
          blk_ctrl.wait_complete(port, NUM_PACKETS/4);
        end
      end
      begin : data_check
        RandTrans   trans;
        chdr_word_t recv_data[$];
        int         num_bytes;
        int         num_words;


        for (int packet_count = 0; packet_count < NUM_PACKETS; packet_count++) begin
          //$display("Checking packet %0d/%0d...", packet_count, NUM_PACKETS);

          blk_ctrl.recv(port, recv_data, num_bytes);
          data_queue.get(trans);

          // Check the length
          `ASSERT_ERROR(
            num_bytes == trans.num_bytes, 
            "Length didn't match expected value"
          );

          // If the generated data was an odd number of chdr_word_t words, we 
          // will get back an extra 0 word at the end. Calculate the correct 
          // number of words so that we ignore any extra at the end.
          num_words = int'($ceil(real'(num_bytes)/($bits(chdr_word_t)/8)));
          for (int i = 0; i < num_words; i++) begin
            `ASSERT_ERROR(
              recv_data[i] == trans.data[i],
              "Received data doesn't match expected value"
            );
          end
        end
      end
    join

    test.end_test();
  endtask : test_random


  //---------------------------------------------------------------------------
  // Test Clearing FIFO Block
  //---------------------------------------------------------------------------

  task test_clear();
   test.start_test("FIFO clear test", 100us);

   // TODO:
   $warning("Need to write a test flushing and resetting the block!");

   test.end_test();
  endtask : test_clear


  //---------------------------------------------------------------------------
  // Test BIST
  //---------------------------------------------------------------------------

  task test_bist();
    logic [31:0] val32;
    logic [63:0] val64;
    int port;
    int num_bytes;

    if (!BIST) return;

    test.start_test("BIST test", 100us);

    port = 0;  // Test the first port
    num_bytes = 2048;

    // Start a test
    write_reg(port, REG_BIST_CTRL, 1 << REG_BIST_CLEAR_POS);
    write_reg(port, REG_BIST_NUM_BYTES_LO, num_bytes);
    write_reg(port, REG_BIST_CTRL, 1 << REG_BIST_START_POS);

    // Make sure running bit gets set
    read_reg(port, REG_BIST_CTRL, val32);
    `ASSERT_ERROR(val32[REG_BIST_RUNNING_POS] == 1'b1, "RUNNING bit not set");

    // Wait for the test to complete
    do begin
      read_reg(port, REG_BIST_CTRL, val32);
    end while(val32[REG_BIST_RUNNING_POS]);

    // Check the results
    read_reg_64(port, REG_BIST_TX_BYTE_COUNT_LO, val64);
    `ASSERT_ERROR(val64 == num_bytes, "TX_BYTE_COUNT is not correct");
    read_reg_64(port, REG_BIST_RX_BYTE_COUNT_LO, val64);
    `ASSERT_ERROR(val64 == num_bytes, "RX_BYTE_COUNT is not correct");
    read_reg_64(port, REG_BIST_ERROR_COUNT_LO, val64);
    `ASSERT_ERROR(val64 == 0, "ERROR_COUNT is not zero");
    read_reg_64(port, REG_BIST_CYCLE_COUNT_LO, val64);
    `ASSERT_ERROR(val64 > 0, "CYCLE_COUNT did not update");

    // TODO:
    $warning("BIST Continuous mode is NOT being tested");
    $warning("BIST error insertion is NOT being tested (errors might be ignored)");

    test.end_test();
  endtask : test_bist


  //---------------------------------------------------------------------------
  // BIST Throughput Test
  //---------------------------------------------------------------------------
  //
  // This test sanity-checks the values returned by the BIST. If run with the
  // other BIST test, it also tests clearing the BIST counters.
  //
  //---------------------------------------------------------------------------

  task test_bist_throughput();
    localparam   port = 0;  // Test the first port
    logic [31:0] val32;
    logic [63:0] val64;
    int          num_bytes;
    longint      rx_byte_count;
    longint      cycle_count;
    real         throughput;
    real         efficiency;

    if (!BIST) return;

    test.start_test("BIST throughput test", 100us);

    num_bytes = 64*1024;

    // Reset the memory probability
    gen_sim_axi_ram[port].sim_axi_ram_i.set_stall_prob(0);

    // Start a test
    write_reg(port, REG_BIST_CTRL, 1 << REG_BIST_CLEAR_POS);
    write_reg(port, REG_BIST_NUM_BYTES_LO, num_bytes);
    write_reg(port, REG_BIST_CTRL, 1 << REG_BIST_START_POS);

    // Make sure running bit gets set
    read_reg(port, REG_BIST_CTRL, val32);
    `ASSERT_ERROR(val32[REG_BIST_RUNNING_POS] == 1'b1, "RUNNING bit not set");

    // Wait for the test to complete
    do begin
      read_reg(port, REG_BIST_CTRL, val32);
    end while(val32[REG_BIST_RUNNING_POS]);

    // Check the results
    read_reg_64(port, REG_BIST_TX_BYTE_COUNT_LO, val64);
    `ASSERT_ERROR(val64 == num_bytes, "TX_BYTE_COUNT is not correct");
    read_reg_64(port, REG_BIST_RX_BYTE_COUNT_LO, rx_byte_count);
    `ASSERT_ERROR(rx_byte_count == num_bytes, "RX_BYTE_COUNT is not correct");
    read_reg_64(port, REG_BIST_ERROR_COUNT_LO, val64);
    `ASSERT_ERROR(val64 == 0, "ERROR_COUNT is not zero");
    read_reg_64(port, REG_BIST_CYCLE_COUNT_LO, cycle_count);
    `ASSERT_ERROR(cycle_count > 0, "CYCLE_COUNT did not update");

    // Throughput = num_bytes / time = num_bytes / (num_cyles * period)
    throughput = real'(rx_byte_count) / (real'(cycle_count) / real'(MEM_CLK_RATE));

    // Efficiency is the actual throughput divided by the theoretical max. We
    // use 0.5x in the calculation because we assume that the memory is a
    // half-duplex read/write memory running at MEM_CLK_RATE, but we're
    // measuring the full-duplex throughput.
    efficiency = throughput / (0.5 * real'(MEM_CLK_RATE) * (MEM_DATA_W/8));

    $display("BIST Throughput: %0.1f MB/s", throughput / 1.0e6);
    $display("BIST Efficiency: %0.1f %%", efficiency * 100.0 );

    `ASSERT_ERROR(efficiency > 0.95, "BIST efficiency was lower than expected");

    // Restore the memory stall probability
    gen_sim_axi_ram[port].sim_axi_ram_i.set_stall_prob(STALL_PROB);

    test.end_test();
  endtask;


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    const int port = 0;
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf(
      "rfnoc_block_axi_ram_fifo_tb\nCHDR_W = %0D, NUM_PORTS = %0D, MEM_DATA_W = %0D, MEM_ADDR_W = %0D, FIFO_ADDR_W = %0D, IN_FIFO_SIZE = %0D, OUT_FIFO_SIZE = %0D, OVERFLOW = %0D, BIST = %0D",
      CHDR_W, NUM_PORTS, MEM_DATA_W, MEM_ADDR_W, FIFO_ADDR_W, IN_FIFO_SIZE, OUT_FIFO_SIZE, OVERFLOW, BIST
    );
    test.start_tb(tb_name);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();
    mem_clk_gen.start();

    // Start the BFMs running
    blk_ctrl.run();

    //
    // Run test procedures
    //
    test_reset();
    test_block_info();
    test_unused();
    test_registers();
    test_basic();
    test_single_byte();
    test_overflow();
    test_read_suppression();
    test_random();
    test_clear();
    test_bist();
    test_bist_throughput();

    // End the TB, but don't $finish, since we don't want to kill other 
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    mem_clk_gen.kill();
  end
endmodule
