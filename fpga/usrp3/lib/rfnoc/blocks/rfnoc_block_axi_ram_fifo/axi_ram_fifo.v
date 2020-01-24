//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axi_ram_fifo
//
// Description:
//
//   Implements a FIFO using a memory-mapped AXI interface as storage. This can 
//   be connected to any memory-mapped AXI4 bus interface, such as DRAM, SRAM, 
//   or AXI interconnect IP. The input and output interfaces to the FIFO are 
//   AXI-Stream.
//
//   The logic is designed to buffer up multiple words so that writes and reads 
//   can be implemented as efficient burst transactions on the AXI4 bus. This 
//   core never crosses 4 KiB boundaries, per AXI4 rules (a burst must not 
//   cross a 4 KiB boundary).
//
//   The FIFO must be at least 4 KiB in size so that the 4 KiB page boundary 
//   protection also handles/prevents the FIFO wrap corner case.
//
// Parameters:
//
//   MEM_ADDR_W     : The width of the byte address to use for the AXI4 memory 
//                    mapped interface.
//
//   MEM_DATA_W     : The width of the data port to use for the AXI4 memory 
//                    mapped interface.
//
//   KEEP_W         : Width of tkeep on the AXI-Stream interface. Set to 1 if 
//                    tkeep is not used.
//
//   FIFO_ADDR_BASE : Default base address to use for this FIFO.
//
//   FIFO_ADDR_MASK : Default byte address mask, which defines which memory 
//                    address bits can be used for the FIFO. For example, an 64 
//                    KiB memory region, or 2^16 bytes, would require the mask 
//                    0xFFFF (16 ones). In other words, the mask should be the 
//                    size of the memory region minus 1.
//
//   BURST_TIMEOUT  : Default number of memory clock cycles to wait for new 
//                    data before performing a short, sub-optimal burst. One 
//                    value per FIFO.
//
//   BIST           : If true, BIST logic will be included in the build.
//
//   CLK_RATE       : Frequency of clk in Hz
//
//   IN_FIFO_SIZE   : The input FIFO size will be 2**IN_FIFO_SIZE in depth.
//
//   OUT_FIFO_SIZE  : The output FIFO size will be 2**OUT_FIFO_SIZE in depth. 
//                    This must be at least 9 so that there is enough space to 
//                    accept a full AXI4 burst and then accept additional 
//                    bursts while the FIFO is reading out.
//

