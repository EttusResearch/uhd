//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cp_removal
//
// Description:
//
//   Removes the cyclic prefix from OFDM symbols. A configuration list allows
//   for queuing up multiple cyclic prefix lengths, and has an optional repeat
//   mode that causes the same list of cyclic prefixes to be reused as new
//   symbols arrive. This allows the block to execute a pattern for cases when
//   CP lengths change symbol to symbol in a repeating pattern.
//
//   This module is designed to match the behavior of cp_insertion_ctrl, but
//   for removal the actual prefix removal is handled inside this module.
//
//   There is a two-clock bubble cycle after every symbol due to returning to
//   the idle state to load the next config, so this block must be clocked at
//   least slightly faster the sample rate. That is:
//
//     Clock rate > Fs * (1 + 2/(CP length + FFT Size))
//
// Parameters:
//
//   DATA_W           : Data/sample AXI-Stream bus width
//   USER_W           : Width of TUSER on the data/sample AXI-Stream bus
//   CP_LEN_W         : Width of the CP length. This determines the maximum
//                      prefix length.
//   MAX_SYM_LEN_LOG2 : Log2 of maximum FFT size
//   DEFAULT_CP_LEN   : Number of samples in cyclic prefix
//   CP_REPEAT        : 1: Cyclic prefix list repeats. 0: Cyclic prefix list
//                      does not repeat, and the last used prefix length will
//                      be used once the list is completed.
//   MAX_LIST_LOG2    : Log base 2 of the size of the prefix length list
//   SET_TLAST        : Always set tlast at the end of each symbol
//
// Signals:
//
//   clear_list       : Clear the CP removal list
//   symbol_len       : Symbol/FFT size to use for generating TLAST
//   s_axis_cp_len    : AXI-Stream cyclic prefix length list input. Use this to
//                      write prefix lengths to the list in order.
//   cp_list_occupied : Number of items in the cyclic prefix list
//   s_axis_data      : AXI-Stream data input on which to do cyclic prefix removal
//   m_axis_data      : AXI-Stream data output with cyclic prefix removed
//

