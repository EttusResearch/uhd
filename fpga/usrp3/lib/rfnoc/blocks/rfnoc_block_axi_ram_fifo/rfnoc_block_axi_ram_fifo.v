//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_axi_ram_fifo
//
// Description: 
//
//   Implements a FIFO using an AXI memory-mapped interface to an external 
//   memory.
//
// Parameters:
//
//   THIS_PORTID    : Control crossbar port to which this block is connected
//   
//   CHDR_W         : CHDR AXI-Stream data bus width
//
//   NUM_PORTS      : Number of independent FIFOs to support, all sharing the 
//                    same memory.
//   
//   MTU            : Maximum transfer unit (maximum packet size) to support, 
//                    in CHDR_W-sized words.
//
//   MEM_DATA_W     : Width of the data bus to use for the AXI memory-mapped 
//                    interface. This must be no bigger than CHDR_W and it must 
//                    evenly divide CHDR_W.
//
//   MEM_ADDR_W     : Width of the byte address to use for RAM addressing. This
//                    effectively sets the maximum combined size of all FIFOs.
//                    This must be less than or equal to AWIDTH.
//
//   AWIDTH         : Width of the address bus for the AXI memory-mapped
//                    interface. This must be at least as big as MEM_DATA_W.
//
//   FIFO_ADDR_BASE : Default base byte address of each FIFO. When NUM_PORTS > 
//                    1, this should be the concatenation of all the FIFO base 
//                    addresses. These values can be reconfigured by software.
//
//   FIFO_ADDR_MASK : Default byte address mask used by each FIFO. It must be 
//                    all ones. The size of the FIFO in bytes will be this 
//                    minus one. These values can be reconfigured by software.
//
//   BURST_TIMEOUT  : Default number of memory clock cycles to wait for new 
//                    data before performing a short, sub-optimal burst. One 
//                    value per FIFO.
//
//   IN_FIFO_SIZE   : Size of the input buffer. This is used to mitigate the 
//                    effects of memory write latency, which can be significant 
//                    when the external memory is DRAM.
//
//   OUT_FIFO_SIZE  : Size of the output buffer. This is used to mitigate the 
//                    effects of memory read latency, which can be significant 
//                    when the external memory is DRAM.
//
//   BIST           : Includes BIST logic when true.
//
//   MEM_CLK_RATE   : Frequency of mem_clk in Hz. This is used by BIST for 
//                    throughput calculation.
//

