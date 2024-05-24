//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_core
//
// Description:
//
//   This module encapsulates the core components that make up a single or
//   multi-channel FFT with cyclic prefix insertion/removal.
//
//   All channels going into this core must be used simultaneously and share
//   the same register settings. In other words, you can't use one channel and
//   leave the other idle. The used channel will stall while waiting for the
//   other channel's data to arrive. This is because all channels share the
//   same cyclic prefix removal logic. You also cannot have different settings
//   per channel within a single fft_core. However, multiple instances of this
//   fft_core can be instantiated to allow for independent channels.
//
//   The maximum cyclic prefix insertion/removal length is always the maximum
//   FFT size minus 1. The length of the list used to track a sequence of CP
//   insertions/removals is set by parameters.
//
// Parameters:
//
//   NUM_CHAN                 : Number of channels to instantiate on this
//                              fft_core instance.
//   NUM_CORES                : Total number of fft_core instances in the
//                              parent RFNoC block, including this one.
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size. Actual
//                              max is 2**MAX_FFT_SIZE_LOG2.
//   MAX_CP_LIST_LEN_INS_LOG2 : Log2 of max length of cyclic prefix insertion
//                              list. Actual max is 2**MAX_CP_LIST_LEN_INS_LOG2.
//   MAX_CP_LIST_LEN_REM_LOG2 : Log2 of max length of cyclic prefix removal
//                              list. Actual max is 2**MAX_CP_LIST_LEN_REM_LOG2.
//   CP_INSERTION_REPEAT      : Enable repeating the CP insertion list. When 1,
//                              the list repeats. When 0, CP insertion will
//                              stop when the list is finished.
//   CP_REMOVAL_REPEAT        : Enable repeating the CP removal list. When 1,
//                              the list repeats. When 0, CP removal will
//                              stop when the list is finished.
//   EN_FFT_BYPASS            : Controls whether to include the FFT bypass logic.
//   EN_FFT_ORDER             : Controls whether to include the FFT reorder logic.
//   EN_MAGNITUDE             : Controls whether to include the magnitude
//                              output calculation logic.
//   EN_MAGNITUDE_SQ          : Controls whether to include the
//                              magnitude-squared output calculation logic.
//   USE_APPROX_MAG           : Controls whether to use the low-resource
//                              approximate calculation (1) or the more exact
//                              and more resource-intensive calculation (0) for
//                              the magnitude calculation.
//

