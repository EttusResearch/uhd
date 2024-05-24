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
//   There is a two-clock bubble cycle after every symbol due to returning to
//   the idle state to load the next config, so this block must be clocked at
//   least slightly faster than the sample rate. That is:
//
//     Clock rate > Fs * (1 + 2/(CP length + FFT Size))
//
// Parameters:
//
//   DATA_W           : Data/sample AXI-Stream bus width
//   USER_W           : Width of TUSER on the data/sample AXI-Stream bus
//   SYM_LEN_W        : Width of the maximum symbol length. The maximum
//                      supported symbol length is 2**SYM_LEN_W - 1.
//   CP_LEN_W         : Width of the maximum cyclic prefix length. The maximum
//                      supported CP length is 2**CP_LEN_W - 1.
//   DEFAULT_CP_LEN   : Default cyclic prefix length to output
//   CP_REPEAT        : 1: Cyclic prefix list repeats. 0: Cyclic prefix list
//                      does not repeat, and the last used prefix length will
//                      be used once the list is completed.
//   MAX_LIST_LOG2    : Log base 2 of the size of the prefix length list
//   SET_TLAST        : 1: Always set tlast at the end of each symbol. 0: Pass
//                      through input tlast unchanged.
//
// Signals:
//
//   clear_list       : Clear the CP removal list
//   symbol_len       : Symbol/FFT size to use for generating TLAST
//   cp_len_t*        : AXI-Stream cyclic prefix length list input. Use this to
//                      write prefix lengths to the list in order.
//   cp_list_occupied : Number of items in the cyclic prefix list
//   i_t*             : AXI-Stream data input on which to do cyclic prefix removal
//   o_t*             : AXI-Stream data output with cyclic prefix removed
//

`default_nettype none


module cp_removal #(
  parameter int DATA_W           = 32,
  parameter int USER_W           = 1,
  parameter int CP_LEN_W         = 16,
  parameter int SYM_LEN_W        = 17,
  parameter int DEFAULT_CP_LEN   = 0,
  parameter bit CP_REPEAT        = 0,
  parameter int MAX_LIST_LOG2    = 5,
  parameter bit SET_TLAST        = 1
) (
  input  wire                 clk,
  input  wire                 rst,
  input  wire                 clear_list,

  // Cyclic prefix length input port
  input  wire [SYM_LEN_W-1:0] symbol_len,
  input  wire [ CP_LEN_W-1:0] cp_len_tdata,
  input  wire                 cp_len_tvalid,
  output wire                 cp_len_tready,
  output wire [         15:0] cp_list_occupied,

  // Symbol data stream input
  input  wire [   DATA_W-1:0] i_tdata,
  input  wire [   USER_W-1:0] i_tuser,
  input  wire                 i_tlast,
  input  wire                 i_tvalid,
  output wire                 i_tready,

  // Symbol data stream output
  output wire [   DATA_W-1:0] o_tdata,
  output wire [   USER_W-1:0] o_tuser,
  output wire                 o_tlast,
  output wire                 o_tvalid,
  input  wire                 o_tready
);
  `include "usrp_utils.svh"

  enum logic [2:0] { S_IDLE, S_CONFIG, S_PREFIX, S_SYMBOL, S_CLEAR } state;

  logic [CP_LEN_W-1:0] fifo_in_tdata,  fifo_out_tdata;
  logic                fifo_in_tvalid, fifo_out_tvalid;
  logic                fifo_in_tready, fifo_out_tready;
  logic                fifo_clear;

  assign fifo_clear = (state == S_CLEAR);

  axi_fifo #(
    .WIDTH(CP_LEN_W),
    .SIZE (MAX_LIST_LOG2)
  ) axi_fifo_config_inst (
    .clk     (clk),
    .reset   (rst),
    .clear   (fifo_clear),
    .i_tdata (fifo_in_tdata),
    .i_tvalid(fifo_in_tvalid),
    .i_tready(fifo_in_tready),
    .o_tdata (fifo_out_tdata),
    .o_tvalid(fifo_out_tvalid),
    .o_tready(fifo_out_tready),
    .space   (),
    .occupied(cp_list_occupied)
  );

  generate
    if (CP_REPEAT == 0) begin
      // No config list loopback. New configs can be written at any time.
      assign fifo_in_tdata   = cp_len_tdata;
      assign fifo_in_tvalid  = (state == S_CLEAR) ? 1'b0 : cp_len_tvalid;
      assign cp_len_tready   = (state == S_CLEAR) ? 1'b0 : fifo_in_tready;
      assign fifo_out_tready = (state == S_CONFIG);
    end else begin
      // Config list loopback enabled. Write current config back into config
      // FIFO in the S_CONFIG state. New configs can be written in any state
      // but S_CONFIG & S_CLEAR.
      assign fifo_in_tdata   = (state == S_CONFIG) ? fifo_out_tdata  :
                                                     cp_len_tdata;
      assign fifo_in_tvalid  = (state == S_CONFIG) ? fifo_out_tvalid :
                               (state == S_CLEAR)  ? 1'b0 :
                                                     cp_len_tvalid;
      assign cp_len_tready   = (state == S_CONFIG) ? 1'b0 :
                               (state == S_CLEAR)  ? 1'b0 :
                                                     fifo_in_tready;
      assign fifo_out_tready = (state == S_CONFIG);
    end
  endgenerate

  localparam COUNT_W = `MAX(SYM_LEN_W, CP_LEN_W);

  logic [ CP_LEN_W-1:0] cp_len_reg      = DEFAULT_CP_LEN;
  logic [SYM_LEN_W-1:0] symbol_len_reg  = '0;
  logic [  COUNT_W-1:0] count           = '0;
  logic                 clear_fifo_hold = 1'b0;

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
        count <= 1;
        if (clear_fifo_hold) begin
          state <= S_CLEAR;
        end else if (i_tvalid) begin
          // Only update the CP length being used if there's a valid one in the
          // list. Otherwise, keep using the previous value.
          if (fifo_out_tvalid) begin
            cp_len_reg <= fifo_out_tdata;
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
        if (i_tvalid & i_tready) begin
          count <= count + 1;
          if (count >= cp_len_reg) begin
            count <= 1;
            if (symbol_len_reg > 0) begin
              state <= S_SYMBOL;
            end else begin
              state <= S_IDLE;
            end
          end
        end
      end
      S_SYMBOL : begin
        if (i_tvalid & i_tready) begin
          count <= count + 1;
          if (count >= symbol_len_reg) begin
            count   <= 1;
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
      count           <= 1;
      state           <= S_IDLE;
    end
  end

  logic new_tlast;
  assign new_tlast = (state == S_SYMBOL) & (count >= symbol_len_reg);

  assign o_tdata  = i_tdata;
  assign o_tuser  = i_tuser;
  assign o_tlast  = (SET_TLAST == 0)    ? i_tlast : new_tlast;
  assign o_tvalid = (state == S_IDLE)   ? 1'b0 :
                    (state == S_PREFIX) ? 1'b0 :
                    (state == S_SYMBOL) ? i_tvalid :
                    (state == S_CLEAR)  ? 1'b0 :
                                          1'b0;
  assign i_tready = (state == S_IDLE)   ? 1'b0 :
                    (state == S_PREFIX) ? 1'b1 :
                    (state == S_SYMBOL) ? o_tready :
                    (state == S_CLEAR)  ? 1'b0 :
                                          1'b0;

endmodule : cp_removal

`default_nettype wire