module axi_ram_fifo #(
  parameter                  MEM_ADDR_W     = 32,
  parameter                  MEM_DATA_W     = 64,
  parameter                  KEEP_W         = 1,
  parameter [MEM_ADDR_W-1:0] FIFO_ADDR_BASE = 'h0,
  parameter [MEM_ADDR_W-1:0] FIFO_ADDR_MASK = 'h00FFFFFF,
  parameter                  BURST_TIMEOUT  = 256,
  parameter                  BIST           = 1,
  parameter                  CLK_RATE       = 200e6,
  parameter                  IN_FIFO_SIZE   = 11,
  parameter                  OUT_FIFO_SIZE  = 10
) (
  
  input clk,
  input rst,

  //--------------------------------------------------------------------------
  // CTRL Port
  //--------------------------------------------------------------------------

  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output wire        s_ctrlport_resp_ack,
  output wire [31:0] s_ctrlport_resp_data,

  //--------------------------------------------------------------------------
  // AXI-Stream Interface
  //--------------------------------------------------------------------------

  // FIFO Input
  input  wire [MEM_DATA_W-1:0] s_tdata,
  input  wire [    KEEP_W-1:0] s_tkeep,
  input  wire                  s_tlast,
  input  wire                  s_tvalid,
  output wire                  s_tready,

  // FIFO Output
  output wire [MEM_DATA_W-1:0] m_tdata,
  output wire [    KEEP_W-1:0] m_tkeep,
  output wire                  m_tlast,
  output wire                  m_tvalid,
  input  wire                  m_tready,

  //--------------------------------------------------------------------------
  // AXI4 Memory Interface
  //--------------------------------------------------------------------------

  // AXI Write Address Channel
  output wire [             0:0] m_axi_awid,     // Write address ID. This signal is the identification tag for the write address signals.
  output wire [  MEM_ADDR_W-1:0] m_axi_awaddr,   // Write address. The write address gives the address of the first transfer in a write burst.
  output wire [             7:0] m_axi_awlen,    // Burst length. The burst length gives the exact number of transfers in a burst.
  output wire [             2:0] m_axi_awsize,   // Burst size. This signal indicates the size of each transfer in the burst.
  output wire [             1:0] m_axi_awburst,  // Burst type. The burst type and the size information, determine how the address is calculated.
  output wire [             0:0] m_axi_awlock,   // Lock type. Provides additional information about the atomic characteristics of the transfer.
  output wire [             3:0] m_axi_awcache,  // Memory type. This signal indicates how transactions are required to progress.
  output wire [             2:0] m_axi_awprot,   // Protection type. This signal indicates the privilege and security level of the transaction.
  output wire [             3:0] m_axi_awqos,    // Quality of Service, QoS. The QoS identifier sent for each write transaction.
  output wire [             3:0] m_axi_awregion, // Region identifier. Permits a single physical interface on a slave to be re-used.
  output wire [             0:0] m_axi_awuser,   // User signal. Optional User-defined signal in the write address channel.
  output wire                    m_axi_awvalid,  // Write address valid. This signal indicates that the channel is signaling valid write addr.
  input  wire                    m_axi_awready,  // Write address ready. This signal indicates that the slave is ready to accept an address.
  //
  // AXI Write Data Channel
  output wire [  MEM_DATA_W-1:0] m_axi_wdata,    // Write data
  output wire [MEM_DATA_W/8-1:0] m_axi_wstrb,    // Write strobes. This signal indicates which byte lanes hold valid data.
  output wire                    m_axi_wlast,    // Write last. This signal indicates the last transfer in a write burst.
  output wire [             0:0] m_axi_wuser,    // User signal. Optional User-defined signal in the write data channel.
  output wire                    m_axi_wvalid,   // Write valid. This signal indicates that valid write data and strobes are available.
  input  wire                    m_axi_wready,   // Write ready. This signal indicates that the slave can accept the write data.
  //
  // AXI Write Response Channel
  input  wire [             0:0] m_axi_bid,      // Response ID tag. This signal is the ID tag of the write response.
  input  wire [             1:0] m_axi_bresp,    // Write response. This signal indicates the status of the write transaction.
  input  wire [             0:0] m_axi_buser,    // User signal. Optional User-defined signal in the write response channel.
  input  wire                    m_axi_bvalid,   // Write response valid. This signal indicates that the channel is signaling a valid response.
  output wire                    m_axi_bready,   // Response ready. This signal indicates that the master can accept a write response.
  //
  // AXI Read Address Channel
  output wire [             0:0] m_axi_arid,     // Read address ID. This signal is the identification tag for the read address group of signals.
  output wire [  MEM_ADDR_W-1:0] m_axi_araddr,   // Read address. The read address gives the address of the first transfer in a read burst.
  output wire [             7:0] m_axi_arlen,    // Burst length. This signal indicates the exact number of transfers in a burst.
  output wire [             2:0] m_axi_arsize,   // Burst size. This signal indicates the size of each transfer in the burst.
  output wire [             1:0] m_axi_arburst,  // Burst type. The burst type and the size information determine how the address for each transfer.
  output wire [             0:0] m_axi_arlock,   // Lock type. This signal provides additional information about the atomic characteristics.
  output wire [             3:0] m_axi_arcache,  // Memory type. This signal indicates how transactions are required to progress.
  output wire [             2:0] m_axi_arprot,   // Protection type. This signal indicates the privilege and security level of the transaction.
  output wire [             3:0] m_axi_arqos,    // Quality of Service, QoS. QoS identifier sent for each read transaction.
  output wire [             3:0] m_axi_arregion, // Region identifier. Permits a single physical interface on a slave to be re-used.
  output wire [             0:0] m_axi_aruser,   // User signal. Optional User-defined signal in the read address channel.
  output wire                    m_axi_arvalid,  // Read address valid. This signal indicates that the channel is signaling valid read addr.
  input  wire                    m_axi_arready,  // Read address ready. This signal indicates that the slave is ready to accept an address.
  //
  // AXI Read Data Channel
  input  wire [             0:0] m_axi_rid,      // Read ID tag. This signal is the identification tag for the read data group of signals.
  input  wire [  MEM_DATA_W-1:0] m_axi_rdata,    // Read data.
  input  wire [             1:0] m_axi_rresp,    // Read response. This signal indicates the status of the read transfer.
  input  wire                    m_axi_rlast,    // Read last. This signal indicates the last transfer in a read burst.
  input  wire [             0:0] m_axi_ruser,    // User signal. Optional User-defined signal in the read data channel.
  input  wire                    m_axi_rvalid,   // Read valid. This signal indicates that the channel is signaling the required read data.
  output wire                    m_axi_rready    // Read ready. This signal indicates that the master can accept the read data and response.
);

  `include "axi_ram_fifo_regs.vh"


  //---------------------------------------------------------------------------
  // Parameter Checking
  //---------------------------------------------------------------------------

  // The input FIFO size must be at least 9 so that there is enough space to 
  // hold an entire burst and be able to accept new data while that burst is 
  // waiting to be ready out.
  if (IN_FIFO_SIZE < 9) begin
    IN_FIFO_SIZE_must_be_at_least_9();
  end

  // The output FIFO size must be at least 9 so that there is enough space to 
  // accept a full AXI4 burst (255 words) and then accept additional bursts 
  // while the FIFO is waiting to be read out.
  if (OUT_FIFO_SIZE < 9) begin
    OUT_FIFO_SIZE_must_be_at_least_9();
  end

  // The memory must be at least as big as the default FIFO mask
  if (2.0**MEM_ADDR_W < FIFO_ADDR_MASK+1) begin
    MEM_ADDR_W_must_be_larger_than_size_indicated_by_FIFO_ADDR_MASK();
  end

  // The FIFO memory must be large enough for a full AXI4 burst + 64 words 
  // that's allocated to allow for read/write reordering.
  // TODO: Is the 64-word extra needed? Why 64?
  //
  // Min size allowed for memory region in bytes
  localparam FIFO_MIN_RAM_SIZE  = (256+64) * (MEM_DATA_W/8); 
  //
  // Equivalent mask
  localparam FIFO_ADDR_MASK_MIN = 2**($clog2(FIFO_MIN_RAM_SIZE))-1;
  //
  // Check the parameter
  if (FIFO_ADDR_MASK < FIFO_ADDR_MASK_MIN) begin
    FIFO_ADDR_MASK_must_be_at_least_256_plus_64_words();
  end

  // The 4 KiB page-crossing detection logic assumes that the memory is more 
  // than 4 kiB in size. This could be fixed in the code, but 8 KiB is already 
  // pretty small for an external memory.
  if (2.0**MEM_ADDR_W < 8192) begin
    MEM_ADDR_W_must_be_at_least_8_KiB();
  end

  // Make sure the default burst timeout is not too big for the register
  if ($clog2(BURST_TIMEOUT+1) > REG_TIMEOUT_W) begin
    BURST_TIMEOUT_must_not_exceed_the_range_of_REG_TIMEOUT_W();
  end


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Width of the timeout counter
  localparam TIMEOUT_W = REG_TIMEOUT_W;

  // Address widths. Each memory byte address can be broken up into the word 
  // address portion (the upper bits) and the byte address portion (lower 
  // bits). Although the memory is byte addressable, we only read/write whole 
  // words.
  localparam BYTE_ADDR_W = $clog2(MEM_DATA_W/8);
  localparam WORD_ADDR_W = MEM_ADDR_W - BYTE_ADDR_W;


  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  function automatic integer min(input integer a, b);
    min = a < b ? a : b;
  endfunction


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Track RAM FIFO state, in number of words
  reg [WORD_ADDR_W:0] space;
  reg [WORD_ADDR_W:0] occupied;
  reg [WORD_ADDR_W:0] occupied_minus_one; // Maintain a -1 version to break critical timing paths

  reg [31:0] out_pkt_count = 0;

  //
  // Input Side
  //
  reg  [MEM_DATA_W-1:0] s_tdata_fifo;
  reg                   s_tvalid_fifo;
  wire                  s_tready_fifo;

  wire [MEM_DATA_W-1:0] m_tdata_fifo;
  wire                  m_tvalid_fifo;
  reg                   m_tready_fifo;

  wire [MEM_DATA_W-1:0] s_tdata_i1;
  wire [    KEEP_W-1:0] s_tkeep_i1;
  wire                  s_tvalid_i1, s_tready_i1, s_tlast_i1;

  wire [MEM_DATA_W-1:0] s_tdata_i2;
  wire                  s_tvalid_i2, s_tready_i2;

  wire [MEM_DATA_W-1:0] s_tdata_i3;
  wire                  s_tvalid_i3;
  reg                   s_tready_i3;

  wire [MEM_DATA_W-1:0] s_tdata_input;
  wire                  s_tvalid_input, s_tready_input;

  wire [15:0] space_input, occupied_input;
  reg  [15:0] space_input_reg;
  reg         suppress_reads;

  //
  // Output Side
  //
  wire [MEM_DATA_W-1:0] m_tdata_output;
  wire                  m_tvalid_output, m_tready_output;

  reg [MEM_DATA_W-1:0] m_tdata_i0;
  reg                  m_tvalid_i0;
  wire                 m_tready_i0;
  
  wire [MEM_DATA_W-1:0] m_tdata_i1;
  wire                  m_tvalid_i1, m_tready_i1;
  
  wire [MEM_DATA_W-1:0] m_tdata_i2;
  wire                  m_tvalid_i2, m_tready_i2;
  
  wire [MEM_DATA_W-1:0] m_tdata_i3;
  wire [    KEEP_W-1:0] m_tkeep_i3;
  wire                  m_tvalid_i3, m_tready_i3, m_tlast_i3;

  wire [15:0] space_output;


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  wire [          15:0] set_suppress_threshold;
  wire [ TIMEOUT_W-1:0] set_timeout;
  wire                  set_clear = 1'b0;       // Clear no longer needed in RFNoC
  wire [MEM_ADDR_W-1:0] set_fifo_addr_base;
  wire [MEM_ADDR_W-1:0] set_fifo_addr_mask;

  wire        s_ctrlport_resp_ack_regs;
  wire [31:0] s_ctrlport_resp_data_regs;

  axi_ram_fifo_regs #(
    .MEM_ADDR_W         (MEM_ADDR_W),
    .MEM_DATA_W         (MEM_DATA_W),
    .FIFO_ADDR_BASE     (FIFO_ADDR_BASE),
    .FIFO_ADDR_MASK     (FIFO_ADDR_MASK),
    .FIFO_ADDR_MASK_MIN (FIFO_ADDR_MASK_MIN),
    .BIST               (BIST),
    .IN_FIFO_SIZE       (IN_FIFO_SIZE),
    .WORD_ADDR_W        (WORD_ADDR_W),
    .BURST_TIMEOUT      (BURST_TIMEOUT),
    .TIMEOUT_W          (TIMEOUT_W)
  ) axi_ram_fifo_regs_i (
    .clk                    (clk),
    .rst                    (rst),
    .s_ctrlport_req_wr      (s_ctrlport_req_wr),
    .s_ctrlport_req_rd      (s_ctrlport_req_rd),
    .s_ctrlport_req_addr    (s_ctrlport_req_addr),
    .s_ctrlport_req_data    (s_ctrlport_req_data),
    .s_ctrlport_resp_ack    (s_ctrlport_resp_ack_regs),
    .s_ctrlport_resp_data   (s_ctrlport_resp_data_regs),
    .rb_out_pkt_count       (out_pkt_count),
    .rb_occupied            (occupied),
    .set_suppress_threshold (set_suppress_threshold),
    .set_timeout            (set_timeout),
    .set_fifo_addr_base     (set_fifo_addr_base),
    .set_fifo_addr_mask     (set_fifo_addr_mask)
  );

  //synthesis translate_off
  // Check the address mask at run-time
  always @(set_fifo_addr_mask) begin
    if (set_fifo_addr_mask < FIFO_ADDR_MASK_MIN) begin
      $display("ERROR: set_fifo_addr_mask was set too small!");
    end
    if (2**$clog2(set_fifo_addr_mask)-1 != set_fifo_addr_mask) begin
      $display("ERROR: set_fifo_addr_mask must be a power of 2, minus 1!");
    end
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // BIST for production testing
  //---------------------------------------------------------------------------

  if (BIST) begin : gen_bist
    wire                  s_ctrlport_resp_ack_bist;
    wire [          31:0] s_ctrlport_resp_data_bist;
    wire [MEM_DATA_W-1:0] m_tdata_bist;
    wire                  m_tvalid_bist;
    reg                   m_tready_bist;
    reg  [MEM_DATA_W-1:0] s_tdata_bist;
    reg                   s_tvalid_bist;
    wire                  s_tready_bist;

    wire bist_running;

    axi_ram_fifo_bist #(
      .DATA_W   (MEM_DATA_W),
      .COUNT_W  (48),
      .CLK_RATE (CLK_RATE),
      .RAND     (1)
    ) axi_ram_fifo_bist_i (
      .clk                  (clk),
      .rst                  (rst),
      .s_ctrlport_req_wr    (s_ctrlport_req_wr),
      .s_ctrlport_req_rd    (s_ctrlport_req_rd),
      .s_ctrlport_req_addr  (s_ctrlport_req_addr),
      .s_ctrlport_req_data  (s_ctrlport_req_data),
      .s_ctrlport_resp_ack  (s_ctrlport_resp_ack_bist),
      .s_ctrlport_resp_data (s_ctrlport_resp_data_bist),
      .m_tdata              (m_tdata_bist),
      .m_tvalid             (m_tvalid_bist),
      .m_tready             (m_tready_bist),
      .s_tdata              (s_tdata_bist),
      .s_tvalid             (s_tvalid_bist),
      .s_tready             (s_tready_bist),
      .running              (bist_running)
    );

    // Use a multiplexer to decide where the data flows, using the BIST when 
    // ever the BIST is running.
    always @(*) begin
      if (bist_running) begin
        // Insert the BIST logic
        s_tdata_fifo  = m_tdata_bist;
        s_tvalid_fifo = m_tvalid_bist;
        m_tready_bist = s_tready_fifo;
        //
        s_tdata_bist  = m_tdata_fifo;
        s_tvalid_bist = m_tvalid_fifo;
        m_tready_fifo = s_tready_bist;

        // Disable output-logic
        s_tready_i3 = 0;
        m_tdata_i0  = m_tdata_fifo;
        m_tvalid_i0 = 0;
      end else begin
        // Disable BIST
        m_tready_bist = 0;
        s_tdata_bist  = m_tdata_fifo;
        s_tvalid_bist = 0;
        
        // Bypass BIST
        s_tdata_fifo  = s_tdata_i3;
        s_tvalid_fifo = s_tvalid_i3;
        s_tready_i3   = s_tready_fifo;
        //
        m_tdata_i0    = m_tdata_fifo;
        m_tvalid_i0   = m_tvalid_fifo;
        m_tready_fifo = m_tready_i0;
      end
    end

    // Combine register responses
    ctrlport_resp_combine #(
      .NUM_SLAVES (2)
    ) ctrlport_resp_combine_i (
      .ctrlport_clk           (clk),
      .ctrlport_rst           (rst),
      .m_ctrlport_resp_ack    ({s_ctrlport_resp_ack_bist, s_ctrlport_resp_ack_regs}),
      .m_ctrlport_resp_status ({2{2'b00}}),
      .m_ctrlport_resp_data   ({s_ctrlport_resp_data_bist, s_ctrlport_resp_data_regs}),
      .s_ctrlport_resp_ack    (s_ctrlport_resp_ack),
      .s_ctrlport_resp_status (),
      .s_ctrlport_resp_data   (s_ctrlport_resp_data)
    );

  end else begin : gen_no_bist
    assign s_ctrlport_resp_ack  = s_ctrlport_resp_ack_regs;
    assign s_ctrlport_resp_data = s_ctrlport_resp_data_regs;
    always @(*) begin
      // Bypass the BIST logic
      s_tdata_fifo  = s_tdata_i3;
      s_tvalid_fifo = s_tvalid_i3;
      s_tready_i3   = s_tready_fifo;
      //
      m_tdata_i0    = m_tdata_fifo;
      m_tvalid_i0   = m_tvalid_fifo;
      m_tready_fifo = m_tready_i0;
      //
    end
  end


  //---------------------------------------------------------------------------
  // Input Handling and Buffer
  //---------------------------------------------------------------------------
  //
  // This block embeds TLAST into the data stream using an escape code and 
  // buffers up input data.
  //
  //---------------------------------------------------------------------------

  // Insert flops to improve timing
  axi_fifo_flop2 #(
    .WIDTH (MEM_DATA_W+1+KEEP_W)
  ) input_pipe_i0 (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  ({s_tkeep, s_tlast, s_tdata}),
    .i_tvalid (s_tvalid),
    .i_tready (s_tready),
    //
    .o_tdata  ({s_tkeep_i1, s_tlast_i1, s_tdata_i1}),
    .o_tvalid (s_tvalid_i1),
    .o_tready (s_tready_i1),
    //
    .space    (),
    .occupied ()
  );

  axi_embed_tlast_tkeep #(
    .DATA_W (MEM_DATA_W),
    .KEEP_W (KEEP_W)
  ) axi_embed_tlast_tkeep_i (
    .clk      (clk),
    .rst      (rst | set_clear),
    //
    .i_tdata  (s_tdata_i1),
    .i_tkeep  (s_tkeep_i1),
    .i_tlast  (s_tlast_i1),
    .i_tvalid (s_tvalid_i1),
    .i_tready (s_tready_i1),
    //
    .o_tdata  (s_tdata_i2),
    .o_tvalid (s_tvalid_i2),
    .o_tready (s_tready_i2)
  );

  // Insert flops to improve timing
  axi_fifo_flop2 #(
    .WIDTH (MEM_DATA_W)
  ) input_pipe_i1 (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  (s_tdata_i2),
    .i_tvalid (s_tvalid_i2),
    .i_tready (s_tready_i2),
    //
    .o_tdata  (s_tdata_i3),
    .o_tvalid (s_tvalid_i3),
    .o_tready (s_tready_i3),
    //
    .space    (),
    .occupied ()
  );

  axi_fifo #(
    .WIDTH (MEM_DATA_W),
    .SIZE  (IN_FIFO_SIZE)
  ) input_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  (s_tdata_fifo),
    .i_tvalid (s_tvalid_fifo),
    .i_tready (s_tready_fifo),
    //
    .o_tdata  (s_tdata_input),
    .o_tvalid (s_tvalid_input),
    .o_tready (s_tready_input),
    //
    .space    (space_input),
    .occupied (occupied_input)
  );


  //---------------------------------------------------------------------------
  // Input (Memory Write) Logic
  //---------------------------------------------------------------------------
  //
  // The input state machine waits for enough entries in input FIFO to trigger 
  // RAM write burst. A timeout can also trigger a burst so that smaller chunks
  // of data are not left to rot in the input FIFO. Also, if enough data is 
  // present in the input FIFO to complete a burst up to the edge of a 4 KiB 
  // page then we do a burst up to the 4 KiB boundary.
  //
  //---------------------------------------------------------------------------

  //
  // Input side declarations
  //
  localparam [2:0] INPUT_IDLE = 0;
  localparam [2:0] INPUT1 = 1;
  localparam [2:0] INPUT2 = 2;
  localparam [2:0] INPUT3 = 3;  
  localparam [2:0] INPUT4 = 4;
  localparam [2:0] INPUT5 = 5;
  localparam [2:0] INPUT6 = 6;

  wire write_ctrl_ready;

  reg [           2:0] input_state;
  reg                  input_timeout_triggered;
  reg                  input_timeout_reset;
  reg [ TIMEOUT_W-1:0] input_timeout_count;
  reg [MEM_ADDR_W-1:0] write_addr;
  reg                  write_ctrl_valid;
  reg [           7:0] write_count             = 0;
  reg [           8:0] write_count_plus_one    = 1; // Maintain a +1 version to break critical timing paths
  reg                  update_write;

  reg [WORD_ADDR_W-1:0] input_page_boundary;

  //
  // Input timeout counter. Timeout count only increments when there is some 
  // data waiting to be written to the RAM.
  //
  always @(posedge clk) begin
    if (rst | set_clear) begin
      input_timeout_count     <= 0;
      input_timeout_triggered <= 0;
    end else if (input_timeout_reset) begin
      input_timeout_count     <= 0;
      input_timeout_triggered <= 0;
    end else if (input_timeout_count == set_timeout) begin
      input_timeout_triggered <= 1;
    end else if (input_state == INPUT_IDLE) begin
      input_timeout_count <= input_timeout_count + ((occupied_input != 0) ? 1 : 0);
    end
  end

  //
  // Input State Machine
  //
  always @(posedge clk)
    if (rst | set_clear) begin
      input_state          <= INPUT_IDLE;
      write_addr           <= set_fifo_addr_base & ~set_fifo_addr_mask;
      input_timeout_reset  <= 1'b0;
      write_ctrl_valid     <= 1'b0;
      write_count          <= 8'd0;
      write_count_plus_one <= 9'd1;
      update_write         <= 1'b0;
    end else begin
      case (input_state)
      //
      // INPUT_IDLE.
      // To start an input transfer to DRAM need:
      // 1) Space in the RAM
      // and either
      // 2) 256 entries in the input FIFO
      // or
      // 3) Timeout occurred while waiting for more data, which can only happen 
      //    if there's at least one word in the input FIFO).
      //
      INPUT_IDLE: begin
        write_ctrl_valid    <= 1'b0;
        update_write        <= 1'b0;
        input_timeout_reset <= 1'b0;
        if (space[WORD_ADDR_W:8] != 'd0) begin    // (space > 255): 256 or more slots in the RAM
          if (occupied_input[15:8] != 'd0) begin  // (occupied_input > 255): 256 or more words in input FIFO
            input_state         <= INPUT1;
            input_timeout_reset <= 1'b1;

            // Calculate the number of entries remaining until next 4 KiB page 
            // boundary is crossed, minus 1. The units of calculation are 
            // words. The address is always word aligned.
            input_page_boundary <= { write_addr[MEM_ADDR_W-1:12], {12-BYTE_ADDR_W{1'b1}} } - 
                                   write_addr[MEM_ADDR_W-1 : BYTE_ADDR_W];
          end else if (input_timeout_triggered) begin  // input FIFO timeout waiting for new data.
            input_state         <= INPUT2;
            input_timeout_reset <= 1'b1;
            // Calculate the number of entries remaining until next 4 KiB page 
            // boundary is crossed, minus 1. The units of calculation are 
            // words. The address is always word-aligned.
            input_page_boundary <= { write_addr[MEM_ADDR_W-1:12], {12-BYTE_ADDR_W{1'b1}} } - 
                                   write_addr[MEM_ADDR_W-1 : BYTE_ADDR_W];  
          end
        end
      end
      //
      // INPUT1.
      // Caused by input FIFO reaching 256 entries.
      // Request write burst of lesser of:
      // 1) Entries until page boundary crossed
      // 2) 256
      //
      INPUT1: begin
        // Replicated write logic to break a read timing critical path for 
        // write_count.
        write_count          <= input_page_boundary[min(12, WORD_ADDR_W)-1:8] == 0 ? 
                                input_page_boundary[7:0] :
                                255;
        write_count_plus_one <= input_page_boundary[min(12, WORD_ADDR_W)-1:8] == 0 ? 
                                input_page_boundary[7:0] + 1 :
                                256;
        write_ctrl_valid     <= 1'b1;
        if (write_ctrl_ready)
          input_state <= INPUT4; // Preemptive ACK
        else
          input_state <= INPUT3; // Wait for ACK
      end
      //
      // INPUT2.
      // Caused by timeout of input FIFO (occupied_input must now be 256 or 
      // less since it was 255 or less in the INPUT_IDLE state; otherwise we 
      // would have gone to INPUT1). Request write burst of lesser of:
      // 1) Entries until page boundary crossed
      // 2) Entries in input FIFO
      //
      INPUT2: begin
        // Replicated write logic to break a read timing critical path for 
        // write_count.
        write_count          <= input_page_boundary < occupied_input[8:0] - 1 ?
                                input_page_boundary[7:0] : 
                                occupied_input[8:0] - 1;  // Max result of 255
        write_count_plus_one <= input_page_boundary < occupied_input[8:0] - 1 ? 
                                input_page_boundary[7:0] + 1 :
                                occupied_input[8:0];
        write_ctrl_valid     <= 1'b1;
        if (write_ctrl_ready)
          input_state <= INPUT4; // Preemptive ACK
        else
          input_state <= INPUT3; // Wait for ACK
      end
      //
      // INPUT3.
      // Wait in this state for AXI4 DMA engine to accept transaction.
      //
      INPUT3: begin
        if (write_ctrl_ready) begin
          write_ctrl_valid <= 1'b0;
          input_state      <= INPUT4; // ACK
        end else begin
          write_ctrl_valid <= 1'b1;
          input_state      <= INPUT3; // Wait for ACK
        end
      end
      //
      // INPUT4.
      // Wait here until write_ctrl_ready_deasserts. This is important as the 
      // next time it asserts we know that a write response was received.
      INPUT4: begin
        write_ctrl_valid <= 1'b0;
        if (!write_ctrl_ready)
          input_state <= INPUT5; // Move on
        else
          input_state <= INPUT4; // Wait for deassert
      end  
      //
      // INPUT5.
      // Transaction has been accepted by AXI4 DMA engine. Now we wait for the 
      // re-assertion of write_ctrl_ready which signals that the AXI4 DMA 
      // engine has received a response for the whole write transaction and we 
      // assume that this means it is committed to DRAM. We are now free to 
      // update write_addr pointer and go back to idle state.
      // 
      INPUT5: begin
        write_ctrl_valid <= 1'b0;
        if (write_ctrl_ready) begin
          write_addr <= ((write_addr + (write_count_plus_one << $clog2(MEM_DATA_W/8))) & set_fifo_addr_mask) | (write_addr & ~set_fifo_addr_mask);
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
        input_state  <= INPUT_IDLE;
        update_write <= 1'b0;
      end

      default: 
        input_state <= INPUT_IDLE;
    endcase // case(input_state)
  end


  //---------------------------------------------------------------------------
  // Read Suppression Logic
  //---------------------------------------------------------------------------
  //
  // Monitor occupied_input to deduce when DRAM FIFO is running short of 
  // bandwidth and there is a danger of back-pressure passing upstream of the 
  // DRAM FIFO. In this situation, we suppress read requests to the DRAM FIFO 
  // so that more bandwidth is available to writes.
  //
  // However, not reading can actually cause the FIFO to fill up and stall, so 
  // if the input is stalled, allow switching back to reads. This allows the 
  // memory to fill up without causing deadlock.
  //
  //---------------------------------------------------------------------------

  reg input_idle, input_idle_d1, input_stalled;

  always @(posedge clk) begin
    // We consider the input to be stalled when the input state machine is idle 
    // for 2 or more clock cycles.
    input_idle    <= (input_state == INPUT_IDLE);
    input_idle_d1 <= input_idle;
    input_stalled <= input_idle && input_idle_d1;

    space_input_reg <= space_input;
    if (space_input_reg < set_suppress_threshold && !input_stalled)
      suppress_reads <= 1'b1;
    else 
      suppress_reads <= 1'b0;
  end


  //---------------------------------------------------------------------------
  // Output Handling and Buffer
  //---------------------------------------------------------------------------
  //
  // This block buffers output data and extracts the TLAS signal that was 
  // embedded into the data stream. 
  //
  //---------------------------------------------------------------------------

  // Large FIFO to buffer data read from DRAM. This FIFO must be large enough 
  // to accept a full burst read.
  axi_fifo #(
    .WIDTH (MEM_DATA_W),
    .SIZE  (OUT_FIFO_SIZE)
  ) output_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  (m_tdata_output),
    .i_tvalid (m_tvalid_output),
    .i_tready (m_tready_output),
    //
    .o_tdata  (m_tdata_fifo),
    .o_tvalid (m_tvalid_fifo),
    .o_tready (m_tready_fifo),
    //
    .space    (space_output),
    .occupied ()
  );

  // Place flops right after FIFO to improve timing
  axi_fifo_flop2 #(
    .WIDTH (MEM_DATA_W)
  ) output_pipe_i0 (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  (m_tdata_i0),
    .i_tvalid (m_tvalid_i0),
    .i_tready (m_tready_i0),
    //
    .o_tdata  (m_tdata_i1),
    .o_tvalid (m_tvalid_i1),
    .o_tready (m_tready_i1),
    //
    .space    (),
    .occupied ()
  );

  // Pipeline flop before TLAST extraction logic
  axi_fifo_flop2 #(
    .WIDTH (MEM_DATA_W)
  ) output_pipe_i1 (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  (m_tdata_i1),
    .i_tvalid (m_tvalid_i1),
    .i_tready (m_tready_i1),
    //
    .o_tdata  (m_tdata_i2),
    .o_tvalid (m_tvalid_i2),
    .o_tready (m_tready_i2),
    //
    .space    (),
    .occupied ()
  );

  axi_extract_tlast_tkeep #(
    .DATA_W (MEM_DATA_W),
    .KEEP_W (KEEP_W)
  ) axi_extract_tlast_tkeep_i (
    .clk      (clk),
    .rst      (rst | set_clear),
    //
    .i_tdata  (m_tdata_i2),
    .i_tvalid (m_tvalid_i2),
    .i_tready (m_tready_i2),
    //
    .o_tdata  (m_tdata_i3),
    .o_tkeep  (m_tkeep_i3),
    .o_tlast  (m_tlast_i3),
    .o_tvalid (m_tvalid_i3),
    .o_tready (m_tready_i3)
  );

  // Pipeline flop after TLAST extraction logic
  axi_fifo_flop2 #(
    .WIDTH (MEM_DATA_W+1+KEEP_W)
  ) output_pipe_i3 (
    .clk      (clk),
    .reset    (rst),
    .clear    (set_clear),
    //
    .i_tdata  ({m_tkeep_i3, m_tlast_i3, m_tdata_i3}),
    .i_tvalid (m_tvalid_i3),
    .i_tready (m_tready_i3),
    //
    .o_tdata  ({m_tkeep, m_tlast, m_tdata}),
    .o_tvalid (m_tvalid),
    .o_tready (m_tready),
    //
    .space    (),
    .occupied ()
  );


  //-------------------------------------------------------------------------
  // Output (Memory Read) Logic
  //-------------------------------------------------------------------------
  //
  // The output state machine Wait for enough entries in RAM to trigger read 
  // burst. A timeout can also trigger a burst so that smaller chunks of data 
  // are not left to rot in the RAM. Also, if enough data is present in the RAM 
  // to complete a burst up to the edge of a 4 KiB page boundary then we do a 
  // burst up to the 4 KiB boundary.
  //
  //---------------------------------------------------------------------------
  
  //
  // Output side declarations
  //
  localparam [2:0] OUTPUT_IDLE = 0;
  localparam [2:0] OUTPUT1 = 1;
  localparam [2:0] OUTPUT2 = 2;
  localparam [2:0] OUTPUT3 = 3;  
  localparam [2:0] OUTPUT4 = 4;
  localparam [2:0] OUTPUT5 = 5;
  localparam [2:0] OUTPUT6 = 6;

  reg  [           2:0] output_state;
  reg                   output_timeout_triggered;
  reg                   output_timeout_reset;
  reg  [ TIMEOUT_W-1:0] output_timeout_count;
  reg  [MEM_ADDR_W-1:0] read_addr;
  reg                   read_ctrl_valid;
  wire                  read_ctrl_ready;
  reg  [           7:0] read_count          = 0;
  reg  [           8:0] read_count_plus_one = 1; // Maintain a +1 version to break critical timing paths
  reg                   update_read;

  reg [WORD_ADDR_W-1:0] output_page_boundary; // Cache in a register to break critical timing paths

  //
  // Output Packet Counter
  //
  always @(posedge clk) begin
    if (rst) begin
      out_pkt_count <= 0;
    end else if (m_tlast & m_tvalid & m_tready) begin
      out_pkt_count <= out_pkt_count + 1;
    end
  end

  //
  // Output timeout counter. Timeout count only increments when there is some 
  // data waiting to be read from the RAM.
  //
  always @(posedge clk) begin
    if (rst | set_clear) begin
      output_timeout_count     <= 0;
      output_timeout_triggered <= 0;
    end else if (output_timeout_reset) begin
      output_timeout_count     <= 0;
      output_timeout_triggered <= 0;
    end else if (output_timeout_count == set_timeout) begin
      output_timeout_triggered <= 1;
    end else if (output_state == OUTPUT_IDLE) begin
      output_timeout_count <= output_timeout_count + ((occupied != 0) ? 1 : 0);
    end
  end

  //
  // Output State Machine
  //
  always @(posedge clk)
    if (rst | set_clear) begin
      output_state         <= OUTPUT_IDLE;
      read_addr            <= set_fifo_addr_base & ~set_fifo_addr_mask;
      output_timeout_reset <= 1'b0;
      read_ctrl_valid      <= 1'b0;
      read_count           <= 8'd0;
      read_count_plus_one  <= 9'd1;
      update_read          <= 1'b0;
    end else begin
      case (output_state)
      //
      // OUTPUT_IDLE.
      // To start an output transfer from DRAM
      // 1) Space in the output FIFO 
      // and either
      // 2) 256 entries in the RAM
      // or
      // 3) Timeout occurred while waiting for more data, which can only happen 
      //    if there's at least one word in the RAM.
      //
      OUTPUT_IDLE: begin
        read_ctrl_valid      <= 1'b0;
        update_read          <= 1'b0;
        output_timeout_reset <= 1'b0;
        if (space_output[15:8] != 'd0 && !suppress_reads) begin // (space_output > 255): 256 or more words in the output FIFO
          if (occupied[WORD_ADDR_W:8] != 'd0) begin             // (occupied > 255): 256 or more words in RAM
            output_state         <= OUTPUT1;
            output_timeout_reset <= 1'b1;

            // Calculate the number of entries remaining until next 4 KiB page 
            // boundary is crossed, minus 1. The units of calculation are 
            // words. The address is always word-aligned.
            output_page_boundary <= { read_addr[MEM_ADDR_W-1:12], {12-BYTE_ADDR_W{1'b1}} } - 
                                    read_addr[MEM_ADDR_W-1 : BYTE_ADDR_W];
          end else if (output_timeout_triggered) begin   // output FIFO timeout waiting for new data.
            output_state         <= OUTPUT2;
            output_timeout_reset <= 1'b1;
            // Calculate the number of entries remaining until next 4 KiB page 
            // boundary is crossed, minus 1. The units of calculation are 
            // words. The address is always word-aligned.
            output_page_boundary <= { read_addr[MEM_ADDR_W-1:12], {12-BYTE_ADDR_W{1'b1}} } - 
                                    read_addr[MEM_ADDR_W-1 : BYTE_ADDR_W];
          end
        end
      end
      //
      // OUTPUT1.
      // Caused by RAM FIFO reaching 256 entries.
      // Request read burst of lesser of lesser of:
      // 1) Entries until page boundary crossed
      // 2) 256
      //
      OUTPUT1: begin
        // Replicated write logic to break a read timing critical path for read_count
        read_count          <= output_page_boundary[min(12, WORD_ADDR_W)-1:8] == 0 ? 
                               output_page_boundary[7:0] : 
                               255;
        read_count_plus_one <= output_page_boundary[min(12, WORD_ADDR_W)-1:8] == 0 ?
                               output_page_boundary[7:0] + 1 :
                               256;
        read_ctrl_valid     <= 1'b1;
        if (read_ctrl_ready)
          output_state <= OUTPUT4; // Preemptive ACK
        else
          output_state <= OUTPUT3; // Wait for ACK
      end
      //
      // OUTPUT2.
      // Caused by timeout of main FIFO
      // Request read burst of lesser of:
      // 1) Entries until page boundary crossed
      // 2) Entries in main FIFO
      //
      OUTPUT2: begin
        // Replicated write logic to break a read timing critical path for read_count
        read_count          <= output_page_boundary < occupied_minus_one ? 
                               output_page_boundary[7:0] :
                               occupied_minus_one[7:0];
        read_count_plus_one <= output_page_boundary < occupied_minus_one ?
                               output_page_boundary[7:0] + 1 :
                               occupied[7:0];
        read_ctrl_valid     <= 1'b1;
        if (read_ctrl_ready)
          output_state <= OUTPUT4; // Preemptive ACK
        else
          output_state <= OUTPUT3; // Wait for ACK
      end
      //
      // OUTPUT3.
      // Wait in this state for AXI4 DMA engine to accept transaction.
      //
      OUTPUT3: begin
        if (read_ctrl_ready) begin
          read_ctrl_valid <= 1'b0;
          output_state    <= OUTPUT4; // ACK
        end else begin
          read_ctrl_valid <= 1'b1;
          output_state    <= OUTPUT3; // Wait for ACK
        end
      end
      //
      // OUTPUT4.
      // Wait here until read_ctrl_ready_deasserts. This is important as the 
      // next time it asserts we know that a read response was received.
      OUTPUT4: begin
        read_ctrl_valid <= 1'b0;
        if (!read_ctrl_ready)
          output_state <= OUTPUT5; // Move on
        else
          output_state <= OUTPUT4; // Wait for deassert
      end  
      //
      // OUTPUT5.
      // Transaction has been accepted by AXI4 DMA engine. Now we wait for the 
      // re-assertion of read_ctrl_ready which signals that the AXI4 DMA engine 
      // has received a last signal and good response for the whole read 
      // transaction. We are now free to update read_addr pointer and go back 
      // to idle state.
      // 
      OUTPUT5: begin
        read_ctrl_valid <= 1'b0;
        if (read_ctrl_ready) begin
          read_addr <= ((read_addr + (read_count_plus_one << $clog2(MEM_DATA_W/8))) & set_fifo_addr_mask) | (read_addr & ~set_fifo_addr_mask);
          output_state <= OUTPUT6;
          update_read <= 1'b1;
        end else begin
          output_state <= OUTPUT5;
        end
      end
      //
      // OUTPUT6.
      // Need to get occupied value updated before checking if there's more to do.
      //
      OUTPUT6: begin
        update_read  <= 1'b0;
        output_state <= OUTPUT_IDLE;
      end

      default: 
        output_state <= OUTPUT_IDLE;
    endcase // case(output_state)
  end


  //---------------------------------------------------------------------------
  // Shared Read/Write Logic
  //---------------------------------------------------------------------------

  //
  // Count number of words stored in the RAM FIFO.
  //
  always @(posedge clk) begin
    if (rst | set_clear) begin
      occupied           <=  0;
      occupied_minus_one <= -1;
    end else begin
      occupied           <= occupied           + (update_write ? write_count_plus_one : 0) - (update_read ? read_count_plus_one : 0);
      occupied_minus_one <= occupied_minus_one + (update_write ? write_count_plus_one : 0) - (update_read ? read_count_plus_one : 0);
    end
  end

  //
  // Count amount of space in the RAM FIFO, in words. 
  //
  always @(posedge clk) begin
    if (rst | set_clear) begin
      // Set to the FIFO size minus 64 words to make allowance for read/write 
      // reordering in DRAM controller.
      // TODO: Is the 64-word extra needed? Why 64?
      space <= set_fifo_addr_mask[MEM_ADDR_W-1 -: WORD_ADDR_W] & ~('d63);
    end else begin
      space <= space - (update_write ? write_count_plus_one : 0) + (update_read ? read_count_plus_one : 0);
    end
  end
  

  //---------------------------------------------------------------------------
  // AXI4 DMA Master
  //---------------------------------------------------------------------------

  axi_dma_master #(
     .AWIDTH (MEM_ADDR_W),
     .DWIDTH (MEM_DATA_W)
  ) axi_dma_master_i (
     .aclk             (clk),
     .areset           (rst | set_clear),
     // Write Address
     .m_axi_awid       (m_axi_awid),
     .m_axi_awaddr     (m_axi_awaddr),
     .m_axi_awlen      (m_axi_awlen),
     .m_axi_awsize     (m_axi_awsize),
     .m_axi_awburst    (m_axi_awburst),
     .m_axi_awvalid    (m_axi_awvalid),
     .m_axi_awready    (m_axi_awready),
     .m_axi_awlock     (m_axi_awlock),
     .m_axi_awcache    (m_axi_awcache),
     .m_axi_awprot     (m_axi_awprot),
     .m_axi_awqos      (m_axi_awqos),
     .m_axi_awregion   (m_axi_awregion),
     .m_axi_awuser     (m_axi_awuser),
     // Write Data
     .m_axi_wdata      (m_axi_wdata),
     .m_axi_wstrb      (m_axi_wstrb),
     .m_axi_wlast      (m_axi_wlast),
     .m_axi_wvalid     (m_axi_wvalid),
     .m_axi_wready     (m_axi_wready),
     .m_axi_wuser      (m_axi_wuser),
     // Write Response
     .m_axi_bid        (m_axi_bid),
     .m_axi_bresp      (m_axi_bresp),
     .m_axi_bvalid     (m_axi_bvalid),
     .m_axi_bready     (m_axi_bready),
     .m_axi_buser      (m_axi_buser),
     // Read Address
     .m_axi_arid       (m_axi_arid),
     .m_axi_araddr     (m_axi_araddr),
     .m_axi_arlen      (m_axi_arlen),
     .m_axi_arsize     (m_axi_arsize),
     .m_axi_arburst    (m_axi_arburst),
     .m_axi_arvalid    (m_axi_arvalid),
     .m_axi_arready    (m_axi_arready),
     .m_axi_arlock     (m_axi_arlock),
     .m_axi_arcache    (m_axi_arcache),
     .m_axi_arprot     (m_axi_arprot),
     .m_axi_arqos      (m_axi_arqos),
     .m_axi_arregion   (m_axi_arregion),
     .m_axi_aruser     (m_axi_aruser),
     // Read Data
     .m_axi_rid        (m_axi_rid),
     .m_axi_rdata      (m_axi_rdata),
     .m_axi_rresp      (m_axi_rresp),
     .m_axi_rlast      (m_axi_rlast),
     .m_axi_rvalid     (m_axi_rvalid),
     .m_axi_rready     (m_axi_rready),
     .m_axi_ruser      (m_axi_ruser),
     //
     // DMA interface for Write transactions
     //
     .write_addr       (write_addr),       // Byte address for start of write transaction (should be 64-bit aligned)
     .write_count      (write_count),      // Count of 64-bit words to write.
     .write_ctrl_valid (write_ctrl_valid),
     .write_ctrl_ready (write_ctrl_ready),
     .write_data       (s_tdata_input),
     .write_data_valid (s_tvalid_input),
     .write_data_ready (s_tready_input),
     //
     // DMA interface for Read transactions
     //
     .read_addr        (read_addr),        // Byte address for start of read transaction (should be 64-bit aligned)
     .read_count       (read_count),       // Count of 64-bit words to read.
     .read_ctrl_valid  (read_ctrl_valid),
     .read_ctrl_ready  (read_ctrl_ready),
     .read_data        (m_tdata_output),
     .read_data_valid  (m_tvalid_output),
     .read_data_ready  (m_tready_output),
     //
     // Debug
     //
     .debug            ()
  );

endmodule

