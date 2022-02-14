//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_replay_tb
//
// Description:
//
//   Testbench for the Replay RFNoC block. This testbench is parameterizable to
//   test different configurations of the IP.
//
// Parameters:
//
//   CHDR_W     : CHDR_W parameter to use for the DUT
//   ITEM_W     : ITEM_W to use for this simulation
//   NUM_PORTS  : NUM_PORTS parameter to use for the DUT
//   MEM_DATA_W : MEM_DATA_W parameter to use for the DUT
//   MEM_ADDR_W : MEM_ADDR_W parameter to use for the DUT
//   TEST_REGS  : Controls whether or not to test the registers
//   TEST_FULL  : Controls whether or not to test filling the memory (may take
//                a long time to simulate).
//   STALL_PROB : Stall probability (0 to 100) to set on BFMs for this test
//

`default_nettype none


module rfnoc_block_replay_tb#(
  parameter int CHDR_W     = 64,
  parameter int ITEM_W     = 32,
  parameter int NUM_PORTS  = 1,
  parameter int MEM_DATA_W = 64,
  parameter int MEM_ADDR_W = 16,
  parameter int TEST_REGS  = 1,
  parameter int TEST_FULL  = 1,
  parameter int STALL_PROB = 25
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  `include "rfnoc_block_replay_regs.vh"

  `define MIN(X,Y) ((X)<(Y)?(X):(Y))


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  // Clock rates
  localparam real CHDR_CLK_PER = 5.0;    // 200 MHz
  localparam real CTRL_CLK_PER = 8.0;    // 125 MHz
  localparam real MEM_CLK_PER  = 3.0;    // 333.333 MHz

  // Block configuration
  localparam [ 9:0] THIS_PORTID = 10'h123;
  localparam [31:0] NOC_ID      = 32'h4E91A000;
  localparam int    MTU         = 10;

  // Data sizes
  localparam int SPP    = 128;                // Samples/items per packet
  localparam int BPP    = SPP * (ITEM_W/8);   // Bytes per packet

  // Simulation Data Randomness
  //
  // Currently, Vivado 2019.1.1 doesn't seed it's RNG properly, making it so we
  // can't regenerate the same sequence. So for now, I've turned off random.
  localparam bit USE_RANDOM = 0;     // Use random or sequential data

  // Useful constants
  localparam int MEM_WORD_SIZE  = MEM_DATA_W/8;
  localparam int CHDR_WORD_SIZE = CHDR_W/8;
  localparam int ITEM_SIZE      = ITEM_W/8;

  // Memory burst size in memory words, as defined in the axis_replay block.
  // This is essentially hard coded to match the maximum transfer size
  // supported by the DMA master.
  const int MEM_BURST_LEN =
    rfnoc_block_replay_i.gen_replay_blocks[0].axis_replay_i.MEM_BURST_LEN;

  // AXI alignment boundary, in bytes
  localparam AXI_ALIGNMENT = 4096;

  // Arbitrary timeout to use for each test. Just needs to be longer than any
  // test would take.
  time TEST_TIMEOUT = 500us;


  //---------------------------------------------------------------------------
  // Clocks and Resets
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
     mem_clk_gen        (.clk(mem_clk), .rst(mem_rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t  chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t       item_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_queue_t item_queue_t;

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], BPP);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // AXI Memory Model
  //---------------------------------------------------------------------------

  // AXI Write Address Channel
  wire [         NUM_PORTS*1-1:0] m_axi_awid;
  wire [    NUM_PORTS*MEM_ADDR_W-1:0] m_axi_awaddr;
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
  wire [    NUM_PORTS*MEM_ADDR_W-1:0] m_axi_araddr;
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
      .AWIDTH      (MEM_ADDR_W),
      .DWIDTH      (MEM_DATA_W),
      .IDWIDTH     (1),
      .BIG_ENDIAN  (0),
      .STALL_PROB  (STALL_PROB)
    ) sim_axi_ram_i (
      .s_aclk        (mem_clk),
      .s_aresetn     (~mem_rst),
      .s_axi_awid    (m_axi_awid[i]),
      .s_axi_awaddr  (m_axi_awaddr[i*MEM_ADDR_W +: MEM_ADDR_W]),
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
      .s_axi_araddr  (m_axi_araddr[i*MEM_ADDR_W +: MEM_ADDR_W]),
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
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tready;

  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT connections
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_replay #(
    .THIS_PORTID (THIS_PORTID),
    .CHDR_W      (CHDR_W),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS),
    .MEM_DATA_W  (MEM_DATA_W),
    .MEM_ADDR_W  (MEM_ADDR_W)
  ) rfnoc_block_replay_i (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
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

  // Write a 32-bit register
  task automatic write_reg(int port, bit [19:0] addr, bit [31:0] value);
    blk_ctrl.reg_write((2**REPLAY_ADDR_W)*port + addr, value);
  endtask : write_reg

  // Write a 64-bit register
  task automatic write_reg_64(int port, bit [19:0] addr, bit [63:0] value);
    blk_ctrl.reg_write((2**REPLAY_ADDR_W)*port + addr + 0, value[31: 0]);
    blk_ctrl.reg_write((2**REPLAY_ADDR_W)*port + addr + 4, value[63:32]);
  endtask : write_reg_64

  // Read a 32-bit register
  task automatic read_reg(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**REPLAY_ADDR_W)*port + addr, value[31: 0]);
  endtask : read_reg

  // Read a 32-bit register
  task automatic read_reg_64(int port, bit [19:0] addr, output logic [63:0] value);
    blk_ctrl.reg_read((2**REPLAY_ADDR_W)*port + addr + 0, value[31: 0]);
    blk_ctrl.reg_read((2**REPLAY_ADDR_W)*port + addr + 4, value[63:32]);
  endtask : read_reg_64


  // Generate a set of data items to use as a test payload. The seed argument
  // can be used to seed the initial value so that the same sequence of data is
  // repeated.
  function automatic item_queue_t gen_test_data(
    int num_items,
    integer seed = 32'hX
  );
    item_t queue[$];

    if (USE_RANDOM) begin
      // Generate random data, to make it unlikely we get correct value by
      // coincidence.
      if (seed !== 32'hX) begin
        // Seed the RNG with the indicated value
        std::process p;
        p = process::self();
        p.srandom(seed);
      end
      for (int i = 0; i < num_items; i++) begin
        queue.push_back($urandom());
      end
    end else begin
      // Generate sequential data, for easier debugging.
      static int count = 0;
      item_t first = 0;
      if (seed !== 32'hX) first = seed;
      else begin
        first = count;   // Continue after the last value from previous run
        count = count + num_items;
      end
      for (int i = 0; i < num_items; i++) begin
        queue.push_back(first+i);
      end
    end

    return queue;
  endfunction : gen_test_data


  // Read out and discard all packets received, stopping after there's been no
  // new packets received for a delay of "timeout".
  task automatic flush_rx(
    input int  port    = 0,
    input time timeout = CHDR_CLK_PER*100
  );
    ChdrPacket #(CHDR_W) chdr_packet;
    time                 prev_time;

    begin
      prev_time = $time;

      while (1) begin
        // Check if there's a frame waiting
        if (blk_ctrl.num_received(port) != 0) begin
          // Read frame
          blk_ctrl.get_chdr(port, chdr_packet);
          // Restart timeout
          prev_time = $time;

        end else begin
          // If timeout has expired, we're done
          if ($time - prev_time > timeout) break;
          // No frame, so wait a cycle
          #(CHDR_CLK_PER);
        end
      end
    end
  endtask : flush_rx


  // Wait until the expected number of bytes are accumulated in the record
  // buffer. Produce a failure if the data never arrives.
  task automatic wait_record_fullness(
    input int  port,
    input int  num_bytes,
    input time timeout = (10 + num_bytes) * CHDR_CLK_PER * 10
  );
    time         prev_time;
    logic [63:0] value;

    // Poll REG_REC_FULLNESS until fullness is reached
    prev_time = $time;
    while (1) begin
      read_reg_64(port, REG_REC_FULLNESS_LO, value);
      if (value >= num_bytes) break;

      // Check if it's taking too long
      if ($time - prev_time > timeout) begin
        `ASSERT_ERROR(0, "Timeout waiting for fullness to be reached");
      end
    end
  endtask : wait_record_fullness


  // Validate record position
  task automatic validate_record_position(
    input int     port,
    input longint position
  );
    logic [63:0] value;

    read_reg_64(port, REG_REC_POS_LO, value);
    `ASSERT_ERROR(value == position, $sformatf("Record position expected at 0x%h, but at 0x%h", position, value));
  endtask : validate_record_position


  // Validate record position
  task automatic validate_play_position(
    input int     port,
    input longint position
  );
    logic [63:0] value;

    read_reg_64(port, REG_PLAY_POS_LO, value);
    `ASSERT_ERROR(value == position, $sformatf("Play position expected at 0x%h, but at 0x%h", position, value));
  endtask : validate_play_position


  // Make sure nothing is received until the timeout has elapsed
  task automatic check_rx_idle(
    input int  port,
    input time timeout = CHDR_CLK_PER * 1000
  );
    int active;
    time start_time;

    @(negedge rfnoc_chdr_clk);
    start_time = $time;
    fork
      begin : tvalid_check
        // Wait for any activity on tvalid (should be 0 initially)
        while (!m_rfnoc_chdr_tvalid[port])
          @(posedge rfnoc_chdr_clk);
        if ($time - start_time < timeout)
          `ASSERT_ERROR(0, "Received additional data during idle");
      end
      begin
        // Just wait for the time to elapse
        #(timeout);
      end
    join_any
    disable tvalid_check;
  endtask : check_rx_idle


  // Record data and start its playback
  //
  //   port        : Replay block port to use
  //   send_items  : Data to send to the replay block to be recorded
  //   buffer_size : Buffer size in bytes to configure for record buffer
  //   num_items   : Number of items to play back
  //   spp         : Samples per packet for playback
  //   base_addr   : Base address to use for record buffer
  //   continuous  : Set to 1 for continuous playback, 0 for num_items only
  //   timestamp   : Timestamp to use for playback
  //
  task automatic start_replay (
    input int              port,
    input item_t           send_items[$],
    input longint unsigned buffer_size = 1024 * MEM_WORD_SIZE,
    input longint unsigned num_items   = 1024,
    input int              spp         = SPP,
    input longint unsigned base_addr   = 0,
    input bit              continuous  = 1'b0,
    input logic [63:0]     timestamp   = 64'bX
  );
    logic [31:0] cmd;
    bit          has_time;
    int          expected_fullness;

    // Check for bad input arguments
    `ASSERT_FATAL(num_items * ITEM_W % MEM_DATA_W == 0,
      "num_items to play back must be a multiple of the memory data word size");
    `ASSERT_FATAL(base_addr < 2**MEM_ADDR_W,
      "Base address is beyond available memory");
    `ASSERT_FATAL(base_addr + buffer_size <= 2**MEM_ADDR_W,
      "Buffer size extends beyond available memory");
    `ASSERT_FATAL(spp * ITEM_SIZE % MEM_WORD_SIZE == 0,
      "Requested SPP must be a multiple of the memory word size");
    `ASSERT_FATAL(spp <= 2**MTU,
      "Requested SPP exceeds MTU");

    // Update record buffer settings
    write_reg_64(port, REG_REC_BASE_ADDR_LO,    base_addr);
    write_reg_64(port, REG_REC_BUFFER_SIZE_LO,  buffer_size);
    write_reg_64(port, REG_PLAY_BASE_ADDR_LO,   base_addr);
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, buffer_size);
    write_reg   (port, REG_PLAY_WORDS_PER_PKT,  spp * ITEM_SIZE / MEM_WORD_SIZE);
    write_reg   (port, REG_PLAY_ITEM_SIZE,      ITEM_W/8);

    // Restart the record buffer
    write_reg(port, REG_REC_RESTART, 0);

    // Write the payload to the record buffer
    blk_ctrl.send_packets_items(port, send_items);

    // Wait until all the data has been written (up to the size of the buffer)
    expected_fullness = send_items.size() * (ITEM_W/8) < buffer_size ?
        send_items.size() * ITEM_SIZE : buffer_size;
    wait_record_fullness(port, expected_fullness);

    // Set the timestamp for playback
    if (timestamp !== 64'bX) begin
      write_reg_64(port, REG_PLAY_CMD_TIME_LO, timestamp);
      has_time = 1'b1;
    end else begin
      has_time = 1'b0;
    end

    // Start playback of the recorded data
    if (num_items != 0) begin
      cmd = continuous ? PLAY_CMD_CONTINUOUS : PLAY_CMD_FINITE;
      cmd = cmd | (32'(has_time) << REG_PLAY_TIMED_POS);
      write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, num_items*ITEM_W/MEM_DATA_W);
      write_reg(port, REG_PLAY_CMD, (32'(has_time) << REG_PLAY_TIMED_POS) | cmd);
    end
  endtask : start_replay


  // Read the data on the indicated port and check that it matches exp_items.
  // Also check that the length of the packets received add up to the length of
  // exp_items, if it takes multiple packets. An error string is returned if
  // there's an error, otherwise an empty string is returned.
  //
  //  port      : Port on which to receive and verify the data
  //  error_msg : Output string to write error message to, if any
  //  exp_items : Queue of the items you expect to receive
  //  eob       : Indicates if we expect EOB to be set for the last item (set
  //              to 1'bX to skip this check)
  //  spp       : Samples per packet to expect. Set to 0 to skip this check.
  // timestamp  : The timestamp we expect to receive for the first item (set to
  //              'X to skip timestamp checking)
  //
  task automatic verify_rx_data(
    input int           port,
    output string       error_msg,
    input item_t        exp_items[$],
    input logic         eob       = 1'b1,
    input int           spp       = SPP,
    input logic [63:0]  timestamp = 64'bX
  );
    item_t           recv_items[$];
    chdr_word_t      md[$];
    packet_info_t    pkt_info;
    int              item_count;
    int              packet_count;
    item_t           expected_value;
    item_t           actual_value;
    chdr_timestamp_t expected_time;

    item_count    = 0;
    packet_count  = 0;
    error_msg     = "";
    expected_time = timestamp;

    while (item_count < exp_items.size()) begin
      // Grab the next frame
      blk_ctrl.recv_items_adv(port, recv_items, md, pkt_info);

      // Check the packet length
      if (item_count + recv_items.size() > exp_items.size()) begin
        $sformat(error_msg,
          "On packet %0d, size is %0d items, which exceeds expected by %0d items.",
          packet_count, recv_items.size(),
          (item_count + recv_items.size()) - exp_items.size());
        return;
      end

      // Check the EOB flag
      if (eob !== 1'bX) begin
        if (item_count + recv_items.size() >= exp_items.size()) begin
          // This is the last packet, so make sure EOB matches expected value
          if (pkt_info.eob != eob) begin
            $sformat(error_msg,
                     "On packet %0d, expected EOB to be %0b, actual is %0b",
                     packet_count, eob, pkt_info.eob);
            return;
          end
        end else begin
          // This is NOT the last packet, so EOB should be 0
          if (pkt_info.eob != 1'b0) begin
            $display("item_count is %0d", item_count);
            $display("recv size is %0d", recv_items.size());
            $display("packet_count is %0d", packet_count);
            $display("Expected items is %0d", exp_items.size());
            $sformat(error_msg,
                     "On packet %0d, expected EOB to be 0 mid-burst, actual is %0b",
                     packet_count, pkt_info.eob);
            return;
          end
        end
      end

      // Check the payload length against expected SPP
      if (spp > 0) begin
        // If this is NOT the last packet, then its length should match the SPP.
        // If it is the last packet, then it can be shorter than SPP.
        if (!pkt_info.eob) begin
          if (recv_items.size() != spp) begin
            $sformat(error_msg,
              "On packet %0d, expected SPP of %0d but packet has %0d items",
              packet_count, spp, recv_items.size());
          end
        end else begin
          if (recv_items.size() > spp) begin
            $sformat(error_msg,
              "On packet %0d (EOB), expected SPP of %0d or less but packet has %0d items",
              packet_count, spp, recv_items.size());
          end
        end
      end

      // Check the timestamp (only first packet should have a timestamp)
      if (timestamp !== 64'bX && packet_count == 0) begin
        if (!pkt_info.timestamp) begin
          $sformat(error_msg,
                   "On packet %0d, timestamp is missing",
                   packet_count);
          return;
        end
        if (expected_time != pkt_info.timestamp) begin
          $sformat(error_msg,
                   "On packet %0d, expected timestamp %X but received %X",
                   packet_count, expected_time, pkt_info.timestamp);
          return;
        end
        expected_time = expected_time + recv_items.size();
      end else begin
        // Make sure we don't have a timestamp unexpectedly
        if (pkt_info.has_time) begin
          $sformat(error_msg,
                   "On packet %0d, expected no timestamp but received one",
                   packet_count);
        end
      end

      packet_count++;

      // Check the payload data
      for (int i = 0; i < recv_items.size(); i++) begin
        expected_value = exp_items[item_count];
        actual_value   = recv_items[i];
        if (actual_value != expected_value) begin
          $sformat(error_msg,
                   "On item %0d (packet %0d, item index %0d), Expected: 0x%x, Received: 0x%x",
                   item_count, packet_count, i, expected_value, actual_value);
          return;
        end
        item_count++;
      end
    end
  endtask : verify_rx_data


  //---------------------------------------------------------------------------
  // Register Test Tasks
  //---------------------------------------------------------------------------

  // Test a read/write register for correct functionality
  //
  //   port          : Replay block port to use
  //   addr          : Register byte address
  //   width         : Register size (32 or 64-bit)
  //   num_bits      : Number of bits actually used by register
  //   initial_value : Value we expect to read initially
  //
  task automatic test_read_write_reg(
    int          port,
    bit   [19:0] addr,
    int          width,
    int          num_bits = width,
    logic [63:0] initial_value = 0
  );
    string err_msg;

    err_msg = $sformatf("Register 0x%X failed read/write test: ", addr);

    if (width <= 32) begin
      logic [31:0] value;
      logic [31:0] expected;

      // Check initial value
      expected = initial_value;
      read_reg(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "initial value"});

      // Test writing 0
      expected = 0;
      write_reg(port, addr, expected);
      read_reg(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "write zero"});

      // Write maximum value
      expected = (64'(1'b1) << num_bits) - 1;
      write_reg(port, addr, '1);
      read_reg(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "write max value"});

      // Restore original value
      write_reg(port, addr, initial_value);

    end else begin
      logic [63:0] value;
      logic [63:0] expected;

      // Check initial value
      expected = initial_value;
      read_reg_64(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "initial value"});

      // Test writing 0
      expected = 0;
      write_reg_64(port, addr, expected);
      read_reg_64(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "write zero"});

      // Write maximum value
      expected = (64'(1'b1) << num_bits) - 1;
      write_reg_64(port, addr, '1);
      read_reg_64(port, addr, value);
      `ASSERT_ERROR(value == expected, {err_msg, "write max value"});

      // Restore original value
      write_reg(port, addr, initial_value);
    end
  endtask : test_read_write_reg


  // Test a read-only register for correct functionality
  //
  //   port      : Replay block port to use
  //   addr      : Register byte address
  //   width     : Register size (32 or 64-bit)
  //   exp_value : Value we expect to read
  //
  task automatic test_read_reg(
    int          port,
    bit   [19:0] addr,
    int          width,
    logic [63:0] exp_value = 0
  );
    string err_msg;
    logic [63:0] value;

    err_msg = $sformatf("Register 0x%X failed read test", addr);
    if (width <= 32) begin
      read_reg(port, addr, value);
      value[63:32] = 0;
    end else begin
      read_reg_64(port, addr, value);
    end

    `ASSERT_ERROR(value == exp_value, err_msg);
  endtask : test_read_reg


  //---------------------------------------------------------------------------
  // Test block info
  //---------------------------------------------------------------------------

  task automatic test_block_info();
    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();
  endtask : test_block_info


  //---------------------------------------------------------------------------
  // Test registers
  //---------------------------------------------------------------------------
  //
  // This test confirms that all read/write registers are appropriately
  // readable, writable, and update as expected.
  //
  //---------------------------------------------------------------------------

  task automatic test_registers(int port = 0);
    if (TEST_REGS) begin
      int major, minor, compat;
      test.start_test("Test registers", TEST_TIMEOUT);

      // Determine expected compat value
      major = rfnoc_block_replay_i.gen_replay_blocks[0].axis_replay_i.COMPAT_MAJOR;
      minor = rfnoc_block_replay_i.gen_replay_blocks[0].axis_replay_i.COMPAT_MINOR;
      compat = (major <<  REG_MAJOR_POS) | (minor <<  REG_MINOR_POS);

      test_read_reg      (port, REG_COMPAT,                32, compat);
      test_read_write_reg(port, REG_REC_BASE_ADDR_LO,      64, MEM_ADDR_W);
      test_read_write_reg(port, REG_REC_BUFFER_SIZE_LO,    64, MEM_ADDR_W+1);
      test_read_write_reg(port, REG_PLAY_BASE_ADDR_LO,     64, MEM_ADDR_W);
      test_read_write_reg(port, REG_PLAY_BUFFER_SIZE_LO,   64, MEM_ADDR_W+1);
      test_read_write_reg(port, REG_PLAY_CMD_NUM_WORDS_LO, 64, 64);
      test_read_write_reg(port, REG_PLAY_CMD_TIME_LO,      64, 64);
      test_read_write_reg(port, REG_PLAY_WORDS_PER_PKT,    32,
        REG_PLAY_WORDS_PER_PKT_LEN, REG_PLAY_WORDS_PER_PKT_INIT);

      // The following registers are read only:
      test_read_reg(port, REG_MEM_SIZE, 32,
        (32'(MEM_DATA_W) <<  REG_DATA_SIZE_POS) |
        (32'(MEM_ADDR_W) <<  REG_ADDR_SIZE_POS)
      );
      test_read_reg(port, REG_REC_FULLNESS_LO, 64);
      test_read_reg(port, REG_PLAY_ITEM_SIZE, 32, 4);
      test_read_reg(port, REG_REC_POS_LO, 64);
      test_read_reg(port, REG_PLAY_POS_LO, 64);
      test_read_reg(port, REG_PLAY_CMD_FIFO_SPACE, 32, 32);

      // The following registers are write only and aren't tested here.
      // REG_REC_RESTART - Tested during every record operation
      // REG_PLAY_CMD    - Tested during every playback operation

      test.end_test();
    end
  endtask : test_registers


  //---------------------------------------------------------------------------
  // Test basic recording and playback
  //---------------------------------------------------------------------------
  //
  // A quick and easy test to make sure replay is working.
  //
  //---------------------------------------------------------------------------

  task automatic test_basic(int port = 0);
    item_t        send_items[$];
    string        error_string;
    logic  [31:0] cmd;

    test.start_test("Basic recording and playback", TEST_TIMEOUT);

    // Configure buffers registers
    write_reg_64(port, REG_REC_BASE_ADDR_LO,    1024);
    write_reg_64(port, REG_REC_BUFFER_SIZE_LO,  BPP);
    write_reg_64(port, REG_PLAY_BASE_ADDR_LO,   1024);
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, BPP);
    write_reg   (port, REG_PLAY_WORDS_PER_PKT,  SPP * ITEM_SIZE/MEM_WORD_SIZE);
    write_reg   (port, REG_REC_RESTART,         0);

    // Send a random packet
    send_items = gen_test_data(SPP);
    blk_ctrl.send_items(port, send_items);

    // Wait until all the data has been written
    wait_record_fullness(port, SPP*ITEM_SIZE);

    // Start replay
    cmd = PLAY_CMD_FINITE;

    write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, SPP*ITEM_SIZE/MEM_WORD_SIZE);
    write_reg(port, REG_PLAY_CMD, cmd);

    // Check the output
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_basic


  //---------------------------------------------------------------------------
  // Test packet sizes
  //---------------------------------------------------------------------------
  //
  // Test boundary conditions where the packet size is close to the memory
  // burst size and the WORDS_PER_PKT size.
  //
  //---------------------------------------------------------------------------

  task automatic test_packet_sizes(int port = 0);
    item_t send_items[$];
    string error_string;
    int    buffer_size;
    int    mem_words, num_items, spp;

    test.start_test("Test packet sizes", TEST_TIMEOUT);

    // Calculate buffer size in bytes (2 memory bursts)
    buffer_size = 2*MEM_BURST_LEN * MEM_WORD_SIZE;

    // Calculate one memory burst size in words
    mem_words = 2*MEM_BURST_LEN;
    num_items = mem_words * MEM_WORD_SIZE / ITEM_SIZE;

    // Generate payload to use for testing (2 memory burst)
    send_items = gen_test_data(num_items);

    // For each test below, we record two memory bursts and playback two memory
    // bursts. Each time we change the playback packet size to test boundary
    // conditions.
    //
    // Note that we can't let the SPP exceed the MTU, so if the burst size is
    // too large, we won't be testing the full size.

    // Test packet size equals burst size
    spp = `MIN(MEM_BURST_LEN * MEM_WORD_SIZE / ITEM_SIZE, 2**MTU);
    start_replay(port, send_items, buffer_size, num_items, spp);
    verify_rx_data(port, error_string, send_items, 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // Test packet is one less than burst size
    spp = `MIN((MEM_BURST_LEN-1) * MEM_WORD_SIZE / ITEM_SIZE, 2**MTU-(MEM_WORD_SIZE / ITEM_SIZE));
    start_replay(port, send_items, buffer_size, num_items, spp);
    verify_rx_data(port, error_string, send_items, 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // Test packet is one more than burst size
    spp = `MIN((MEM_BURST_LEN+1) * MEM_WORD_SIZE / ITEM_SIZE, 2**MTU);
    start_replay(port, send_items, buffer_size, num_items, spp);
    verify_rx_data(port, error_string, send_items, 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // For each test below, we record two memory bursts and playback one memory
    // burst plus or minus one word, keeping the packet size the same.
    spp = `MIN(MEM_BURST_LEN * MEM_WORD_SIZE / ITEM_SIZE, 2**MTU);

    // Playback one less than burst/packet size
    start_replay(port, send_items, buffer_size, spp-MEM_WORD_SIZE/ITEM_SIZE, spp);
    verify_rx_data(port, error_string, send_items[0:spp-1-MEM_WORD_SIZE/ITEM_SIZE], 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // Playback one more than burst/packet size
    start_replay(port, send_items, buffer_size, spp+MEM_WORD_SIZE/ITEM_SIZE, spp);
    verify_rx_data(port, error_string, send_items[0:spp-1+MEM_WORD_SIZE/ITEM_SIZE], 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_packet_sizes


  //---------------------------------------------------------------------------
  // Test small replay
  //---------------------------------------------------------------------------
  //
  // Make sure the smallest possible replay size works correctly.
  //
  //---------------------------------------------------------------------------

  task automatic test_small_replay(int port = 0);
    item_t send_items[$];
    string error_string;
    int    mem_words, num_items;

    test.start_test("Test small replay", TEST_TIMEOUT);

    // Test the smallest size we can
    mem_words = 1;
    num_items = mem_words * MEM_WORD_SIZE / ITEM_SIZE;

    send_items = gen_test_data(num_items);
    start_replay(port, send_items, BPP, num_items);
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    test.end_test();
  endtask : test_small_replay


  //---------------------------------------------------------------------------
  // Test playback that's larger than buffer
  //---------------------------------------------------------------------------
  //
  // We want to make sure that playback wraps as expected back to the beginning
  // of the buffer and that buffers that aren't a multiple of the burst size
  // wrap correctly.
  //
  //---------------------------------------------------------------------------

  task automatic test_oversized_playback(int port = 0);
    item_t       send_items[$];
    item_t       exp_items[$];
    string       error_string;
    logic [31:0] cmd;
    int          buffer_size;
    int          num_items_rec;
    int          num_items_play;

    test.start_test("Test oversized playback", TEST_TIMEOUT);

    // Set number of words to test
    buffer_size    = (3 * MEM_BURST_LEN) / 2 * MEM_WORD_SIZE; // 1.5 memory bursts in size (in bytes)
    num_items_rec  = buffer_size / ITEM_SIZE;                 // 1.5 memory bursts in size (in samples)
    num_items_play = 2 * MEM_BURST_LEN * MEM_WORD_SIZE /      // 2 memory bursts in size (in samples)
                     ITEM_SIZE;

    // Start playback of data
    send_items = gen_test_data(num_items_rec);
    start_replay(port, send_items, buffer_size, num_items_play);

    // Since we recorded 1.5 memory bursts and are playing back 2, we should
    // get the a repeat of the first third of data.
    exp_items = { send_items, send_items[0:num_items_rec/3-1] };
    verify_rx_data(port, error_string, exp_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_oversized_playback


  //---------------------------------------------------------------------------
  // Test continuous (infinite) playback
  //---------------------------------------------------------------------------

  task automatic test_continuous(int port = 0);
    item_t       send_items[$];
    string       error_string;
    logic [31:0] cmd;
    int          num_items, mem_words, spp;

    test.start_test("Test continuous mode", TEST_TIMEOUT);

    mem_words = 70;
    num_items = mem_words * MEM_WORD_SIZE / ITEM_SIZE;

    // Make sure the spp evenly divides our test size, or else we'll end a
    // check mid packet, which verify_rx_data doesn't support.
    spp = num_items / 2;
    `ASSERT_ERROR(num_items % spp == 0,
      "num_items should be a multiple of spp for this test");

    // Update record buffer settings
    write_reg_64(port, REG_REC_BASE_ADDR_LO,    0);
    write_reg_64(port, REG_REC_BUFFER_SIZE_LO,  num_items*ITEM_SIZE);
    write_reg_64(port, REG_PLAY_BASE_ADDR_LO,   0);
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, num_items*ITEM_SIZE);
    write_reg   (port, REG_PLAY_WORDS_PER_PKT,  spp * ITEM_SIZE/MEM_WORD_SIZE);

    // Restart the record buffer
    write_reg(port, REG_REC_RESTART, 0);

    // Write num_items to record buffer
    send_items = gen_test_data(num_items);
    blk_ctrl.send_items(port, send_items);

    // Wait until all the data has been written
    wait_record_fullness(port, num_items*ITEM_SIZE);

    // Make sure the REG_PLAY_CMD_NUM_WORDS value is ignored by setting it to
    // something smaller than what we receive.
    write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, 0);

    // Send command for continuous playback
    cmd = PLAY_CMD_CONTINUOUS;
    write_reg(port, REG_PLAY_CMD, cmd);

    // Check the output, looking for the full set of data, multiple times
    repeat (5) begin
      verify_rx_data(port, error_string, send_items, 0, spp);
      `ASSERT_ERROR(error_string == "", error_string);
    end

    // Send the stop command
    cmd = PLAY_CMD_STOP;
    write_reg(port, REG_PLAY_CMD, cmd);

    // Keep reading packets until we get the EOB
    begin
      item_t        recv_items[$];
      chdr_word_t   md[$];
      packet_info_t pkt_info;
      item_t        expected_value;
      item_t        actual_value;
      int           item_count = 0;
      do begin
        blk_ctrl.recv_items_adv(port, recv_items, md, pkt_info);

        // Check the data
        for (int i = 0; i < recv_items.size(); i++) begin
          expected_value = send_items[item_count];
          actual_value   = recv_items[i];
          `ASSERT_ERROR(
            actual_value == expected_value,
            $sformatf("Data mismatch while waiting for EOB. Expected: 0x%x, Received: 0x%x",
                      expected_value, actual_value)
          );
          item_count++;
          if (item_count >= send_items.size()) item_count = 0;
        end
      end while (pkt_info.eob != 1);
    end

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_continuous


  //---------------------------------------------------------------------------
  // Test changing the offset
  //---------------------------------------------------------------------------
  //
  // Change the offset to be near the maximum memory address then test filling
  // the buffer and playing it back.
  //
  //---------------------------------------------------------------------------

  task automatic test_offset(int port = 0);
    item_t           send_items[$];
    string           error_string;
    int              mem_words, num_items;
    int              buffer_size;
    longint unsigned base_addr;
    logic [63:0]     val64;

    test.start_test("Test offset", TEST_TIMEOUT);

    mem_words   = 32;    // Number of memory words to send for each record
    num_items   = mem_words * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = mem_words * MEM_WORD_SIZE;

    // Make our offset buffer_size before the end of the buffer
    base_addr = 2**MEM_ADDR_W - buffer_size;

    // Record and playback the data
    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, SPP, base_addr);

    // Check the result
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    // Check the fullness
    read_reg_64(port, REG_REC_FULLNESS_LO, val64);
    `ASSERT_ERROR(val64 == buffer_size, "Memory fullness is not correct");

    // Send more data, even though buffer should be full
    send_items = gen_test_data(num_items);
    blk_ctrl.send_items(port, send_items);

    // Give extra time for the data to get through
    #(CHDR_CLK_PER * num_items * 20);

    // Make sure fullness didn't change
    read_reg_64(port, REG_REC_FULLNESS_LO, val64);
    `ASSERT_ERROR(val64 == buffer_size, "Memory fullness is not correct");

    // Restart recording, to get the rest of the data
    write_reg(port, REG_REC_RESTART, 0);

    // Wait for the rest of the data to be recorded
    wait_record_fullness(port, buffer_size);

    // Restart recording
    write_reg(port, REG_REC_RESTART, 0);

    // Make sure the fullness went back to 0
    read_reg_64(port, REG_REC_FULLNESS_LO, val64);
    `ASSERT_ERROR(val64 == 0, "Memory fullness is not correct");

    test.end_test();
  endtask : test_offset


  //---------------------------------------------------------------------------
  // Test stopping with multiple commands queued
  //---------------------------------------------------------------------------

  task automatic test_stop_queue(int port = 0);
    item_t send_items[$];
    int    buffer_size;
    int    num_items;
    int    play_words;

    test.start_test("Test stopping with queued commands", TEST_TIMEOUT);

    // Configure a small buffer
    num_items = 64 * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = num_items * ITEM_SIZE;

    // Choose a huge number for playback
    play_words = 'h01000000;

    // Start playing back this huge number
    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, play_words);

    // Queue up a bunch more commands
    repeat (4) begin
      write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
    end

    // Stop playback (empty the queue)
    write_reg(port, REG_PLAY_CMD, PLAY_CMD_STOP);

    // Keep reading packets until we get the EOB.
    begin
      item_t        recv_items[$];
      chdr_word_t   md[$];
      packet_info_t pkt_info;
      item_t        expected_value;
      item_t        actual_value;
      int           item_count = 0;
      do begin
        blk_ctrl.recv_items_adv(port, recv_items, md, pkt_info);

        // Check the data
        for (int i = 0; i < recv_items.size(); i++) begin
          expected_value = send_items[item_count];
          actual_value   = recv_items[i];
          `ASSERT_ERROR(
            actual_value == expected_value,
            $sformatf("Data mismatch while waiting for EOB. Expected: 0x%x, Received: 0x%x",
                      expected_value, actual_value)
          );
          item_count++;
          if (item_count >= send_items.size()) item_count = 0;
        end
      end while (pkt_info.eob != 1);
    end

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_stop_queue


  //---------------------------------------------------------------------------
  // Test overfilled record buffer
  //---------------------------------------------------------------------------
  //
  // Record more words than the buffer can fit. Make sure we don't overflow our
  // buffer and make sure reading it back plays only the data that should have
  // been captured.
  //
  //---------------------------------------------------------------------------

  task automatic test_overfilled_record(int port = 0);
    item_t       send_items[$];
    item_t       exp_items[$];
    string       error_string;
    int          num_items, spp;
    int          num_items_buf;
    logic [63:0] val64;

    test.start_test("Test overfilled record buffer", TEST_TIMEOUT);

    // Choose the sizes we want to use for this test
    num_items     = 98*MEM_WORD_SIZE/ITEM_SIZE;    // Number of items to record
    num_items_buf = 44*MEM_WORD_SIZE/ITEM_SIZE;    // Size of buffer to use in items

    // Make sure the spp evenly divides our test size, or else we'll end a
    // check mid packet, which verify_rx_data doesn't support.
    spp = num_items_buf / 4;
    `ASSERT_ERROR(num_items_buf % spp == 0,
      "num_items_buf should be a multiple of spp for this test");

    // Restart the record buffer
    write_reg(port, REG_REC_RESTART, 0);

    // Generate more record data than can fit in the buffer
    send_items = gen_test_data(num_items);

    // Start playback of the larger size
    start_replay(port, send_items, num_items_buf*ITEM_SIZE, num_items, spp);

    // We should get two frames of num_items_buf, then one smaller frame to
    // bring us up to num_items total.
    exp_items = send_items[0 : num_items_buf-1];
    for (int i = 0; i < 2; i ++) begin
      verify_rx_data(port, error_string, exp_items, 0, spp);
      `ASSERT_ERROR(error_string == "", error_string);
    end
    exp_items = exp_items[0 : (num_items % num_items_buf)-1];
    verify_rx_data(port, error_string, exp_items, 1, spp);
    `ASSERT_ERROR(error_string == "", error_string);

    // Make sure REG_REC_FULLNESS didn't keep increasing
    read_reg_64(port, REG_REC_FULLNESS_LO, val64);
    `ASSERT_ERROR(
      val64 == num_items_buf*ITEM_SIZE,
      "REG_REC_FULLNESS went beyond expected bounds"
    );

    // Reset record buffer so that it accepts the rest of the data that's
    // stalled in input FIFO.
    write_reg_64(port, REG_REC_BUFFER_SIZE_LO, num_items*ITEM_SIZE);
    write_reg(port, REG_REC_RESTART, 0);

    // Wait until all the data has been written
    wait_record_fullness(port, (num_items - num_items_buf)*ITEM_SIZE);

    test.end_test();
  endtask : test_overfilled_record


  //---------------------------------------------------------------------------
  // Test burst size
  //---------------------------------------------------------------------------
  //
  // Record amount of data that's larger than the configured RAM burst length
  // to make sure full-length bursts are handled correctly.
  //
  //---------------------------------------------------------------------------

  task automatic test_burst_size(int port = 0);
    item_t send_items[$];
    string error_string;
    int    num_items, mem_words;
    int    buffer_size;

    test.start_test("Test burst size", TEST_TIMEOUT);

    mem_words = 4*MEM_BURST_LEN;                  // Multiple of the burst size
    num_items = mem_words * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = mem_words * MEM_WORD_SIZE;      // Size in bytes

    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items);
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    test.end_test();
  endtask : test_burst_size


  //---------------------------------------------------------------------------
  // Test 4K AXI boundary
  //---------------------------------------------------------------------------
  //
  // AXI doesn't allow bursts to cross a 4 KiB boundary. Make sure that we can
  // correctly replay up to and across this boundary.
  //
  //---------------------------------------------------------------------------

  task automatic test_4k_boundary(int port = 0);
    item_t send_items[$];
    string error_string;
    int    num_items, mem_words;
    int    buffer_size;
    int    base_addr;

    test.start_test("Test 4K AXI Boundary", TEST_TIMEOUT);

    //
    // Test bursting up to and after boundary
    //

    // Setup two bursts
    mem_words   = 2*MEM_BURST_LEN;
    num_items   = mem_words * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = mem_words * MEM_WORD_SIZE;    // Size in bytes

    // Choose a base address such that we end the first burst at the 4 KiB
    // boundary and start the next burst on the boundary.
    if (mem_words/2 * MEM_WORD_SIZE >= AXI_ALIGNMENT) begin
      // In this case our memory burst size is bigger than 4K, so we're
      // guaranteed to cross the 4K alignment boundary.
      base_addr = 0;
    end else begin
      base_addr = AXI_ALIGNMENT - (mem_words/2)*MEM_WORD_SIZE;
    end

    // Record data across the 4K boundary then play it back
    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, SPP, base_addr);

    // Verify the data
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    //
    // Test bursting across boundary
    //

    // Setup a single burst across the 4 KiB boundary
    mem_words   = MEM_BURST_LEN;
    num_items   = mem_words * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = mem_words * MEM_WORD_SIZE;    // Size in bytes

    // Choose a base address such that we end a burst on the 4 KiB boundary,
    // then continue on the other side.
    if (mem_words/2 * MEM_WORD_SIZE >= AXI_ALIGNMENT) begin
      // In this case our memory burst size is bigger than 4K, so we're
      // guaranteed to cross the 4K alignment boundary.
      base_addr = 0;
    end else begin
      base_addr = AXI_ALIGNMENT - (mem_words/2)*MEM_WORD_SIZE;
    end

    // Record data across the 4K boundary then play it back
    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, SPP, base_addr);

    // Verify the data received
    verify_rx_data(port, error_string, send_items, 1);
    `ASSERT_ERROR(error_string == "", error_string);

    test.end_test();
  endtask : test_4k_boundary


  //---------------------------------------------------------------------------
  // Test small packet size (smaller than memory burst size)
  //---------------------------------------------------------------------------

  task test_small_packet(int port = 0);
    item_t       send_items[$];
    string       error_string;
    logic [31:0] cmd;
    int          buffer_size;
    int          num_items;
    int          pkt_size_words;
    int          pkt_size_items;

    test.start_test("Test small packet size", TEST_TIMEOUT);

    //
    // Test smaller than burst size
    //

    buffer_size = 2 * MEM_BURST_LEN * MEM_WORD_SIZE;   // 2 memory bursts in size (in bytes)
    num_items   = buffer_size / ITEM_SIZE;             // Same as buffer_size (in items)

    pkt_size_words = MEM_BURST_LEN / 4;
    pkt_size_items = pkt_size_words * MEM_WORD_SIZE / ITEM_SIZE;

    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, pkt_size_items);

    // We should get 8 small packets with EOB set on the last packet.
    for (int k = 0; k < 8; k ++) begin
      verify_rx_data(port, error_string,
                     send_items[pkt_size_items*k : pkt_size_items*(k+1)-1],
                     (k == 7 ? 1 : 0), pkt_size_items);
      `ASSERT_ERROR(error_string == "", error_string);
    end

    //
    // Test shortest supported packet size (WPP = 2)
    //

    buffer_size = 2 * MEM_BURST_LEN * MEM_WORD_SIZE;   // 2 memory bursts in size (in bytes)
    num_items   = 20 * MEM_WORD_SIZE / ITEM_SIZE;      // 20 memory words

    pkt_size_words = 2;
    pkt_size_items = pkt_size_words * MEM_WORD_SIZE / ITEM_SIZE;

    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, pkt_size_items);

    // We should get many packets with length equal to the memory word size,
    // with EOB set on the last packet.
    for (int i=0; i < num_items; i += pkt_size_items) begin
      verify_rx_data(port, error_string, send_items[i:i+pkt_size_items-1],
        i == num_items-pkt_size_items ? 1 : 0, pkt_size_items);
      `ASSERT_FATAL(error_string == "", error_string);
    end

    test.end_test();
  endtask : test_small_packet


  //---------------------------------------------------------------------------
  // Test timed playback
  //---------------------------------------------------------------------------

  task automatic test_timed_playback(int port = 0);
    item_t       send_items[$];
    string       error_string;
    logic [64:0] timestamp;
    int          num_items, mem_words;
    int          buffer_size;
    int          spp;

    test.start_test("Test timed playback", TEST_TIMEOUT);

    mem_words   = MEM_BURST_LEN;
    num_items   = mem_words * MEM_WORD_SIZE / ITEM_SIZE;
    buffer_size = num_items * ITEM_SIZE;
    timestamp   = 64'h0123456789ABCDEF;

    // Set the packet size small enough so that we get multiple packets
    // (multiple timestamps).
    spp = num_items/8;

    send_items = gen_test_data(num_items);
    start_replay(port, send_items, buffer_size, num_items, spp, 0, 0, timestamp);

    verify_rx_data(port, error_string, send_items, 1, spp, timestamp);
    `ASSERT_ERROR(error_string == "", error_string);

    test.end_test();
  endtask : test_timed_playback


  //---------------------------------------------------------------------------
  // Test multiple ports
  //---------------------------------------------------------------------------
  //
  // This test ensures that working with one port isn't accidentally affecting
  // another port. Their operation should be independent.
  //
  //---------------------------------------------------------------------------

  task automatic test_multiple_ports(int port = 0);
    // This test only applies if there is more than one port
    if (NUM_PORTS > 1) begin
      bit [63:0] val64;
      int        num_items;
      int        mem_words;
      item_t     test_data[$], port0_data[$], port1_data[$];
      string     error_str;

      // We test port+0 and port+1
      `ASSERT_FATAL(NUM_PORTS > port+1, "Not enough ports for this test");

      test.start_test("Test multiple ports", TEST_TIMEOUT);

      //-----------------------------------------
      // Verify that registers are independent
      //-----------------------------------------

      write_reg_64(port+0, REG_REC_BASE_ADDR_LO, 'hA);
      write_reg_64(port+1, REG_REC_BASE_ADDR_LO, 'hB);
      read_reg_64(port+0, REG_REC_BASE_ADDR_LO, val64);
      write_reg(port+0, REG_PLAY_WORDS_PER_PKT, 'hC);
      write_reg(port+1, REG_PLAY_WORDS_PER_PKT, 'hD);
      `ASSERT_ERROR(val64 == 'hA, "Register didn't read correct value");
      read_reg_64(port+1, REG_REC_BASE_ADDR_LO, val64);
      `ASSERT_ERROR(val64 == 'hB, "Register didn't read correct value");
      read_reg(port+0, REG_PLAY_WORDS_PER_PKT, val64);
      `ASSERT_ERROR(val64 == 'hC, "Register didn't read correct value");
      read_reg(port+1, REG_PLAY_WORDS_PER_PKT, val64);
      `ASSERT_ERROR(val64 == 'hD, "Register didn't read correct value");

      //-----------------------------------------------
      // Verify the memory interfaces are independent
      //-----------------------------------------------

      // Configure the two interfaces using the same settings and make sure
      // they read independently. This assumes that each port has its own
      // address space in the attached memories.

      for (int i = port; i < 2; i++) begin
        write_reg_64(i, REG_REC_BASE_ADDR_LO,    0);
        write_reg_64(i, REG_REC_BUFFER_SIZE_LO,  BPP);
        write_reg_64(i, REG_PLAY_BASE_ADDR_LO,   0);
        write_reg_64(i, REG_PLAY_BUFFER_SIZE_LO, BPP);
        write_reg(i, REG_REC_RESTART, 0);
      end

      num_items = BPP / ITEM_SIZE;
      mem_words = BPP / MEM_WORD_SIZE;
      test_data = gen_test_data(2*num_items);
      port0_data = test_data[        0 :   num_items-1];
      port1_data = test_data[num_items : 2*num_items-1];

      // Record the two test payloads
      blk_ctrl.send_items(port+0, port0_data);
      blk_ctrl.send_items(port+1, port1_data);

      // Play back on each port
      write_reg(port+0, REG_PLAY_WORDS_PER_PKT, SPP*ITEM_SIZE/MEM_WORD_SIZE);
      write_reg(port+1, REG_PLAY_WORDS_PER_PKT, SPP*ITEM_SIZE/MEM_WORD_SIZE);
      write_reg_64(port+0, REG_PLAY_CMD_NUM_WORDS_LO, mem_words);
      write_reg_64(port+1, REG_PLAY_CMD_NUM_WORDS_LO, mem_words);
      write_reg(port+0, REG_PLAY_CMD, PLAY_CMD_FINITE);
      write_reg(port+1, REG_PLAY_CMD, PLAY_CMD_FINITE);

      // Check the output from each port
      verify_rx_data(port+0, error_str, port0_data, 1);
      `ASSERT_ERROR(error_str == "", error_str);
      verify_rx_data(port+1, error_str, port1_data, 1);
      `ASSERT_ERROR(error_str == "", error_str);

      test.end_test();
    end

  endtask : test_multiple_ports


  //---------------------------------------------------------------------------
  // Test Record and Play position
  //---------------------------------------------------------------------------
  //
  // Test record and play positions advance properly.  Checks position after
  // recording and playing each packet and checks proper position during wraps.
  //
  //---------------------------------------------------------------------------

  task test_position(int port = 0);
    item_t           send_items[$];
    item_t           recv_items[$];
    int              num_items, num_packets;
    longint unsigned mem_size;
    logic [63:0]     val64;

    test.start_test("Test record and play positions", 2ms);

    mem_size    = 2**MEM_ADDR_W;    // Memory size in bytes
    num_items   = 2**$clog2(SPP);   // Pick a power of 2 near SPP
    num_packets = 5;

    // Set up the record buffer
    write_reg_64(port, REG_REC_BASE_ADDR_LO,    0);
    write_reg_64(port, REG_REC_BUFFER_SIZE_LO,  num_packets*num_items*ITEM_SIZE);
    write_reg   (port, REG_REC_RESTART,         0);

    // Fill the buffer and validate the record position after each packet.
    for (int i = 0; i < num_packets; i++) begin
      // Send a different random sequence for each packet
      send_items = gen_test_data(num_items, i*num_items);
      blk_ctrl.send_items(port, send_items);
      wait_record_fullness(port, (i+1)*num_items*ITEM_SIZE);
      validate_record_position(port, ((i+1)*num_items*ITEM_SIZE));
    end

    // Send one additional packet to ensure the position does not advance
    send_items = gen_test_data(num_items, num_packets*num_items);
    blk_ctrl.send_items(port, send_items);
  
    // Give extra time for the last packet
    #(CHDR_CLK_PER * num_items * 20);

    // Make sure the position has not advanced and the fullness has not changed
    validate_record_position(port, num_packets*num_items*ITEM_SIZE);
    read_reg_64(port, REG_REC_FULLNESS_LO, val64);
    `ASSERT_ERROR(val64 == num_packets*num_items*ITEM_SIZE, "Memory fullness is not correct");

    // Play back the data one packet at a time and validate play position
    write_reg(port, REG_PLAY_WORDS_PER_PKT, num_items*ITEM_SIZE/MEM_WORD_SIZE);
    write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, num_items*ITEM_SIZE/MEM_WORD_SIZE);
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, num_items*ITEM_SIZE);
    for (int i = 0; i < num_packets; i++) begin
      write_reg_64(port, REG_PLAY_BASE_ADDR_LO, i*num_items*ITEM_SIZE);
      write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
      send_items = gen_test_data(num_items, i*num_items);
      blk_ctrl.recv_items(port, recv_items);
      `ASSERT_ERROR(
        ChdrData#(CHDR_W, ITEM_W)::item_equal(send_items, recv_items),
        "Playback data did not match"
      );
      validate_play_position(port, ((i+1)*num_items*ITEM_SIZE));
    end

    // Restart recording to get the extra packet we sent at the beginning
    // to validate the record position wraps.
    write_reg(port, REG_REC_RESTART, 0);
    wait_record_fullness(port, num_items*ITEM_SIZE);
    validate_record_position(port, num_items*ITEM_SIZE);

    // Playback the new data, which should continue the values from the last
    // record operation.
    write_reg_64(port, REG_PLAY_BASE_ADDR_LO, 0);
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, num_items*ITEM_SIZE);
    write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, num_items*ITEM_SIZE/MEM_WORD_SIZE);
    write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
    send_items = gen_test_data(num_items, num_packets*num_items);
    blk_ctrl.recv_items(port, recv_items);
    `ASSERT_ERROR(
      ChdrData#(CHDR_W, ITEM_W)::item_equal(send_items, recv_items),
      "Playback data did not match"
    );
    validate_play_position(port, num_items*ITEM_SIZE);

    // Test the play wrap.  Play num_packets plus one and validate position.
    // This is done at the end of the memory space to test the wrap within the
    // buffer space as well as the wrap at the end of the memory space.
    write_reg_64(port, REG_PLAY_BASE_ADDR_LO, mem_size-(num_packets*num_items*ITEM_SIZE));
    write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, num_packets*num_items*ITEM_SIZE);
    write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, (num_packets+1)*num_items*ITEM_SIZE/MEM_WORD_SIZE);
    write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
    for (int i = 0; i < num_packets+1; i++) begin
      blk_ctrl.recv_items(port, recv_items);
    end
    validate_play_position(port, mem_size-((num_packets-1)*num_items*ITEM_SIZE));

    // Make sure there are no more packets
    check_rx_idle(port);

    test.end_test();
  endtask : test_position


  //---------------------------------------------------------------------------
  // Test Filling the memory
  //---------------------------------------------------------------------------
  //
  // In this test we configure the buffers to use the entire memory, then send
  // more than a full memory worth of data. This verifies that the fullness is
  // correct up to the maximum size and that no overflow occurs.
  //
  //---------------------------------------------------------------------------

  task test_full_memory(int port = 0);
    // This test can take a long time, so we only run it if enabled by the
    // parameter.
    if (TEST_FULL) begin
      item_t           send_items[$];
      item_t           recv_items[$];
      int              num_items, num_packets;
      longint unsigned mem_size;
      logic [63:0]     val64;

      test.start_test("Test full memory", 2ms);

      mem_size    = 2**MEM_ADDR_W;    // Memory size in bytes
      num_items   = 2**$clog2(SPP);   // Pick a power of 2 near SPP
      num_packets = mem_size / ITEM_SIZE / num_items;

      // Set up entire memory as the record/playback buffer
      write_reg_64(port, REG_REC_BASE_ADDR_LO,    0);
      write_reg_64(port, REG_REC_BUFFER_SIZE_LO,  mem_size);
      write_reg_64(port, REG_PLAY_BASE_ADDR_LO,   0);
      write_reg_64(port, REG_PLAY_BUFFER_SIZE_LO, mem_size);
      write_reg   (port, REG_PLAY_WORDS_PER_PKT,  num_items * ITEM_SIZE / MEM_WORD_SIZE);
      write_reg   (port, REG_REC_RESTART,         0);

      // Send enough data to fill the buffer, plus an extra packet
      for (int i = 0; i < num_packets+1; i++) begin
        // Send a different random sequence for each packet
        send_items = gen_test_data(num_items, i*num_items);
        blk_ctrl.send_items(port, send_items);
      end

      // Wait for the memory to fill
      wait_record_fullness(port, mem_size);

      // Give extra time for the last packet
      #(CHDR_CLK_PER * num_items * 20);

      // Check the fullness
      read_reg_64(port, REG_REC_FULLNESS_LO, val64);
      `ASSERT_ERROR(val64 == mem_size, "Memory fullness is not correct");

      // Play back the entire memory, plus one word, which should wrap around
      write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, mem_size / MEM_WORD_SIZE + 1);
      write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
      for (int i = 0; i < num_packets; i++) begin
        // Regenerate the same sequence of test data
        send_items = gen_test_data(num_items, i*num_items);
        blk_ctrl.recv_items(port, recv_items);
        `ASSERT_ERROR(
          ChdrData#(CHDR_W, ITEM_W)::item_equal(send_items, recv_items),
          "Playback data did not match"
        );
      end
      // Verify the last word
      send_items = gen_test_data(MEM_WORD_SIZE/ITEM_SIZE, 0);
      blk_ctrl.recv_items(port, recv_items);
      $display("received: %p", recv_items);
      $display("expected: %p", send_items);
      `ASSERT_ERROR(
       ChdrData#(CHDR_W, ITEM_W)::item_equal(send_items, recv_items),
       "Playback data did not match on last word"
      );

      // Restart recording to get the extra packet we sent at the beginning
      write_reg(port, REG_REC_RESTART, 0);
      wait_record_fullness(port, num_items*ITEM_SIZE);

      // Playback the new data, which should continue the values from the last
      // record operation.
      write_reg_64(port, REG_PLAY_CMD_NUM_WORDS_LO, num_items * ITEM_SIZE / MEM_WORD_SIZE);
      write_reg(port, REG_PLAY_CMD, PLAY_CMD_FINITE);
      send_items = gen_test_data(num_items, num_packets*num_items);
      blk_ctrl.recv_items(port, recv_items);
      `ASSERT_ERROR(
        ChdrData#(CHDR_W, ITEM_W)::item_equal(send_items, recv_items),
        "Playback data did not match"
      );

      // Make sure there are no more packets
      check_rx_idle(port);

      test.end_test();
    end
  endtask : test_full_memory


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf( {
      "rfnoc_block_replay_tb\n",
      "CHDR_W     = %03d, ITEM_W     = %02d, NUM_PORTS = %0d,\n",
      "MEM_DATA_W = %03d, MEM_ADDR_W = %02d,\n",
      "TEST_REGS  = %03d, TEST_FULL  = %02d,\n",
      "STALL_PROB = %03d" },
      CHDR_W, ITEM_W, NUM_PORTS, MEM_DATA_W, MEM_ADDR_W, TEST_REGS, TEST_FULL, STALL_PROB
    );

    // Initialize the test exec object for this testbench
    test.start_tb(tb_name);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();
    mem_clk_gen.start();

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_block_info();
    for (int port = 0; port < NUM_PORTS; port++) begin
      // Run the basic tests on all ports
      test_registers(port);
      test_basic(port);
    end
    test_multiple_ports();
    test_packet_sizes();
    test_small_replay();
    test_oversized_playback();
    test_continuous();
    test_stop_queue();
    test_offset();
    test_overfilled_record();
    test_burst_size();
    test_4k_boundary();
    test_small_packet();
    test_timed_playback();
    test_position();
    test_full_memory();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results, bot don't $finish
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    mem_clk_gen.kill();
  end : tb_main

endmodule : rfnoc_block_replay_tb


`default_nettype wire
