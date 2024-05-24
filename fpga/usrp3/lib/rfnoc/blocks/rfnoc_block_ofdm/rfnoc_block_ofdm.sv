//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ofdm
//
// Description:
//
//   FFT/IFFT plus cyclic prefix insertion/removal.
//
// User Parameters:
//
//   THIS_PORTID              : Control crossbar port to which this block is connected
//   CHDR_W                   : AXIS-CHDR data bus width
//   MTU                      : Log2 of maximum transmission unit
//   NUM_PORTS                : Number of channels
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size
//   MAX_CP_LEN_LOG2          : Log2 of maximum cyclic prefix length
//   MAX_CP_LIST_LEN_INS_LOG2 : Log2 of max length of cyclic prefix insertion list
//   MAX_CP_LIST_LEN_REM_LOG2 : Log2 of max length of cyclic prefix removal list
//   CP_INSERTION_REPEAT      : Enable (1) or disable (0) CP insertion list FIFO loopback
//   CP_REMOVAL_REPEAT        : Enable (1) or disable (0) CP removal list FIFO loopback
//

`default_nettype none

module rfnoc_block_ofdm #(
  logic [9:0] THIS_PORTID              = 10'd0,
  int         CHDR_W                   = 64,
  logic [5:0] MTU                      = 6'd10,
  int         NUM_PORTS                = 2,
  int         MAX_FFT_SIZE_LOG2        = 12,
  int         MAX_CP_LEN_LOG2          = 12,
  int         MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int         MAX_CP_LIST_LEN_REM_LOG2 = 5,
  int         CP_INSERTION_REPEAT      = 1,
  int         CP_REMOVAL_REPEAT        = 1
)(
  // RFNoC Framework Clocks and Resets
  input  wire                           rfnoc_chdr_clk,
  input  wire                           rfnoc_ctrl_clk,
  input  wire                           ce_clk,
  // RFNoC Backend Interface
  input  wire [511:0]                   rfnoc_core_config,
  output wire [511:0]                   rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]                    s_rfnoc_ctrl_tdata,
  input  wire                           s_rfnoc_ctrl_tlast,
  input  wire                           s_rfnoc_ctrl_tvalid,
  output wire                           s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]                    m_rfnoc_ctrl_tdata,
  output wire                           m_rfnoc_ctrl_tlast,
  output wire                           m_rfnoc_ctrl_tvalid,
  input  wire                           m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // FFT Configuration Interface Constants
  //---------------------------------------------------------------------------

  localparam int MAX_FFT_SIZE = 2**MAX_FFT_SIZE_LOG2;

  localparam int FFT_SCALE_PAD_W =
    MAX_FFT_SIZE == 4096  ? 3 :
    MAX_FFT_SIZE == 8192  ? 1 :
    MAX_FFT_SIZE == 16384 ? 1 :
    MAX_FFT_SIZE == 32768 ? 7 :
    MAX_FFT_SIZE == 65536 ? 7 : -1;
  localparam int FFT_SCALE_W =
    MAX_FFT_SIZE == 4096  ? 12 :
    MAX_FFT_SIZE == 8192  ? 14 :
    MAX_FFT_SIZE == 16384 ? 14 :
    MAX_FFT_SIZE == 32768 ? 16 :
    MAX_FFT_SIZE == 65536 ? 16 : -1;
  localparam int FFT_FWD_INV_W = 1;
  localparam int FFT_CP_LEN_PAD_W =
    MAX_FFT_SIZE == 4096  ? 4 :
    MAX_FFT_SIZE == 8192  ? 3 :
    MAX_FFT_SIZE == 16384 ? 2 :
    MAX_FFT_SIZE == 32768 ? 1 :
    MAX_FFT_SIZE == 65536 ? 0 : -1;
  localparam int FFT_CP_LEN_W =
    MAX_FFT_SIZE == 4096  ? 12 :
    MAX_FFT_SIZE == 8192  ? 13 :
    MAX_FFT_SIZE == 16384 ? 14 :
    MAX_FFT_SIZE == 32768 ? 15 :
    MAX_FFT_SIZE == 65536 ? 16 : -1;
  localparam int FFT_NFFT_PAD_W = 3;
  localparam int FFT_NFFT_W = 5;

  localparam int FFT_CONFIG_W =
    FFT_SCALE_PAD_W + FFT_SCALE_W + FFT_FWD_INV_W +
    FFT_CP_LEN_PAD_W + FFT_CP_LEN_W +
    FFT_NFFT_PAD_W + FFT_NFFT_W;

  // Use conservative 1/N scaling by default
  localparam [FFT_SCALE_W-1:0] DEFAULT_FFT_SCALING = {FFT_SCALE_W/2{2'b10}};


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               ctrlport_clk;
  wire               ctrlport_rst;
  wire               ctrlport_rst_noc_shell;
  wire               axis_data_clk;
  wire               axis_data_rst;
  wire               axis_data_rst_noc_shell;
  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  reg                m_ctrlport_resp_ack = 1'b0;
  reg [31:0]         m_ctrlport_resp_data;
  // Data Stream to User Logic: in
  wire [NUM_PORTS*32*1-1:0]   m_in_axis_tdata;
  wire [NUM_PORTS*1-1:0]      m_in_axis_tkeep;
  wire [NUM_PORTS-1:0]        m_in_axis_tlast;
  wire [NUM_PORTS-1:0]        m_in_axis_tvalid;
  wire [NUM_PORTS-1:0]        m_in_axis_tready;
  wire [NUM_PORTS*64-1:0]     m_in_axis_ttimestamp;
  wire [NUM_PORTS-1:0]        m_in_axis_thas_time;
  wire [NUM_PORTS*16-1:0]     m_in_axis_tlength;
  wire [NUM_PORTS-1:0]        m_in_axis_teov;
  wire [NUM_PORTS-1:0]        m_in_axis_teob;
  // Data Stream from User Logic: out
  wire [NUM_PORTS*32*1-1:0]   s_out_axis_tdata;
  wire [NUM_PORTS*1-1:0]      s_out_axis_tkeep;
  wire [NUM_PORTS-1:0]        s_out_axis_tlast;
  wire [NUM_PORTS-1:0]        s_out_axis_tvalid;
  wire [NUM_PORTS-1:0]        s_out_axis_tready;
  wire [NUM_PORTS*64-1:0]     s_out_axis_ttimestamp;
  wire [NUM_PORTS-1:0]        s_out_axis_thas_time;
  wire [NUM_PORTS*16-1:0]     s_out_axis_tlength;
  wire [NUM_PORTS-1:0]        s_out_axis_teov;
  wire [NUM_PORTS-1:0]        s_out_axis_teob;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_ofdm #(
    .CHDR_W              (CHDR_W),
    .THIS_PORTID         (THIS_PORTID),
    .MTU                 (MTU),
    .NUM_PORTS           (NUM_PORTS)
  ) noc_shell_ofdm_i (
    //---------------------
    // Framework Interface
    //---------------------
    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (),
    .rfnoc_ctrl_rst      (),
    .ce_rst              (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),
    //---------------------
    // Client Interface
    //---------------------
    // CtrlPort Clock and Reset
    .ctrlport_clk          (ctrlport_clk),
    .ctrlport_rst          (ctrlport_rst_noc_shell),
    // CtrlPort Master
    .m_ctrlport_req_wr     (m_ctrlport_req_wr),
    .m_ctrlport_req_rd     (m_ctrlport_req_rd),
    .m_ctrlport_req_addr   (m_ctrlport_req_addr),
    .m_ctrlport_req_data   (m_ctrlport_req_data),
    .m_ctrlport_resp_ack   (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data  (m_ctrlport_resp_data),
    // AXI-Stream Clock and Reset
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst_noc_shell),
    // Data Stream to User Logic: in
    .m_in_axis_tdata       (m_in_axis_tdata),
    .m_in_axis_tkeep       (m_in_axis_tkeep),
    .m_in_axis_tlast       (m_in_axis_tlast),
    .m_in_axis_tvalid      (m_in_axis_tvalid),
    .m_in_axis_tready      (m_in_axis_tready),
    .m_in_axis_ttimestamp  (m_in_axis_ttimestamp),
    .m_in_axis_thas_time   (m_in_axis_thas_time),
    .m_in_axis_tlength     (m_in_axis_tlength),
    .m_in_axis_teov        (m_in_axis_teov),
    .m_in_axis_teob        (m_in_axis_teob),
    // Data Stream from User Logic: out
    .s_out_axis_tdata      (s_out_axis_tdata),
    .s_out_axis_tkeep      (s_out_axis_tkeep),
    .s_out_axis_tlast      (s_out_axis_tlast),
    .s_out_axis_tvalid     (s_out_axis_tvalid),
    .s_out_axis_tready     (s_out_axis_tready),
    .s_out_axis_ttimestamp (s_out_axis_ttimestamp),
    .s_out_axis_thas_time  (s_out_axis_thas_time),
    .s_out_axis_tlength    (s_out_axis_tlength),
    .s_out_axis_teov       (s_out_axis_teov),
    .s_out_axis_teob       (s_out_axis_teob)
  );

  //---------------------------------------------------------------------------
  // Control Registers
  //---------------------------------------------------------------------------
  // FFT Direction
  localparam [0:0] FFT_REVERSE = 0;
  localparam [0:0] FFT_FORWARD = 1;

  localparam FFT_SIZE_WIDTH = MAX_FFT_SIZE_LOG2+1;
  localparam CP_LEN_WIDTH   = MAX_CP_LEN_LOG2+1;

  // TODO: Add compat register

  // Register addresses, multiplied by 4 for 32-bit word alignment
  localparam REG_USER_RESET_ADDR                     =  0;   // Any write to this reg forces a reset for 16 clock cycles.
  localparam REG_FFT_SIZE_LOG2_ADDR                  =  8;   // Log2 of FFT size
  localparam REG_FFT_SCALING_ADDR                    =  9;   // FFT scaling word, see Xilinx documentation
  localparam REG_FFT_DIRECTION_ADDR                  = 10;   // FFT or IFFT
  localparam REG_FFT_COMMIT_ADDR                     = 11;   // Commit the currently configured settings to the FFT IP
  localparam REG_CP_INSERTION_CP_LEN_ADDR            = 16;   // Cyclic Prefix (CP) length (insertion)
  localparam REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR  = 17;   // Any write to this reg will load a length into the cyclic prefix insertion FIFO
  localparam REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR   = 18;   // Any write to this reg will clear the cyclic prefix insertion FIFO
  localparam REG_CP_INSERTION_CP_LEN_FIFO_OCC_ADDR   = 19;   // Read only, fullness of cyclic prefix insertion FIFO
  localparam REG_CP_REMOVAL_CP_LEN_ADDR              = 32;   // Cyclic Prefix (CP) length (removal)
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR    = 33;   // Any write to this reg will load a length into the cyclic prefix removal FIFO
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR     = 34;   // Any write to this reg will clear the cyclic prefix removal FIFO
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_OCC_ADDR     = 35;   // Read only, fullness of cyclic prefix removal config FIFO

  localparam REG_USER_RESET_WIDTH                    = 1;
  localparam REG_FFT_SIZE_LOG2_WIDTH                 = FFT_NFFT_W;
  localparam REG_FFT_SCALING_WIDTH                   = FFT_SCALE_W;
  localparam REG_FFT_DIRECTION_WIDTH                 = FFT_FWD_INV_W;
  localparam REG_CP_INSERTION_CP_LEN_WIDTH           = CP_LEN_WIDTH;
  localparam REG_CP_REMOVAL_CP_LEN_WIDTH             = CP_LEN_WIDTH;
  localparam REG_CP_INSERTION_CP_LEN_FIFO_LOAD_WIDTH = 1;
  localparam REG_CP_INSERTION_CP_LEN_FIFO_CLR_WIDTH  = 1;
  localparam REG_CP_INSERTION_CP_LEN_FIFO_OCC_WIDTH  = 16;
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_WIDTH   = 1;
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_CLR_WIDTH    = 1;
  localparam REG_CP_REMOVAL_CP_LEN_FIFO_OCC_WIDTH    = 16;

  localparam DEFAULT_FFT_SIZE_LOG2                   = 12;                // 2^12 = 4096
  localparam DEFAULT_FFT_DIRECTION                   = FFT_REVERSE;       // Set for FFT
  localparam DEFAULT_CP_LEN                          = 0;

  reg  [REG_USER_RESET_WIDTH-1:0]                    reg_user_reset                     = 1'b0;
  reg  [REG_FFT_SIZE_LOG2_WIDTH-1:0]                 reg_fft_size_log2                  = DEFAULT_FFT_SIZE_LOG2;
  reg  [REG_FFT_SCALING_WIDTH-1:0]                   reg_fft_scaling                    = DEFAULT_FFT_SCALING;
  reg  [REG_FFT_DIRECTION_WIDTH-1:0]                 reg_fft_direction                  = DEFAULT_FFT_DIRECTION;
  reg                                                reg_fft_commit                     = 1'b0;
  reg  [REG_CP_INSERTION_CP_LEN_WIDTH-1:0]           reg_cp_insertion_len               = DEFAULT_CP_LEN;
  reg  [REG_CP_REMOVAL_CP_LEN_WIDTH-1:0]             reg_cp_removal_len                 = DEFAULT_CP_LEN;
  reg  [REG_CP_INSERTION_CP_LEN_FIFO_LOAD_WIDTH-1:0] reg_cp_insertion_cp_len_fifo_load  = 1'b0;
  reg  [REG_CP_INSERTION_CP_LEN_FIFO_CLR_WIDTH -1:0] reg_cp_insertion_cp_len_fifo_clr   = 1'b0;
  reg  [REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_WIDTH-1:0]   reg_cp_removal_cp_len_fifo_load    = 1'b0;
  reg  [REG_CP_REMOVAL_CP_LEN_FIFO_CLR_WIDTH -1:0]   reg_cp_removal_cp_len_fifo_clr     = 1'b0;
  wire [REG_CP_INSERTION_CP_LEN_FIFO_OCC_WIDTH -1:0] reg_cp_insertion_cp_len_fifo_occupied;
  wire [REG_CP_REMOVAL_CP_LEN_FIFO_OCC_WIDTH -1:0]   reg_cp_removal_cp_len_fifo_occupied;

  wire reg_fft_commit_next;

  wire [17:0] m_ctrlport_req_addr_aligned = m_ctrlport_req_addr[19:2];

  always @(posedge ctrlport_clk) begin
    // Default assignment
    m_ctrlport_resp_ack   <= 0;

    // Always clear these regs after being set
    reg_user_reset                     <= 1'b0;
    reg_cp_insertion_cp_len_fifo_load  <= 1'b0;
    reg_cp_insertion_cp_len_fifo_clr   <= 1'b0;
    reg_cp_removal_cp_len_fifo_load    <= 1'b0;
    reg_cp_removal_cp_len_fifo_clr     <= 1'b0;

    reg_fft_commit <= reg_fft_commit_next;

    // Read user registers
    if (m_ctrlport_req_rd) begin // Read request
      m_ctrlport_resp_ack  <= 1; // Always immediately ack
      m_ctrlport_resp_data <= 0; // Zero out by default
      case (m_ctrlport_req_addr_aligned)
        REG_USER_RESET_ADDR:                      m_ctrlport_resp_data <= reg_user_reset;
        REG_FFT_SIZE_LOG2_ADDR:                   m_ctrlport_resp_data <= reg_fft_size_log2;
        REG_FFT_SCALING_ADDR:                     m_ctrlport_resp_data <= reg_fft_scaling;
        REG_FFT_DIRECTION_ADDR:                   m_ctrlport_resp_data <= reg_fft_direction;
        REG_CP_INSERTION_CP_LEN_ADDR:             m_ctrlport_resp_data <= reg_cp_insertion_len;
        REG_CP_REMOVAL_CP_LEN_ADDR:               m_ctrlport_resp_data <= reg_cp_removal_len;
        REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR:   m_ctrlport_resp_data <= reg_cp_insertion_cp_len_fifo_load;
        REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR:    m_ctrlport_resp_data <= reg_cp_insertion_cp_len_fifo_clr;
        REG_CP_INSERTION_CP_LEN_FIFO_OCC_ADDR:    m_ctrlport_resp_data <= reg_cp_insertion_cp_len_fifo_occupied;
        REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR:     m_ctrlport_resp_data <= reg_cp_removal_cp_len_fifo_load;
        REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR:      m_ctrlport_resp_data <= reg_cp_removal_cp_len_fifo_clr;
        REG_CP_REMOVAL_CP_LEN_FIFO_OCC_ADDR:      m_ctrlport_resp_data <= reg_cp_removal_cp_len_fifo_occupied;
        default:                                  m_ctrlport_resp_data <= 32'h0BAD_C0DE;
      endcase
    end

    // Write user registers
    if (m_ctrlport_req_wr) begin // Write request
      m_ctrlport_resp_ack <= 1; // Always immediately ack
      case (m_ctrlport_req_addr_aligned)
        REG_USER_RESET_ADDR:                      reg_user_reset                    <= 1'b1; // Strobe, i.e. will be cleared on the next clock cycle
        REG_FFT_SIZE_LOG2_ADDR:                   reg_fft_size_log2                 <= m_ctrlport_req_data[REG_FFT_SIZE_LOG2_WIDTH-1:0];
        REG_FFT_SCALING_ADDR:                     reg_fft_scaling                   <= m_ctrlport_req_data[REG_FFT_SCALING_WIDTH-1:0];
        REG_FFT_DIRECTION_ADDR:                   reg_fft_direction                 <= m_ctrlport_req_data[REG_FFT_DIRECTION_WIDTH-1:0];
        REG_FFT_COMMIT_ADDR:                      reg_fft_commit                    <= 1'b1;
        REG_CP_INSERTION_CP_LEN_ADDR:             reg_cp_insertion_len              <= m_ctrlport_req_data[REG_CP_INSERTION_CP_LEN_WIDTH-1:0];
        REG_CP_REMOVAL_CP_LEN_ADDR:               reg_cp_removal_len                <= m_ctrlport_req_data[REG_CP_REMOVAL_CP_LEN_WIDTH-1:0];
        REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR:   reg_cp_insertion_cp_len_fifo_load <= 1'b1; // Strobe
        REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR:    reg_cp_insertion_cp_len_fifo_clr  <= 1'b1; // Strobe
        REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR:     reg_cp_removal_cp_len_fifo_load   <= 1'b1; // Strobe
        REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR:      reg_cp_removal_cp_len_fifo_clr    <= 1'b1; // Strobe
        REG_CP_INSERTION_CP_LEN_FIFO_OCC_ADDR:    ; // Read only
      endcase
    end
    if (ctrlport_rst) begin
      m_ctrlport_resp_ack               <= 1'b0;
      reg_user_reset                    <= 1'b0;
      reg_fft_size_log2                 <= DEFAULT_FFT_SIZE_LOG2;
      reg_fft_scaling                   <= DEFAULT_FFT_SCALING;
      reg_fft_direction                 <= DEFAULT_FFT_DIRECTION;
      reg_fft_commit                    <= 1'b0;
      reg_cp_insertion_len              <= DEFAULT_CP_LEN;
      reg_cp_removal_len                <= DEFAULT_CP_LEN;
      reg_cp_insertion_cp_len_fifo_load <= 1'b0;
      reg_cp_insertion_cp_len_fifo_clr  <= 1'b0;
      reg_cp_removal_cp_len_fifo_load   <= 1'b0;
      reg_cp_removal_cp_len_fifo_clr    <= 1'b0;
    end 
  end

  //---------------------------------------------------------------------------
  // Reset Logic
  //---------------------------------------------------------------------------
  localparam RESET_PULSE_LEN  = 8;
  localparam RESET_CNT_WIDTH  = $clog2(RESET_PULSE_LEN);

  localparam S_RESET_IDLE     = 1'd0;
  localparam S_RESET_ASSERT   = 1'd1;
  reg        reset_state      = S_RESET_ASSERT;

  reg  [RESET_CNT_WIDTH:0] user_reset_cnt   = 'd0;
  reg                      user_reset       = 1'b1;
  assign                   axis_data_rst    = user_reset | axis_data_rst_noc_shell;
  assign                   ctrlport_rst     = user_reset | ctrlport_rst_noc_shell;

  always @(posedge axis_data_clk) begin
    case (reset_state)
      S_RESET_IDLE : begin
        user_reset_cnt  <= 'd0;
        user_reset      <= 1'b0;
        if (reg_user_reset) begin
          user_reset    <= 1'b1;
          reset_state   <= S_RESET_ASSERT;
        end
      end
      S_RESET_ASSERT : begin
        user_reset_cnt  <= user_reset_cnt + 1;
        if (user_reset_cnt == RESET_PULSE_LEN-1) begin
          user_reset_cnt    <= 'd0;
          user_reset        <= 1'b0;
          reset_state       <= S_RESET_IDLE;
        end
      end
    endcase
    if (axis_data_rst_noc_shell) begin
      user_reset      <= 1'b1;
      user_reset_cnt  <= 'd0;
      reset_state     <= S_RESET_ASSERT;
    end
  end

  //---------------------------------------------------------------------------
  // Packetization
  //---------------------------------------------------------------------------
  wire [NUM_PORTS*32*1-1:0]   m_axis_user_tdata;
  wire [NUM_PORTS-1:0]        m_axis_user_teob;
  wire [NUM_PORTS-1:0]        m_axis_user_tlast;
  wire [NUM_PORTS-1:0]        m_axis_user_tvalid;
  wire [NUM_PORTS-1:0]        m_axis_user_tready;
  wire [NUM_PORTS*32*1-1:0]   s_axis_user_tdata;
  wire [NUM_PORTS-1:0]        s_axis_user_teob;
  wire [NUM_PORTS-1:0]        s_axis_user_teov;
  wire [NUM_PORTS-1:0]        s_axis_user_tlast;
  wire [NUM_PORTS-1:0]        s_axis_user_tvalid;
  wire [NUM_PORTS-1:0]        s_axis_user_tready;

  for (genvar i = 0; i < NUM_PORTS; i = i + 1) begin : gen_packetize
    axis_data_if_packetize #(
      .WIDTH                       (32),
      .CHDR_W                      (CHDR_W),
      .SIDEBAND_FWD_FIFO_SIZE_LOG2 (1)) // Increase if using many small bursts
    axis_data_if_packetize_inst (
      .clk                (axis_data_clk),
      .reset              (axis_data_rst),
      .spp                (12'd0), // Use the input packet size
      .s_axis_tdata       (m_in_axis_tdata[32*i +: 32]),
      .s_axis_tlast       (m_in_axis_tlast[i]),
      .s_axis_tkeep       (m_in_axis_tkeep[i]),
      .s_axis_tvalid      (m_in_axis_tvalid[i]),
      .s_axis_tready      (m_in_axis_tready[i]),
      .s_axis_ttimestamp  (m_in_axis_ttimestamp[64*i +: 64]),
      .s_axis_thas_time   (m_in_axis_thas_time[i]),
      .s_axis_tlength     (m_in_axis_tlength[16*i +: 16]),
      .s_axis_teov        (m_in_axis_teov[i]),
      .s_axis_teob        (m_in_axis_teob[i]),
      .m_axis_tdata       (s_out_axis_tdata[32*i +: 32]),
      .m_axis_tkeep       (s_out_axis_tkeep[i]),
      .m_axis_tlast       (s_out_axis_tlast[i]),
      .m_axis_tvalid      (s_out_axis_tvalid[i]),
      .m_axis_tready      (s_out_axis_tready[i]),
      .m_axis_ttimestamp  (s_out_axis_ttimestamp[64*i +: 64]),
      .m_axis_thas_time   (s_out_axis_thas_time[i]),
      .m_axis_tlength     (s_out_axis_tlength[16*i +: 16]),
      .m_axis_teov        (s_out_axis_teov[i]),
      .m_axis_teob        (s_out_axis_teob[i]),
      .m_axis_user_tdata  (m_axis_user_tdata[i*32 +: 32]),
      .m_axis_user_tkeep  (), // Unused
      .m_axis_user_teob   (m_axis_user_teob[i]),
      .m_axis_user_teov   (), // Unused
      .m_axis_user_tlast  (m_axis_user_tlast[i]),
      .m_axis_user_tvalid (m_axis_user_tvalid[i]),
      .m_axis_user_tready (m_axis_user_tready[i]),
      .s_axis_user_tdata  (s_axis_user_tdata[i*32 +: 32]),
      .s_axis_user_tkeep  (1'b1), // Keep all samples
      .s_axis_user_teob   (s_axis_user_teob[i]),
      .s_axis_user_teov   (s_axis_user_teov[i]),
      .s_axis_user_tlast  (s_axis_user_tlast[i]),
      .s_axis_user_tvalid (s_axis_user_tvalid[i]),
      .s_axis_user_tready (s_axis_user_tready[i])
    );
  end

  // Combine input streams into one. This lets us run the FFT instances (one per channel)
  // in lock-step by sharing valid/ready signals. Removes the need for multiple instances of
  // FFT configuration logic as well.
  wire [NUM_PORTS*32-1:0]    m_axis_combine_tdata;
  wire [NUM_PORTS-1:0]       m_axis_combine_teob;
  wire                      m_axis_combine_tlast;
  wire                      m_axis_combine_tvalid;
  wire                      m_axis_combine_tready;

  axis_combine #(
    .SIZE           (NUM_PORTS),
    .WIDTH          (32),
    .USER_WIDTH     (1),
    .FIFO_SIZE_LOG2 (0))
  axis_combine_inst (
    .clk            (axis_data_clk),
    .reset          (axis_data_rst),
    .s_axis_tdata   (m_axis_user_tdata),
    .s_axis_tuser   (m_axis_user_teob),
    .s_axis_tlast   (m_axis_user_tlast),
    .s_axis_tvalid  (m_axis_user_tvalid),
    .s_axis_tready  (m_axis_user_tready),
    .m_axis_tdata   (m_axis_combine_tdata),
    .m_axis_tuser   (m_axis_combine_teob),
    .m_axis_tlast   (m_axis_combine_tlast),
    .m_axis_tvalid  (m_axis_combine_tvalid),
    .m_axis_tready  (m_axis_combine_tready)
  );

  //---------------------------------------------------------------------------
  // FFT
  //---------------------------------------------------------------------------

  wire [NUM_PORTS*32-1:0] m_axis_cp_removal_tdata;
  wire [NUM_PORTS-1:0]    m_axis_cp_removal_teob;
  wire [NUM_PORTS-1:0]    m_axis_cp_removal_teov;
  wire                    m_axis_cp_removal_tlast;
  wire                    m_axis_cp_removal_tvalid;
  wire                    m_axis_cp_removal_tready;
  wire [FFT_CONFIG_W-1:0] s_axis_fft_config_tdata;
  wire                    s_axis_fft_config_tvalid;
  wire [NUM_PORTS-1:0]    s_axis_fft_config_tready;
  wire [NUM_PORTS*32-1:0] s_axis_fft_data_tdata;
  wire                    s_axis_fft_data_tlast;
  wire                    s_axis_fft_data_tvalid;
  wire [NUM_PORTS-1:0]    s_axis_fft_data_tready;
  wire [NUM_PORTS*32-1:0] m_axis_fft_data_tdata;
  wire [NUM_PORTS-1:0]    m_axis_fft_data_tlast;
  wire [NUM_PORTS-1:0]    m_axis_fft_data_tvalid;
  wire                    m_axis_fft_data_tready;
  wire [NUM_PORTS-1:0]    event_frame_started;
  wire [NUM_PORTS-1:0]    event_fft_overflow;
  wire [NUM_PORTS-1:0]    event_frame_tlast_unexpected;
  wire [NUM_PORTS-1:0]    event_frame_tlast_missing;
  wire [NUM_PORTS*32-1:0] s_axis_split_bus_tdata;
  wire [NUM_PORTS-1:0]    s_axis_split_bus_teob;
  wire [NUM_PORTS-1:0]    s_axis_split_bus_teov;
  wire                    s_axis_split_bus_tlast;
  wire                    s_axis_split_bus_tvalid;
  wire                    s_axis_split_bus_tready;

  // Change packet length to FFT size
  wire [FFT_SIZE_WIDTH-1:0] fft_size = 1 << reg_fft_size_log2[$clog2(FFT_SIZE_WIDTH)-1:0];

  // Clear the commit flag after the FFT has been configured
  assign reg_fft_commit_next = (s_axis_fft_config_tvalid && s_axis_fft_config_tready) ?
    1'b0 : reg_fft_commit;

  integer dbg_removal_in_counter = 0, dbg_removal_out_counter = 0;
  integer dbg_removal_in_length, dbg_removal_out_length;
  reg dbg_removal_in_eop, dbg_removal_out_eop;
  always @(posedge axis_data_clk) begin
    if (m_axis_combine_tvalid && m_axis_combine_tready) begin
      if (m_axis_combine_tlast && m_axis_combine_teob) begin
        dbg_removal_in_counter <= 0;
        dbg_removal_in_eop <= 1;
        dbg_removal_in_length <= dbg_removal_in_counter+1;
      end else begin
        dbg_removal_in_eop <= 0;
        dbg_removal_in_counter <= dbg_removal_in_counter + 1;
      end
    end
    if (m_axis_cp_removal_tvalid && m_axis_cp_removal_tready) begin
      if (m_axis_cp_removal_tlast) begin
        dbg_removal_out_counter <= 0;
        dbg_removal_out_eop <= 1;
        dbg_removal_out_length <= dbg_removal_out_counter+1;
      end else begin
        dbg_removal_out_eop <= 0;
        dbg_removal_out_counter <= dbg_removal_out_counter + 1;
      end
    end
  end

  // Remove cyclic prefix
  // Also sets the packet size (i.e. tlast) to the FFT size
  axis_ofdm_cyclic_prefix_removal #(
    .WIDTH                 (NUM_PORTS*32),
    .USER_WIDTH            (NUM_PORTS),
    .MAX_CP_LEN_LOG2       (CP_LEN_WIDTH),
    .MAX_FFT_SIZE_LOG2     (FFT_SIZE_WIDTH),
    .DEFAULT_CP_LEN        (DEFAULT_CP_LEN),
    .CP_LEN_FIFO_MODE      (CP_REMOVAL_REPEAT),
    .CP_LEN_FIFO_SIZE_LOG2 (MAX_CP_LIST_LEN_REM_LOG2),
    .SET_TLAST             (1))
  axis_ofdm_cyclic_prefix_removal_inst (
    .clk                  (axis_data_clk),
    .reset                (axis_data_rst),
    .clear_fifo           (reg_cp_removal_cp_len_fifo_clr),
    .fft_size             (fft_size),
    .m_axis_cp_len_tdata  (reg_cp_removal_len),
    .m_axis_cp_len_tvalid (reg_cp_removal_cp_len_fifo_load),
    // No backpressure needed as the block controller checks "cp_len_fifo_occupied"
    // before writing to avoid overflowing FIFO
    .m_axis_cp_len_tready (),
    .cp_len_fifo_occupied (reg_cp_removal_cp_len_fifo_occupied),
    .s_axis_data_tdata    (m_axis_combine_tdata),
    .s_axis_data_tuser    (m_axis_combine_teob),
    .s_axis_data_tlast    (m_axis_combine_tlast),
    .s_axis_data_tvalid   (m_axis_combine_tvalid),
    .s_axis_data_tready   (m_axis_combine_tready),
    .m_axis_data_tdata    (m_axis_cp_removal_tdata),
    .m_axis_data_tuser    (m_axis_cp_removal_teob),
    .m_axis_data_tlast    (m_axis_cp_removal_tlast),
    .m_axis_data_tvalid   (m_axis_cp_removal_tvalid),
    .m_axis_data_tready   (m_axis_cp_removal_tready)
  );
  // We can create a teov from tlast because the packet size is the same as the FFT size
  assign m_axis_cp_removal_teov = {(NUM_PORTS){m_axis_cp_removal_tlast}};

  // Below is a FIFO for a configurable cyclic prefix length list that cycles until
  // the FIFO is cleared. This allows the cyclic prefix length to dynamically change
  // at runtime on a FFT by FFT basis up to the depth of the FIFO.
  // For example:
  // - User writes 32, 64, 128 into the FIFO
  // - FFT cyclic prefix length will be configured in the following order:
  //   32, 64, 128, 32, 64, 128, etc until FIFO is cleared
  // Example #2:
  // - User writes 32 into FIFO
  // - FFT cyclic prefix length will be 32 until FIFO is cleared
  //
  // If the FIFO does not have any entries, cylic prefix length defaults to
  // CP_LEN_DEFAULT (see FFT configuration state machine below).
  // User should generally not load or clear the FIFO while streaming, but the logic
  // does not prevent it.
  //
  wire [CP_LEN_WIDTH-1:0] s_axis_cp_len_fifo_tdata;
  wire                    s_axis_cp_len_fifo_tvalid;
  wire [CP_LEN_WIDTH-1:0] m_axis_cp_len_fifo_tdata;
  wire                    m_axis_cp_len_fifo_tvalid;
  wire                    m_axis_cp_len_fifo_tready;
  wire                    cp_len_fifo_empty = (reg_cp_insertion_cp_len_fifo_occupied == 0);

  axi_fifo #(
    .WIDTH    (CP_LEN_WIDTH),
    .SIZE     (MAX_CP_LIST_LEN_INS_LOG2))
  axi_fifo_cp_insertion_cp_len_inst (
    .clk      (axis_data_clk),
    .reset    (axis_data_rst),
    .clear    (reg_cp_insertion_cp_len_fifo_clr),
    .i_tdata  (s_axis_cp_len_fifo_tdata),
    .i_tvalid (s_axis_cp_len_fifo_tvalid),
    .i_tready (/* No backpressure needed as block controller checks cp_len_fifo_occupied to not overflow FIFO */),
    .o_tdata  (m_axis_cp_len_fifo_tdata),
    .o_tvalid (m_axis_cp_len_fifo_tvalid), // Unused
    .o_tready (m_axis_cp_len_fifo_tready),
    .space    (),
    .occupied (reg_cp_insertion_cp_len_fifo_occupied)
  );

  // FFT IP configuration state machine
  // Loads a new configuration at the start of every FFT
  localparam S_FFT_CONFIG         = 1'd0;
  localparam S_FFT_WAIT_FOR_TLAST = 1'd1;
  reg        fft_config_state     = S_FFT_CONFIG;

  always @(posedge axis_data_clk) begin
    case (fft_config_state)
      S_FFT_CONFIG: begin
        if (s_axis_fft_data_tvalid & s_axis_fft_data_tready[0]) begin
          fft_config_state <= S_FFT_WAIT_FOR_TLAST;
        end
      end
      S_FFT_WAIT_FOR_TLAST: begin
        if (s_axis_fft_data_tvalid & s_axis_fft_data_tready[0] & s_axis_fft_data_tlast) begin
          fft_config_state <= S_FFT_CONFIG;
        end
      end
    endcase
    if (axis_data_rst) begin
      fft_config_state <= S_FFT_CONFIG;
    end
  end

  assign s_axis_fft_config_tdata = {
    {FFT_SCALE_PAD_W{1'b0}},
    reg_fft_scaling,
    reg_fft_direction,
    {FFT_CP_LEN_PAD_W{1'b0}},
    FFT_CP_LEN_W'(cp_len_fifo_empty ? DEFAULT_CP_LEN : m_axis_cp_len_fifo_tdata),
    {FFT_NFFT_PAD_W{1'b0}},
    reg_fft_size_log2
  };

  //assign s_axis_fft_config_tdata[47:41] = '0;
  //assign s_axis_fft_config_tdata[40:25] = reg_fft_scaling;
  //assign s_axis_fft_config_tdata[24] = reg_fft_direction;
  //assign s_axis_fft_config_tdata[23] = '0;
  //assign s_axis_fft_config_tdata[22:8] = FFT_CP_LEN_W'(cp_len_fifo_empty ? DEFAULT_CP_LEN : m_axis_cp_len_fifo_tdata);
  //assign s_axis_fft_config_tdata[7:5] = '0;
  //assign s_axis_fft_config_tdata[4:0] = reg_fft_size_log2;


  //assign s_axis_fft_config_tvalid = reg_fft_commit;
  assign s_axis_fft_config_tvalid = (fft_config_state == S_FFT_CONFIG) ? s_axis_fft_data_tvalid : 1'b0;

  // No cyclic prefix fifo loopback. New configs can be written at any time.
  if (CP_INSERTION_REPEAT == 0) begin : gen_cp_ins_repeat
    assign s_axis_cp_len_fifo_tdata  = reg_cp_insertion_len;
    assign s_axis_cp_len_fifo_tvalid = reg_cp_insertion_cp_len_fifo_load;
    //assign m_axis_cp_len_fifo_tready = s_axis_fft_config_tvalid & s_axis_fft_config_tready[0];
    assign m_axis_cp_len_fifo_tready = (fft_config_state == S_FFT_CONFIG) & s_axis_fft_config_tvalid & s_axis_fft_config_tready[0];
  // Cyclic prefix fifo fifo loopback enabled, writes current cp length back into fifo in the S_FFT_CONFIG_LOAD state.
  // New cp lengths can only be written in the S_FFT_CONFIG_IDLE state.
  end else begin : gen_cp_ins_no_repeat
    assign s_axis_cp_len_fifo_tdata  = reg_cp_insertion_cp_len_fifo_load ? reg_cp_insertion_len : m_axis_cp_len_fifo_tdata;
    assign s_axis_cp_len_fifo_tvalid =
      (m_axis_cp_len_fifo_tvalid & (s_axis_fft_config_tvalid & s_axis_fft_config_tready[0])) |
      reg_cp_insertion_cp_len_fifo_load;
    assign m_axis_cp_len_fifo_tready = s_axis_fft_config_tvalid & s_axis_fft_config_tready[0];
  end

  // Xilinx FFT IP lacks a pass-through tuser signal so this adds one for our EOB signal
  axis_sideband_tuser #(
    .WIDTH                 (NUM_PORTS*32),
    .USER_WIDTH            (2*NUM_PORTS),
    .FIFO_SIZE_LOG2        (5),
    .PACKET_MODE           (2)) // tuser is EOB and static for the entire packet,
                                // so we can use this more effecient packet mode
  axis_sideband_xfft_tuser (
    .clk                   (axis_data_clk),
    .reset                 (axis_data_rst),
    // Input bus with a tuser signal
    .s_axis_tdata          (m_axis_cp_removal_tdata),
    .s_axis_tuser          ({m_axis_cp_removal_teob, m_axis_cp_removal_teov}),
    .s_axis_tlast          (m_axis_cp_removal_tlast),
    .s_axis_tvalid         (m_axis_cp_removal_tvalid),
    .s_axis_tready         (m_axis_cp_removal_tready),
    // To/from our module, i.e. Xilinx FFT, but without the tuser signal.
    // This modules keeps track of the tuser signal values internally
    // and will automatically add them back to the output stream.
    .m_axis_mod_tdata      (s_axis_fft_data_tdata),
    .m_axis_mod_tlast      (s_axis_fft_data_tlast),
    .m_axis_mod_tvalid     (s_axis_fft_data_tvalid),
    .m_axis_mod_tready     (s_axis_fft_data_tready[0]),
    .s_axis_mod_tdata      (m_axis_fft_data_tdata),
    .s_axis_mod_tlast      (m_axis_fft_data_tlast[0]),
    .s_axis_mod_tvalid     (m_axis_fft_data_tvalid[0]),
    .s_axis_mod_tready     (m_axis_fft_data_tready),
    // Output bus, i.e. Xilinx FFT output stream, but with the tuser from the input added on
    .m_axis_tdata          (s_axis_split_bus_tdata),
    .m_axis_tuser          ({s_axis_split_bus_teob, s_axis_split_bus_teov}),
    .m_axis_tlast          (s_axis_split_bus_tlast),
    .m_axis_tvalid         (s_axis_split_bus_tvalid),
    .m_axis_tready         (s_axis_split_bus_tready)
  );

  integer dbg_fft_in_counter = 0, dbg_fft_out_counter = 0;
  integer dbg_fft_in_length, dbg_fft_out_length;
  reg dbg_fft_in_eop, dbg_fft_out_eop;
  genvar i;
  for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_fft

    always @(posedge axis_data_clk) begin
      if (s_axis_fft_data_tvalid && s_axis_fft_data_tready) begin
        if (s_axis_fft_data_tlast) begin
          dbg_fft_in_counter <= 0;
          dbg_fft_in_eop <= 1;
          dbg_fft_in_length <= dbg_fft_in_counter+1;
        end else begin
          dbg_fft_in_eop <= 0;
          dbg_fft_in_counter <= dbg_fft_in_counter + 1;
        end
      end

      if (m_axis_fft_data_tvalid[i] && m_axis_fft_data_tready) begin
        if (m_axis_fft_data_tlast[i]) begin
          dbg_fft_out_counter <= 0;
          dbg_fft_out_eop <= 1;
          dbg_fft_out_length <= dbg_fft_out_counter+1;
        end else begin
          dbg_fft_out_eop <= 0;
          dbg_fft_out_counter <= dbg_fft_out_counter + 1;
        end
      end
    end

    if (MAX_FFT_SIZE == 4096) begin : gen_4k_FFT
      xfft_4k_16b xfft_4k_16b_i (
        .aclk                        (axis_data_clk),
        .aresetn                     (~axis_data_rst),
        .s_axis_config_tdata         (s_axis_fft_config_tdata),
        .s_axis_config_tvalid        (s_axis_fft_config_tvalid),
        .s_axis_config_tready        (s_axis_fft_config_tready[i]),
        .s_axis_data_tdata           ({ s_axis_fft_data_tdata[32*i +: 16], s_axis_fft_data_tdata[32*i+16 +: 16] }),
        .s_axis_data_tlast           (s_axis_fft_data_tlast),
        .s_axis_data_tvalid          (s_axis_fft_data_tvalid),
        .s_axis_data_tready          (s_axis_fft_data_tready[i]),
        .m_axis_data_tdata           ({ m_axis_fft_data_tdata[32*i +: 16], m_axis_fft_data_tdata[32*i+16 +: 16] }),
        .m_axis_data_tuser           (),
        .m_axis_data_tlast           (m_axis_fft_data_tlast[i]),
        .m_axis_data_tvalid          (m_axis_fft_data_tvalid[i]),
        .m_axis_data_tready          (m_axis_fft_data_tready),
        .m_axis_status_tdata         (),
        .m_axis_status_tvalid        (),
        .m_axis_status_tready        (1'b1),
        .event_frame_started         (event_frame_started[i]),
        .event_tlast_unexpected      (event_frame_tlast_unexpected[i]),
        .event_tlast_missing         (event_frame_tlast_missing[i]),
        .event_fft_overflow          (event_fft_overflow[i]),
        .event_status_channel_halt   (),
        .event_data_in_channel_halt  (),
        .event_data_out_channel_halt ()
      );
    end else if (MAX_FFT_SIZE == 8192) begin : gen_8k_fft
      xfft_8k_16b xfft_8k_16b_i (
        .aclk                        (axis_data_clk),
        .aresetn                     (~axis_data_rst),
        .s_axis_config_tdata         (s_axis_fft_config_tdata),
        .s_axis_config_tvalid        (s_axis_fft_config_tvalid),
        .s_axis_config_tready        (s_axis_fft_config_tready[i]),
        .s_axis_data_tdata           ({ s_axis_fft_data_tdata[32*i +: 16], s_axis_fft_data_tdata[32*i+16 +: 16] }),
        .s_axis_data_tlast           (s_axis_fft_data_tlast),
        .s_axis_data_tvalid          (s_axis_fft_data_tvalid),
        .s_axis_data_tready          (s_axis_fft_data_tready[i]),
        .m_axis_data_tdata           ({ m_axis_fft_data_tdata[32*i +: 16], m_axis_fft_data_tdata[32*i+16 +: 16] }),
        .m_axis_data_tuser           (),
        .m_axis_data_tlast           (m_axis_fft_data_tlast[i]),
        .m_axis_data_tvalid          (m_axis_fft_data_tvalid[i]),
        .m_axis_data_tready          (m_axis_fft_data_tready),
        .m_axis_status_tdata         (),
        .m_axis_status_tvalid        (),
        .m_axis_status_tready        (1'b1),
        .event_frame_started         (event_frame_started[i]),
        .event_tlast_unexpected      (event_frame_tlast_unexpected[i]),
        .event_tlast_missing         (event_frame_tlast_missing[i]),
        .event_fft_overflow          (event_fft_overflow[i]),
        .event_status_channel_halt   (),
        .event_data_in_channel_halt  (),
        .event_data_out_channel_halt ()
      );
    end else if (MAX_FFT_SIZE == 32768) begin : gen_32k_fft
      xfft_32k_16b xfft_32k_16b_i (
        .aclk                        (axis_data_clk),
        .aresetn                     (~axis_data_rst),
        .s_axis_config_tdata         (s_axis_fft_config_tdata),
        .s_axis_config_tvalid        (s_axis_fft_config_tvalid),
        .s_axis_config_tready        (s_axis_fft_config_tready[i]),
        .s_axis_data_tdata           ({ s_axis_fft_data_tdata[32*i +: 16], s_axis_fft_data_tdata[32*i+16 +: 16] }),
        .s_axis_data_tlast           (s_axis_fft_data_tlast),
        .s_axis_data_tvalid          (s_axis_fft_data_tvalid),
        .s_axis_data_tready          (s_axis_fft_data_tready[i]),
        .m_axis_data_tdata           ({ m_axis_fft_data_tdata[32*i +: 16], m_axis_fft_data_tdata[32*i+16 +: 16] }),
        .m_axis_data_tuser           (),
        .m_axis_data_tlast           (m_axis_fft_data_tlast[i]),
        .m_axis_data_tvalid          (m_axis_fft_data_tvalid[i]),
        .m_axis_data_tready          (m_axis_fft_data_tready),
        .m_axis_status_tdata         (),
        .m_axis_status_tvalid        (),
        .m_axis_status_tready        (1'b1),
        .event_frame_started         (event_frame_started[i]),
        .event_tlast_unexpected      (event_frame_tlast_unexpected[i]),
        .event_tlast_missing         (event_frame_tlast_missing[i]),
        .event_fft_overflow          (event_fft_overflow[i]),
        .event_status_channel_halt   (),
        .event_data_in_channel_halt  (),
        .event_data_out_channel_halt ()
      );
    end
  end

  // Split back into multiple streams
  axis_split_bus #(
    .WIDTH          (32),
    .USER_WIDTH     (2),
    .NUM_PORTS      (NUM_PORTS))
  axis_split_bus_inst (
    .clk            (axis_data_clk),
    .reset          (axis_data_rst),
    .s_axis_tdata   (s_axis_split_bus_tdata),
    .s_axis_tuser   ({s_axis_split_bus_teob, s_axis_split_bus_teov}),
    .s_axis_tlast   (s_axis_split_bus_tlast),
    .s_axis_tvalid  (s_axis_split_bus_tvalid),
    .s_axis_tready  (s_axis_split_bus_tready),
    .m_axis_tdata   (s_axis_user_tdata),
    .m_axis_tuser   ({s_axis_user_teob, s_axis_user_teov}),
    .m_axis_tlast   (s_axis_user_tlast),
    .m_axis_tvalid  (s_axis_user_tvalid),
    .m_axis_tready  (s_axis_user_tready)
  );

/*
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS*32*1-1:0]   m_in_axis_tdata_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_in_axis_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_in_axis_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_in_axis_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS*32*1-1:0]   s_out_axis_tdata_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_out_axis_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_out_axis_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_out_axis_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [32-1:0]              m_axis_combine_tdata_debug[0:NUM_PORTS-1];
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_axis_combine_teob_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_combine_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_combine_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_combine_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [32-1:0]              m_axis_cp_removal_tdata_debug[0:NUM_PORTS-1];
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_axis_cp_removal_teob_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_cp_removal_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_cp_removal_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_cp_removal_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [32-1:0]              s_axis_fft_data_tdata_debug[0:NUM_PORTS-1];
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_fft_data_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_fft_data_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_axis_fft_data_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [32-1:0]              m_axis_fft_data_tdata_debug[0:NUM_PORTS-1];
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_axis_fft_data_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        m_axis_fft_data_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_fft_data_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        event_frame_started_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        event_fft_overflow_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        event_frame_tlast_unexpected_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        event_frame_tlast_missing_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [32-1:0]              s_axis_split_bus_tdata_debug[0:NUM_PORTS-1];
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_axis_split_bus_teob_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_split_bus_tlast_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_split_bus_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_split_bus_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [CP_LEN_WIDTH-1:0]    m_axis_cp_len_fifo_tdata_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_cp_len_fifo_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       m_axis_cp_len_fifo_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [FFT_SIZE_WIDTH-1:0]  fft_size_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [39:0]                s_axis_fft_config_tdata_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       s_axis_fft_config_tvalid_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg [NUM_PORTS-1:0]        s_axis_fft_config_tready_debug;
  (* dont_touch = "true", mark_debug = "true"*) reg                       fft_config_state_debug;

  always @(posedge axis_data_clk) begin
    m_in_axis_tdata_debug                                               <= m_in_axis_tdata;
    m_in_axis_tlast_debug                                               <= m_in_axis_tlast;
    m_in_axis_tvalid_debug                                              <= m_in_axis_tvalid;
    m_in_axis_tready_debug                                              <= m_in_axis_tready;
    s_out_axis_tdata_debug                                              <= s_out_axis_tdata;
    s_out_axis_tlast_debug                                              <= s_out_axis_tlast;
    s_out_axis_tvalid_debug                                             <= s_out_axis_tvalid;
    s_out_axis_tready_debug                                             <= s_out_axis_tready;
    {m_axis_combine_tdata_debug[1],m_axis_combine_tdata_debug[0]}       <= m_axis_combine_tdata;
    m_axis_combine_teob_debug                                           <= m_axis_combine_teob;
    m_axis_combine_tlast_debug                                          <= m_axis_combine_tlast;
    m_axis_combine_tvalid_debug                                         <= m_axis_combine_tvalid;
    m_axis_combine_tready_debug                                         <= m_axis_combine_tready;
    {m_axis_cp_removal_tdata_debug[1],m_axis_cp_removal_tdata_debug[0]} <= m_axis_cp_removal_tdata;
    m_axis_cp_removal_teob_debug                                        <= m_axis_cp_removal_teob;
    m_axis_cp_removal_tlast_debug                                       <= m_axis_cp_removal_tlast;
    m_axis_cp_removal_tvalid_debug                                      <= m_axis_cp_removal_tvalid;
    m_axis_cp_removal_tready_debug                                      <= m_axis_cp_removal_tready;
    {s_axis_fft_data_tdata_debug[1],s_axis_fft_data_tdata_debug[0]}     <= s_axis_fft_data_tdata;
    s_axis_fft_data_tlast_debug                                         <= s_axis_fft_data_tlast;
    s_axis_fft_data_tvalid_debug                                        <= s_axis_fft_data_tvalid;
    s_axis_fft_data_tready_debug                                        <= s_axis_fft_data_tready;
    {m_axis_fft_data_tdata_debug[1],m_axis_fft_data_tdata_debug[0]}     <= m_axis_fft_data_tdata;
    m_axis_fft_data_tlast_debug                                         <= m_axis_fft_data_tlast;
    m_axis_fft_data_tvalid_debug                                        <= m_axis_fft_data_tvalid;
    m_axis_fft_data_tready_debug                                        <= m_axis_fft_data_tready;
    event_frame_started_debug                                           <= event_frame_started;
    event_fft_overflow_debug                                            <= event_fft_overflow;
    event_frame_tlast_unexpected_debug                                  <= event_frame_tlast_unexpected;
    event_frame_tlast_missing_debug                                     <= event_frame_tlast_missing;
    {s_axis_split_bus_tdata_debug[1],s_axis_split_bus_tdata_debug[0]}   <= s_axis_split_bus_tdata;
    s_axis_split_bus_teob_debug                                         <= s_axis_split_bus_teob;
    s_axis_split_bus_tlast_debug                                        <= s_axis_split_bus_tlast;
    s_axis_split_bus_tvalid_debug                                       <= s_axis_split_bus_tvalid;
    s_axis_split_bus_tready_debug                                       <= s_axis_split_bus_tready;
    m_axis_cp_len_fifo_tdata_debug                                      <= m_axis_cp_len_fifo_tdata;
    m_axis_cp_len_fifo_tvalid_debug                                     <= m_axis_cp_len_fifo_tvalid;
    m_axis_cp_len_fifo_tready_debug                                     <= m_axis_cp_len_fifo_tready;
    fft_size_debug                                                      <= fft_size;
    s_axis_fft_config_tdata_debug                                       <= s_axis_fft_config_tdata;
    s_axis_fft_config_tvalid_debug                                      <= s_axis_fft_config_tvalid;
    s_axis_fft_config_tready_debug                                      <= s_axis_fft_config_tready;
    fft_config_state_debug                                              <= fft_config_state;
  end
*/
endmodule : rfnoc_block_ofdm

`default_nettype wire
