//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Drop packets that are larger or smaller than the allowed packet size.
//

module axi_drop_partial_packet #(
  parameter WIDTH = 32,
  parameter MAX_PKT_SIZE = 1024,
  parameter HOLD_LAST_WORD = 0,       // Hold off sending last word until next full packet arrives
  parameter SR_PKT_SIZE_ADDR = 1,
  parameter DEFAULT_PKT_SIZE = 1
)(
  input clk, input reset, input clear,
  input flush,  // If using HOLD_LAST_WORD, will forcibly release all words in FIFO
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  generate
    // Packet size of 1 means it is impossible to form a partial packet, so this module does nothing...
    if (MAX_PKT_SIZE == 1) begin
      assign o_tdata  = i_tdata;
      assign o_tlast  = i_tlast;
      assign o_tvalid = i_tvalid;
      assign i_tready = o_tready;
    // All other packet sizes
    end else begin
      // Settings register
      wire [$clog2(MAX_PKT_SIZE+1)-1:0] sr_pkt_size;
      setting_reg #(.my_addr(SR_PKT_SIZE_ADDR), .width($clog2(MAX_PKT_SIZE+1)), .at_reset(DEFAULT_PKT_SIZE)) set_pkt_size (
        .clk(clk), .rst(reset), .strobe(set_stb), .addr(set_addr), .in(set_data),
        .out(sr_pkt_size), .changed());

      // Do not change n unless block is not active
      reg active;
      reg [$clog2(MAX_PKT_SIZE+1)-1:0] pkt_size = 1;
      always @(posedge clk) begin
        if (reset | clear) begin
          active <= 1'b0;
        end else begin
          if (i_tready & i_tvalid) begin
            active <= 1'b1;
          end
        end
        if (clear | ~active) begin
          pkt_size <= (sr_pkt_size == 0) ? 1 : sr_pkt_size;
        end
      end

      wire [WIDTH-1:0] int_tdata;
      wire int_tlast, int_tvalid, int_tready;
      wire i_tlast_int, i_terror;

      reg small_pkt, large_pkt;
      wire hold_last_sample;
      reg release_last;
      reg [$clog2(MAX_PKT_SIZE+1)-1:0] in_cnt;
      reg [15:0] in_pkt_cnt, in_pkt_cnt_hold, out_pkt_cnt;
      always @(posedge clk) begin
        if (reset | clear) begin
          small_pkt             <= 1'b0;
          large_pkt             <= 1'b0;
          release_last          <= 1'b0;
          in_cnt                <= 1;
          in_pkt_cnt            <= 0;
          in_pkt_cnt_hold       <= 0;
          out_pkt_cnt           <= 0;
        end else begin
          if (i_tvalid & i_tready) begin
            if (in_cnt == pkt_size | i_tlast_int) begin
              in_cnt     <= 1;
            end else begin
              in_cnt     <= in_cnt + 1;
            end
          end
          if (pkt_size == 1) begin
            small_pkt      <= 1'b0;
            large_pkt      <= 1'b0;
          end else begin
            if (i_tvalid & i_tready) begin
              if ((in_cnt == pkt_size-1'b1) & ~i_tlast) begin
                small_pkt  <= 1'b0;
              end else begin
                small_pkt  <= 1'b1;
              end
              if ((in_cnt == pkt_size) & ~i_tlast) begin
                large_pkt  <= 1'b1;
              end
              if (large_pkt) begin
                large_pkt  <= 1'b0;
              end
            end
          end
          if (i_tvalid & i_tready & i_tlast & ~i_terror) begin
            in_pkt_cnt     <= in_pkt_cnt + 1'b1;
          end
          if (int_tvalid & int_tready & int_tlast & ~hold_last_sample) begin
            out_pkt_cnt    <= out_pkt_cnt + 1'b1;
          end
          if ((i_tvalid & i_tready & i_terror) | flush) begin
            release_last         <= 1'b1;
            in_pkt_cnt_hold      <= in_pkt_cnt;
          end else if (in_pkt_cnt_hold == out_pkt_cnt) begin
            release_last         <= 1'b0;
          end
        end
      end

      assign hold_last_sample = ((in_pkt_cnt == out_pkt_cnt) | ((in_pkt_cnt == out_pkt_cnt+1) & ~release_last)) & (pkt_size != 1);

      assign i_tlast_int = i_tlast | large_pkt;
      assign i_terror    = i_tlast & i_tvalid & (small_pkt | large_pkt);

      // FIFO with ability to rewind write pointer back if input packet is flagged as bad
      axi_packet_gate #(.WIDTH(WIDTH+1), .SIZE($clog2(MAX_PKT_SIZE+1)), .USE_AS_BUFF(1)) pkt_gate_i (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tlast(i_tlast_int), .i_terror(i_terror), .i_tready(i_tready),
        .o_tdata({int_tlast,int_tdata}), .o_tvalid(int_tvalid), .o_tlast(), .o_tready(int_tready & ~(hold_last_sample & int_tlast)));

      // Generate output register to hold on to last word
      if (HOLD_LAST_WORD) begin
        axi_fifo_flop2 #(.WIDTH(WIDTH+1)) axi_fifo_flop2 (
          .clk(clk), .reset(reset), .clear(clear),
          .i_tdata({int_tlast,int_tdata}), .i_tvalid(int_tvalid & ~(hold_last_sample & int_tlast)), .i_tready(int_tready),
          .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
          .space(), .occupied());
      end else begin
        assign o_tdata    = int_tdata;
        assign o_tlast    = int_tlast;
        assign o_tvalid   = int_tvalid;
        assign int_tready = o_tready;
      end
    end
  endgenerate

endmodule
