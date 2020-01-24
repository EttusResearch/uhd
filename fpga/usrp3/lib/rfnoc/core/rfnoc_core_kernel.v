//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_core_kernel
// Description:
//   The main utility and software interface module for an
//   assembled rfnoc design
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - DEVICE_TYPE: The device type to use in the Device Info register
//   - DEVICE_FAMILY: The device family (to pass to Xilinx primitives)
//   - SAFE_START_CLKS: Instantiate logic to ensure that all output
//                      clocks are glitch-free and startup safely
//   - NUM_BLOCKS: Number of blocks instantiated in the design
//   - NUM_STREAM_ENDPOINTS: Number of stream EPs instantiated in the design
//   - NUM_ENDPOINTS_CTRL: Number of stream EPs connected to the ctrl crossbar
//   - NUM_TRANSPORTS: Number of transports instantiated in the design
//   - NUM_EDGES: Number of edges of static connection in the design
//   - CHDR_XBAR_PRESENT: 1 if the CHDR crossbar is present. If 0 then
//                        transports are directly connected to SEPs
//   - EDGE_TBL_FILE: The memory init file for the static connection
//                    adjacency list
//
// Signals:
//   - chdr_aclk : The input CHDR clock (may be unbuffered if SAFE_START_CLKS=1)
//   - chdr_aclk_locked : The PLL locked pin for the input CHDR clock (unused if SAFE_START_CLKS=0)
//   - ctrl_aclk : The input Control clock (may be unbuffered if SAFE_START_CLKS=1)
//   - ctrl_aclk_locked : The PLL locked pin for the input Control clock (unused if SAFE_START_CLKS=0)
//   - core_chdr_clk: Output stable CHDR clock for the rest of the design
//   - core_chdr_rst: Output sync CHDR reset for all infrastructure modules (not blocks)
//   - core_ctrl_clk: Output stable Control clock for the rest of the design
//   - core_ctrl_rst: Output sync Control reset for all infrastructure modules (not blocks)
//   - s_axis_ctrl_* : Slave AXIS-Ctrl for the primary (zero'th) control endpoint
//   - m_axis_ctrl_* : Master AXIS-Ctrl for the primary (zero'th) control endpoint
//   - device_id: The dynamic device_id to read through the Device Info register (domain: core_chdr_clk)
//   - rfnoc_core_config: The backend config port for all blocks in the design (domain: core_ctrl_clk)
//   - rfnoc_core_status: The backend status port for all blocks in the design (domain: core_ctrl_clk)