`default_nettype none


module cp_removal #(
  parameter DATA_W           = 32,
  parameter USER_W           = 1,
  parameter CP_LEN_W         = 16,
  parameter MAX_SYM_LEN_LOG2 = 16,
  parameter DEFAULT_CP_LEN   = 0,
  parameter CP_REPEAT        = 0,
  parameter MAX_LIST_LOG2    = 5,
  parameter SET_TLAST        = 1
) (
  input  wire                        clk,
  input  wire                        rst,
  input  wire                        clear_list,
  input  wire [MAX_SYM_LEN_LOG2-1:0] symbol_len,
  input  wire [        CP_LEN_W-1:0] s_axis_cp_len_tdata,
  input  wire                        s_axis_cp_len_tvalid,
  output wire                        s_axis_cp_len_tready,
  output wire [                15:0] cp_list_occupied,
  input  wire [          DATA_W-1:0] s_axis_data_tdata,
  input  wire [          USER_W-1:0] s_axis_data_tuser,
  input  wire                        s_axis_data_tlast,
  input  wire                        s_axis_data_tvalid,
  output wire                        s_axis_data_tready,
  output wire [          DATA_W-1:0] m_axis_data_tdata,
  output wire [          USER_W-1:0] m_axis_data_tuser,
  output wire                        m_axis_data_tlast,
  output wire                        m_axis_data_tvalid,
  input  wire                        m_axis_data_tready
);

  enum logic [2:0] { S_IDLE, S_CONFIG, S_PREFIX, S_SYMBOL, S_CLEAR } state;

  logic [CP_LEN_W-1:0] cp_len_fifo_in_tdata, cp_len_fifo_out_tdata;
  logic                cp_len_fifo_in_tvalid, cp_len_fifo_out_tvalid;
  logic                cp_len_fifo_in_tready, cp_len_fifo_out_tready;
  logic clear_config_fifo;

  assign clear_config_fifo = (state == S_CLEAR);

  axi_fifo #(
    .WIDTH(CP_LEN_W),
    .SIZE (MAX_LIST_LOG2)
  ) axi_fifo_config_inst (
    .clk     (clk),
    .reset   (rst),
    .clear   (clear_config_fifo),
    .i_tdata (cp_len_fifo_in_tdata),
    .i_tvalid(cp_len_fifo_in_tvalid),
    .i_tready(cp_len_fifo_in_tready),
    .o_tdata (cp_len_fifo_out_tdata),
    .o_tvalid(cp_len_fifo_out_tvalid),
    .o_tready(cp_len_fifo_out_tready),
    .space   (),
    .occupied(cp_list_occupied)
  );

  generate
    if (CP_REPEAT == 0) begin
      // No config list loopback. New configs can be written at any time.
      assign cp_len_fifo_in_tdata   = s_axis_cp_len_tdata;
      assign cp_len_fifo_in_tvalid  = (state == S_CLEAR) ? 1'b0 : s_axis_cp_len_tvalid;
      assign s_axis_cp_len_tready   = (state == S_CLEAR) ? 1'b0 : cp_len_fifo_in_tready;
      assign cp_len_fifo_out_tready = (state == S_CONFIG);
    end else begin
      // Config list loopback enabled. Write current config back into config
      // FIFO in the S_CONFIG state. New configs can be written in any state
      // but S_CONFIG & S_CLEAR.
      assign cp_len_fifo_in_tdata   = (state == S_CONFIG) ? cp_len_fifo_out_tdata  :
                                                            s_axis_cp_len_tdata;
      assign cp_len_fifo_in_tvalid  = (state == S_CONFIG) ? cp_len_fifo_out_tvalid :
                                      (state == S_CLEAR)  ? 1'b0 :
                                                            s_axis_cp_len_tvalid;
      assign s_axis_cp_len_tready   = (state == S_CONFIG) ? 1'b0 :
                                      (state == S_CLEAR)  ? 1'b0 :
                                                           cp_len_fifo_in_tready;
      assign cp_len_fifo_out_tready = (state == S_CONFIG);
    end
  endgenerate

  // CP length should generally not be larger than the FFT size, but this
  // calculation is just in case someone tries to use this block in an unusual
  // way.
  localparam CNT_WIDTH = MAX_SYM_LEN_LOG2 > CP_LEN_W ?
    MAX_SYM_LEN_LOG2 : CP_LEN_W;

  reg [        CP_LEN_W-1:0] cp_len_reg      = DEFAULT_CP_LEN;
  reg [MAX_SYM_LEN_LOG2-1:0] symbol_len_reg  = '0;
  reg [       CNT_WIDTH-1:0] cnt             = '0;
  reg                        clear_fifo_hold = 1'b0;

  always @(posedge clk) begin
    // Latch FIFO clear
    if (clear_list) begin
      clear_fifo_hold <= 1'b1;
    end

    // State machine
    case (state)
      // Wait in idle state until either a configuration list clear is
      // requested or we get a new data input.
      S_IDLE : begin
        cnt <= 1;
        if (clear_fifo_hold) begin
          state <= S_CLEAR;
        end else if (s_axis_data_tvalid) begin
          // Only update the CP length being used if there's a valid one in the
          // list. Otherwise, keep using the previous value.
          if (cp_len_fifo_out_tvalid) begin
            cp_len_reg <= cp_len_fifo_out_tdata;
          end
          symbol_len_reg <= symbol_len;
          state          <= S_CONFIG;
        end
      end
      S_CONFIG : begin
        if (cp_len_reg > 0) begin
          state <= S_PREFIX;
        end else if (symbol_len_reg > 0) begin
          state <= S_SYMBOL;
        end else begin
          state <= S_IDLE;
        end
      end
      S_PREFIX : begin
        if (s_axis_data_tvalid & s_axis_data_tready) begin
          cnt <= cnt + 1;
          if (cnt >= cp_len_reg) begin
            cnt <= 1;
            if (symbol_len_reg > 0) begin
              state <= S_SYMBOL;
            end else begin
              state <= S_IDLE;
            end
          end
        end
      end
      S_SYMBOL : begin
        if (s_axis_data_tvalid & s_axis_data_tready) begin
          cnt <= cnt + 1;
          if (cnt >= symbol_len_reg) begin
            cnt   <= 1;
            state <= S_IDLE;
          end
        end
      end
      S_CLEAR : begin
        clear_fifo_hold <= 1'b0;
        cp_len_reg      <= DEFAULT_CP_LEN;
        state           <= S_IDLE;
      end
      default : state <= S_IDLE;
    endcase

    if (rst) begin
      clear_fifo_hold <= 1'b0;
      cp_len_reg      <= DEFAULT_CP_LEN;
      cnt             <= 1;
      state           <= S_IDLE;
    end
  end

  logic new_tlast;
  assign new_tlast = (state == S_SYMBOL) & (cnt >= symbol_len_reg);

  assign m_axis_data_tdata  = s_axis_data_tdata;
  assign m_axis_data_tuser  = s_axis_data_tuser;
  assign m_axis_data_tlast  = (SET_TLAST == 0)    ? s_axis_data_tlast :
                                                    new_tlast;
  assign m_axis_data_tvalid = (state == S_IDLE)   ? 1'b0 :
                              (state == S_PREFIX) ? 1'b0 :
                              (state == S_SYMBOL) ? s_axis_data_tvalid :
                              (state == S_CLEAR)  ? 1'b0 :
                                                    1'b0;
  assign s_axis_data_tready = (state == S_IDLE)   ? 1'b0 :
                              (state == S_PREFIX) ? 1'b1 :
                              (state == S_SYMBOL) ? m_axis_data_tready :
                              (state == S_CLEAR)  ? 1'b0 :
                                                    1'b0;

endmodule : cp_removal

`default_nettype wire
