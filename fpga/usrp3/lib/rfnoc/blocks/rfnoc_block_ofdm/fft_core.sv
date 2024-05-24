//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_core
//
// Description:
//
//   FFT/IFFT plus cyclic prefix insertion/removal.
//
// Parameters:
//
//   CHDR_W                   : RFNoC block CHDR width
//   NUM_CHAN                 : Number of channels
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size
//   MAX_CP_LEN_LOG2          : Log2 of maximum cyclic prefix length
//   MAX_CP_LIST_LEN_INS_LOG2 : Log2 of max length of cyclic prefix insertion list
//   MAX_CP_LIST_LEN_REM_LOG2 : Log2 of max length of cyclic prefix removal list
//   CP_INSERTION_REPEAT      : Enable (1) or disable (0) CP insertion list FIFO loopback
//   CP_REMOVAL_REPEAT        : Enable (1) or disable (0) CP removal list FIFO loopback
//

`default_nettype none

module fft_core #(
  int CHDR_W                   = 64,
  int NUM_CHAN                 = 2,
  int MAX_FFT_SIZE_LOG2        = 12,
  int MAX_CP_LEN_LOG2          = 12,
  int MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int MAX_CP_LIST_LEN_REM_LOG2 = 5,
  int CP_INSERTION_REPEAT      = 1,
  int CP_REMOVAL_REPEAT        = 1
) (
  input  wire                      ce_clk,
  input  wire                      ce_rst,

  // CtrlPort Register Interface
  input  wire                      m_ctrlport_req_wr,
  input  wire                      m_ctrlport_req_rd,
  input  wire  [             19:0] m_ctrlport_req_addr,
  input  wire  [             31:0] m_ctrlport_req_data,
  output logic                     m_ctrlport_resp_ack,
  output logic [             31:0] m_ctrlport_resp_data,

  // Data Input Packets
  input  wire  [NUM_CHAN*32*1-1:0] m_in_axis_tdata,
  input  wire  [   NUM_CHAN*1-1:0] m_in_axis_tkeep,
  input  wire  [     NUM_CHAN-1:0] m_in_axis_tlast,
  input  wire  [     NUM_CHAN-1:0] m_in_axis_tvalid,
  output logic [     NUM_CHAN-1:0] m_in_axis_tready,
  input  wire  [  NUM_CHAN*64-1:0] m_in_axis_ttimestamp,
  input  wire  [     NUM_CHAN-1:0] m_in_axis_thas_time,
  input  wire  [  NUM_CHAN*16-1:0] m_in_axis_tlength,
  input  wire  [     NUM_CHAN-1:0] m_in_axis_teov,
  input  wire  [     NUM_CHAN-1:0] m_in_axis_teob,

  // Data Output Packets
  output wire [NUM_CHAN*32*1-1:0] s_out_axis_tdata,
  output wire [   NUM_CHAN*1-1:0] s_out_axis_tkeep,
  output wire [     NUM_CHAN-1:0] s_out_axis_tlast,
  output wire [     NUM_CHAN-1:0] s_out_axis_tvalid,
  input  wire [     NUM_CHAN-1:0] s_out_axis_tready,
  output wire [  NUM_CHAN*64-1:0] s_out_axis_ttimestamp,
  output wire [     NUM_CHAN-1:0] s_out_axis_thas_time,
  output wire [  NUM_CHAN*16-1:0] s_out_axis_tlength,
  output wire [     NUM_CHAN-1:0] s_out_axis_teov,
  output wire [     NUM_CHAN-1:0] s_out_axis_teob
);
  // Import utilities for working with Xilinx FFT block
  import xfft_config_pkg::*;

  // Import register descriptions
  import fft_core_regs_pkg::*;


  //---------------------------------------------------------------------------
  // FFT Configuration Interface Constants
  //---------------------------------------------------------------------------

  localparam int MAX_FFT_SIZE = 2**MAX_FFT_SIZE_LOG2;

  localparam int FFT_SCALE_W = fft_scale_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_FWD_INV_W = fft_fwd_inv_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_CP_LEN_W = fft_cp_len_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_NFFT_W = fft_nfft_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_CONFIG_W = fft_config_w(MAX_FFT_SIZE_LOG2);

  // Use conservative 1/N scaling by default
  localparam [FFT_SCALE_W-1:0] DEFAULT_FFT_SCALING = fft_scale_default(MAX_FFT_SIZE_LOG2);

  //synthesis translate_off
  initial begin
    $display("FFT_SCALE_W   : %0d", FFT_SCALE_W);
    $display("FFT_FWD_INV_W : %0d", FFT_FWD_INV_W);
    $display("FFT_CP_LEN_W  : %0d", FFT_CP_LEN_W);
    $display("FFT_NFFT_W    : %0d", FFT_NFFT_W);
    $display("");
    $display("FFT_CONFIG_W  : %0d", FFT_CONFIG_W);
    $display("");
    print_fft_config_fields(MAX_FFT_SIZE_LOG2);
  end
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  // FFT Direction
  localparam bit FFT_REVERSE = 0;
  localparam bit FFT_FORWARD = 1;

  localparam FFT_SIZE_W = MAX_FFT_SIZE_LOG2+1;
  localparam CP_LEN_W   = MAX_CP_LEN_LOG2+1;

  localparam int REG_FFT_SIZE_LOG2_WIDTH = FFT_NFFT_W;
  localparam int REG_FFT_SCALING_WIDTH   = FFT_SCALE_W;
  localparam int REG_CP_INS_LEN_WIDTH    = CP_LEN_W;
  localparam int REG_CP_REM_LEN_WIDTH    = CP_LEN_W;

  localparam bit [REG_COMPAT_WIDTH-1:0] COMPAT  = {16'h01, 16'h00};
  localparam bit [REG_CAPABILITIES_WIDTH-1:0] CAPABILITIES = {
    8'(MAX_CP_LIST_LEN_INS_LOG2),
    8'(MAX_CP_LIST_LEN_REM_LOG2),
    8'(MAX_CP_LEN_LOG2),
    8'(MAX_FFT_SIZE_LOG2)
  };

  localparam int DEFAULT_FFT_SIZE_LOG2 = MAX_FFT_SIZE_LOG2;
  localparam int DEFAULT_FFT_DIRECTION = FFT_REVERSE;
  localparam int DEFAULT_CP_LEN        = 0;

  logic core_rst;

  logic [      REG_USER_RESET_WIDTH-1:0] reg_user_reset                    = 1'b0;
  logic [   REG_FFT_SIZE_LOG2_WIDTH-1:0] reg_fft_size_log2                 = DEFAULT_FFT_SIZE_LOG2;
  logic [     REG_FFT_SCALING_WIDTH-1:0] reg_fft_scaling                   = DEFAULT_FFT_SCALING;
  logic [              FFT_CP_LEN_W-1:0] reg_cp_length                     = DEFAULT_CP_LEN;
  logic [   REG_FFT_DIRECTION_WIDTH-1:0] reg_fft_direction                 = DEFAULT_FFT_DIRECTION;
  logic [      REG_CP_INS_LEN_WIDTH-1:0] reg_cp_insertion_len              = DEFAULT_CP_LEN;
  logic [      REG_CP_REM_LEN_WIDTH-1:0] reg_cp_removal_len                = DEFAULT_CP_LEN;
  logic [REG_CP_INS_LIST_LOAD_WIDTH-1:0] reg_cp_insertion_cp_len_fifo_load = 1'b0;
  logic [ REG_CP_INS_LIST_CLR_WIDTH-1:0] reg_cp_insertion_cp_len_fifo_clr  = 1'b0;
  logic [REG_CP_REM_LIST_LOAD_WIDTH-1:0] reg_cp_removal_cp_len_fifo_load   = 1'b0;
  logic [ REG_CP_REM_LIST_CLR_WIDTH-1:0] reg_cp_removal_cp_len_fifo_clr    = 1'b0;
  logic [          REG_BYPASS_WIDTH-1:0] reg_bypass                        = 1'b0;

  logic [ REG_CP_INS_LIST_OCC_WIDTH-1:0] reg_cp_insertion_cp_len_fifo_occupied;
  logic [ REG_CP_REM_LIST_OCC_WIDTH-1:0] reg_cp_removal_cp_len_fifo_occupied;

  wire [REG_ADDR_W-1:0] m_ctrlport_req_addr_aligned = REG_ADDR_W'(m_ctrlport_req_addr);

  always_ff @(posedge ce_clk) begin
    // Default assignment
    m_ctrlport_resp_ack <= 0;

    // Always clear these regs after being set
    reg_user_reset                     <= 1'b0;
    reg_cp_insertion_cp_len_fifo_load  <= 1'b0;
    reg_cp_insertion_cp_len_fifo_clr   <= 1'b0;
    reg_cp_removal_cp_len_fifo_load    <= 1'b0;
    reg_cp_removal_cp_len_fifo_clr     <= 1'b0;

    // Read user registers
    if (m_ctrlport_req_rd) begin // Read request
      m_ctrlport_resp_ack  <= 1; // Always immediately ack
      m_ctrlport_resp_data <= 0; // Zero out by default
      case (m_ctrlport_req_addr_aligned)
        REG_COMPAT_ADDR:          m_ctrlport_resp_data <= 32'(COMPAT);
        REG_CAPABILITIES_ADDR:    m_ctrlport_resp_data <= CAPABILITIES;
        REG_FFT_SIZE_LOG2_ADDR:   m_ctrlport_resp_data <= reg_fft_size_log2;
        REG_FFT_SCALING_ADDR:     m_ctrlport_resp_data <= reg_fft_scaling;
        REG_FFT_DIRECTION_ADDR:   m_ctrlport_resp_data <= reg_fft_direction;
        REG_CP_INS_LEN_ADDR:      m_ctrlport_resp_data <= reg_cp_insertion_len;
        REG_CP_REM_LEN_ADDR:      m_ctrlport_resp_data <= reg_cp_removal_len;
        REG_CP_INS_LIST_OCC_ADDR: m_ctrlport_resp_data <= reg_cp_insertion_cp_len_fifo_occupied;
        REG_CP_REM_LIST_OCC_ADDR: m_ctrlport_resp_data <= reg_cp_removal_cp_len_fifo_occupied;
        REG_BYPASS_ADDR:          m_ctrlport_resp_data <= 32'(reg_bypass);
        default:                  m_ctrlport_resp_data <= 32'h0BAD_C0DE;
      endcase
    end

    // Write user registers
    if (m_ctrlport_req_wr) begin // Write request
      m_ctrlport_resp_ack <= 1; // Always immediately ack
      case (m_ctrlport_req_addr_aligned)
        REG_USER_RESET_ADDR:       reg_user_reset                    <= 1'b1; // Strobe
        REG_FFT_SIZE_LOG2_ADDR:    reg_fft_size_log2                 <= m_ctrlport_req_data[REG_FFT_SIZE_LOG2_WIDTH-1:0];
        REG_FFT_SCALING_ADDR:      reg_fft_scaling                   <= m_ctrlport_req_data[REG_FFT_SCALING_WIDTH-1:0];
        REG_FFT_DIRECTION_ADDR:    reg_fft_direction                 <= m_ctrlport_req_data[REG_FFT_DIRECTION_WIDTH-1:0];
        REG_CP_INS_LEN_ADDR:       reg_cp_insertion_len              <= m_ctrlport_req_data[REG_CP_INS_LEN_WIDTH-1:0];
        REG_CP_REM_LEN_ADDR:       reg_cp_removal_len                <= m_ctrlport_req_data[REG_CP_REM_LEN_WIDTH-1:0];
        REG_CP_INS_LIST_LOAD_ADDR: reg_cp_insertion_cp_len_fifo_load <= 1'b1; // Strobe
        REG_CP_INS_LIST_CLR_ADDR:  reg_cp_insertion_cp_len_fifo_clr  <= 1'b1; // Strobe
        REG_CP_REM_LIST_LOAD_ADDR: reg_cp_removal_cp_len_fifo_load   <= 1'b1; // Strobe
        REG_CP_REM_LIST_CLR_ADDR:  reg_cp_removal_cp_len_fifo_clr    <= 1'b1; // Strobe
        REG_BYPASS_ADDR:           reg_bypass                        <= m_ctrlport_req_data[REG_BYPASS_WIDTH-1:0];
      endcase
    end
    if (core_rst) begin
      m_ctrlport_resp_ack               <= 1'b0;
      reg_user_reset                    <= 1'b0;
      reg_fft_size_log2                 <= DEFAULT_FFT_SIZE_LOG2;
      reg_fft_scaling                   <= DEFAULT_FFT_SCALING;
      reg_fft_direction                 <= DEFAULT_FFT_DIRECTION;
      reg_cp_insertion_len              <= DEFAULT_CP_LEN;
      reg_cp_removal_len                <= DEFAULT_CP_LEN;
      reg_cp_insertion_cp_len_fifo_load <= 1'b0;
      reg_cp_insertion_cp_len_fifo_clr  <= 1'b0;
      reg_cp_removal_cp_len_fifo_load   <= 1'b0;
      reg_cp_removal_cp_len_fifo_clr    <= 1'b0;
      reg_bypass                        <= 1'b0;
    end
  end


  //---------------------------------------------------------------------------
  // Reset Logic
  //---------------------------------------------------------------------------

  localparam RESET_PULSE_LEN  = 8;
  localparam RESET_CNT_WIDTH  = $clog2(RESET_PULSE_LEN);

  enum logic [0:0] { S_RESET_IDLE, S_RESET_ASSERT } reset_state = S_RESET_ASSERT;

  reg  [RESET_CNT_WIDTH:0] user_reset_cnt   = 'd0;
  reg                      user_reset       = 1'b1;

  always_ff @(posedge ce_clk) begin
    core_rst <= user_reset | ce_rst;
    case (reset_state)
      S_RESET_IDLE : begin
        user_reset_cnt <= 'd0;
        user_reset     <= 1'b0;
        if (reg_user_reset) begin
          user_reset   <= 1'b1;
          reset_state  <= S_RESET_ASSERT;
        end
      end
      S_RESET_ASSERT : begin
        user_reset_cnt  <= user_reset_cnt + 1;
        if (user_reset_cnt == RESET_PULSE_LEN-1) begin
          user_reset_cnt <= 'd0;
          user_reset     <= 1'b0;
          reset_state    <= S_RESET_IDLE;
        end
      end
    endcase
    if (ce_rst) begin
      user_reset     <= 1'b1;
      user_reset_cnt <= 'd0;
      reset_state    <= S_RESET_ASSERT;
    end
  end


  //---------------------------------------------------------------------------
  // Packetization
  //---------------------------------------------------------------------------

  wire [NUM_CHAN*32*1-1:0]   m_axis_user_tdata;
  wire [NUM_CHAN-1:0]        m_axis_user_teob;
  wire [NUM_CHAN-1:0]        m_axis_user_tlast;
  wire [NUM_CHAN-1:0]        m_axis_user_tvalid;
  wire [NUM_CHAN-1:0]        m_axis_user_tready;
  wire [NUM_CHAN*32*1-1:0]   s_axis_user_tdata;
  wire [NUM_CHAN-1:0]        s_axis_user_teob;
  wire [NUM_CHAN-1:0]        s_axis_user_teov;
  wire [NUM_CHAN-1:0]        s_axis_user_tlast;
  wire [NUM_CHAN-1:0]        s_axis_user_tvalid;
  wire [NUM_CHAN-1:0]        s_axis_user_tready;

  for (genvar i = 0; i < NUM_CHAN; i = i + 1) begin : gen_packetize
    axis_data_if_packetize #(
      .WIDTH                       (32),
      .SIDEBAND_FWD_FIFO_SIZE_LOG2 (1)) // Increase if using many small bursts
    axis_data_if_packetize_inst (
      .clk                (ce_clk),
      .reset              (core_rst),
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

  // Combine input streams into one. This lets us run the FFT instances (one
  // per channel) in lock-step by sharing valid/ready signals. Removes the need
  // for multiple instances of FFT configuration logic as well.
  wire [NUM_CHAN*32-1:0]    m_axis_combine_tdata;
  wire [NUM_CHAN-1:0]       m_axis_combine_teob;
  wire                      m_axis_combine_tlast;
  wire                      m_axis_combine_tvalid;
  wire                      m_axis_combine_tready;

  axis_combine #(
    .SIZE           (NUM_CHAN),
    .WIDTH          (32),
    .USER_WIDTH     (1),
    .FIFO_SIZE_LOG2 (0))
  axis_combine_inst (
    .clk            (ce_clk),
    .reset          (core_rst),
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

  logic [ NUM_CHAN*32-1:0] m_axis_cp_removal_tdata;
  logic [    NUM_CHAN-1:0] m_axis_cp_removal_teob;
  logic [    NUM_CHAN-1:0] m_axis_cp_removal_teov;
  logic                    m_axis_cp_removal_tlast;
  logic                    m_axis_cp_removal_tvalid;
  logic                    m_axis_cp_removal_tready;
  logic [FFT_CONFIG_W-1:0] s_axis_fft_config_tdata;
  logic                    s_axis_fft_config_tvalid;
  logic [    NUM_CHAN-1:0] s_axis_fft_config_tready;
  logic [ NUM_CHAN*32-1:0] s_axis_fft_data_tdata;
  logic                    s_axis_fft_data_tlast;
  logic                    s_axis_fft_data_tvalid;
  logic [    NUM_CHAN-1:0] s_axis_fft_data_tready;
  logic [ NUM_CHAN*32-1:0] m_axis_fft_data_tdata;
  logic [    NUM_CHAN-1:0] m_axis_fft_data_tlast;
  logic [    NUM_CHAN-1:0] m_axis_fft_data_tvalid;
  logic                    m_axis_fft_data_tready;
  logic [    NUM_CHAN-1:0] event_frame_started;
  logic [    NUM_CHAN-1:0] event_fft_overflow;
  logic [    NUM_CHAN-1:0] event_frame_tlast_unexpected;
  logic [    NUM_CHAN-1:0] event_frame_tlast_missing;
  logic [ NUM_CHAN*32-1:0] s_axis_split_bus_tdata;
  logic [    NUM_CHAN-1:0] s_axis_split_bus_teob;
  logic [    NUM_CHAN-1:0] s_axis_split_bus_teov;
  logic                    s_axis_split_bus_tlast;
  logic                    s_axis_split_bus_tvalid;
  logic                    s_axis_split_bus_tready;

  // Change packet length to FFT size
  wire [FFT_SIZE_W-1:0] fft_size = 1 << reg_fft_size_log2[$clog2(FFT_SIZE_W)-1:0];

  //synthesis translate_off
  integer dbg_removal_in_counter = 0, dbg_removal_out_counter = 0;
  integer dbg_removal_in_length, dbg_removal_out_length;
  reg dbg_removal_in_eop, dbg_removal_out_eop;
  always_ff @(posedge ce_clk) begin
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
  //synthesis translate_on


  //---------------------------------------------------------------------------
  // Cyclic Prefix Removal
  //---------------------------------------------------------------------------
  //
  // This block does the cyclic prefix removal and also sets tlast to ensure
  // that the packets going into the FFT block have the expected length.
  //
  //---------------------------------------------------------------------------

  // Also sets the packet size (i.e. tlast) to the FFT size
  cp_removal #(
    .DATA_W          (NUM_CHAN*32),
    .USER_W          (NUM_CHAN),
    .CP_LEN_W        (CP_LEN_W),
    .MAX_SYM_LEN_LOG2(FFT_SIZE_W),
    .DEFAULT_CP_LEN  (DEFAULT_CP_LEN),
    .CP_REPEAT       (CP_REMOVAL_REPEAT),
    .MAX_LIST_LOG2   (MAX_CP_LIST_LEN_REM_LOG2),
    .SET_TLAST       (1)
  ) cp_removal_i (
    .clk                 (ce_clk),
    .rst                 (core_rst),
    .clear_list          (reg_cp_removal_cp_len_fifo_clr),
    .symbol_len          (fft_size),
    .s_axis_cp_len_tdata (reg_cp_removal_len),
    .s_axis_cp_len_tvalid(reg_cp_removal_cp_len_fifo_load),
    // No back-pressure needed as the block controller checks "cp_len_fifo_occupied"
    // before writing to avoid overflowing FIFO
    .s_axis_cp_len_tready(),
    .cp_list_occupied    (reg_cp_removal_cp_len_fifo_occupied),
    .s_axis_data_tdata   (m_axis_combine_tdata),
    .s_axis_data_tuser   (m_axis_combine_teob),
    .s_axis_data_tlast   (m_axis_combine_tlast),
    .s_axis_data_tvalid  (m_axis_combine_tvalid),
    .s_axis_data_tready  (m_axis_combine_tready),
    .m_axis_data_tdata   (m_axis_cp_removal_tdata),
    .m_axis_data_tuser   (m_axis_cp_removal_teob),
    .m_axis_data_tlast   (m_axis_cp_removal_tlast),
    .m_axis_data_tvalid  (m_axis_cp_removal_tvalid),
    .m_axis_data_tready  (m_axis_cp_removal_tready)
  );

  // We can create a teov from tlast because the packet size is the same as the FFT size
  assign m_axis_cp_removal_teov = {NUM_CHAN{m_axis_cp_removal_tlast}};


  //---------------------------------------------------------------------------
  // Cyclic Prefix Insertion
  //---------------------------------------------------------------------------
  //
  // This block manages the cyclic prefix insertion list. The actual insertion
  // is done by the FFT block.
  //
  //---------------------------------------------------------------------------

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
  // If the FIFO does not have any entries, cyclic prefix length defaults to
  // DEFAULT_CP_LEN (see FFT configuration state machine below).
  // User should generally not load or clear the FIFO while streaming, but the logic
  // does not prevent it.
  //
  logic [CP_LEN_W-1:0] s_axis_cp_len_fifo_tdata;
  logic                s_axis_cp_len_fifo_tvalid;
  logic [CP_LEN_W-1:0] m_axis_cp_len_fifo_tdata;
  logic                m_axis_cp_len_fifo_tvalid;
  logic                m_axis_cp_len_fifo_tready;
  logic                cp_len_fifo_empty;

  assign cp_len_fifo_empty = (reg_cp_insertion_cp_len_fifo_occupied == 0);

  axi_fifo #(
    .WIDTH(CP_LEN_W),
    .SIZE (MAX_CP_LIST_LEN_INS_LOG2)
  ) axi_fifo_cp_insertion_cp_len_inst (
    .clk     (ce_clk),
    .reset   (core_rst),
    .clear   (reg_cp_insertion_cp_len_fifo_clr),
    .i_tdata (s_axis_cp_len_fifo_tdata),
    .i_tvalid(s_axis_cp_len_fifo_tvalid),
    .i_tready(), // No back-pressure needed since block controller checks cp_len_fifo_occupied to not overflow FIFO
    .o_tdata (m_axis_cp_len_fifo_tdata),
    .o_tvalid(m_axis_cp_len_fifo_tvalid), // Unused
    .o_tready(m_axis_cp_len_fifo_tready),
    .space   (),
    .occupied(reg_cp_insertion_cp_len_fifo_occupied)
  );

  cp_insertion_ctrl #(
    .CP_LEN_W        (CP_LEN_W),
    .MAX_SYM_LEN_LOG2(FFT_SIZE_W),
    .DEFAULT_CP_LEN  (DEFAULT_CP_LEN),
    .CP_REPEAT       (CP_REMOVAL_REPEAT),
    .MAX_LIST_LOG2   (MAX_CP_LIST_LEN_REM_LOG2),
    .SET_TLAST       (1)
  ) cp_insertion_ctrl_i (
    .clk                 (ce_clk),
    .rst                 (core_rst),
    .clear_list          (reg_cp_insertion_cp_len_fifo_clr),
    .s_axis_cp_len_tdata (s_axis_cp_len_fifo_tdata),
    .s_axis_cp_len_tvalid(s_axis_cp_len_fifo_tvalid),
    // No back-pressure needed as the block controller checks "cp_len_fifo_occupied"
    // before writing to avoid overflowing FIFO
    .s_axis_cp_len_tready(),
    .cp_list_occupied    (),  // TODO: Connect these port
    .cp_len              (),
    .cp_len_ack          (m_axis_cp_len_fifo_tready)
  );


  //---------------------------------------------------------------------------
  // FFT Configuration State Machine
  //---------------------------------------------------------------------------

  // FFT IP configuration state machine
  // Loads a new configuration at the start of every FFT
  enum logic [0:0] { S_FFT_CONFIG, S_FFT_WAIT_FOR_TLAST } fft_config_state = S_FFT_CONFIG;

  always_ff @(posedge ce_clk) begin
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
    if (core_rst) begin
      fft_config_state <= S_FFT_CONFIG;
    end
  end

  always_ff @(posedge ce_clk) begin : shift_cp_length
    // The CP insertion length is always clog2(fft_size) bits wide and must be
    // shifted into the uppermost bits of the field.
    if (cp_len_fifo_empty) begin
      reg_cp_length <= DEFAULT_CP_LEN << (MAX_FFT_SIZE_LOG2 - reg_fft_size_log2);
    end else begin
      reg_cp_length <= m_axis_cp_len_fifo_tdata << (MAX_FFT_SIZE_LOG2 - reg_fft_size_log2);
    end
  end

  assign s_axis_fft_config_tdata = build_fft_config(
    MAX_FFT_SIZE_LOG2,
    reg_fft_scaling,
    reg_fft_direction,
    reg_cp_length,
    reg_fft_size_log2
  );

  assign s_axis_fft_config_tvalid = (fft_config_state == S_FFT_CONFIG) ? s_axis_fft_data_tvalid && s_axis_fft_data_tready : 1'b0;

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
    .WIDTH                 (NUM_CHAN*32),
    .USER_WIDTH            (2*NUM_CHAN),
    .FIFO_SIZE_LOG2        (5),
    .PACKET_MODE           (2)) // tuser is EOB and static for the entire packet,
                                // so we can use this more efficient packet mode
  axis_sideband_xfft_tuser (
    .clk                   (ce_clk),
    .reset                 (core_rst),
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

  //synthesis translate_off
  integer dbg_fft_in_counter = 0, dbg_fft_out_counter = 0;
  integer dbg_fft_in_length, dbg_fft_out_length;
  reg dbg_fft_in_eop, dbg_fft_out_eop;

  always_ff @(posedge ce_clk) begin
    if (s_axis_fft_data_tvalid && s_axis_fft_data_tready[0]) begin
      if (s_axis_fft_data_tlast) begin
        dbg_fft_in_counter <= 0;
        dbg_fft_in_eop <= 1;
        dbg_fft_in_length <= dbg_fft_in_counter+1;
      end else begin
        dbg_fft_in_eop <= 0;
        dbg_fft_in_counter <= dbg_fft_in_counter + 1;
      end
    end

    if (m_axis_fft_data_tvalid[0] && m_axis_fft_data_tready) begin
      if (m_axis_fft_data_tlast[0]) begin
        dbg_fft_out_counter <= 0;
        dbg_fft_out_eop <= 1;
        dbg_fft_out_length <= dbg_fft_out_counter+1;
      end else begin
        dbg_fft_out_eop <= 0;
        dbg_fft_out_counter <= dbg_fft_out_counter + 1;
      end
    end
  end
  //synthesis translate_on

  //---------------------------------------------------------------------------
  // FFT Bypass
  //---------------------------------------------------------------------------

  logic [NUM_CHAN*32-1:0] s_fft_data_tdata;
  logic [   NUM_CHAN-1:0] s_fft_data_tlast;
  logic [   NUM_CHAN-1:0] s_fft_data_tvalid;
  logic [   NUM_CHAN-1:0] s_fft_data_tready;
  logic [NUM_CHAN*32-1:0] m_fft_data_tdata;
  logic [   NUM_CHAN-1:0] m_fft_data_tlast;
  logic [   NUM_CHAN-1:0] m_fft_data_tvalid;
  logic [   NUM_CHAN-1:0] m_fft_data_tready;

  logic [NUM_CHAN*32-1:0] s_fifo_data_tdata;
  logic [   NUM_CHAN-1:0] s_fifo_data_tlast;
  logic [   NUM_CHAN-1:0] s_fifo_data_tvalid;
  logic [   NUM_CHAN-1:0] s_fifo_data_tready;
  logic [NUM_CHAN*32-1:0] m_fifo_data_tdata;
  logic [   NUM_CHAN-1:0] m_fifo_data_tlast;
  logic [   NUM_CHAN-1:0] m_fifo_data_tvalid;
  logic [   NUM_CHAN-1:0] m_fifo_data_tready;

  always_comb begin
    if (reg_bypass) begin
      // FIFO connections pass through
      s_fifo_data_tdata      = s_axis_fft_data_tdata;
      s_fifo_data_tlast      = {NUM_CHAN{s_axis_fft_data_tlast}};
      s_fifo_data_tvalid     = {NUM_CHAN{s_axis_fft_data_tvalid}};
      s_axis_fft_data_tready = s_fifo_data_tready;
      m_axis_fft_data_tdata  = m_fifo_data_tdata;
      m_axis_fft_data_tlast  = m_fifo_data_tlast;
      m_axis_fft_data_tvalid = m_fifo_data_tvalid;
      m_fifo_data_tready     = {NUM_CHAN{m_axis_fft_data_tready}};

      // Tie off bypassed FFT connections
      s_fft_data_tdata       = s_axis_fft_data_tdata;
      s_fft_data_tlast       = {NUM_CHAN{s_axis_fft_data_tlast}};
      s_fft_data_tvalid      = {NUM_CHAN{1'b0}};
      m_fft_data_tready      = {NUM_CHAN{1'b1}};
    end else begin
      // Tie off bypassed FIFO connections
      s_fifo_data_tdata  = s_axis_fft_data_tdata;
      s_fifo_data_tlast  = {NUM_CHAN{s_axis_fft_data_tlast}};
      s_fifo_data_tvalid = {NUM_CHAN{1'b0}};
      m_fifo_data_tready = {NUM_CHAN{1'b1}};

      // FFT connections pass through like normal
      s_fft_data_tdata       = s_axis_fft_data_tdata;
      s_fft_data_tlast       = {NUM_CHAN{s_axis_fft_data_tlast}};
      s_fft_data_tvalid      = {NUM_CHAN{s_axis_fft_data_tvalid}};
      s_axis_fft_data_tready = s_fft_data_tready;
      m_axis_fft_data_tdata  = m_fft_data_tdata;
      m_axis_fft_data_tlast  = m_fft_data_tlast;
      m_axis_fft_data_tvalid = m_fft_data_tvalid;
      m_fft_data_tready      = {NUM_CHAN{m_axis_fft_data_tready}};
    end
  end

  //---------------------------------------------------------------------------
  // FFT Core
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_CHAN; i = i + 1) begin : gen_fft
    axi_fifo #(
      .WIDTH(NUM_CHAN*33),
      .SIZE (MAX_FFT_SIZE_LOG2)
    ) axi_fifo_bypass (
      .clk     (ce_clk),
      .reset   (core_rst),
      .clear   (1'b0),
      .i_tdata ({s_fifo_data_tlast[i], s_fifo_data_tdata[32*i +: 32]}),
      .i_tvalid(s_fifo_data_tvalid[i]),
      .i_tready(s_fifo_data_tready[i]),
      .o_tdata ({m_fifo_data_tlast[i], m_fifo_data_tdata[32*i +: 32]}),
      .o_tvalid(m_fifo_data_tvalid[i]),
      .o_tready(m_fifo_data_tready[i]),
      .space   (),
      .occupied()
    );

    xfft_wrapper #(
      .MAX_FFT_SIZE_LOG2 (MAX_FFT_SIZE_LOG2)
    ) xfft_wrapper_i (
      .aclk                        (ce_clk),
      .aresetn                     (~core_rst),
      .s_axis_config_tdata         (s_axis_fft_config_tdata),
      .s_axis_config_tvalid        (s_axis_fft_config_tvalid),
      .s_axis_config_tready        (s_axis_fft_config_tready[i]),
      .s_axis_data_tdata           ({ s_fft_data_tdata[32*i +: 16], s_fft_data_tdata[32*i+16 +: 16] }),
      .s_axis_data_tlast           (s_fft_data_tlast[i]),
      .s_axis_data_tvalid          (s_fft_data_tvalid[i]),
      .s_axis_data_tready          (s_fft_data_tready[i]),
      .m_axis_data_tdata           ({ m_fft_data_tdata[32*i +: 16], m_fft_data_tdata[32*i+16 +: 16] }),
      .m_axis_data_tuser           (),
      .m_axis_data_tlast           (m_fft_data_tlast[i]),
      .m_axis_data_tvalid          (m_fft_data_tvalid[i]),
      .m_axis_data_tready          (m_fft_data_tready[i]),
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

  // Split back into multiple streams
  axis_split_bus #(
    .WIDTH          (32),
    .USER_WIDTH     (2),
    .NUM_PORTS      (NUM_CHAN)
  ) axis_split_bus_i (
    .clk            (ce_clk),
    .reset          (core_rst),
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

endmodule : fft_core

`default_nettype wire