module rfnoc_block_axi_ram_fifo #(
  parameter                            THIS_PORTID    = 0,
  parameter                            CHDR_W         = 64,
  parameter                            NUM_PORTS      = 1,
  parameter                            MTU            = 10,
  parameter                            MEM_DATA_W     = CHDR_W,
  parameter                            MEM_ADDR_W     = 32,
  parameter                            AWIDTH         = 32,
  parameter [NUM_PORTS*MEM_ADDR_W-1:0] FIFO_ADDR_BASE = {NUM_PORTS{ {MEM_ADDR_W{1'b0}} }},
  parameter [NUM_PORTS*MEM_ADDR_W-1:0] FIFO_ADDR_MASK = {NUM_PORTS{ {(MEM_ADDR_W-$clog2(NUM_PORTS)){1'b1}} }},
  parameter [        NUM_PORTS*32-1:0] BURST_TIMEOUT  = {NUM_PORTS{ 32'd256 }},
  parameter                            IN_FIFO_SIZE   = 11,
  parameter                            OUT_FIFO_SIZE  = 11,
  parameter                            BIST           = 1,
  parameter                            MEM_CLK_RATE   = 200e6
) (
  //---------------------------------------------------------------------------
  // AXIS CHDR Port
  //---------------------------------------------------------------------------

  input wire rfnoc_chdr_clk,

  // CHDR inputs from framework
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,

  // CHDR outputs to framework
  output wire [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,

  // Backend interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,


  //---------------------------------------------------------------------------
  // AXIS CTRL Port
  //---------------------------------------------------------------------------

  input wire rfnoc_ctrl_clk,

  // CTRL port requests from framework
  input  wire [31:0] s_rfnoc_ctrl_tdata,
  input  wire        s_rfnoc_ctrl_tlast,
  input  wire        s_rfnoc_ctrl_tvalid,
  output wire        s_rfnoc_ctrl_tready,

  // CTRL port requests to framework
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

  // AXI Write Address Channel
  output wire [         NUM_PORTS*1-1:0] m_axi_awid,     // Write address ID. This signal is the identification tag for the write address signals
  output wire [    NUM_PORTS*AWIDTH-1:0] m_axi_awaddr,   // Write address. The write address gives the address of the first transfer in a write burst
  output wire [         NUM_PORTS*8-1:0] m_axi_awlen,    // Burst length. The burst length gives the exact number of transfers in a burst.
  output wire [         NUM_PORTS*3-1:0] m_axi_awsize,   // Burst size. This signal indicates the size of each transfer in the burst.
  output wire [         NUM_PORTS*2-1:0] m_axi_awburst,  // Burst type. The burst type and the size information, determine how the address is calculated
  output wire [         NUM_PORTS*1-1:0] m_axi_awlock,   // Lock type. Provides additional information about the atomic characteristics of the transfer.
  output wire [         NUM_PORTS*4-1:0] m_axi_awcache,  // Memory type. This signal indicates how transactions are required to progress
  output wire [         NUM_PORTS*3-1:0] m_axi_awprot,   // Protection type. This signal indicates the privilege and security level of the transaction
  output wire [         NUM_PORTS*4-1:0] m_axi_awqos,    // Quality of Service, QoS. The QoS identifier sent for each write transaction
  output wire [         NUM_PORTS*4-1:0] m_axi_awregion, // Region identifier. Permits a single physical interface on a slave to be re-used.
  output wire [         NUM_PORTS*1-1:0] m_axi_awuser,   // User signal. Optional User-defined signal in the write address channel.
  output wire [         NUM_PORTS*1-1:0] m_axi_awvalid,  // Write address valid. This signal indicates that the channel is signaling valid write addr
  input  wire [         NUM_PORTS*1-1:0] m_axi_awready,  // Write address ready. This signal indicates that the slave is ready to accept an address
  
  // AXI Write Data Channel
  output wire [  NUM_PORTS*MEM_DATA_W-1:0] m_axi_wdata,  // Write data
  output wire [NUM_PORTS*MEM_DATA_W/8-1:0] m_axi_wstrb,  // Write strobes. This signal indicates which byte lanes hold valid data.
  output wire [           NUM_PORTS*1-1:0] m_axi_wlast,  // Write last. This signal indicates the last transfer in a write burst
  output wire [           NUM_PORTS*1-1:0] m_axi_wuser,  // User signal. Optional User-defined signal in the write data channel.
  output wire [           NUM_PORTS*1-1:0] m_axi_wvalid, // Write valid. This signal indicates that valid write data and strobes are available.
  input  wire [           NUM_PORTS*1-1:0] m_axi_wready, // Write ready. This signal indicates that the slave can accept the write data.
  
  // AXI Write Response Channel
  input  wire [         NUM_PORTS*1-1:0] m_axi_bid,      // Response ID tag. This signal is the ID tag of the write response.
  input  wire [         NUM_PORTS*2-1:0] m_axi_bresp,    // Write response. This signal indicates the status of the write transaction.
  input  wire [         NUM_PORTS*1-1:0] m_axi_buser,    // User signal. Optional User-defined signal in the write response channel.
  input  wire [         NUM_PORTS*1-1:0] m_axi_bvalid,   // Write response valid. This signal indicates that the channel is signaling a valid response
  output wire [         NUM_PORTS*1-1:0] m_axi_bready,   // Response ready. This signal indicates that the master can accept a write response
  
  // AXI Read Address Channel
  output wire [         NUM_PORTS*1-1:0] m_axi_arid,     // Read address ID. This signal is the identification tag for the read address group of signals
  output wire [    NUM_PORTS*AWIDTH-1:0] m_axi_araddr,   // Read address. The read address gives the address of the first transfer in a read burst
  output wire [         NUM_PORTS*8-1:0] m_axi_arlen,    // Burst length. This signal indicates the exact number of transfers in a burst.
  output wire [         NUM_PORTS*3-1:0] m_axi_arsize,   // Burst size. This signal indicates the size of each transfer in the burst.
  output wire [         NUM_PORTS*2-1:0] m_axi_arburst,  // Burst type. The burst type and the size information determine how the address for each transfer
  output wire [         NUM_PORTS*1-1:0] m_axi_arlock,   // Lock type. This signal provides additional information about the atomic characteristics
  output wire [         NUM_PORTS*4-1:0] m_axi_arcache,  // Memory type. This signal indicates how transactions are required to progress
  output wire [         NUM_PORTS*3-1:0] m_axi_arprot,   // Protection type. This signal indicates the privilege and security level of the transaction
  output wire [         NUM_PORTS*4-1:0] m_axi_arqos,    // Quality of Service, QoS. QoS identifier sent for each read transaction.
  output wire [         NUM_PORTS*4-1:0] m_axi_arregion, // Region identifier. Permits a single physical interface on a slave to be re-used
  output wire [         NUM_PORTS*1-1:0] m_axi_aruser,   // User signal. Optional User-defined signal in the read address channel.
  output wire [         NUM_PORTS*1-1:0] m_axi_arvalid,  // Read address valid. This signal indicates that the channel is signaling valid read addr
  input  wire [         NUM_PORTS*1-1:0] m_axi_arready,  // Read address ready. This signal indicates that the slave is ready to accept an address
  
  // AXI Read Data Channel
  input  wire [         NUM_PORTS*1-1:0] m_axi_rid,      // Read ID tag. This signal is the identification tag for the read data group of signals
  input  wire [NUM_PORTS*MEM_DATA_W-1:0] m_axi_rdata,    // Read data.
  input  wire [         NUM_PORTS*2-1:0] m_axi_rresp,    // Read response. This signal indicates the status of the read transfer
  input  wire [         NUM_PORTS*1-1:0] m_axi_rlast,    // Read last. This signal indicates the last transfer in a read burst.
  input  wire [         NUM_PORTS*1-1:0] m_axi_ruser,    // User signal. Optional User-defined signal in the read data channel.
  input  wire [         NUM_PORTS*1-1:0] m_axi_rvalid,   // Read valid. This signal indicates that the channel is signaling the required read data.
  output wire [         NUM_PORTS*1-1:0] m_axi_rready    // Read ready. This signal indicates that the master can accept the read data and response
);

  `include "axi_ram_fifo_regs.vh"

  localparam NOC_ID = 'hF1F0_0000;

  // If the memory width is larger than the CHDR width, then we need to use 
  // tkeep to track which CHDR words are valid as they go through the FIFO. 
  // Calculate the TKEEP width here. Set to 1 if it's not needed.
  localparam KEEP_W = (MEM_DATA_W/CHDR_W) > 1 ? (MEM_DATA_W/CHDR_W) : 1;


  //---------------------------------------------------------------------------
  // Parameter Checks
  //---------------------------------------------------------------------------
  
  if (CHDR_W % MEM_DATA_W != 0 && MEM_DATA_W % CHDR_W != 0)
    CHDR_W_must_be_a_multiple_of_MEM_DATA_W_or_vice_versa();

  if (MEM_ADDR_W > AWIDTH)
    MEM_ADDR_W_must_be_greater_than_AWIDTH();


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire        rfnoc_chdr_rst;

  wire        ctrlport_req_wr;
  wire        ctrlport_req_rd;
  wire [19:0] ctrlport_req_addr;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;

  wire [NUM_PORTS*MEM_DATA_W-1:0] m_axis_data_tdata;
  wire [    NUM_PORTS*KEEP_W-1:0] m_axis_data_tkeep;
  wire [           NUM_PORTS-1:0] m_axis_data_tlast;
  wire [           NUM_PORTS-1:0] m_axis_data_tvalid;
  wire [           NUM_PORTS-1:0] m_axis_data_tready;

  wire [NUM_PORTS*MEM_DATA_W-1:0] s_axis_data_tdata;
  wire [    NUM_PORTS*KEEP_W-1:0] s_axis_data_tkeep;
  wire [           NUM_PORTS-1:0] s_axis_data_tlast;
  wire [           NUM_PORTS-1:0] s_axis_data_tvalid;
  wire [           NUM_PORTS-1:0] s_axis_data_tready;

  noc_shell_axi_ram_fifo #(
    .NOC_ID           (NOC_ID),
    .THIS_PORTID      (THIS_PORTID),
    .CHDR_W           (CHDR_W),
    .DATA_W           (MEM_DATA_W),
    .CTRL_FIFO_SIZE   (5),
    .CTRLPORT_MST_EN  (1),
    .CTRLPORT_SLV_EN  (0),
    .NUM_DATA_I       (NUM_PORTS),
    .NUM_DATA_O       (NUM_PORTS),
    .MTU              (MTU),
    .SYNC_DATA_CLOCKS (0)
  ) noc_shell_axi_ram_fifo_i (
    .rfnoc_chdr_clk            (rfnoc_chdr_clk),
    .rfnoc_chdr_rst            (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk            (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst            (),
    .rfnoc_core_config         (rfnoc_core_config),
    .rfnoc_core_status         (rfnoc_core_status),
    .s_rfnoc_chdr_tdata        (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast        (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid       (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready       (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata        (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast        (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid       (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready       (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata        (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast        (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid       (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready       (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata        (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast        (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid       (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready       (m_rfnoc_ctrl_tready),
    .ctrlport_clk              (mem_clk),
    .ctrlport_rst              (axi_rst),
    .m_ctrlport_req_wr         (ctrlport_req_wr),
    .m_ctrlport_req_rd         (ctrlport_req_rd),
    .m_ctrlport_req_addr       (ctrlport_req_addr),
    .m_ctrlport_req_data       (ctrlport_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (ctrlport_resp_ack),
    .m_ctrlport_resp_status    (2'b0),
    .m_ctrlport_resp_data      (ctrlport_resp_data),
    .s_ctrlport_req_wr         (1'b0),
    .s_ctrlport_req_rd         (1'b0),
    .s_ctrlport_req_addr       (20'b0),
    .s_ctrlport_req_portid     (10'b0),
    .s_ctrlport_req_rem_epid   (16'b0),
    .s_ctrlport_req_rem_portid (10'b0),
    .s_ctrlport_req_data       (32'b0),
    .s_ctrlport_req_byte_en    (4'b0),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      (),
    .axis_data_clk             (mem_clk),
    .axis_data_rst             (axi_rst),
    .m_axis_tdata              (m_axis_data_tdata),
    .m_axis_tkeep              (m_axis_data_tkeep),
    .m_axis_tlast              (m_axis_data_tlast),
    .m_axis_tvalid             (m_axis_data_tvalid),
    .m_axis_tready             (m_axis_data_tready),
    .s_axis_tdata              (s_axis_data_tdata),
    .s_axis_tkeep              (s_axis_data_tkeep),
    .s_axis_tlast              (s_axis_data_tlast),
    .s_axis_tvalid             (s_axis_data_tvalid),
    .s_axis_tready             (s_axis_data_tready)
  );

  wire rfnoc_chdr_rst_mem_clk;
  reg  mem_rst_block;

  // Cross the CHDR reset to the mem_clk domain
  pulse_synchronizer #(
    .MODE ("POSEDGE")
  ) ctrl_rst_sync_i (
    .clk_a   (rfnoc_chdr_clk),
    .rst_a   (1'b0),
    .pulse_a (rfnoc_chdr_rst),
    .busy_a  (),
    .clk_b   (mem_clk),
    .pulse_b (rfnoc_chdr_rst_mem_clk)
  );

  // Combine the resets in a glitch-free manner
  always @(posedge mem_clk) begin
    mem_rst_block <= axi_rst | rfnoc_chdr_rst_mem_clk;
  end


  //---------------------------------------------------------------------------
  // CTRL Port Splitter
  //---------------------------------------------------------------------------

  wire [   NUM_PORTS-1:0] m_ctrlport_req_wr;
  wire [   NUM_PORTS-1:0] m_ctrlport_req_rd;
  wire [20*NUM_PORTS-1:0] m_ctrlport_req_addr;
  wire [32*NUM_PORTS-1:0] m_ctrlport_req_data;
  wire [   NUM_PORTS-1:0] m_ctrlport_resp_ack;
  wire [32*NUM_PORTS-1:0] m_ctrlport_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES   (NUM_PORTS),
    .BASE_ADDR    (0),
    .SLAVE_ADDR_W (RAM_FIFO_ADDR_W)
  ) ctrlport_splitter_i (
    .ctrlport_clk            (mem_clk),
    .ctrlport_rst            (mem_rst_block),
    .s_ctrlport_req_wr       (ctrlport_req_wr),
    .s_ctrlport_req_rd       (ctrlport_req_rd),
    .s_ctrlport_req_addr     (ctrlport_req_addr),
    .s_ctrlport_req_data     (ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'b1111),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (ctrlport_resp_ack),
    .s_ctrlport_resp_status  (),
    .s_ctrlport_resp_data    (ctrlport_resp_data),
    .m_ctrlport_req_wr       (m_ctrlport_req_wr),
    .m_ctrlport_req_rd       (m_ctrlport_req_rd),
    .m_ctrlport_req_addr     (m_ctrlport_req_addr),
    .m_ctrlport_req_data     (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status  ({NUM_PORTS*2{1'b0}}),
    .m_ctrlport_resp_data    (m_ctrlport_resp_data)
  );


  //---------------------------------------------------------------------------
  // FIFO Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_ram_fifos

    wire [MEM_ADDR_W-1:0] m_axi_awaddr_int;
    wire [MEM_ADDR_W-1:0] m_axi_araddr_int;

    // Resize the addresses from MEM_ADDR_W to AWIDTH
    assign m_axi_awaddr[(AWIDTH*(i+1))-1:AWIDTH*i] = m_axi_awaddr_int;
    assign m_axi_araddr[(AWIDTH*(i+1))-1:AWIDTH*i] = m_axi_araddr_int;

    axi_ram_fifo #(
      .MEM_ADDR_W     (MEM_ADDR_W),
      .MEM_DATA_W     (MEM_DATA_W),
      .KEEP_W         (KEEP_W),
      .FIFO_ADDR_BASE (FIFO_ADDR_BASE[MEM_ADDR_W*i +: MEM_ADDR_W]),
      .FIFO_ADDR_MASK (FIFO_ADDR_MASK[MEM_ADDR_W*i +: MEM_ADDR_W]),
      .BURST_TIMEOUT  (BURST_TIMEOUT[32*i +: 32]),
      .BIST           (BIST),
      .CLK_RATE       (MEM_CLK_RATE),
      .IN_FIFO_SIZE   (IN_FIFO_SIZE),
      .OUT_FIFO_SIZE  (OUT_FIFO_SIZE)
    ) axi_ram_fifo_i (

      .clk(mem_clk),
      .rst(mem_rst_block),

      //-----------------------------------------------------------------------
      // Control Port
      //-----------------------------------------------------------------------

      .s_ctrlport_req_wr      (m_ctrlport_req_wr[i]),
      .s_ctrlport_req_rd      (m_ctrlport_req_rd[i]),
      .s_ctrlport_req_addr    (m_ctrlport_req_addr[20*i +: 20]),
      .s_ctrlport_req_data    (m_ctrlport_req_data[32*i +: 32]),
      .s_ctrlport_resp_ack    (m_ctrlport_resp_ack[i]),
      .s_ctrlport_resp_data   (m_ctrlport_resp_data[32*i +: 32]),

      //-----------------------------------------------------------------------
      // AXI-Stream FIFO Interface
      //-----------------------------------------------------------------------

      // AXI-Stream Input
      .s_tdata        (m_axis_data_tdata[MEM_DATA_W*i +: MEM_DATA_W]),
      .s_tkeep        (m_axis_data_tkeep[KEEP_W*i +: KEEP_W]),
      .s_tlast        (m_axis_data_tlast[i]),
      .s_tvalid       (m_axis_data_tvalid[i]),
      .s_tready       (m_axis_data_tready[i]),
      //
      //  AXI-Stream Output
      .m_tdata        (s_axis_data_tdata[MEM_DATA_W*i +: MEM_DATA_W]),
      .m_tkeep        (s_axis_data_tkeep[KEEP_W*i +: KEEP_W]),
      .m_tlast        (s_axis_data_tlast[i]),
      .m_tvalid       (s_axis_data_tvalid[i]),
      .m_tready       (s_axis_data_tready[i]),

      //-----------------------------------------------------------------------
      // AXI4 Memory Interface
      //-----------------------------------------------------------------------

      // AXI Write address channel
      .m_axi_awid     (m_axi_awid[i]),
      .m_axi_awaddr   (m_axi_awaddr_int),
      .m_axi_awlen    (m_axi_awlen[(8*(i+1))-1:8*i]),
      .m_axi_awsize   (m_axi_awsize[(3*(i+1))-1:3*i]),
      .m_axi_awburst  (m_axi_awburst[(2*(i+1))-1:2*i]),
      .m_axi_awlock   (m_axi_awlock[i]),
      .m_axi_awcache  (m_axi_awcache[(4*(i+1))-1:4*i]),
      .m_axi_awprot   (m_axi_awprot[(3*(i+1))-1:3*i]),
      .m_axi_awqos    (m_axi_awqos[(4*(i+1))-1:4*i]),
      .m_axi_awregion (m_axi_awregion[(4*(i+1))-1:4*i]),
      .m_axi_awuser   (m_axi_awuser[i]),
      .m_axi_awvalid  (m_axi_awvalid[i]),
      .m_axi_awready  (m_axi_awready[i]),
      //
      // AXI Write data channel.
      .m_axi_wdata    (m_axi_wdata[(MEM_DATA_W*(i+1))-1:MEM_DATA_W*i]),
      .m_axi_wstrb    (m_axi_wstrb[((MEM_DATA_W/8)*(i+1))-1:(MEM_DATA_W/8)*i]),
      .m_axi_wlast    (m_axi_wlast[i]),
      .m_axi_wuser    (m_axi_wuser[i]),
      .m_axi_wvalid   (m_axi_wvalid[i]),
      .m_axi_wready   (m_axi_wready[i]),
      //
      // AXI Write response channel signals
      .m_axi_bid      (m_axi_bid[i]),
      .m_axi_bresp    (m_axi_bresp[(2*(i+1))-1:2*i]),
      .m_axi_buser    (m_axi_buser[i]),
      .m_axi_bvalid   (m_axi_bvalid[i]),
      .m_axi_bready   (m_axi_bready[i]),
      //
      // AXI Read address channel
      .m_axi_arid     (m_axi_arid[i]),
      .m_axi_araddr   (m_axi_araddr_int),
      .m_axi_arlen    (m_axi_arlen[(8*(i+1))-1:8*i]),
      .m_axi_arsize   (m_axi_arsize[(3*(i+1))-1:3*i]),
      .m_axi_arburst  (m_axi_arburst[(2*(i+1))-1:2*i]),
      .m_axi_arlock   (m_axi_arlock[i]),
      .m_axi_arcache  (m_axi_arcache[(4*(i+1))-1:4*i]),
      .m_axi_arprot   (m_axi_arprot[(3*(i+1))-1:3*i]),
      .m_axi_arqos    (m_axi_arqos[(4*(i+1))-1:4*i]),
      .m_axi_arregion (m_axi_arregion[(4*(i+1))-1:4*i]),
      .m_axi_aruser   (m_axi_aruser[i]),
      .m_axi_arvalid  (m_axi_arvalid[i]),
      .m_axi_arready  (m_axi_arready[i]),
      //
      // AXI Read data channel
      .m_axi_rid      (m_axi_rid[i]),
      .m_axi_rdata    (m_axi_rdata[(MEM_DATA_W*(i+1))-1:MEM_DATA_W*i]),
      .m_axi_rresp    (m_axi_rresp[(2*(i+1))-1:2*i]),
      .m_axi_rlast    (m_axi_rlast[i]),
      .m_axi_ruser    (m_axi_ruser[i]),
      .m_axi_rvalid   (m_axi_rvalid[i]),
      .m_axi_rready   (m_axi_rready[i])
    );

  end

endmodule
