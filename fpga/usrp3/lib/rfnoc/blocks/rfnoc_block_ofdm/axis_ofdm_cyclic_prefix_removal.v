//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ofdm_cyclic_prefix_removal
//
// Description:
//
//   Removes cyclic prefix from OFDM symbols. Configuration FIFO allows for
//   either queuing up multiple configs with an optional loopback mode that
//   writes the current config back into the FIFO. This allows the block
//   to execute a pattern, for example if CP lengths can change symbol to symbol
//   in a repeating pattern.
//
//   Note: There is a two clock bubble cycle after every OFDM symbol due to returning to
//         the idle state to load the next config, so this block must be clocked at least
//         slightly faster the sample rate, i.e. Clock rate > (Fs)*(1 + 2/(CP length + FFT Size)).
//
// Parameters:
//
//   WIDTH                  : Data / Sample AXIS bus width
//   MAX_CP_LEN_LOG2        : Log2 of maximum cyclic prefix length
//   MAX_FFT_SIZE_LOG2      : Log2 of maximum FFT size
//   DEFAULT_CP_LEN         : Number of samples in cyclic prefix
//   CP_LEN_FIFO_MODE       : 0: FIFO without loopback, 1: FIFO with loopback
//   CP_LEN_FIFO_SIZE_LOG2  : Log2 depth of the FIFO
//   SET_TLAST              : Always set tlast at the end of each symbol
//

