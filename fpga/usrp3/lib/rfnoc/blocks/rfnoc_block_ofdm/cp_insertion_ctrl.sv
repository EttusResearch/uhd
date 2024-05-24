//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cp_insertion_ctrl
//
// Description:
//
//   Tracks the cyclic prefix insertion for OFDM symbols. A configuration list
//   allows for queuing up multiple cyclic prefix lengths, and has an optional
//   repeat mode that causes the same list of cyclic prefixes to be reused as
//   new symbols arrive. This allows the block to execute a pattern for cases
//   when CP lengths change symbol to symbol in a repeating pattern.
//
//   This module is designed to match the behavior of axis_cp_removal, but for
//   insertion the actual insertion is handled outside this module.
//
// Parameters:
//
//   CP_LEN_W         : Width of the CP length. This determines the maximum
//                      prefix length.
//   DEFAULT_CP_LEN   : Number of samples in cyclic prefix
//   CP_REPEAT        : 1: Cyclic prefix list repeats. 0: Cyclic prefix list
//                      does not repeat, and the last used prefix length will
//                      be used once the list is completed.
//   MAX_LIST_LOG2    : Log base 2 of the size of the prefix length list
//
// Signals:
//
//   clear_list       : Clear the CP removal list
//   s_axis_cp_len    : AXI-Stream cyclic prefix length list input. Use this to
//                      write prefix lengths to the list in order.
//   cp_list_occupied : Number of items in the cyclic prefix list
//   cp_len           : The next cyclic prefix length to be inserted.
//   cp_len_ack       : A single-cycle pulse indicates cp_len has been used.
//                      There must be at least one idle cycle between acks.
//

`default_nettype none


module cp_insertion_ctrl #(
  parameter int CP_LEN_W         = 16,
  parameter int MAX_SYM_LEN_LOG2 = 16,
  parameter int DEFAULT_CP_LEN   = 0,
  parameter int CP_REPEAT        = 0,
  parameter int MAX_LIST_LOG2    = 5,
  parameter int SET_TLAST        = 1
) (
  input  wire                        clk,
  input  wire                        rst,
  input  wire                        clear_list,
  input  wire [        CP_LEN_W-1:0] s_axis_cp_len_tdata,
  input  wire                        s_axis_cp_len_tvalid,
  output wire                        s_axis_cp_len_tready,
  output wire [                15:0] cp_list_occupied,
  output wire [        CP_LEN_W-1:0] cp_len,
  input  wire                        cp_len_ack
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

  logic [        CP_LEN_W-1:0] cp_len_reg      = DEFAULT_CP_LEN;
  logic [MAX_SYM_LEN_LOG2-1:0] symbol_len_reg  = '0;
  logic [       CNT_WIDTH-1:0] cnt             = '0;
  logic                        clear_fifo_hold = 1'b0;

  always @(posedge clk) begin
    // Latch FIFO clear
    if (clear_list) begin
      clear_fifo_hold <= 1'b1;
    end

    // State machine
    case (state)
      S_IDLE : begin
        if (cp_len_ack) begin
          // Only update the CP length being used if there's a valid one in the
          // list. Otherwise, keep using the previous value.
          if (cp_len_fifo_out_tvalid) begin
            cp_len_reg <= cp_len_fifo_out_tdata;
          end
          state <= S_CONFIG;
        end else if (clear_fifo_hold) begin
          state <= S_CLEAR;
        end
      end
      S_CONFIG : begin
        // The cp_len_fifo updates during this cycle. Technically this isn't
        // required, but doing it this way means we can have identical logic
        // for the cp_removal module.
        state <= S_IDLE;
        //synthesis translate_off
        // Make sure we don't miss an ack
        assert (cp_len_ack == 1'b0) else
          $error("There must be an idle cycle between cp_len_ack pulses.");
        //synthesis translate_on
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
      state           <= S_IDLE;
    end
  end

  assign cp_len = cp_len_reg;

endmodule : cp_insertion_ctrl

`default_nettype wire
