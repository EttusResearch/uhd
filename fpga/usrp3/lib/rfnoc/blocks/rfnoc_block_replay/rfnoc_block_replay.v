//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_replay
//
// Description:
//
//   RFNoC data record and playback block. This block has the ability to
//   capture all of the data that is sent to it and store it into an attached
//   memory using an AXI memory-mapped interface. It can then play back any
//   part of the data on demand or continuously. Timed playback is also
//   supported. See axis_replay.v for details of replay operation.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   NUM_PORTS   : Number of replay instances to instantiate. Each one will
//                 have its own register set and memory interface.
//   MEM_DATA_W  : Data width to use for the memory interface.
//   MEM_ADDR_W  : Byte address width to use for the memory interface.
//

`default_nettype none


module rfnoc_block_replay #(
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter       CHDR_W      = 64,
  parameter [5:0] MTU         = 10,
  parameter       NUM_PORTS   = 1,
  parameter       MEM_DATA_W  = 64,
  parameter       MEM_ADDR_W  = 30
) (
  //---------------------------------------------------------------------------
  // AXIS-CHDR Port
  //---------------------------------------------------------------------------

  // RFNoC Framework Clocks and Resets
  input wire rfnoc_chdr_clk,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tlast,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tvalid,
  output wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tready,

  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tlast,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tready,

  // RFNoC Backend Interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,

  //---------------------------------------------------------------------------
  // AXIS-Ctrl Port
  //---------------------------------------------------------------------------

  input wire rfnoc_ctrl_clk,

  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0] s_rfnoc_ctrl_tdata,
  input  wire        s_rfnoc_ctrl_tlast,
  input  wire        s_rfnoc_ctrl_tvalid,
  output wire        s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0] m_rfnoc_ctrl_tdata,
  output wire        m_rfnoc_ctrl_tlast,
  output wire        m_rfnoc_ctrl_tvalid,
  input  wire        m_rfnoc_ctrl_tready,

  //---------------------------------------------------------------------------
  // AXI Memory Mapped Interface
  //---------------------------------------------------------------------------

  // AXI Interface Clock and Reset
  input wire mem_clk,
  input wire axi_rst,

  // AXI Write address channel
  output wire [           (NUM_PORTS*1)-1:0] m_axi_awid,
  output wire [  (NUM_PORTS*MEM_ADDR_W)-1:0] m_axi_awaddr,
  output wire [           (NUM_PORTS*8)-1:0] m_axi_awlen,
  output wire [           (NUM_PORTS*3)-1:0] m_axi_awsize,
  output wire [           (NUM_PORTS*2)-1:0] m_axi_awburst,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_awlock,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_awcache,
  output wire [           (NUM_PORTS*3)-1:0] m_axi_awprot,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_awqos,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_awregion,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_awuser,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_awvalid,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_awready,
  // AXI Write data channel
  output wire [  (NUM_PORTS*MEM_DATA_W)-1:0] m_axi_wdata,
  output wire [(NUM_PORTS*MEM_DATA_W/8)-1:0] m_axi_wstrb,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_wlast,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_wuser,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_wvalid,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_wready,
  // AXI Write response channel signals
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_bid,
  input  wire [           (NUM_PORTS*2)-1:0] m_axi_bresp,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_buser,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_bvalid,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_bready,
  // AXI Read address channel
  output wire [           (NUM_PORTS*1)-1:0] m_axi_arid,
  output wire [  (NUM_PORTS*MEM_ADDR_W)-1:0] m_axi_araddr,
  output wire [           (NUM_PORTS*8)-1:0] m_axi_arlen,
  output wire [           (NUM_PORTS*3)-1:0] m_axi_arsize,
  output wire [           (NUM_PORTS*2)-1:0] m_axi_arburst,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_arlock,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_arcache,
  output wire [           (NUM_PORTS*3)-1:0] m_axi_arprot,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_arqos,
  output wire [           (NUM_PORTS*4)-1:0] m_axi_arregion,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_aruser,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_arvalid,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_arready,
  // AXI Read data channel
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_rid,
  input  wire [  (NUM_PORTS*MEM_DATA_W)-1:0] m_axi_rdata,
  input  wire [           (NUM_PORTS*2)-1:0] m_axi_rresp,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_rlast,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_ruser,
  input  wire [           (NUM_PORTS*1)-1:0] m_axi_rvalid,
  output wire [           (NUM_PORTS*1)-1:0] m_axi_rready
);

  `include "rfnoc_block_replay_regs.vh"


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // CtrlPort Master
  wire        ctrlport_req_wr;
  wire        ctrlport_req_rd;
  wire [19:0] ctrlport_req_addr;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;

  // Data Stream to User Logic: in
  wire [NUM_PORTS*MEM_DATA_W*1-1:0] in_axis_tdata;
  wire [             NUM_PORTS-1:0] in_axis_tlast;
  wire [             NUM_PORTS-1:0] in_axis_tvalid;
  wire [             NUM_PORTS-1:0] in_axis_tready;

  // Data Stream to User Logic: out
  wire [NUM_PORTS*MEM_DATA_W*1-1:0] out_axis_tdata;
  wire [             NUM_PORTS-1:0] out_axis_tlast;
  wire [             NUM_PORTS-1:0] out_axis_tvalid;
  wire [             NUM_PORTS-1:0] out_axis_tready;
  wire [          NUM_PORTS*64-1:0] out_axis_ttimestamp;
  wire [             NUM_PORTS-1:0] out_axis_thas_time;
  wire [             NUM_PORTS-1:0] out_axis_teob;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire mem_rst_noc_shell;

  noc_shell_replay #(
    .THIS_PORTID (THIS_PORTID),
    .CHDR_W      (CHDR_W),
    .MEM_DATA_W  (MEM_DATA_W),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS)
  ) noc_shell_replay_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk        (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk        (rfnoc_ctrl_clk),
    .mem_clk               (mem_clk),
    // Reset Outputs
    .rfnoc_chdr_rst        (),
    .rfnoc_ctrl_rst        (),
    .mem_rst               (mem_rst_noc_shell),
    // RFNoC Backend Interface
    .rfnoc_core_config     (rfnoc_core_config),
    .rfnoc_core_status     (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata    (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast    (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid   (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready   (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata    (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast    (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid   (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready   (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata    (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast    (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid   (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready   (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata    (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast    (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid   (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready   (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk          (),
    .ctrlport_rst          (),
    // CtrlPort Master
    .m_ctrlport_req_wr     (ctrlport_req_wr),
    .m_ctrlport_req_rd     (ctrlport_req_rd),
    .m_ctrlport_req_addr   (ctrlport_req_addr),
    .m_ctrlport_req_data   (ctrlport_req_data),
    .m_ctrlport_resp_ack   (ctrlport_resp_ack),
    .m_ctrlport_resp_data  (ctrlport_resp_data),

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk         (),
    .axis_data_rst         (),
    // Data Stream to User Logic: in
    .m_in_axis_tdata       (in_axis_tdata),
    .m_in_axis_tkeep       (),
    .m_in_axis_tlast       (in_axis_tlast),
    .m_in_axis_tvalid      (in_axis_tvalid),
    .m_in_axis_tready      (in_axis_tready),
    .m_in_axis_ttimestamp  (),
    .m_in_axis_thas_time   (),
    .m_in_axis_tlength     (),
    .m_in_axis_teov        (),
    .m_in_axis_teob        (),
    // Data Stream from User Logic: out
    .s_out_axis_tdata      (out_axis_tdata),
    .s_out_axis_tkeep      ({NUM_PORTS*MEM_DATA_W/32{1'b1}}),
    .s_out_axis_tlast      (out_axis_tlast),
    .s_out_axis_tvalid     (out_axis_tvalid),
    .s_out_axis_tready     (out_axis_tready),
    .s_out_axis_ttimestamp (out_axis_ttimestamp),
    .s_out_axis_thas_time  (out_axis_thas_time),
    .s_out_axis_tlength    ({NUM_PORTS{16'b0}}),    // Not used when SIDEBAND_AT_END = 1
    .s_out_axis_teov       ({NUM_PORTS{1'b0}}),
    .s_out_axis_teob       (out_axis_teob)
  );

  reg mem_rst;

  // Combine the NoC Shell and AXI resets
  always @(posedge mem_clk) begin
    mem_rst <= axi_rst | mem_rst_noc_shell;
  end


  //---------------------------------------------------------------------------
  // CtrlPort Splitter
  //---------------------------------------------------------------------------

  wire [ 1*NUM_PORTS-1:0] dec_ctrlport_req_wr;
  wire [ 1*NUM_PORTS-1:0] dec_ctrlport_req_rd;
  wire [20*NUM_PORTS-1:0] dec_ctrlport_req_addr;
  wire [32*NUM_PORTS-1:0] dec_ctrlport_req_data;
  wire [ 1*NUM_PORTS-1:0] dec_ctrlport_resp_ack;
  wire [32*NUM_PORTS-1:0] dec_ctrlport_resp_data;

  generate
    if (NUM_PORTS > 1) begin : gen_ctrlport_decoder
      ctrlport_decoder #(
        .NUM_SLAVES   (NUM_PORTS),
        .BASE_ADDR    (0),
        .SLAVE_ADDR_W (REPLAY_ADDR_W)
      ) ctrlport_decoder_i (
        .ctrlport_clk            (mem_clk),
        .ctrlport_rst            (mem_rst),
        .s_ctrlport_req_wr       (ctrlport_req_wr),
        .s_ctrlport_req_rd       (ctrlport_req_rd),
        .s_ctrlport_req_addr     (ctrlport_req_addr),
        .s_ctrlport_req_data     (ctrlport_req_data),
        .s_ctrlport_req_byte_en  (4'hF),
        .s_ctrlport_req_has_time (1'b0),
        .s_ctrlport_req_time     (64'b0),
        .s_ctrlport_resp_ack     (ctrlport_resp_ack),
        .s_ctrlport_resp_status  (),
        .s_ctrlport_resp_data    (ctrlport_resp_data),
        .m_ctrlport_req_wr       (dec_ctrlport_req_wr),
        .m_ctrlport_req_rd       (dec_ctrlport_req_rd),
        .m_ctrlport_req_addr     (dec_ctrlport_req_addr),
        .m_ctrlport_req_data     (dec_ctrlport_req_data),
        .m_ctrlport_req_byte_en  (),
        .m_ctrlport_req_has_time (),
        .m_ctrlport_req_time     (),
        .m_ctrlport_resp_ack     (dec_ctrlport_resp_ack),
        .m_ctrlport_resp_status  ({NUM_PORTS{2'b0}}),
        .m_ctrlport_resp_data    (dec_ctrlport_resp_data)
      );
    end else begin : gen_no_decoder
      assign dec_ctrlport_req_wr   = ctrlport_req_wr;
      assign dec_ctrlport_req_rd   = ctrlport_req_rd;
      assign dec_ctrlport_req_addr = {{20-REPLAY_ADDR_W{1'b0}},
                                     ctrlport_req_addr[REPLAY_ADDR_W-1:0]};
      assign dec_ctrlport_req_data = ctrlport_req_data;
      assign ctrlport_resp_ack     = dec_ctrlport_resp_ack;
      assign ctrlport_resp_data    = dec_ctrlport_resp_data;
    end
  endgenerate


  //---------------------------------------------------------------------------
  // Replay Block Instances
  //---------------------------------------------------------------------------

  // Width of memory transfer count. This controls the maximum burst length
  // supported by the Replay block. For AXI compatibility, it must be 8 or less
  // and should not represent more than 4 KiB. Here we set it to 2 KiB by
  // default.
  localparam MEM_COUNT_W = (MEM_DATA_W <= 64) ? 8 :        // Max width allowed
                           $clog2(2048 / (MEM_DATA_W/8));  // 2 KiB

  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i+1) begin : gen_replay_blocks

      wire [ MEM_ADDR_W-1:0] write_addr;
      wire [MEM_COUNT_W-1:0] write_count;
      wire                   write_ctrl_valid;
      wire                   write_ctrl_ready;
      wire [ MEM_DATA_W-1:0] write_data;
      wire                   write_data_valid;
      wire                   write_data_ready;

      wire [ MEM_ADDR_W-1:0] read_addr;
      wire [MEM_COUNT_W-1:0] read_count;
      wire                   read_ctrl_valid;
      wire                   read_ctrl_ready;
      wire [ MEM_DATA_W-1:0] read_data;
      wire                   read_data_valid;
      wire                   read_data_ready;

      //-----------------------------------------------------------------------
      // Replay Handler
      //-----------------------------------------------------------------------
      //
      // This block implements the state machine and control logic for
      // recording and playback of data.
      //
      //-----------------------------------------------------------------------

      axis_replay #(
        .MEM_DATA_W  (MEM_DATA_W),
        .MEM_ADDR_W  (MEM_ADDR_W),
        .MEM_COUNT_W (MEM_COUNT_W)
      ) axis_replay_i (
        .clk (mem_clk),
        .rst (mem_rst),

        // CtrlPort Interface
        .s_ctrlport_req_wr    (dec_ctrlport_req_wr    [ 1*i +:  1]),
        .s_ctrlport_req_rd    (dec_ctrlport_req_rd    [ 1*i +:  1]),
        .s_ctrlport_req_addr  (dec_ctrlport_req_addr  [20*i +: 20]),
        .s_ctrlport_req_data  (dec_ctrlport_req_data  [32*i +: 32]),
        .s_ctrlport_resp_ack  (dec_ctrlport_resp_ack  [ 1*i +:  1]),
        .s_ctrlport_resp_data (dec_ctrlport_resp_data [32*i +: 32]),

        // AXI Stream Interface
        //
        // Input
        .i_tdata  (in_axis_tdata [MEM_DATA_W*i +: MEM_DATA_W]),
        .i_tvalid (in_axis_tvalid[         1*i +:          1]),
        .i_tlast  (in_axis_tlast [         1*i +:          1]),
        .i_tready (in_axis_tready[         1*i +:          1]),
        //
        // Output
        .o_tdata      (out_axis_tdata     [MEM_DATA_W*i +: MEM_DATA_W]),
        .o_ttimestamp (out_axis_ttimestamp[        64*i +:         64]),
        .o_thas_time  (out_axis_thas_time [         1*i +:          1]),
        .o_teob       (out_axis_teob      [         1*i +:          1]),
        .o_tvalid     (out_axis_tvalid    [         1*i +:          1]),
        .o_tlast      (out_axis_tlast     [         1*i +:          1]),
        .o_tready     (out_axis_tready    [         1*i +:          1]),

        // Memory Interface
        //
        // Write interface
        .write_addr       (write_addr),
        .write_count      (write_count),
        .write_ctrl_valid (write_ctrl_valid),
        .write_ctrl_ready (write_ctrl_ready),
        .write_data       (write_data),
        .write_data_valid (write_data_valid),
        .write_data_ready (write_data_ready),
        //
        // Read interface
        .read_addr        (read_addr),
        .read_count       (read_count),
        .read_ctrl_valid  (read_ctrl_valid),
        .read_ctrl_ready  (read_ctrl_ready),
        .read_data        (read_data),
        .read_data_valid  (read_data_valid),
        .read_data_ready  (read_data_ready)
      );

      //-----------------------------------------------------------------------
      // AXI DMA Master
      //-----------------------------------------------------------------------
      //
      // This block translates simple read and write requests to AXI4
      // memory-mapped reads and writes for the RAM interface.
      //
      //-----------------------------------------------------------------------

      // Resize the count signals, in case they are not 8 bit.
      wire [7:0] write_count_8 = write_count;
      wire [7:0] read_count_8  = read_count;

      axi_dma_master #(
        .AWIDTH (MEM_ADDR_W),
        .DWIDTH (MEM_DATA_W)
      ) axi_dma_master_i (
        //
        // AXI4 Memory Mapped Interface to DRAM
        //
        .aclk   (mem_clk),
        .areset (mem_rst),

        // Write control
        .m_axi_awid     (m_axi_awid    [         1*i +:          1]),
        .m_axi_awaddr   (m_axi_awaddr  [MEM_ADDR_W*i +: MEM_ADDR_W]),
        .m_axi_awlen    (m_axi_awlen   [         8*i +:          8]),
        .m_axi_awsize   (m_axi_awsize  [         3*i +:          3]),
        .m_axi_awburst  (m_axi_awburst [         2*i +:          2]),
        .m_axi_awvalid  (m_axi_awvalid [         1*i +:          1]),
        .m_axi_awready  (m_axi_awready [         1*i +:          1]),
        .m_axi_awlock   (m_axi_awlock  [         1*i +:          1]),
        .m_axi_awcache  (m_axi_awcache [         4*i +:          4]),
        .m_axi_awprot   (m_axi_awprot  [         3*i +:          3]),
        .m_axi_awqos    (m_axi_awqos   [         4*i +:          4]),
        .m_axi_awregion (m_axi_awregion[         4*i +:          4]),
        .m_axi_awuser   (m_axi_awuser  [         1*i +:          1]),

        // Write Data
        .m_axi_wdata  (m_axi_wdata [    MEM_DATA_W*i +:     MEM_DATA_W]),
        .m_axi_wstrb  (m_axi_wstrb [(MEM_DATA_W/8)*i +: (MEM_DATA_W/8)]),
        .m_axi_wlast  (m_axi_wlast [             1*i +:              1]),
        .m_axi_wvalid (m_axi_wvalid[             1*i +:              1]),
        .m_axi_wready (m_axi_wready[             1*i +:              1]),
        .m_axi_wuser  (m_axi_wuser [             1*i +:              1]),

        // Write Response
        .m_axi_bid    (m_axi_bid   [1*i +: 1]),
        .m_axi_bresp  (m_axi_bresp [2*i +: 2]),
        .m_axi_buser  (m_axi_buser [1*i +: 1]),
        .m_axi_bvalid (m_axi_bvalid[1*i +: 1]),
        .m_axi_bready (m_axi_bready[1*i +: 1]),

        // Read Control
        .m_axi_arid     (m_axi_arid    [         1*i +:          1]),
        .m_axi_araddr   (m_axi_araddr  [MEM_ADDR_W*i +: MEM_ADDR_W]),
        .m_axi_arlen    (m_axi_arlen   [         8*i +:          8]),
        .m_axi_arsize   (m_axi_arsize  [         3*i +:          3]),
        .m_axi_arburst  (m_axi_arburst [         2*i +:          2]),
        .m_axi_arvalid  (m_axi_arvalid [         1*i +:          1]),
        .m_axi_arready  (m_axi_arready [         1*i +:          1]),
        .m_axi_arlock   (m_axi_arlock  [         1*i +:          1]),
        .m_axi_arcache  (m_axi_arcache [         4*i +:          4]),
        .m_axi_arprot   (m_axi_arprot  [         3*i +:          3]),
        .m_axi_arqos    (m_axi_arqos   [         4*i +:          4]),
        .m_axi_arregion (m_axi_arregion[         4*i +:          4]),
        .m_axi_aruser   (m_axi_aruser  [         1*i +:          1]),

        // Read Data
        .m_axi_rid    (m_axi_rid   [         1*i +:          1]),
        .m_axi_rdata  (m_axi_rdata [MEM_DATA_W*i +: MEM_DATA_W]),
        .m_axi_rresp  (m_axi_rresp [         2*i +:          2]),
        .m_axi_rlast  (m_axi_rlast [         1*i +:          1]),
        .m_axi_ruser  (m_axi_ruser [         1*i +:          1]),
        .m_axi_rvalid (m_axi_rvalid[         1*i +:          1]),
        .m_axi_rready (m_axi_rready[         1*i +:          1]),

        //
        // Interface for Write transactions
        //
        .write_addr       (write_addr),
        .write_count      (write_count_8),
        .write_ctrl_valid (write_ctrl_valid),
        .write_ctrl_ready (write_ctrl_ready),
        .write_data       (write_data),
        .write_data_valid (write_data_valid),
        .write_data_ready (write_data_ready),

        //
        // Interface for Read transactions
        //
        .read_addr       (read_addr),
        .read_count      (read_count_8),
        .read_ctrl_valid (read_ctrl_valid),
        .read_ctrl_ready (read_ctrl_ready),
        .read_data       (read_data),
        .read_data_valid (read_data_valid),
        .read_data_ready (read_data_ready),

        //
        // Debug
        //
        .debug ()
      );

    end
  endgenerate

endmodule // rfnoc_block_replay


`default_nettype wire