`default_nettype none

module axis_ofdm_cyclic_prefix_removal #(
  parameter WIDTH                 = 32,
  parameter USER_WIDTH            = 1,
  parameter MAX_CP_LEN_LOG2       = 16,
  parameter MAX_FFT_SIZE_LOG2     = 16,
  parameter DEFAULT_CP_LEN        = 288,
  parameter CP_LEN_FIFO_MODE      = 0,
  parameter CP_LEN_FIFO_SIZE_LOG2 = 5,
  parameter SET_TLAST             = 1
)(
  input  wire                         clk,
  input  wire                         reset,
  input  wire                         clear_fifo, // Clears cyclic prefix length FIFO, but only when safe to do so
  input  wire [MAX_FFT_SIZE_LOG2-1:0] fft_size,
  input  wire [MAX_CP_LEN_LOG2-1:0]   m_axis_cp_len_tdata,
  input  wire                         m_axis_cp_len_tvalid,
  output wire                         m_axis_cp_len_tready,
  output wire [15:0]                  cp_len_fifo_occupied,
  input  wire [WIDTH-1:0]             s_axis_data_tdata,
  input  wire [USER_WIDTH-1:0]        s_axis_data_tuser,
  input  wire                         s_axis_data_tlast,
  input  wire                         s_axis_data_tvalid,
  output wire                         s_axis_data_tready,
  output wire [WIDTH-1:0]             m_axis_data_tdata,
  output wire [USER_WIDTH-1:0]        m_axis_data_tuser,
  output wire                         m_axis_data_tlast,
  output wire                         m_axis_data_tvalid,
  input  wire                         m_axis_data_tready
);

  localparam S_IDLE          = 3'd0;
  localparam S_CONFIG        = 3'd1;
  localparam S_CYCLIC_PREFIX = 3'd2;
  localparam S_OFDM_SYMBOL   = 3'd3;
  localparam S_CLEAR_FIFO    = 3'd4;
  reg [2:0] state;

  wire [MAX_CP_LEN_LOG2-1:0] cp_len_fifo_in_tdata, cp_len_fifo_out_tdata;
  wire                       cp_len_fifo_in_tvalid, cp_len_fifo_out_tvalid;
  wire                       cp_len_fifo_in_tready, cp_len_fifo_out_tready;

  wire clear_config_fifo = (state == S_CLEAR_FIFO);

  axi_fifo #(
    .WIDTH    (MAX_CP_LEN_LOG2),
    .SIZE     (CP_LEN_FIFO_SIZE_LOG2))
  axi_fifo_config_inst (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear_config_fifo),
    .i_tdata  (cp_len_fifo_in_tdata),
    .i_tvalid (cp_len_fifo_in_tvalid),
    .i_tready (cp_len_fifo_in_tready),
    .o_tdata  (cp_len_fifo_out_tdata),
    .o_tvalid (cp_len_fifo_out_tvalid),
    .o_tready (cp_len_fifo_out_tready),
    .space    (),
    .occupied (cp_len_fifo_occupied)
  );

  generate
    // No config fifo loopback. New configs can be written at any time.
    if (CP_LEN_FIFO_MODE == 0) begin
      assign cp_len_fifo_in_tdata   = m_axis_cp_len_tdata;
      assign cp_len_fifo_in_tvalid  = (state == S_CLEAR_FIFO) ? 1'b0 : m_axis_cp_len_tvalid;
      assign m_axis_cp_len_tready   = (state == S_CLEAR_FIFO) ? 1'b0 : cp_len_fifo_in_tready;
      assign cp_len_fifo_out_tready = (state == S_CONFIG);
    // Config fifo loopback enabled, writes current config back into config fifo in the S_CONFIG state.
    // New configs can be written in any state but S_CONFIG & S_CLEAR.
    end else begin
      assign cp_len_fifo_in_tdata   = (state == S_CONFIG)     ? cp_len_fifo_out_tdata  : m_axis_cp_len_tdata;
      assign cp_len_fifo_in_tvalid  = (state == S_CONFIG)     ? cp_len_fifo_out_tvalid :
                                      (state == S_CLEAR_FIFO) ? 1'b0 :
                                      /* Else */                m_axis_cp_len_tvalid;
      assign m_axis_cp_len_tready   = (state == S_CONFIG)     ? 1'b0 :
                                      (state == S_CLEAR_FIFO) ? 1'b0 :
                                      /* Else */                cp_len_fifo_in_tready;
      assign cp_len_fifo_out_tready = (state == S_CONFIG);
    end
  endgenerate

  // CP length should generally not be larger than the FFT size, but this calculation is just in case
  // someone tries to use this block in an unusual way
  localparam CNT_WIDTH = MAX_FFT_SIZE_LOG2 > MAX_CP_LEN_LOG2 ? MAX_FFT_SIZE_LOG2 : MAX_CP_LEN_LOG2;

  reg [MAX_CP_LEN_LOG2-1:0]   cp_len_reg      = DEFAULT_CP_LEN;
  reg [MAX_FFT_SIZE_LOG2-1:0] fft_size_reg    = 'd0;
  reg [CNT_WIDTH-1:0]         cnt             = 'd0;
  reg                         clear_fifo_hold = 1'b0;

  always @(posedge clk) begin
    // Latch FIFO clear
    if (clear_fifo) begin
      clear_fifo_hold <= 1'b1;
    end
    // State machine
    case (state)
      // Wait in idle state until either a configuration FIFO clear is requested
      // or 
      S_IDLE: begin
        cnt <= 1;
        if (clear_fifo_hold) begin
          state <= S_CLEAR_FIFO;
        end else if (s_axis_data_tvalid) begin
          // Note: Only update cyclic prefix length and fft size if new values are available
          if (cp_len_fifo_out_tvalid) begin
            cp_len_reg <= cp_len_fifo_out_tdata;
          end
          fft_size_reg <= fft_size;
          state        <= S_CONFIG;
        end
      end
      S_CONFIG: begin
        if (cp_len_reg > 0) begin
          state <= S_CYCLIC_PREFIX;
        end else if (fft_size_reg > 0) begin
          state <= S_OFDM_SYMBOL;
        end else begin
          state <= S_IDLE;
        end
      end
      S_CYCLIC_PREFIX : begin
        if (s_axis_data_tvalid & s_axis_data_tready) begin
          cnt <= cnt + 1;
          if (cnt >= cp_len_reg) begin
            cnt   <= 1;
            if (fft_size_reg > 0) begin
              state <= S_OFDM_SYMBOL;
            end else begin
              state <= S_IDLE;
            end
          end
        end
      end
      S_OFDM_SYMBOL : begin
        if (s_axis_data_tvalid & s_axis_data_tready) begin
          cnt <= cnt + 1;
          if (cnt >= fft_size_reg) begin
            cnt   <= 1;
            state <= S_IDLE;
          end
        end
      end
      S_CLEAR_FIFO : begin
        clear_fifo_hold   <= 1'b0;
        cp_len_reg        <= DEFAULT_CP_LEN;
        state             <= S_IDLE;
      end
      default : state <= S_IDLE;
    endcase
    if (reset) begin
      clear_fifo_hold   <= 1'b0;
      cp_len_reg        <= DEFAULT_CP_LEN;
      cnt               <= 1;
      state             <= S_IDLE;
    end
  end

  assign m_axis_data_tdata    = s_axis_data_tdata;
  assign m_axis_data_tuser    = s_axis_data_tuser;
  assign m_axis_data_tlast    = (SET_TLAST == 0) ? s_axis_data_tlast : (state == S_OFDM_SYMBOL) & (cnt >= fft_size_reg);
  assign m_axis_data_tvalid   = (state == S_IDLE)           ? 1'b0 :
                                (state == S_CYCLIC_PREFIX)  ? 1'b0 :
                                (state == S_OFDM_SYMBOL)    ? s_axis_data_tvalid :
                                (state == S_CLEAR_FIFO)     ? 1'b0 :
                                /* Else */                    1'b0;
  assign s_axis_data_tready   = (state == S_IDLE)           ? 1'b0 :
                                (state == S_CYCLIC_PREFIX)  ? 1'b1 :
                                (state == S_OFDM_SYMBOL)    ? m_axis_data_tready :
                                (state == S_CLEAR_FIFO)     ? 1'b0 :
                                /* Else */                    1'b0;

endmodule

`default_nettype wire