`default_nettype none


module fft_core
  import rfnoc_chdr_utils_pkg::*;
  import ctrlport_pkg::*;
#(
  int NUM_CHAN                 = 1,
  int NUM_CORES                = 1,
  int MAX_FFT_SIZE_LOG2        = 12,
  int MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int MAX_CP_LIST_LEN_REM_LOG2 = 5,
  bit CP_INSERTION_REPEAT      = 1,
  bit CP_REMOVAL_REPEAT        = 1,
  bit EN_FFT_BYPASS            = 1,
  bit EN_FFT_ORDER             = 1,
  bit EN_MAGNITUDE             = 1,
  bit EN_MAGNITUDE_SQ          = 1,
  bit USE_APPROX_MAG           = 1,

  // Data width of each FFT channel
  localparam int ITEM_W = 32,
  localparam int DATA_W = ITEM_W,
  localparam int KEEP_W = 1
) (
  input  wire                                  ce_clk,
  input  wire                                  ce_rst,

  // CtrlPort Register Interface
  input  wire                                  s_ctrlport_req_wr,
  input  wire                                  s_ctrlport_req_rd,
  input  wire  [          CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  wire  [          CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output logic                                 s_ctrlport_resp_ack,
  output logic [          CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // Data Input Packets
  input  wire  [          DATA_W*NUM_CHAN-1:0] s_in_axis_tdata,
  input  wire  [          KEEP_W*NUM_CHAN-1:0] s_in_axis_tkeep,
  input  wire  [                 NUM_CHAN-1:0] s_in_axis_tlast,
  input  wire  [                 NUM_CHAN-1:0] s_in_axis_tvalid,
  output logic [                 NUM_CHAN-1:0] s_in_axis_tready,
  input  wire  [CHDR_TIMESTAMP_W*NUM_CHAN-1:0] s_in_axis_ttimestamp,
  input  wire  [                 NUM_CHAN-1:0] s_in_axis_thas_time,
  input  wire  [   CHDR_LENGTH_W*NUM_CHAN-1:0] s_in_axis_tlength,
  input  wire  [                 NUM_CHAN-1:0] s_in_axis_teov,
  input  wire  [                 NUM_CHAN-1:0] s_in_axis_teob,

  // Data Output Packets
  output wire  [          DATA_W*NUM_CHAN-1:0] m_out_axis_tdata,
  output wire  [          KEEP_W*NUM_CHAN-1:0] m_out_axis_tkeep,
  output wire  [                 NUM_CHAN-1:0] m_out_axis_tlast,
  output wire  [                 NUM_CHAN-1:0] m_out_axis_tvalid,
  input  wire  [                 NUM_CHAN-1:0] m_out_axis_tready,
  output wire  [CHDR_TIMESTAMP_W*NUM_CHAN-1:0] m_out_axis_ttimestamp,
  output wire  [                 NUM_CHAN-1:0] m_out_axis_thas_time,
  output wire  [   CHDR_LENGTH_W*NUM_CHAN-1:0] m_out_axis_tlength,
  output wire  [                 NUM_CHAN-1:0] m_out_axis_teov,
  output wire  [                 NUM_CHAN-1:0] m_out_axis_teob
);
  // Import utilities for working with Xilinx FFT block
  import xfft_config_pkg::*;

  // Import register descriptions
  import fft_core_regs_pkg::*;

  `include "usrp_utils.svh"


  //---------------------------------------------------------------------------
  // FFT Configuration Interface Constants
  //---------------------------------------------------------------------------

  localparam int MAX_FFT_SIZE    = 2**MAX_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN_LOG2 = MAX_FFT_SIZE_LOG2;

  // Calculate the widths needed for the configuration settings of the Xilinx
  // FFT core. The widths change depending on the configuration.
  localparam int FFT_SCALE_W   = fft_scale_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_FWD_INV_W = fft_fwd_inv_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_CP_LEN_W  = fft_cp_len_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_NFFT_W    = fft_nfft_w(MAX_FFT_SIZE_LOG2);
  localparam int FFT_CONFIG_W  = fft_config_w(MAX_FFT_SIZE_LOG2);

  // Use conservative 1/N scaling by default
  localparam [FFT_SCALE_W-1:0] DEFAULT_FFT_SCALING = fft_scale_default(MAX_FFT_SIZE_LOG2);


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  localparam FFT_SIZE_LOG2_W = $clog2(MAX_FFT_SIZE_LOG2+1);
  localparam FFT_SIZE_W      = MAX_FFT_SIZE_LOG2+1;
  localparam CP_LEN_W        = MAX_CP_LEN_LOG2;

  localparam int REG_LENGTH_LOG2_WIDTH = FFT_NFFT_W;
  localparam int REG_SCALING_WIDTH     = FFT_SCALE_W;
  localparam int REG_CP_INS_LEN_WIDTH  = CP_LEN_W;
  localparam int REG_CP_REM_LEN_WIDTH  = CP_LEN_W;

  localparam bit [REG_COMPAT_WIDTH-1:0] COMPAT  = {16'h03, 16'h00};
  localparam bit [REG_CAPABILITIES_WIDTH-1:0] CAPABILITIES = {
    8'(MAX_CP_LIST_LEN_INS_LOG2),
    8'(MAX_CP_LIST_LEN_REM_LOG2),
    8'(MAX_CP_LEN_LOG2),
    8'(MAX_FFT_SIZE_LOG2)
  };
  localparam bit [REG_CAPABILITIES2_WIDTH-1:0] CAPABILITIES2 = {
    1'(EN_MAGNITUDE_SQ),
    1'(EN_MAGNITUDE),
    1'(EN_FFT_ORDER),
    1'(EN_FFT_BYPASS)
  };
  localparam bit [REG_PORT_CONFIG_WIDTH-1:0] PORT_CONFIG = {
    REG_NUM_CORES_W'(NUM_CORES),
    REG_NUM_CHAN_W'(NUM_CHAN)
  };

  localparam int DEFAULT_FFT_SIZE_LOG2 = MAX_FFT_SIZE_LOG2;
  localparam int DEFAULT_FFT_DIRECTION = FFT_INVERSE;
  localparam int DEFAULT_CP_LEN        = 0;

  logic core_rst;

  logic [           REG_RESET_WIDTH-1:0] reg_user_reset       = '0;
  logic [     REG_LENGTH_LOG2_WIDTH-1:0] reg_fft_size_log2    = DEFAULT_FFT_SIZE_LOG2;
  logic [         REG_SCALING_WIDTH-1:0] reg_fft_scaling      = DEFAULT_FFT_SCALING;
  logic [       REG_DIRECTION_WIDTH-1:0] reg_fft_direction    = DEFAULT_FFT_DIRECTION;
  logic [      REG_CP_INS_LEN_WIDTH-1:0] reg_cp_ins_length    = DEFAULT_CP_LEN;
  logic [      REG_CP_REM_LEN_WIDTH-1:0] reg_cp_rem_length    = DEFAULT_CP_LEN;
  logic [REG_CP_INS_LIST_LOAD_WIDTH-1:0] reg_cp_ins_list_load = '0;
  logic [ REG_CP_INS_LIST_CLR_WIDTH-1:0] reg_cp_ins_list_clr  = '0;
  logic [REG_CP_REM_LIST_LOAD_WIDTH-1:0] reg_cp_rem_list_load = '0;
  logic [ REG_CP_REM_LIST_CLR_WIDTH-1:0] reg_cp_rem_list_clr  = '0;
  logic [          REG_BYPASS_WIDTH-1:0] reg_fft_bypass       = '0;
  logic [           REG_ORDER_WIDTH-1:0] reg_fft_order        = FFT_ORDER_NORMAL;
  logic [       REG_MAGNITUDE_WIDTH-1:0] reg_magnitude        = '0;
  logic [                  NUM_CHAN-1:0] reg_overflow         = '0;
  logic [ REG_CP_INS_LIST_OCC_WIDTH-1:0] reg_cp_ins_list_occupied;
  logic [ REG_CP_REM_LIST_OCC_WIDTH-1:0] reg_cp_rem_list_occupied;

  logic [FFT_SIZE_LOG2_W-1:0] fft_size_log2;
  assign fft_size_log2 = FFT_SIZE_LOG2_W'(reg_fft_size_log2);

  logic [FFT_SIZE_W-1:0] fft_size;
  assign fft_size = 1 << reg_fft_size_log2[FFT_SIZE_LOG2_W-1:0];

  logic [REG_ADDR_W-1:0] s_ctrlport_req_addr_aligned;
  assign s_ctrlport_req_addr_aligned = REG_ADDR_W'({s_ctrlport_req_addr[19:2], 2'b0});

  logic [NUM_CHAN-1:0] event_fft_overflow;

  always_ff @(posedge ce_clk) begin
    // Default assignment
    s_ctrlport_resp_ack <= 0;

    // Always clear these regs after being set
    reg_user_reset       <= 1'b0;
    reg_cp_ins_list_load <= 1'b0;
    reg_cp_ins_list_clr  <= 1'b0;
    reg_cp_rem_list_load <= 1'b0;
    reg_cp_rem_list_clr  <= 1'b0;

    // Read user registers
    if (s_ctrlport_req_rd) begin // Read request
      s_ctrlport_resp_ack  <= 1; // Always immediately ack
      s_ctrlport_resp_data <= 0; // Zero out by default
      case (s_ctrlport_req_addr_aligned)
        REG_COMPAT_ADDR:          s_ctrlport_resp_data <= 32'(COMPAT);
        REG_PORT_CONFIG_ADDR:     s_ctrlport_resp_data <= 32'(PORT_CONFIG);
        REG_CAPABILITIES_ADDR:    s_ctrlport_resp_data <= 32'(CAPABILITIES);
        REG_CAPABILITIES2_ADDR:   s_ctrlport_resp_data <= 32'(CAPABILITIES2);
        REG_OVERFLOW_ADDR:        s_ctrlport_resp_data <= 32'(reg_overflow);
        REG_LENGTH_LOG2_ADDR:     s_ctrlport_resp_data <= 32'(reg_fft_size_log2);
        REG_SCALING_ADDR:         s_ctrlport_resp_data <= 32'(reg_fft_scaling);
        REG_DIRECTION_ADDR:       s_ctrlport_resp_data <= 32'(reg_fft_direction);
        REG_CP_INS_LEN_ADDR:      s_ctrlport_resp_data <= 32'(reg_cp_ins_length);
        REG_CP_REM_LEN_ADDR:      s_ctrlport_resp_data <= 32'(reg_cp_rem_length);
        REG_CP_INS_LIST_OCC_ADDR: s_ctrlport_resp_data <= 32'(reg_cp_ins_list_occupied);
        REG_CP_REM_LIST_OCC_ADDR: s_ctrlport_resp_data <= 32'(reg_cp_rem_list_occupied);
        REG_BYPASS_ADDR:          s_ctrlport_resp_data <= EN_FFT_BYPASS ? 32'(reg_fft_bypass) : '0;
        REG_ORDER_ADDR:           s_ctrlport_resp_data <= EN_FFT_ORDER ? 32'(reg_fft_order) : '0;
        REG_MAGNITUDE_ADDR:       s_ctrlport_resp_data <= (EN_MAGNITUDE || EN_MAGNITUDE_SQ) ?
                                                          32'(reg_magnitude) : 0;
        default:                  s_ctrlport_resp_data <= 32'h0BAD_C0DE;
      endcase
    end

    // Write user registers
    if (s_ctrlport_req_wr) begin // Write request
      s_ctrlport_resp_ack <= 1; // Always immediately ack
      case (s_ctrlport_req_addr_aligned)
        REG_RESET_ADDR:
          reg_user_reset           <= 1'b1; // Strobe
        REG_LENGTH_LOG2_ADDR:
          reg_fft_size_log2        <= s_ctrlport_req_data[REG_LENGTH_LOG2_WIDTH-1:0];
        REG_SCALING_ADDR:
          reg_fft_scaling          <= s_ctrlport_req_data[REG_SCALING_WIDTH-1:0];
        REG_DIRECTION_ADDR:
          reg_fft_direction        <= s_ctrlport_req_data[REG_DIRECTION_WIDTH-1:0];
        REG_CP_INS_LEN_ADDR:
          reg_cp_ins_length        <= s_ctrlport_req_data[REG_CP_INS_LEN_WIDTH-1:0];
        REG_CP_REM_LEN_ADDR:
          reg_cp_rem_length        <= s_ctrlport_req_data[REG_CP_REM_LEN_WIDTH-1:0];
        REG_CP_INS_LIST_LOAD_ADDR:
          reg_cp_ins_list_load     <= 1'b1; // Strobe
        REG_CP_INS_LIST_CLR_ADDR:
          reg_cp_ins_list_clr      <= 1'b1; // Strobe
        REG_CP_REM_LIST_LOAD_ADDR:
          reg_cp_rem_list_load     <= 1'b1; // Strobe
        REG_CP_REM_LIST_CLR_ADDR:
          reg_cp_rem_list_clr      <= 1'b1; // Strobe
        REG_BYPASS_ADDR:
          reg_fft_bypass           <= s_ctrlport_req_data[REG_BYPASS_WIDTH-1:0];
        REG_ORDER_ADDR:
          reg_fft_order            <= s_ctrlport_req_data[REG_ORDER_WIDTH-1:0];
        REG_MAGNITUDE_ADDR:
          reg_magnitude            <= s_ctrlport_req_data[REG_MAGNITUDE_WIDTH-1:0];
      endcase
    end

    // Store whether or not we had overflow events on this clock cycle. These
    // bits are sticky and clear on read.
    if (s_ctrlport_req_rd && s_ctrlport_req_addr_aligned == REG_OVERFLOW_ADDR) begin
      reg_overflow <= event_fft_overflow;
    end else begin
      reg_overflow <= reg_overflow | event_fft_overflow;
    end

    if (core_rst) begin
      s_ctrlport_resp_ack  <= '0;
      reg_user_reset       <= '0;
      reg_fft_size_log2    <= DEFAULT_FFT_SIZE_LOG2;
      reg_fft_scaling      <= DEFAULT_FFT_SCALING;
      reg_fft_direction    <= DEFAULT_FFT_DIRECTION;
      reg_cp_ins_length    <= DEFAULT_CP_LEN;
      reg_cp_rem_length    <= DEFAULT_CP_LEN;
      reg_cp_ins_list_load <= '0;
      reg_cp_ins_list_clr  <= '0;
      reg_cp_rem_list_load <= '0;
      reg_cp_rem_list_clr  <= '0;
      reg_fft_bypass       <= '0;
      reg_fft_order        <= FFT_ORDER_NORMAL;
      reg_magnitude        <= '0;
      reg_overflow         <= '0;
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

  wire [DATA_W*NUM_CHAN-1:0] user_in_tdata;
  wire [       NUM_CHAN-1:0] user_in_teob;
  wire [       NUM_CHAN-1:0] user_in_tlast;
  wire [       NUM_CHAN-1:0] user_in_tvalid;
  wire [       NUM_CHAN-1:0] user_in_tready;
  wire [DATA_W*NUM_CHAN-1:0] user_out_tdata;
  wire [       NUM_CHAN-1:0] user_out_teob;
  wire [       NUM_CHAN-1:0] user_out_teov;
  wire [       NUM_CHAN-1:0] user_out_tlast;
  wire [       NUM_CHAN-1:0] user_out_tvalid;
  wire [       NUM_CHAN-1:0] user_out_tready;

  for (genvar ch_i = 0; ch_i < NUM_CHAN; ch_i = ch_i + 1) begin : gen_packetize
    axis_data_if_packetize #(
      .NIPC                       (1     ),
      .ITEM_W                     (ITEM_W),
      .SIDEBAND_FWD_FIFO_SIZE_LOG2(1     )
    ) axis_data_if_packetize_i (
      .clk               (ce_clk                                               ),
      .reset             (core_rst                                             ),
      .spp               ('0                                                   ),
      .s_axis_tdata      (`BUS_I(s_in_axis_tdata,                 DATA_W, ch_i)),
      .s_axis_tlast      (`BUS_I(s_in_axis_tlast,                      1, ch_i)),
      .s_axis_tkeep      (`BUS_I(s_in_axis_tkeep,                 KEEP_W, ch_i)),
      .s_axis_tvalid     (`BUS_I(s_in_axis_tvalid,                     1, ch_i)),
      .s_axis_tready     (`BUS_I(s_in_axis_tready,                     1, ch_i)),
      .s_axis_ttimestamp (`BUS_I(s_in_axis_ttimestamp,  CHDR_TIMESTAMP_W, ch_i)),
      .s_axis_thas_time  (`BUS_I(s_in_axis_thas_time,                  1, ch_i)),
      .s_axis_tlength    (`BUS_I(s_in_axis_tlength,        CHDR_LENGTH_W, ch_i)),
      .s_axis_teov       (`BUS_I(s_in_axis_teov,                       1, ch_i)),
      .s_axis_teob       (`BUS_I(s_in_axis_teob,                       1, ch_i)),
      .m_axis_tdata      (`BUS_I(m_out_axis_tdata,                DATA_W, ch_i)),
      .m_axis_tkeep      (`BUS_I(m_out_axis_tkeep,                KEEP_W, ch_i)),
      .m_axis_tlast      (`BUS_I(m_out_axis_tlast,                     1, ch_i)),
      .m_axis_tvalid     (`BUS_I(m_out_axis_tvalid,                    1, ch_i)),
      .m_axis_tready     (`BUS_I(m_out_axis_tready,                    1, ch_i)),
      .m_axis_ttimestamp (`BUS_I(m_out_axis_ttimestamp, CHDR_TIMESTAMP_W, ch_i)),
      .m_axis_thas_time  (`BUS_I(m_out_axis_thas_time,                 1, ch_i)),
      .m_axis_tlength    (`BUS_I(m_out_axis_tlength,       CHDR_LENGTH_W, ch_i)),
      .m_axis_teov       (`BUS_I(m_out_axis_teov,                      1, ch_i)),
      .m_axis_teob       (`BUS_I(m_out_axis_teob,                      1, ch_i)),
      .m_axis_user_tdata (`BUS_I(user_in_tdata,                   DATA_W, ch_i)),
      .m_axis_user_tkeep (                                                     ),
      .m_axis_user_teob  (`BUS_I(user_in_teob,                         1, ch_i)),
      .m_axis_user_teov  (                                                     ),
      .m_axis_user_tlast (`BUS_I(user_in_tlast,                        1, ch_i)),
      .m_axis_user_tvalid(`BUS_I(user_in_tvalid,                       1, ch_i)),
      .m_axis_user_tready(`BUS_I(user_in_tready,                       1, ch_i)),
      .s_axis_user_tdata (`BUS_I(user_out_tdata,                  DATA_W, ch_i)),
      .s_axis_user_tkeep ('1                                                   ),
      .s_axis_user_teob  (`BUS_I(user_out_teob,                        1, ch_i)),
      .s_axis_user_teov  (`BUS_I(user_out_teov,                        1, ch_i)),
      .s_axis_user_tlast (`BUS_I(user_out_tlast,                       1, ch_i)),
      .s_axis_user_tvalid(`BUS_I(user_out_tvalid,                      1, ch_i)),
      .s_axis_user_tready(`BUS_I(user_out_tready,                      1, ch_i))
    );
  end


  //---------------------------------------------------------------------------
  // Combine Streams
  //---------------------------------------------------------------------------
  //
  // Combine input streams into one AXI-Stream bus. This lets us run the FFT
  // instances (one per channel) in lock-step by sharing valid/ready signals.
  // Also removes the need for multiple instances of FFT configuration logic.
  //
  //---------------------------------------------------------------------------

  wire [DATA_W*NUM_CHAN-1:0] combine_out_tdata;
  wire [       NUM_CHAN-1:0] combine_out_teob;
  wire                       combine_out_tlast;
  wire                       combine_out_tvalid;
  wire                       combine_out_tready;

  axis_combine #(
    .SIZE           (NUM_CHAN),
    .WIDTH          (DATA_W),
    .USER_WIDTH     (1),
    .FIFO_SIZE_LOG2 (0))
  axis_combine_inst (
    .clk            (ce_clk),
    .reset          (core_rst),
    .s_axis_tdata   (user_in_tdata),
    .s_axis_tuser   (user_in_teob),
    .s_axis_tlast   (user_in_tlast),
    .s_axis_tvalid  (user_in_tvalid),
    .s_axis_tready  (user_in_tready),
    .m_axis_tdata   (combine_out_tdata),
    .m_axis_tuser   (combine_out_teob),
    .m_axis_tlast   (combine_out_tlast),
    .m_axis_tvalid  (combine_out_tvalid),
    .m_axis_tready  (combine_out_tready)
  );


  //---------------------------------------------------------------------------
  // Cyclic Prefix Removal
  //---------------------------------------------------------------------------
  //
  // This block does the cyclic prefix removal and also sets tlast to ensure
  // that the packets going into the FFT block have the expected length.
  //
  //---------------------------------------------------------------------------

  logic [DATA_W*NUM_CHAN-1:0] cp_removal_out_tdata;
  logic [       NUM_CHAN-1:0] cp_removal_out_teob;
  logic [       NUM_CHAN-1:0] cp_removal_out_teov;

  logic                    cp_removal_out_tlast;
  logic                    cp_removal_out_tvalid;
  logic                    cp_removal_out_tready;

  // Also sets the packet size (i.e. tlast) to the FFT size
  cp_removal #(
    .DATA_W        (NUM_CHAN*DATA_W         ),
    .USER_W        (NUM_CHAN                ),
    .CP_LEN_W      (CP_LEN_W                ),
    .SYM_LEN_W     (FFT_SIZE_W              ),
    .DEFAULT_CP_LEN(DEFAULT_CP_LEN          ),
    .CP_REPEAT     (CP_REMOVAL_REPEAT       ),
    .MAX_LIST_LOG2 (MAX_CP_LIST_LEN_REM_LOG2),
    .SET_TLAST     (1                       )
  ) cp_removal_i (
    .clk             (ce_clk                  ),
    .rst             (core_rst                ),
    .clear_list      (reg_cp_rem_list_clr     ),
    .symbol_len      (fft_size                ),
    .cp_len_tdata    (reg_cp_rem_length       ),
    .cp_len_tvalid   (reg_cp_rem_list_load    ),
    // No back-pressure needed since block controller checks
    // cp_len_fifo_occupied to not overflow FIFO.
    .cp_len_tready   (                        ),
    .cp_list_occupied(reg_cp_rem_list_occupied),
    .i_tdata         (combine_out_tdata       ),
    .i_tuser         (combine_out_teob        ),
    .i_tlast         (combine_out_tlast       ),
    .i_tvalid        (combine_out_tvalid      ),
    .i_tready        (combine_out_tready      ),
    .o_tdata         (cp_removal_out_tdata    ),
    .o_tuser         (cp_removal_out_teob     ),
    .o_tlast         (cp_removal_out_tlast    ),
    .o_tvalid        (cp_removal_out_tvalid   ),
    .o_tready        (cp_removal_out_tready   )
  );

  // We can create a teov from tlast because the packet size is the same as the FFT size
  assign cp_removal_out_teov = {NUM_CHAN{cp_removal_out_tlast}};


  //---------------------------------------------------------------------------
  // Configuration State Machine
  //---------------------------------------------------------------------------

  logic [ITEM_W*NUM_CHAN-1:0] fft_data_in_tdata;
  logic                       fft_data_in_tlast;
  logic                       fft_data_in_tvalid;
  logic [       NUM_CHAN-1:0] fft_data_in_tready;

  // Loads a new configuration at the start of every FFT
  enum logic [0:0] { S_FFT_CONFIG, S_FFT_WAIT_FOR_TLAST } fft_config_state = S_FFT_CONFIG;

  always_ff @(posedge ce_clk) begin
    case (fft_config_state)
      S_FFT_CONFIG: begin
        if (fft_data_in_tvalid & fft_data_in_tready[0]) begin
          fft_config_state <= S_FFT_WAIT_FOR_TLAST;
        end
      end
      S_FFT_WAIT_FOR_TLAST: begin
        if (fft_data_in_tvalid & fft_data_in_tready[0] & fft_data_in_tlast) begin
          fft_config_state <= S_FFT_CONFIG;
        end
      end
    endcase
    if (core_rst) begin
      fft_config_state <= S_FFT_CONFIG;
    end
  end


  //---------------------------------------------------------------------------
  // FFT Configuration
  //---------------------------------------------------------------------------

  logic [FFT_CONFIG_W-1:0] fft_config_tdata;
  logic                    fft_config_tvalid;
  logic [    NUM_CHAN-1:0] fft_config_tready;

  assign fft_config_tdata = build_fft_config(
    MAX_FFT_SIZE_LOG2,
    reg_fft_scaling,
    reg_fft_direction,
    reg_fft_size_log2
  );

  assign fft_config_tvalid = (fft_config_state == S_FFT_CONFIG) ?
    fft_data_in_tvalid && fft_data_in_tready : 1'b0;


  //---------------------------------------------------------------------------
  // Sideband Info Bypass
  //---------------------------------------------------------------------------
  //
  // The Xilinx FFT IP lacks a TUSER signal so this adds one to pass through
  // our EOB and EOV signals.
  //
  //---------------------------------------------------------------------------

  logic [DATA_W*NUM_CHAN-1:0] fft_data_out_tdata;
  logic [       NUM_CHAN-1:0] fft_data_out_tlast;
  logic [       NUM_CHAN-1:0] fft_data_out_tvalid;
  logic                       fft_data_out_tready; // One bit shared by all channels

  logic [DATA_W*NUM_CHAN-1:0] split_in_tdata;
  logic [       NUM_CHAN-1:0] split_in_teob;
  logic [       NUM_CHAN-1:0] split_in_teov;
  logic                       split_in_tlast;
  logic                       split_in_tvalid;
  logic                       split_in_tready;

  // TUSER is EOB and static for the entire packet, so we can use
  // PACKET_MODE=2, which is more efficient.
  axis_sideband_tuser #(
    .WIDTH         (NUM_CHAN*DATA_W),
    .USER_WIDTH    (NUM_CHAN*2     ),
    .FIFO_SIZE_LOG2(5              ),
    .PACKET_MODE   (2              )
  ) axis_sideband_tuser_i (
    .clk              (ce_clk                                    ),
    .reset            (core_rst                                  ),
    // Input bus with a TUSER signal
    .s_axis_tdata     (cp_removal_out_tdata                      ),
    .s_axis_tuser     ({cp_removal_out_teob, cp_removal_out_teov}),
    .s_axis_tlast     (cp_removal_out_tlast                      ),
    .s_axis_tvalid    (cp_removal_out_tvalid                     ),
    .s_axis_tready    (cp_removal_out_tready                     ),
    // Input bus with TUSER removed, going to our FFT block
    .m_axis_mod_tdata (fft_data_in_tdata                         ),
    .m_axis_mod_tlast (fft_data_in_tlast                         ),
    .m_axis_mod_tvalid(fft_data_in_tvalid                        ),
    .m_axis_mod_tready(fft_data_in_tready[0]                     ),
    // Output bus from FFT block
    .s_axis_mod_tdata (fft_data_out_tdata                        ),
    .s_axis_mod_tlast (fft_data_out_tlast[0]                     ),
    .s_axis_mod_tvalid(fft_data_out_tvalid[0]                    ),
    .s_axis_mod_tready(fft_data_out_tready                       ),
    // Output bus from FFT block with TUSER added back on
    .m_axis_tdata     (split_in_tdata                            ),
    .m_axis_tuser     ({split_in_teob, split_in_teov}            ),
    .m_axis_tlast     (split_in_tlast                            ),
    .m_axis_tvalid    (split_in_tvalid                           ),
    .m_axis_tready    (split_in_tready                           )
  );


  //---------------------------------------------------------------------------
  // Cyclic Prefix Insertion List
  //---------------------------------------------------------------------------

  // Output from CP list
  logic [CP_LEN_W-1:0] cp_ins_list_tdata;
  logic                cp_ins_list_tvalid;
  logic                cp_ins_list_tready;

  logic [MAX_CP_LIST_LEN_INS_LOG2:0] cp_ins_list_occupied;
  assign reg_cp_ins_list_occupied = REG_CP_INS_LIST_OCC_WIDTH'(cp_ins_list_occupied);

  axis_cp_list #(
    .ADDR_W (MAX_CP_LIST_LEN_INS_LOG2),
    .DATA_W (CP_LEN_W                ),
    .REPEAT (1                       ),
    .DEFAULT('0                      )
  ) axis_cp_list_ins (
    .clk     (ce_clk              ),
    .rst     (core_rst            ),
    .clear   (reg_cp_ins_list_clr ),
    .i_tdata (reg_cp_ins_length   ),
    .i_tvalid(reg_cp_ins_list_load),
    .i_tready(                    ),
    .o_tdata (cp_ins_list_tdata   ),
    .o_tvalid(cp_ins_list_tvalid  ),
    .o_tready(cp_ins_list_tready  ),
    .occupied(cp_ins_list_occupied)
  );

  logic [DATA_W*NUM_CHAN-1:0] fft_mux_out_tdata;
  logic [       NUM_CHAN-1:0] fft_mux_out_tlast;
  logic [       NUM_CHAN-1:0] fft_mux_out_tvalid;
  logic [       NUM_CHAN-1:0] fft_mux_out_tready;

  logic fft_mux_out_tstart = '1; // Indicates first word transfer of packet

  always_ff @(posedge ce_clk) begin
    // Create a register that indicates when the first transfer of a packet
    // occurs (analogous to TLAST).
    if (fft_mux_out_tvalid[0] && fft_mux_out_tready[0]) begin
      fft_mux_out_tstart <= fft_mux_out_tlast[0];
    end

    if (ce_rst) begin
      fft_mux_out_tstart <= '1;
    end
  end

  // Pop off the next list item after the start of each packet.
  assign cp_ins_list_tready =
    fft_mux_out_tstart && fft_mux_out_tvalid[0] && fft_mux_out_tready[0];


  //---------------------------------------------------------------------------
  // FFT Bypass
  //---------------------------------------------------------------------------

  logic [DATA_W*NUM_CHAN-1:0] xfft_in_tdata;
  logic [       NUM_CHAN-1:0] xfft_in_tlast;
  logic [       NUM_CHAN-1:0] xfft_in_tvalid;
  logic [       NUM_CHAN-1:0] xfft_in_tready;

  logic [DATA_W*NUM_CHAN-1:0] xfft_out_tdata;
  logic [       NUM_CHAN-1:0] xfft_out_tlast;
  logic [       NUM_CHAN-1:0] xfft_out_tvalid;
  logic [       NUM_CHAN-1:0] xfft_out_tready;

  logic [DATA_W*NUM_CHAN-1:0] xfft_bypass_in_tdata;
  logic [       NUM_CHAN-1:0] xfft_bypass_in_tlast;
  logic [       NUM_CHAN-1:0] xfft_bypass_in_tvalid;
  logic [       NUM_CHAN-1:0] xfft_bypass_in_tready;

  logic [DATA_W*NUM_CHAN-1:0] xfft_bypass_out_tdata;
  logic [       NUM_CHAN-1:0] xfft_bypass_out_tlast;
  logic [       NUM_CHAN-1:0] xfft_bypass_out_tvalid;
  logic [       NUM_CHAN-1:0] xfft_bypass_out_tready;

  always_comb begin
    if (reg_fft_bypass && EN_FFT_BYPASS) begin
      // FIFO connections pass through
      xfft_bypass_in_tdata   = fft_data_in_tdata;
      xfft_bypass_in_tlast   = {NUM_CHAN{fft_data_in_tlast}};
      xfft_bypass_in_tvalid  = {NUM_CHAN{fft_data_in_tvalid}};
      fft_data_in_tready     = xfft_bypass_in_tready;
      //
      fft_mux_out_tdata      = xfft_bypass_out_tdata;
      fft_mux_out_tlast      = xfft_bypass_out_tlast;
      fft_mux_out_tvalid     = xfft_bypass_out_tvalid;
      xfft_bypass_out_tready = {NUM_CHAN{fft_mux_out_tready}};

      // Tie off bypassed FFT connections
      xfft_in_tdata         = fft_data_in_tdata;
      xfft_in_tlast         = {NUM_CHAN{fft_data_in_tlast}};
      xfft_in_tvalid        = {NUM_CHAN{1'b0}};
      xfft_out_tready       = {NUM_CHAN{1'b1}};
    end else begin
      // Tie off bypassed FIFO connections
      xfft_bypass_in_tdata   = fft_data_in_tdata;
      xfft_bypass_in_tlast   = {NUM_CHAN{fft_data_in_tlast}};
      xfft_bypass_in_tvalid  = {NUM_CHAN{1'b0}};
      xfft_bypass_out_tready = {NUM_CHAN{1'b1}};

      // FFT connections pass through like normal
      xfft_in_tdata          = fft_data_in_tdata;
      xfft_in_tlast          = {NUM_CHAN{fft_data_in_tlast}};
      xfft_in_tvalid         = {NUM_CHAN{fft_data_in_tvalid}};
      fft_data_in_tready     = xfft_in_tready;
      //
      fft_mux_out_tdata      = xfft_out_tdata;
      fft_mux_out_tlast      = xfft_out_tlast;
      fft_mux_out_tvalid     = xfft_out_tvalid;
      xfft_out_tready        = fft_mux_out_tready;
    end
  end


  //---------------------------------------------------------------------------
  // FFT Core
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0] event_frame_started;
  logic [NUM_CHAN-1:0] event_frame_tlast_unexpected;
  logic [NUM_CHAN-1:0] event_frame_tlast_missing;

  for (genvar fft_i = 0; fft_i < NUM_CHAN; fft_i = fft_i + 1) begin : gen_fft
    if (EN_FFT_BYPASS) begin : gen_bypass_fifo
      axi_fifo #(
        .WIDTH(DATA_W+1         ),
        .SIZE (MAX_FFT_SIZE_LOG2)
      ) axi_fifo_bypass (
        .clk     (ce_clk                                          ),
        .reset   (core_rst                                        ),
        .clear   (1'b0                                            ),
        .i_tdata ({ xfft_bypass_in_tlast[fft_i],
                    xfft_bypass_in_tdata[DATA_W*fft_i +: DATA_W]} ),
        .i_tvalid(xfft_bypass_in_tvalid[fft_i]                    ),
        .i_tready(xfft_bypass_in_tready[fft_i]                    ),
        .o_tdata ({ xfft_bypass_out_tlast[fft_i],
                    xfft_bypass_out_tdata[DATA_W*fft_i +: DATA_W]}),
        .o_tvalid(xfft_bypass_out_tvalid[fft_i]                   ),
        .o_tready(xfft_bypass_out_tready[fft_i]                   ),
        .space   (                                                ),
        .occupied(                                                )
      );
    end : gen_bypass_fifo

    xfft_wrapper #(
      .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2)
    ) xfft_wrapper_i (
      .aclk                       (ce_clk                               ),
      .aresetn                    (~core_rst                            ),
      .s_axis_config_tdata        (fft_config_tdata                     ),
      .s_axis_config_tvalid       (fft_config_tvalid                    ),
      .s_axis_config_tready       (fft_config_tready[fft_i]             ),
      .s_axis_data_tdata          ({ xfft_in_tdata[32*fft_i +: 16],
                                     xfft_in_tdata[32*fft_i+16 +: 16] } ),
      .s_axis_data_tlast          (xfft_in_tlast[fft_i]                 ),
      .s_axis_data_tvalid         (xfft_in_tvalid[fft_i]                ),
      .s_axis_data_tready         (xfft_in_tready[fft_i]                ),
      .m_axis_data_tdata          ({ xfft_out_tdata[32*fft_i +: 16],
                                     xfft_out_tdata[32*fft_i+16 +: 16] }),
      .m_axis_data_tuser          (                                     ),
      .m_axis_data_tlast          (xfft_out_tlast[fft_i]                ),
      .m_axis_data_tvalid         (xfft_out_tvalid[fft_i]               ),
      .m_axis_data_tready         (xfft_out_tready[fft_i]               ),
      .m_axis_status_tdata        (                                     ),
      .m_axis_status_tvalid       (                                     ),
      .m_axis_status_tready       (1'b1                                 ),
      .event_frame_started        (event_frame_started[fft_i]           ),
      .event_tlast_unexpected     (event_frame_tlast_unexpected[fft_i]  ),
      .event_tlast_missing        (event_frame_tlast_missing[fft_i]     ),
      .event_fft_overflow         (event_fft_overflow[fft_i]            ),
      .event_status_channel_halt  (                                     ),
      .event_data_in_channel_halt (                                     ),
      .event_data_out_channel_halt(                                     )
    );

    if (EN_FFT_ORDER || EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin : gen_fft_post_processing
      fft_post_processing #(
        .EN_FFT_ORDER     (EN_FFT_ORDER     ),
        .EN_MAGNITUDE     (EN_MAGNITUDE     ),
        .EN_MAGNITUDE_SQ  (EN_MAGNITUDE_SQ  ),
        .USE_APPROX_MAG   (USE_APPROX_MAG   ),
        .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2)
      ) fft_post_processing_i (
        .clk          (ce_clk                                     ),
        .rst          (core_rst                                   ),
        .fft_order_sel(reg_fft_order                              ),
        .magnitude_sel(reg_magnitude                              ),
        .fft_size_log2(reg_fft_size_log2[FFT_SIZE_LOG2_W-1:0]     ),
        .s_axis_tdata (fft_mux_out_tdata [DATA_W*fft_i +: DATA_W] ),
        .s_axis_tuser (cp_ins_list_tdata                          ),
        .s_axis_tlast (fft_mux_out_tlast [fft_i]                  ),
        .s_axis_tvalid(fft_mux_out_tvalid[fft_i]                  ),
        .s_axis_tready(fft_mux_out_tready[fft_i]                  ),
        .m_axis_tdata (fft_data_out_tdata [DATA_W*fft_i +: DATA_W]),
        .m_axis_tlast (fft_data_out_tlast [fft_i]                 ),
        .m_axis_tvalid(fft_data_out_tvalid[fft_i]                 ),
        .m_axis_tready(fft_data_out_tready                        )
      );
    end else begin : gen_no_fft_post_processing
      assign fft_data_out_tdata  = fft_mux_out_tdata;
      assign fft_data_out_tlast  = fft_mux_out_tlast;
      assign fft_data_out_tvalid = fft_mux_out_tvalid;
      assign fft_mux_out_tready  = {NUM_CHAN{fft_data_out_tready}};
    end
  end


  //---------------------------------------------------------------------------
  // Split Streams
  //---------------------------------------------------------------------------
  //
  // Take the time-aligned streams and make them independent AXI-Stream buses.
  //
  //---------------------------------------------------------------------------

  // Split back into multiple streams
  axis_split_bus #(
    .WIDTH     (DATA_W  ),
    .USER_WIDTH(2       ),
    .NUM_PORTS (NUM_CHAN)
  ) axis_split_bus_i (
    .clk          (ce_clk                        ),
    .reset        (core_rst                      ),
    .s_axis_tdata (split_in_tdata                ),
    .s_axis_tuser ({split_in_teob, split_in_teov}),
    .s_axis_tlast (split_in_tlast                ),
    .s_axis_tvalid(split_in_tvalid               ),
    .s_axis_tready(split_in_tready               ),
    .m_axis_tdata (user_out_tdata                ),
    .m_axis_tuser ({user_out_teob, user_out_teov}),
    .m_axis_tlast (user_out_tlast                ),
    .m_axis_tvalid(user_out_tvalid               ),
    .m_axis_tready(user_out_tready               )
  );

endmodule : fft_core


`default_nettype wire