module rfnoc_core_kernel #(
  parameter [15:0] PROTOVER             = {8'd1, 8'd0},
  parameter [15:0] DEVICE_TYPE          = 16'd0,
  parameter        DEVICE_FAMILY        = "7SERIES",
  parameter        SAFE_START_CLKS      = 0,
  parameter [9:0]  NUM_BLOCKS           = 0,
  parameter [9:0]  NUM_STREAM_ENDPOINTS = 0,
  parameter [9:0]  NUM_ENDPOINTS_CTRL   = 0,
  parameter [9:0]  NUM_TRANSPORTS       = 0,
  parameter [11:0] NUM_EDGES            = 0,
  parameter [0:0]  CHDR_XBAR_PRESENT    = 1,
  parameter        EDGE_TBL_FILE        = ""
)(
  // Input clocks and resets
  input  wire                        chdr_aclk,
  input  wire                        chdr_aclk_locked,
  input  wire                        ctrl_aclk,
  input  wire                        ctrl_aclk_locked,
  input  wire                        core_arst,
  // Output clocks and resets
  output wire                        core_chdr_clk,
  output wire                        core_chdr_rst,
  output wire                        core_ctrl_clk,
  output wire                        core_ctrl_rst,
  // AXIS-Control Bus
  input  wire [31:0]                 s_axis_ctrl_tdata,
  input  wire                        s_axis_ctrl_tlast,
  input  wire                        s_axis_ctrl_tvalid,
  output wire                        s_axis_ctrl_tready,
  output wire [31:0]                 m_axis_ctrl_tdata,
  output wire                        m_axis_ctrl_tlast,
  output wire                        m_axis_ctrl_tvalid,
  input  wire                        m_axis_ctrl_tready,
  // Global info (domain: core_chdr_clk)
  input  wire [15:0]                 device_id,
  // Backend config/status for each block (domain: core_ctrl_clk)
  output wire [(512*NUM_BLOCKS)-1:0] rfnoc_core_config,
  input  wire [(512*NUM_BLOCKS)-1:0] rfnoc_core_status
);

  `include "rfnoc_axis_ctrl_utils.vh"
  `include "rfnoc_backend_iface.vh"

  // -----------------------------------
  // Clocking and Resets
  // -----------------------------------

  generate if (SAFE_START_CLKS == 1) begin
    // Safe startup logic for the CHDR and Control clocks:
    // chdr_aclk and ctrl_aclk can be unbuffered.
    // Use a BUFGCE to disable the clock until the upstream
    // PLLs have locked.

    wire chdr_ce_clk, ctrl_ce_clk;
    (* keep = "true" *) (* async_reg = "true" *) reg [7:0] chdr_clk_ce_shreg = 8'h0;
    (* keep = "true" *) (* async_reg = "true" *) reg [7:0] ctrl_clk_ce_shreg = 8'h0;

    // A glitch-free clock buffer with an enable
    BUFGCE chdr_clk_buf_i (
      .I (chdr_aclk),
      .CE(chdr_clk_ce_shreg[7]),
      .O (core_chdr_clk)
    );
    // A separate clock buffer for the CE signal
    // We instantiate this manually to prevent the tools from instantiating
    // the more scare BUFG here. There are a lot more BUFHs than BUFGs
    BUFH chdr_ce_buf_i (
      .I(chdr_aclk),
      .O(chdr_ce_clk)
    );
    always @(posedge chdr_ce_clk) begin
      chdr_clk_ce_shreg <= {chdr_clk_ce_shreg[6:0], chdr_aclk_locked};
    end

    // A glitch-free clock buffer with an enable
    BUFGCE ctrl_clk_buf_i (
      .I (ctrl_aclk),
      .CE(ctrl_clk_ce_shreg[7]),
      .O (core_ctrl_clk)
    );
    // A separate clock buffer for the CE signal
    // We instantiate this manually to prevent the tools from instantiating
    // the more scare BUFG here. There are a lot more BUFHs than BUFGs
    BUFH ctrl_ce_buf_i (
      .I(ctrl_aclk),
      .O(ctrl_ce_clk)
    );
    always @(posedge ctrl_ce_clk) begin
      ctrl_clk_ce_shreg <= {ctrl_clk_ce_shreg[6:0], ctrl_aclk_locked};
    end
  end else begin
    // We assume that chdr_aclk and ctrl_aclk start safely and are glitch-free
    assign core_chdr_clk = chdr_aclk;
    assign core_ctrl_clk = ctrl_aclk;
  end endgenerate

  reset_sync rst_sync_chdr_i (
    .clk(core_chdr_clk), .reset_in(core_arst), .reset_out(core_chdr_rst)
  );
  reset_sync rst_sync_ctrl_i (
    .clk(core_ctrl_clk), .reset_in(core_arst), .reset_out(core_ctrl_rst)
  );

  // -----------------------------------
  // AXIS-Ctrl Slave
  // -----------------------------------

  wire         ctrlport_req_wr;
  wire         ctrlport_req_rd;
  wire [19:0]  ctrlport_req_addr;
  wire [31:0]  ctrlport_req_data;
  reg          ctrlport_resp_ack;
  reg  [31:0]  ctrlport_resp_data;

  // The port ID of this endpoint must be zero
  localparam [9:0] RFNOC_CORE_PORT_ID = 10'd0;

  ctrlport_endpoint #(
    .THIS_PORTID(RFNOC_CORE_PORT_ID), .SYNC_CLKS(1),
    .AXIS_CTRL_MST_EN(0), .AXIS_CTRL_SLV_EN(1),
    .SLAVE_FIFO_SIZE(5)
  ) ctrlport_ep_i (
    .rfnoc_ctrl_clk           (core_ctrl_clk      ),
    .rfnoc_ctrl_rst           (core_ctrl_rst      ),
    .ctrlport_clk             (core_ctrl_clk      ),
    .ctrlport_rst             (core_ctrl_rst      ),
    .s_rfnoc_ctrl_tdata       (s_axis_ctrl_tdata  ),
    .s_rfnoc_ctrl_tlast       (s_axis_ctrl_tlast  ),
    .s_rfnoc_ctrl_tvalid      (s_axis_ctrl_tvalid ),
    .s_rfnoc_ctrl_tready      (s_axis_ctrl_tready ),
    .m_rfnoc_ctrl_tdata       (m_axis_ctrl_tdata  ),
    .m_rfnoc_ctrl_tlast       (m_axis_ctrl_tlast  ),
    .m_rfnoc_ctrl_tvalid      (m_axis_ctrl_tvalid ),
    .m_rfnoc_ctrl_tready      (m_axis_ctrl_tready ),
    .m_ctrlport_req_wr        (ctrlport_req_wr    ),
    .m_ctrlport_req_rd        (ctrlport_req_rd    ),
    .m_ctrlport_req_addr      (ctrlport_req_addr  ),
    .m_ctrlport_req_data      (ctrlport_req_data  ),
    .m_ctrlport_req_byte_en   (/* not supported */),
    .m_ctrlport_req_has_time  (/* not supported */),
    .m_ctrlport_req_time      (/* not supported */),
    .m_ctrlport_resp_ack      (ctrlport_resp_ack  ),
    .m_ctrlport_resp_status   (AXIS_CTRL_STS_OKAY ),
    .m_ctrlport_resp_data     (ctrlport_resp_data ),
    .s_ctrlport_req_wr        (1'b0               ),
    .s_ctrlport_req_rd        (1'b0               ),
    .s_ctrlport_req_addr      (20'd0              ),
    .s_ctrlport_req_portid    (10'd0              ),
    .s_ctrlport_req_rem_epid  (16'd0              ),
    .s_ctrlport_req_rem_portid(10'd0              ),
    .s_ctrlport_req_data      (32'h0              ),
    .s_ctrlport_req_byte_en   (4'h0               ),
    .s_ctrlport_req_has_time  (1'b0               ),
    .s_ctrlport_req_time      (1'b0               ),
    .s_ctrlport_resp_ack      (/* unused */       ),
    .s_ctrlport_resp_status   (/* unused */       ),
    .s_ctrlport_resp_data     (/* unused */       )
  );

  // ------------------------------------------------
  // Segment Address space into the three functions:
  // - Block Specific (incl. global regs)
  // - Connections
  // ------------------------------------------------

  reg  [15:0] req_addr      = 16'h0;
  reg  [31:0] req_data      = 32'h0;
  reg         blk_req_wr    = 1'b0;
  reg         blk_req_rd    = 1'b0;
  reg         blk_resp_ack  = 1'b0;
  reg  [31:0] blk_resp_data = 32'h0;
  reg         con_req_wr    = 1'b0;
  reg         con_req_rd    = 1'b0;
  reg         con_resp_ack  = 1'b0;
  reg  [31:0] con_resp_data = 32'h0;

  // Shortcuts
  wire blk_addr_space = (ctrlport_req_addr[19:16] == 4'd0);
  wire con_addr_space = (ctrlport_req_addr[19:16] == 4'd1);

  // ControlPort MUX
  always @(posedge core_ctrl_clk) begin
    // Write strobe
    blk_req_wr <= ctrlport_req_wr & blk_addr_space;
    con_req_wr <= ctrlport_req_wr & con_addr_space;
    // Read strobe
    blk_req_rd <= ctrlport_req_rd & blk_addr_space;
    con_req_rd <= ctrlport_req_rd & con_addr_space;
    // Address and Data (shared)
    req_addr <= ctrlport_req_addr[15:0];
    req_data <= ctrlport_req_data;
    // Response
    ctrlport_resp_ack <= blk_resp_ack | con_resp_ack;
    if (blk_resp_ack)
      ctrlport_resp_data <= blk_resp_data;
    else
      ctrlport_resp_data <= con_resp_data;
  end

  // -----------------------------------
  // Block Address Space
  // -----------------------------------

  // Arrange the backend block wires into a 2-d array where the
  // outer index represents the slot number and the inner index represents
  // a register index for that slot. We have 512 bits of read/write
  // data which translates to 16 32-bit registers per slot. The first slot
  // belongs to this endpoint, the next N slots map to the instantiated
  // stream endpoints and the remaining slots map to block control and
  // status endpoint. The slot number has a 1-to-1 mapping to the port
  // number on the control crossbar.
  localparam NUM_REGS_PER_SLOT = 512/32;
  localparam NUM_SLOTS = 1 /*this*/ + NUM_STREAM_ENDPOINTS + NUM_BLOCKS;
  localparam BLOCK_OFFSET = 1 /*this*/ + NUM_STREAM_ENDPOINTS;

  reg  [31:0] config_arr_2d [0:NUM_SLOTS-1][0:NUM_REGS_PER_SLOT-1];
  wire [31:0] status_arr_2d [0:NUM_SLOTS-1][0:NUM_REGS_PER_SLOT-1];

  genvar b, i;
  generate
    for (b = 0; b < NUM_BLOCKS; b=b+1) begin
      for (i = 0; i < NUM_REGS_PER_SLOT; i=i+1) begin
        assign rfnoc_core_config[(b*512)+(i*32) +: 32] = config_arr_2d[b+BLOCK_OFFSET][i];
        assign status_arr_2d[b+BLOCK_OFFSET][i] = rfnoc_core_status[(b*512)+(i*32) +: 32];
      end
    end
  endgenerate

  integer m, n;
  always @(posedge core_ctrl_clk) begin
    if (core_ctrl_rst) begin
      blk_resp_ack <= 1'b0;
      for (m = 0; m < NUM_SLOTS; m = m + 1) begin
        for (n = 0; n < NUM_REGS_PER_SLOT; n = n + 1) begin
          config_arr_2d[m][n] <= BEC_DEFAULT_VAL[(n*32)+:32];
        end
      end
    end else begin
      // All transactions finish in 1 cycle
      blk_resp_ack <= blk_req_wr | blk_req_rd;
      // Handle register writes
      if (blk_req_wr) begin
        config_arr_2d[req_addr[$clog2(NUM_SLOTS)+5:6]][req_addr[5:2]] <= req_data;
      end
      // Handle register reads
      if (blk_req_rd) begin
        blk_resp_data <= status_arr_2d[req_addr[$clog2(NUM_SLOTS)+5:6]][req_addr[5:2]];
      end
    end
  end

  // Global Registers
  localparam [3:0] REG_GLOBAL_PROTOVER          = 4'd0;  // Offset = 0x00
  localparam [3:0] REG_GLOBAL_PORT_CNT          = 4'd1;  // Offset = 0x04
  localparam [3:0] REG_GLOBAL_EDGE_CNT          = 4'd2;  // Offset = 0x08
  localparam [3:0] REG_GLOBAL_DEVICE_INFO       = 4'd3;  // Offset = 0x0C
  localparam [3:0] REG_GLOBAL_ENDPOINT_CTRL_CNT = 4'd4;  // Offset = 0x10

  // Clock-crossing for device_id.
  // FIFO going from core_chdr_clk domain to core_ctrl_clk.
  wire        device_id_fifo_ovalid;
  wire [15:0] device_id_fifo_odata;
  axi_fifo_2clk # (
    .WIDTH     (16),
    .SIZE      (2)
  ) device_id_fifo_i (
    .reset     (1'b0),
    .i_aclk    (core_chdr_clk),
    .i_tdata   (device_id),
    .i_tvalid  (1'b1),
    .i_tready  (),
    .o_aclk    (core_ctrl_clk),
    .o_tdata   (device_id_fifo_odata),
    .o_tvalid  (device_id_fifo_ovalid),
    .o_tready  (1'b1)
  );
  // Register the FIFO's output to always have valid data available.
  reg  [15:0] device_id_ctrl_clk = 16'h0;
  always @(posedge core_ctrl_clk) begin
    if (device_id_fifo_ovalid) begin
      device_id_ctrl_clk <= device_id_fifo_odata;
    end
  end

  // Signature and protocol version
  assign status_arr_2d[RFNOC_CORE_PORT_ID][REG_GLOBAL_PROTOVER] = {16'h12C6, PROTOVER[15:0]};

  // Global port count register
  localparam [0:0] STATIC_ROUTER_PRESENT = (NUM_EDGES == 12'd0) ? 1'b0 : 1'b1;
  assign status_arr_2d[RFNOC_CORE_PORT_ID][REG_GLOBAL_PORT_CNT] =
    {STATIC_ROUTER_PRESENT, CHDR_XBAR_PRESENT,
     NUM_TRANSPORTS[9:0], NUM_BLOCKS[9:0], NUM_STREAM_ENDPOINTS[9:0]};
  // Global edge count register
  assign status_arr_2d[RFNOC_CORE_PORT_ID][REG_GLOBAL_EDGE_CNT] = {20'd0, NUM_EDGES[11:0]};
  // Device information
  assign status_arr_2d[RFNOC_CORE_PORT_ID][REG_GLOBAL_DEVICE_INFO] = {DEVICE_TYPE, device_id_ctrl_clk};
  // Number of stream endpoint connected to the ctrl crossbar
  assign status_arr_2d[RFNOC_CORE_PORT_ID][REG_GLOBAL_ENDPOINT_CTRL_CNT] = {22'b0, NUM_ENDPOINTS_CTRL[9:0]};

  // -----------------------------------
  // Connections Address Space
  // -----------------------------------

  // All inter-block static connections must be stored in a memory
  // file which will be used to initialize a ROM that can be read
  // by software for topology discovery. The format of the memory
  // must be as follows:
  // * Word Width: 32 bits
  // * Maximum Depth: 16384 entries
  // * Layout:
  //   - 0x000 : HEADER
  //   - 0x001 : EDGE_0_DEF
  //   - 0x002 : EDGE_1_DEF
  //   ...
  //   - 0xFFF : EDGE_4094_DEF
  //
  // where:
  // * HEADER       = {18'd0, NumEntries[13:0]}
  // * EDGE_<N>_DEF = {SrcBlkIndex[9:0], SrcBlkPort[5:0], DstBlkIndex[9:0], DstBlkPort[5:0]}
  //
  // The BlkIndex is the port number of the block on the control crossbar amd the BlkPort is
  // the index of the input or output port of the block.

  generate if (EDGE_TBL_FILE == "" || NUM_EDGES == 0) begin
    // If no file is specified or if the number of edges is zero
    // then just return zero for all transactions
    always @(posedge core_ctrl_clk) begin
      con_resp_ack  <= (con_req_wr | con_req_rd);
      con_resp_data <= 32'h0;
    end
  end else begin
    // Initialize ROM from file and read it during a reg transaction
    reg [31:0] edge_tbl_rom[0:NUM_EDGES];
    initial begin
      $readmemh(EDGE_TBL_FILE, edge_tbl_rom, 0, NUM_EDGES);
    end
    always @(posedge core_ctrl_clk) begin
      con_resp_ack  <= (con_req_wr | con_req_rd);
      con_resp_data <= edge_tbl_rom[req_addr[$clog2(NUM_EDGES+1)+1:2]];
    end
  end endgenerate

endmodule // rfnoc_core_kernel

