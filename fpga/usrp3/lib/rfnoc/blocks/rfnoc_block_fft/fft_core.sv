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
//   NIPC                     : Number of items/samples per clock cycle on each
//                              channel. It must be a power of 2.
//   NUM_CHAN                 : Number of channels to instantiate on this
//                              fft_core instance.
//   NUM_CORES                : Total number of fft_core instances in the
//                              parent RFNoC block, including this one.
//   MAX_PKT_SIZE_LOG2        : Log2 of maximum RFNoC packet size. Actual
//                              max is 2**MAX_PKT_SIZE_LOG2 items.
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size. Actual
//                              max is 2**MAX_FFT_SIZE_LOG2 items.
//   EN_CP_INSERTION          : Controls whether to include the cyclic prefix
//                              insertion logic. If included, EN_FFT_ORDER must
//                              be 1.
//   EN_CP_REMOVAL            : Controls whether to include the cyclic prefix
//                              removal logic.
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
  int NIPC                     = 1,
  int NUM_CHAN                 = 1,
  int NUM_CORES                = 1,
  int MAX_PKT_SIZE_LOG2        = 14,
  int MAX_FFT_SIZE_LOG2        = 12,
  int MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int MAX_CP_LIST_LEN_REM_LOG2 = 5,
  bit EN_CP_REMOVAL            = 1,
  bit EN_CP_INSERTION          = 1,
  bit CP_INSERTION_REPEAT      = 1,
  bit CP_REMOVAL_REPEAT        = 1,
  bit EN_FFT_BYPASS            = 1,
  bit EN_FFT_ORDER             = 1,
  bit EN_MAGNITUDE             = 1,
  bit EN_MAGNITUDE_SQ          = 1,
  bit USE_APPROX_MAG           = 1,

  // Data width of each FFT channel
  localparam int ITEM_W = 32,
  localparam int DATA_W = NIPC*ITEM_W,
  localparam int KEEP_W = NIPC
) (
  input  wire                                       ce_clk,
  input  wire                                       ce_rst,

  // CtrlPort Register Interface
  input  wire                                       s_ctrlport_req_wr,
  input  wire                                       s_ctrlport_req_rd,
  input  wire  [               CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  wire  [               CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output logic                                      s_ctrlport_resp_ack,
  output logic [               CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // Data Input Packets
  input  wire  [NUM_CHAN-1:0][          DATA_W-1:0] s_in_axis_tdata,
  input  wire  [NUM_CHAN-1:0][          KEEP_W-1:0] s_in_axis_tkeep,
  input  wire  [NUM_CHAN-1:0][                 0:0] s_in_axis_tlast,
  input  wire  [NUM_CHAN-1:0][                 0:0] s_in_axis_tvalid,
  output logic [NUM_CHAN-1:0][                 0:0] s_in_axis_tready,
  input  wire  [NUM_CHAN-1:0][CHDR_TIMESTAMP_W-1:0] s_in_axis_ttimestamp,
  input  wire  [NUM_CHAN-1:0][                 0:0] s_in_axis_thas_time,
  input  wire  [NUM_CHAN-1:0][   CHDR_LENGTH_W-1:0] s_in_axis_tlength,
  input  wire  [NUM_CHAN-1:0][                 0:0] s_in_axis_teov,
  input  wire  [NUM_CHAN-1:0][                 0:0] s_in_axis_teob,

  // Data Output Packets
  output wire  [NUM_CHAN-1:0][          DATA_W-1:0] m_out_axis_tdata,
  output wire  [NUM_CHAN-1:0][          KEEP_W-1:0] m_out_axis_tkeep,
  output wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_tlast,
  output wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_tvalid,
  input  wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_tready,
  output wire  [NUM_CHAN-1:0][CHDR_TIMESTAMP_W-1:0] m_out_axis_ttimestamp,
  output wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_thas_time,
  output wire  [NUM_CHAN-1:0][   CHDR_LENGTH_W-1:0] m_out_axis_tlength,
  output wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_teov,
  output wire  [NUM_CHAN-1:0][                 0:0] m_out_axis_teob
);
  // Import utilities for working with Xilinx FFT block
  import xfft_config_pkg::*;

  // Import register descriptions
  import fft_core_regs_pkg::*;

  // Import FFT packetizer definitions
  import fft_packetize_pkg::*;

  `include "usrp_utils.svh"

  //---------------------------------------------------------------------------
  // Check Parameters
  //---------------------------------------------------------------------------

  if (EN_CP_INSERTION && !EN_FFT_ORDER) begin : gen_insertion_assertion
    // The cyclic prefix insertion logic is implemented in the FFT reorder
    // buffer, so that must also be included. This is done to avoid having two
    // separate buffers for storing the current FFT data (one for CP insertion
    // and the other for reordering the data).
    $error("If EN_CP_INSERTION is enabled then EN_FFT_ORDER must also be enabled");
  end

  if (NIPC != 2**$clog2(NIPC)) begin : gen_nipc_assertion
    // NIPC must be a power of 2. This is currently a requirement of the
    // packetizer, and also the fact that we store log2 of the NIPC in the
    // capabilities register to save bits.
    $error("NIPC must be a power of 2");
  end


  //---------------------------------------------------------------------------
  // FFT Configuration Interface Constants
  //---------------------------------------------------------------------------

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
  localparam CP_LEN_W        = MAX_CP_LEN_LOG2;

  localparam int REG_LENGTH_LOG2_WIDTH = FFT_NFFT_W;
  localparam int REG_SCALING_WIDTH     = FFT_SCALE_W;
  localparam int REG_CP_INS_LEN_WIDTH  = CP_LEN_W;
  localparam int REG_CP_REM_LEN_WIDTH  = CP_LEN_W;

  localparam bit [REG_COMPAT_WIDTH-1:0] COMPAT  = {16'h03, 16'h01};
  localparam bit [REG_CAPABILITIES_WIDTH-1:0] CAPABILITIES = {
    8'(MAX_CP_LIST_LEN_INS_LOG2),
    8'(MAX_CP_LIST_LEN_REM_LOG2),
    8'(MAX_CP_LEN_LOG2),
    8'(MAX_FFT_SIZE_LOG2)
  };
  localparam bit [REG_CAPABILITIES2_WIDTH-1:0] CAPABILITIES2 = {
    4'($clog2(NIPC)),
    2'(0),
    1'(EN_CP_INSERTION),
    1'(EN_CP_REMOVAL),
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
  localparam int DEFAULT_FFT_DIRECTION = FFT_FORWARD;
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
  // Cyclic Prefix Insertion List
  //---------------------------------------------------------------------------

  logic [CP_LEN_W-1:0] cp_ins_list_tdata;
  logic                cp_ins_list_tvalid;
  logic                cp_ins_list_tready;

  if (EN_CP_INSERTION) begin : gen_cp_insertion_list
    logic [MAX_CP_LIST_LEN_INS_LOG2:0] cp_ins_list_occupied;
    assign reg_cp_ins_list_occupied = REG_CP_INS_LIST_OCC_WIDTH'(cp_ins_list_occupied);

    axis_cp_list #(
      .ADDR_W (MAX_CP_LIST_LEN_INS_LOG2),
      .DATA_W (CP_LEN_W                ),
      .REPEAT (CP_INSERTION_REPEAT     ),
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
  end else begin : gen_no_cp_insertion_list
    assign cp_ins_list_tdata        = '0;
    assign cp_ins_list_tvalid       = '0;
    assign reg_cp_ins_list_occupied = '0;
  end


  //---------------------------------------------------------------------------
  // Cyclic Prefix Removal List
  //---------------------------------------------------------------------------

  logic [CP_LEN_W-1:0] cp_rem_list_tdata;
  logic                cp_rem_list_tvalid;
  logic                cp_rem_list_tready;

  if (EN_CP_REMOVAL) begin : gen_cp_removal_list
    logic [MAX_CP_LIST_LEN_REM_LOG2:0] cp_rem_list_occupied;
    assign reg_cp_rem_list_occupied = REG_CP_REM_LIST_OCC_WIDTH'(cp_rem_list_occupied);

    axis_cp_list #(
      .ADDR_W (MAX_CP_LIST_LEN_REM_LOG2),
      .DATA_W (CP_LEN_W                ),
      .REPEAT (CP_REMOVAL_REPEAT       ),
      .DEFAULT('0                      )
    ) axis_cp_list_rem (
      .clk     (ce_clk              ),
      .rst     (core_rst            ),
      .clear   (reg_cp_rem_list_clr ),
      .i_tdata (reg_cp_rem_length   ),
      .i_tvalid(reg_cp_rem_list_load),
      .i_tready(                    ),
      .o_tdata (cp_rem_list_tdata   ),
      .o_tvalid(cp_rem_list_tvalid  ),
      .o_tready(cp_rem_list_tready  ),
      .occupied(cp_rem_list_occupied)
    );
  end else begin : gen_no_cp_removal_list
    assign cp_rem_list_tdata        = '0;
    assign cp_rem_list_tvalid       = '0;
    assign reg_cp_rem_list_occupied = '0;
  end


  //---------------------------------------------------------------------------
  // Bypass Demultiplexer
  //---------------------------------------------------------------------------
  //
  // This logic splits the data flow, allowing us to select whether to send the
  // data through the FFT logic or bypass it entirely. This is useful for
  // debugging.
  //
  //---------------------------------------------------------------------------

  // Create a constant for the width of TUSER for holding all the NoC shell
  // sideband data.
  localparam COMBINE_USER_W = CHDR_TIMESTAMP_W + 1 + CHDR_LENGTH_W + 2;

  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] s_in_axis_tuser;

  logic [NUM_CHAN-1:0][        DATA_W-1:0] combine_in_tdata;
  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] combine_in_tuser;
  logic [NUM_CHAN-1:0]                     combine_in_tlast;
  logic [NUM_CHAN-1:0]                     combine_in_tvalid;
  logic [NUM_CHAN-1:0]                     combine_in_tready;

  logic [NUM_CHAN-1:0][        DATA_W-1:0] bypass_tdata;
  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] bypass_tuser;
  logic [NUM_CHAN-1:0]                     bypass_tlast;
  logic [NUM_CHAN-1:0]                     bypass_tvalid;
  logic [NUM_CHAN-1:0]                     bypass_tready;

  for (genvar ch_i = 0; ch_i < NUM_CHAN; ch_i++) begin : gen_for_bypass_demux
    assign s_in_axis_tuser[ch_i] = {
      s_in_axis_ttimestamp[ch_i],
      s_in_axis_thas_time[ch_i],
      s_in_axis_tlength[ch_i],
      s_in_axis_teov[ch_i],
      s_in_axis_teob[ch_i]
    };

    if (EN_FFT_BYPASS) begin : gen_fft_bypass_demux
      axi_demux #(
        .WIDTH         (COMBINE_USER_W+DATA_W),
        .SIZE          (2                    ),
        .PRE_FIFO_SIZE (1                    ),
        .POST_FIFO_SIZE(1                    )
      ) axi_demux_i (
        .clk     (ce_clk                                            ),
        .reset   (core_rst                                          ),
        .clear   (1'b0                                              ),
        .header  (                                                  ),
        .dest    (reg_fft_bypass                                    ),
        .i_tdata ({s_in_axis_tuser [ch_i], s_in_axis_tdata  [ch_i]} ),
        .i_tlast (s_in_axis_tlast  [ch_i]                           ),
        .i_tvalid(s_in_axis_tvalid [ch_i]                           ),
        .i_tready(s_in_axis_tready [ch_i]                           ),
        .o_tdata ({{bypass_tuser   [ch_i], bypass_tdata     [ch_i]},
                  {combine_in_tuser[ch_i], combine_in_tdata [ch_i]}}),
        .o_tlast ({bypass_tlast    [ch_i], combine_in_tlast [ch_i]} ),
        .o_tvalid({bypass_tvalid   [ch_i], combine_in_tvalid[ch_i]} ),
        .o_tready({bypass_tready   [ch_i], combine_in_tready[ch_i]} )
      );
    end else begin : gen_no_fft_bypass_demux
      assign combine_in_tdata [ch_i] = s_in_axis_tdata  [ch_i];
      assign combine_in_tuser [ch_i] = s_in_axis_tuser  [ch_i];
      assign combine_in_tlast [ch_i] = s_in_axis_tlast  [ch_i];
      assign combine_in_tvalid[ch_i] = s_in_axis_tvalid [ch_i];
      assign s_in_axis_tready [ch_i] = combine_in_tready[ch_i];
    end
  end


  //---------------------------------------------------------------------------
  // Combine Streams
  //---------------------------------------------------------------------------
  //
  // Combine input channels into one AXI-Stream bus. This lets us run the FFT
  // instances (one per channel) in lock-step by sharing valid/ready signals.
  // This allows us to share logic between channels.
  //
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0][          DATA_W-1:0] combine_out_tdata;
  logic [NUM_CHAN-1:0][  COMBINE_USER_W-1:0] combine_out_tuser;
  logic                                      combine_out_tlast;
  logic                                      combine_out_tvalid;
  logic                                      combine_out_tready;
  logic               [CHDR_TIMESTAMP_W-1:0] combine_out_ttimestamp;
  logic                                      combine_out_thas_time;
  logic               [   CHDR_LENGTH_W-1:0] combine_out_tlength;
  logic                                      combine_out_teov;
  logic                                      combine_out_teob;

  if (NUM_CHAN > 1) begin : gen_combine
    axis_combine #(
      .SIZE           (NUM_CHAN),
      .WIDTH          (DATA_W),
      .USER_WIDTH     (COMBINE_USER_W),
      .FIFO_SIZE_LOG2 (1)
    ) axis_combine_i (
      .clk            (ce_clk),
      .reset          (core_rst),
      .s_axis_tdata   (combine_in_tdata  ),
      .s_axis_tuser   (combine_in_tuser  ),
      .s_axis_tlast   (combine_in_tlast  ),
      .s_axis_tvalid  (combine_in_tvalid ),
      .s_axis_tready  (combine_in_tready ),
      .m_axis_tdata   (combine_out_tdata ),
      .m_axis_tuser   (combine_out_tuser ),
      .m_axis_tlast   (combine_out_tlast ),
      .m_axis_tvalid  (combine_out_tvalid),
      .m_axis_tready  (combine_out_tready)
    );
  end else begin : gen_no_combine
    assign combine_out_tdata  = combine_in_tdata;
    assign combine_out_tuser  = combine_in_tuser;
    assign combine_out_tlast  = combine_in_tlast;
    assign combine_out_tvalid = combine_in_tvalid;
    assign combine_in_tready  = combine_out_tready;
  end

  // We only need the sideband for the first channel, since they're all being
  // synchronized.
  assign {
    combine_out_ttimestamp,
    combine_out_thas_time,
    combine_out_tlength,
    combine_out_teov,
    combine_out_teob
  } = combine_out_tuser[0][COMBINE_USER_W-1:0];


  //---------------------------------------------------------------------------
  // Convert RFNoC Packets to FFT Packets
  //---------------------------------------------------------------------------

  logic [CP_LEN_W-1:0] cp_ins_fft_tdata;
  logic                cp_ins_fft_tvalid;
  logic                cp_ins_fft_tready;

  logic [CP_LEN_W-1:0] cp_rem_fft_tdata;
  logic                cp_rem_fft_tvalid;
  logic                cp_rem_fft_tready;

  logic [NUM_CHAN-1:0][DATA_W-1:0] noc_to_fft_tdata;
  logic                            noc_to_fft_tlast;
  logic                            noc_to_fft_tvalid;
  logic                            noc_to_fft_tready;

  burst_info_t burst_tdata;
  logic        burst_tvalid;
  logic        burst_tready;

  symbol_info_t symbol_tdata;
  logic         symbol_tvalid;
  logic         symbol_tready;

  // Make the symbol FIFO large enough to hold the maximum number of symbols
  // that could be in flight, which is a bit more than the maximum packet size
  // full of the smallest FFT size (8). But make it at least 32 (SRL FIFO size).
  localparam int SYMB_FIFO_SIZE_LOG2 = `MAX(5, $clog2(2**MAX_PKT_SIZE_LOG2 / 8)+1);

  fft_packetize #(
    .ITEM_W              (ITEM_W             ),
    .NIPC                (NIPC               ),
    .NUM_CHAN            (NUM_CHAN           ),
    .EN_CP_REMOVAL       (EN_CP_REMOVAL      ),
    .MAX_PKT_SIZE_LOG2   (MAX_PKT_SIZE_LOG2  ),
    .MAX_FFT_SIZE_LOG2   (MAX_FFT_SIZE_LOG2  ),
    .DATA_FIFO_SIZE_LOG2 (-1                 ),
    .CP_FIFO_SIZE_LOG2   (5                  ),
    .BURST_FIFO_SIZE_LOG2(5                  ),
    .SYMB_FIFO_SIZE_LOG2 (SYMB_FIFO_SIZE_LOG2)
  ) fft_packetize_i (
    .clk             (ce_clk                ),
    .rst             (core_rst              ),
    .fft_size_log2   (fft_size_log2         ),
    .i_cp_rem_tdata  (cp_rem_list_tdata     ),
    .i_cp_rem_tvalid (cp_rem_list_tvalid    ),
    .i_cp_rem_tready (cp_rem_list_tready    ),
    .o_cp_rem_tdata  (cp_rem_fft_tdata      ),
    .o_cp_rem_tvalid (cp_rem_fft_tvalid     ),
    .o_cp_rem_tready (cp_rem_fft_tready     ),
    .i_noc_tdata     (combine_out_tdata     ),
    .i_noc_tkeep     ('1                    ),
    .i_noc_tlast     (combine_out_tlast     ),
    .i_noc_tvalid    (combine_out_tvalid    ),
    .i_noc_tready    (combine_out_tready    ),
    .i_noc_ttimestamp(combine_out_ttimestamp),
    .i_noc_thas_time (combine_out_thas_time ),
    .i_noc_tlength   (combine_out_tlength   ),
    .i_noc_teov      (1'b0                  ),
    .i_noc_teob      (combine_out_teob      ),
    .o_fft_tdata     (noc_to_fft_tdata      ),
    .o_fft_tkeep     (                      ),
    .o_fft_tlast     (noc_to_fft_tlast      ),
    .o_fft_tvalid    (noc_to_fft_tvalid     ),
    .o_fft_tready    (noc_to_fft_tready     ),
    .o_burst_tdata   (burst_tdata           ),
    .o_burst_tvalid  (burst_tvalid          ),
    .o_burst_tready  (burst_tready          ),
    .o_symbol_tdata  (symbol_tdata          ),
    .o_symbol_tvalid (symbol_tvalid         ),
    .o_symbol_tready (symbol_tready         )
  );


  //---------------------------------------------------------------------------
  // FFT Processing
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0][DATA_W-1:0] fft_data_in_tdata;
  logic [NUM_CHAN-1:0][       0:0] fft_data_in_tlast;
  logic [NUM_CHAN-1:0][       0:0] fft_data_in_tvalid;
  logic [NUM_CHAN-1:0][       0:0] fft_data_in_tready;

  logic [NUM_CHAN-1:0][DATA_W-1:0] fft_data_out_tdata;
  logic [NUM_CHAN-1:0][       0:0] fft_data_out_tlast;
  logic [NUM_CHAN-1:0][       0:0] fft_data_out_tvalid;
  logic [NUM_CHAN-1:0][       0:0] fft_data_out_tready;

  logic [NUM_CHAN-1:0] array_cp_rem_fft_tready;
  logic [NUM_CHAN-1:0] array_cp_ins_fft_tready;

  logic [FFT_CONFIG_W-1:0] fft_config;

  assign fft_config = build_fft_config(
    MAX_FFT_SIZE_LOG2,
    reg_fft_scaling,
    reg_fft_direction,
    reg_fft_size_log2
  );

  // All channels should be perfectly synchronized, so use the tready and
  // tvalid from channel 0.
  assign cp_rem_fft_tready  = array_cp_rem_fft_tready[0];
  assign cp_ins_fft_tready  = array_cp_ins_fft_tready[0];

  assign noc_to_fft_tready  = fft_data_in_tready[0];
  assign fft_data_in_tdata  = noc_to_fft_tdata;
  assign fft_data_in_tvalid = {NUM_CHAN{noc_to_fft_tvalid}};
  assign fft_data_in_tlast  = {NUM_CHAN{noc_to_fft_tlast}};

  for (genvar ch_i = 0; ch_i < NUM_CHAN; ch_i = ch_i + 1) begin : gen_fft
    fft_pipeline_wrapper #(
      .NIPC             (NIPC             ),
      .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2),
      .EN_CONFIG_FIFO   (0                ),
      .EN_CP_REMOVAL    (EN_CP_REMOVAL    ),
      .EN_CP_INSERTION  (EN_CP_INSERTION  ),
      .EN_FFT_ORDER     (EN_FFT_ORDER     ),
      .EN_MAGNITUDE     (EN_MAGNITUDE     ),
      .EN_MAGNITUDE_SQ  (EN_MAGNITUDE_SQ  ),
      .USE_APPROX_MAG   (USE_APPROX_MAG   )
    ) fft_pipeline_wrapper_i (
      .clk               (ce_clk                       ),
      .rst               (ce_rst                       ),
      .fft_order         (reg_fft_order                ),
      .magnitude         (reg_magnitude                ),
      .fft_config        (fft_config                   ),
      .fft_size_log2     (fft_size_log2                ),
      .fft_config_tdata  ('0                           ),
      .fft_config_tvalid (1'b0                         ),
      .fft_config_tready (                             ),
      .cp_rem_tdata      (cp_rem_fft_tdata             ),
      .cp_rem_tvalid     (cp_rem_fft_tvalid            ),
      .cp_rem_tready     (array_cp_rem_fft_tready[ch_i]),
      .cp_ins_tdata      (cp_ins_fft_tdata             ),
      .cp_ins_tvalid     (cp_ins_fft_tvalid            ),
      .cp_ins_tready     (array_cp_ins_fft_tready[ch_i]),
      .event_fft_overflow(event_fft_overflow     [ch_i]),
      .i_tdata           (fft_data_in_tdata      [ch_i]),
      .i_tlast           (fft_data_in_tlast      [ch_i]),
      .i_tvalid          (fft_data_in_tvalid     [ch_i]),
      .i_tready          (fft_data_in_tready     [ch_i]),
      .o_tdata           (fft_data_out_tdata     [ch_i]),
      .o_tlast           (fft_data_out_tlast     [ch_i]),
      .o_tvalid          (fft_data_out_tvalid    [ch_i]),
      .o_tready          (fft_data_out_tready    [ch_i])
    );
  end : gen_fft


  //---------------------------------------------------------------------------
  // Convert FFT Packets to RFNoC packets
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0][DATA_W-1:0] split_in_tdata;
  logic                            split_in_tlast;
  logic                            split_in_tvalid;
  logic                            split_in_tready;
  logic [    CHDR_TIMESTAMP_W-1:0] split_in_ttimestamp;
  logic                            split_in_thas_time;
  logic [       CHDR_LENGTH_W-1:0] split_in_tlength;
  logic                            split_in_teov;
  logic                            split_in_teob;

  logic [NUM_CHAN-1:0][DATA_W-1:0] fft_to_noc_tdata;
  logic                            fft_to_noc_tlast;
  logic                            fft_to_noc_tvalid;
  logic                            fft_to_noc_tready;

  // All channels should be perfectly synchronized, so use the tready and
  // tvalid from channel 0.
  assign fft_to_noc_tdata    = fft_data_out_tdata;
  assign fft_to_noc_tlast    = fft_data_out_tlast[0];
  assign fft_to_noc_tvalid   = fft_data_out_tvalid[0];
  assign fft_data_out_tready = {NUM_CHAN{fft_to_noc_tready}};

  // With the current packtize/depacketize design, we must have at least one
  // packet worth of buffer between the packetizer and the depacketizer. So,
  // this buffer can be removed if it's guaranteed that FFT logic can buffer a
  // whole packet. This is the case when the FFT size is always bigger than the
  // packet size, or if the reorder buffer is used and twice the FFT size is
  // bigger than the packet size. Since we can't know what FFT size and packet
  // size the user will use, we assume the worst and add a maximum-packet-sized
  // buffer here.
  localparam int DATA_FIFO_SIZE_LOG2 = MAX_PKT_SIZE_LOG2;

  fft_depacketize #(
    .ITEM_W             (ITEM_W             ),
    .NIPC               (NIPC               ),
    .NUM_CHAN           (NUM_CHAN           ),
    .EN_CP_INSERTION    (EN_CP_INSERTION    ),
    .MAX_PKT_SIZE_LOG2  (MAX_PKT_SIZE_LOG2  ),
    .MAX_FFT_SIZE_LOG2  (MAX_FFT_SIZE_LOG2  ),
    .DATA_FIFO_SIZE_LOG2(DATA_FIFO_SIZE_LOG2),
    .CP_FIFO_SIZE_LOG2  (5                  ),
    .SYMB_FIFO_SIZE_LOG2(5                  ),
    .EN_TIME_ALL_PKTS   (1                  )
  ) fft_depacketize (
    .clk             (ce_clk             ),
    .rst             (core_rst           ),
    .fft_size_log2   (fft_size_log2      ),
    .i_burst_tdata   (burst_tdata        ),
    .i_burst_tvalid  (burst_tvalid       ),
    .i_burst_tready  (burst_tready       ),
    .i_symbol_tdata  (symbol_tdata       ),
    .i_symbol_tvalid (symbol_tvalid      ),
    .i_symbol_tready (symbol_tready      ),
    .i_cp_ins_tdata  (cp_ins_list_tdata  ),
    .i_cp_ins_tvalid (cp_ins_list_tvalid ),
    .i_cp_ins_tready (cp_ins_list_tready ),
    .o_cp_ins_tdata  (cp_ins_fft_tdata   ),
    .o_cp_ins_tvalid (cp_ins_fft_tvalid  ),
    .o_cp_ins_tready (cp_ins_fft_tready  ),
    .i_fft_tdata     (fft_to_noc_tdata   ),
    .i_fft_tkeep     ('1                 ),
    .i_fft_tlast     (fft_to_noc_tlast   ),
    .i_fft_tvalid    (fft_to_noc_tvalid  ),
    .i_fft_tready    (fft_to_noc_tready  ),
    .o_noc_tdata     (split_in_tdata     ),
    .o_noc_tkeep     (                   ),
    .o_noc_tlast     (split_in_tlast     ),
    .o_noc_tvalid    (split_in_tvalid    ),
    .o_noc_tready    (split_in_tready    ),
    .o_noc_ttimestamp(split_in_ttimestamp),
    .o_noc_thas_time (split_in_thas_time ),
    .o_noc_tlength   (split_in_tlength   ),
    .o_noc_teov      (split_in_teov      ),
    .o_noc_teob      (split_in_teob      )
  );


  //---------------------------------------------------------------------------
  // Split Streams
  //---------------------------------------------------------------------------
  //
  // Take the time-aligned streams and make them independent AXI-Stream buses.
  //
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] split_in_tuser;

  logic [NUM_CHAN-1:0][        DATA_W-1:0] split_out_tdata;
  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] split_out_tuser;
  logic [NUM_CHAN-1:0][               0:0] split_out_tlast;
  logic [NUM_CHAN-1:0][               0:0] split_out_tvalid;
  logic [NUM_CHAN-1:0][               0:0] split_out_tready;

  // We replicate the sideband for the first channel, since they're all
  // synchronized.
  assign split_in_tuser = { NUM_CHAN {
    split_in_ttimestamp,
    split_in_thas_time,
    split_in_tlength,
    split_in_teov,
    split_in_teob
  }};

  if (NUM_CHAN > 1) begin : gen_split
    // Split back into multiple streams
    axis_split_bus #(
      .WIDTH     (DATA_W        ),
      .USER_WIDTH(COMBINE_USER_W),
      .NUM_PORTS (NUM_CHAN      )
    ) axis_split_bus_i (
      .clk          (ce_clk          ),
      .reset        (core_rst        ),
      .s_axis_tdata (split_in_tdata  ),
      .s_axis_tuser (split_in_tuser  ),
      .s_axis_tlast (split_in_tlast  ),
      .s_axis_tvalid(split_in_tvalid ),
      .s_axis_tready(split_in_tready ),
      .m_axis_tdata (split_out_tdata ),
      .m_axis_tuser (split_out_tuser ),
      .m_axis_tlast (split_out_tlast ),
      .m_axis_tvalid(split_out_tvalid),
      .m_axis_tready(split_out_tready)
    );
  end else begin : gen_no_split
    assign split_out_tdata  = split_in_tdata;
    assign split_out_tuser  = split_in_tuser;
    assign split_out_tlast  = split_in_tlast;
    assign split_out_tvalid = split_in_tvalid;
    assign split_in_tready  = split_out_tready;
  end


  //---------------------------------------------------------------------------
  // Bypass Multiplexer
  //---------------------------------------------------------------------------
  //
  // This selects between the FFT logic output path and the bypass path.
  //
  //---------------------------------------------------------------------------

  logic [NUM_CHAN-1:0][COMBINE_USER_W-1:0] m_out_axis_tuser;

  for (genvar ch_i = 0; ch_i < NUM_CHAN; ch_i++) begin : gen_for_bypass_mux
    if (EN_FFT_BYPASS) begin : gen_fft_bypass_mux
      axi_mux #(
        .PRIO          (1                    ),
        .WIDTH         (COMBINE_USER_W+DATA_W),
        .SIZE          (2                    ),
        .PRE_FIFO_SIZE (1                    ),
        .POST_FIFO_SIZE(1                    )
      ) axi_demux_i (
        .clk     (ce_clk                                          ),
        .reset   (core_rst                                        ),
        .clear   (1'b0                                            ),
        .i_tdata ({{bypass_tuser   [ch_i], bypass_tdata    [ch_i]},
                  {split_out_tuser [ch_i], split_out_tdata [ch_i]}}),
        .i_tlast ({bypass_tlast    [ch_i], split_out_tlast [ch_i]}),
        .i_tvalid({bypass_tvalid   [ch_i], split_out_tvalid[ch_i]}),
        .i_tready({bypass_tready   [ch_i], split_out_tready[ch_i]}),
        .o_tdata ({m_out_axis_tuser[ch_i], m_out_axis_tdata[ch_i]}),
        .o_tlast (m_out_axis_tlast [ch_i]                         ),
        .o_tvalid(m_out_axis_tvalid[ch_i]                         ),
        .o_tready(m_out_axis_tready[ch_i]                         )
      );
    end else begin : gen_no_fft_bypass_mux
      assign m_out_axis_tdata  = split_out_tdata;
      assign m_out_axis_tuser  = split_out_tuser;
      assign m_out_axis_tlast  = split_out_tlast;
      assign m_out_axis_tvalid = split_out_tvalid;
      assign split_out_tready  = m_out_axis_tready;
    end

    assign m_out_axis_tkeep[ch_i] = '1;

    assign {
      m_out_axis_ttimestamp[ch_i],
      m_out_axis_thas_time[ch_i],
      m_out_axis_tlength[ch_i],
      m_out_axis_teov[ch_i],
      m_out_axis_teob[ch_i]
    } = m_out_axis_tuser[ch_i];
  end : gen_for_bypass_mux


endmodule : fft_core


`default_nettype wire
