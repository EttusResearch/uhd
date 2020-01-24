//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_to_axis_pyld_ctxt
// Description:
//   A header deframer module for CHDR data packets.
//   Accepts an input CHDR stream, and produces two output streams:
//   1) Payload, which contains the payload of the packet
//   2) Context, which contains the header info in the packet i.e.
//      CHDR header, timestamp and metadata (marked with a tuser)
//   This module also performs an optional clock crossing and data
//   width convertion from CHDR_W to a user requested width for the
//   payload bus.
//   Context and data packets are interleaved i.e. a context packet
//   will arrive before its corresponding data packet. However, if
//   context prefetching is enabled, the context for the next packet
//   might arrive before the data for the current packet has been
//   consumed. In the case of a rate reduction, this allows the module
//   to sustain a gapless stream of payload items and a bursty
//   sideband context path.
//
// Parameters:
//   - CHDR_W: Width of the input CHDR bus in bits
//   - ITEM_W: Width of the output item bus in bits
//   - NIPC: The number of output items delievered per cycle
//   - SYNC_CLKS: Are the CHDR and data clocks synchronous to each other?
//   - CONTEXT_FIFO_SIZE: FIFO size for the context path
//   - PAYLOAD_FIFO_SIZE: FIFO size for the payload path
//   - CONTEXT_PREFETCH_EN: Is context prefetching enabled?
//
// Signals:
//   - s_axis_chdr_* : Input CHDR stream (AXI-Stream)
//   - m_axis_payload_* : Output payload stream (AXI-Stream)
//   - m_axis_context_* : Output context stream (AXI-Stream)
//   - flush_* : Signals for flush control and status
//

module chdr_to_axis_pyld_ctxt #(
  parameter CHDR_W              = 256,
  parameter ITEM_W              = 32,
  parameter NIPC                = 2,
  parameter SYNC_CLKS           = 0,
  parameter CONTEXT_FIFO_SIZE   = 1,
  parameter PAYLOAD_FIFO_SIZE   = 1,
  parameter CONTEXT_PREFETCH_EN = 1
)(
  // Clock, reset and settings
  input  wire                     axis_chdr_clk,
  input  wire                     axis_chdr_rst,
  input  wire                     axis_data_clk,
  input  wire                     axis_data_rst,
  // CHDR in (AXI-Stream)
  input  wire [CHDR_W-1:0]        s_axis_chdr_tdata,
  input  wire                     s_axis_chdr_tlast,
  input  wire                     s_axis_chdr_tvalid,
  output wire                     s_axis_chdr_tready,
  // Payload stream out (AXI-Stream)
  output wire [(ITEM_W*NIPC)-1:0] m_axis_payload_tdata,
  output wire [NIPC-1:0]          m_axis_payload_tkeep,
  output wire                     m_axis_payload_tlast,
  output wire                     m_axis_payload_tvalid,
  input  wire                     m_axis_payload_tready,
  // Context stream out (AXI-Stream)
  output wire [CHDR_W-1:0]        m_axis_context_tdata,
  output wire [3:0]               m_axis_context_tuser,
  output wire                     m_axis_context_tlast,
  output wire                     m_axis_context_tvalid,
  input  wire                     m_axis_context_tready,
  // Flush signals
  input  wire                     flush_en,
  input  wire [31:0]              flush_timeout,
  output wire                     flush_active,
  output wire                     flush_done
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  // ---------------------------------------------------
  //  Pipeline
  // ---------------------------------------------------
  localparam CHDR_KEEP_W = CHDR_W/ITEM_W;

  wire [CHDR_W-1:0]       in_chdr_tdata;
  wire [CHDR_KEEP_W-1:0]  in_chdr_tkeep;
  wire                    in_chdr_tlast, in_chdr_tvalid;
  reg                     in_chdr_tready;

  axi_fifo_flop2 #(.WIDTH(CHDR_W+1)) in_pipe_i (
    .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(1'b0),
    .i_tdata({s_axis_chdr_tlast, s_axis_chdr_tdata}),
    .i_tvalid(s_axis_chdr_tvalid), .i_tready(s_axis_chdr_tready),
    .o_tdata({in_chdr_tlast, in_chdr_tdata}),
    .o_tvalid(in_chdr_tvalid), .o_tready(in_chdr_tready),
    .space(), .occupied()
  );

  chdr_compute_tkeep #(.CHDR_W(CHDR_W), .ITEM_W(ITEM_W)) tkeep_gen_i (
    .clk(axis_chdr_clk), .rst(axis_chdr_rst),
    .axis_tdata(in_chdr_tdata), .axis_tlast(in_chdr_tlast),
    .axis_tvalid(in_chdr_tvalid), .axis_tready(in_chdr_tready),
    .axis_tkeep(in_chdr_tkeep)
  );

  // ---------------------------------------------------
  //  Input State Machine
  // ---------------------------------------------------
  wire [CHDR_W-1:0]       in_pyld_tdata;
  wire [CHDR_KEEP_W-1:0]  in_pyld_tkeep;
  wire                    in_pyld_tlast, in_pyld_tvalid, in_pyld_tready;

  wire [CHDR_W-1:0] in_ctxt_tdata;
  reg  [3:0]        in_ctxt_tuser;
  wire              in_ctxt_tlast, in_ctxt_tvalid, in_ctxt_tready;


  localparam [2:0] ST_HDR   = 3'd0;   // Processing the input CHDR header
  localparam [2:0] ST_TS    = 3'd1;   // Processing the input CHDR timestamp
  localparam [2:0] ST_MDATA = 3'd2;   // Processing the input CHDR metadata word
  localparam [2:0] ST_BODY  = 3'd3;   // Processing the input CHDR payload word
  localparam [2:0] ST_DROP  = 3'd4;   // Something went wrong... Dropping packet

  reg [2:0] state = ST_HDR;
  reg [4:0] mdata_pending = CHDR_NO_MDATA;
  reg       last_ctxt_line;

  // Shortcuts: CHDR header
  wire [2:0] in_pkt_type = chdr_get_pkt_type(in_chdr_tdata[63:0]);
  wire [4:0] in_num_mdata = chdr_get_num_mdata(in_chdr_tdata[63:0]);

  always @(posedge axis_chdr_clk) begin
    if (axis_chdr_rst) begin
      state <= ST_HDR;
    end else if (in_chdr_tvalid & in_chdr_tready) begin
      case (state)

        // ST_HDR: CHDR Header
        // -------------------
        ST_HDR: begin
          // Always cache the number of metadata words
          mdata_pending <= in_num_mdata;
          // Figure out the next state
          if (!in_chdr_tlast) begin
            if (CHDR_W > 64) begin
              // When CHDR_W > 64, the timestamp is a part of the header word.
              // If this is a data packet (with/without a TS), we move on to the metadata/body
              // state otherwise we drop it. Non-data packets should never reach here.
              if (in_pkt_type == CHDR_PKT_TYPE_DATA || in_pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
                if (in_num_mdata != CHDR_NO_MDATA) begin
                  state <= ST_MDATA;
                end else begin
                  state <= ST_BODY;
                end
              end else begin
                state <= ST_DROP;
              end
            end else begin
              // When CHDR_W == 64, the timestamp comes after the header. Check if this is a data
              // packet with a TS to figure out the next state. If no TS, then check for metadata
              // to move to the next state. Drop any non-data packets.
              if (in_pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
                state <= ST_TS;
              end else if (in_pkt_type == CHDR_PKT_TYPE_DATA) begin
                if (in_num_mdata != CHDR_NO_MDATA) begin
                  state <= ST_MDATA;
                end else begin
                  state <= ST_BODY;
                end
              end else begin
                state <= ST_DROP;
              end
            end
          end else begin    // Premature termination
            // Packets must have at least one payload line
            state <= ST_HDR;
          end
        end

        // ST_TS: Timestamp (CHDR_W == 64 only)
        // ------------------------------------
        ST_TS: begin
          if (!in_chdr_tlast) begin
            if (mdata_pending != CHDR_NO_MDATA) begin
              state <= ST_MDATA;
            end else begin
              state <= ST_BODY;
            end
          end else begin    // Premature termination
            // Packets must have at least one payload line
            state <= ST_HDR;
          end
        end

        // ST_MDATA: Metadata word
        // -----------------------
        ST_MDATA: begin
          if (!in_chdr_tlast) begin
            // Count down metadata and stop at 1
            if (mdata_pending == 5'd1) begin
              state <= ST_BODY;
            end else begin
              mdata_pending <= mdata_pending - 5'd1;
            end
          end else begin    // Premature termination
            // Packets must have at least one payload line
            state <= ST_HDR;
          end
        end

        // ST_BODY: Payload word
        // ---------------------
        ST_BODY: begin
          if (in_chdr_tlast) begin
            state <= ST_HDR;
          end
        end

        // ST_DROP: Drop current packet
        // ----------------------------
        ST_DROP: begin
          if (in_chdr_tlast) begin
            state <= ST_HDR;
          end
        end

        default: begin
          // We should never get here
          state <= ST_HDR;
        end
      endcase
    end
  end

  // CHDR data goes to the payload stream only in the BODY state.
  // Packets are expected to have at least one payload word so the
  // CHDR tlast can be used as the payload tlast
  assign in_pyld_tdata  = in_chdr_tdata;
  assign in_pyld_tkeep  = in_chdr_tkeep;
  assign in_pyld_tlast  = in_chdr_tlast;
  assign in_pyld_tvalid = in_chdr_tvalid && (state == ST_BODY);

  // CHDR data goes to the context stream in the HDR,TS,MDATA state.
  // tlast has to be recomputed for the context stream, however, we 
  // still need to correctly handle an errant packet without a payload
  assign in_ctxt_tdata  = in_chdr_tdata;
  assign in_ctxt_tlast  = in_chdr_tlast || last_ctxt_line;
  assign in_ctxt_tvalid = in_chdr_tvalid && (state != ST_BODY && state != ST_DROP);

  always @(*) begin
    case (state)
      ST_HDR: begin
        // The header goes to the context stream
        in_chdr_tready <= in_ctxt_tready;
        in_ctxt_tuser  <= (CHDR_W > 64) ? CONTEXT_FIELD_HDR_TS : CONTEXT_FIELD_HDR;
        last_ctxt_line <= (in_num_mdata == 7'd0) && (
                           in_pkt_type == CHDR_PKT_TYPE_DATA || 
                          (in_pkt_type == CHDR_PKT_TYPE_DATA_TS && CHDR_W > 64));
      end
      ST_TS: begin
        // The timestamp goes to the context stream
        in_chdr_tready <= in_ctxt_tready;
        in_ctxt_tuser  <= CONTEXT_FIELD_TS;
        last_ctxt_line <= (mdata_pending == CHDR_NO_MDATA);
      end
      ST_MDATA: begin
        // The metadata goes to the context stream
        in_chdr_tready <= in_ctxt_tready;
        in_ctxt_tuser  <= CONTEXT_FIELD_MDATA;
        last_ctxt_line <= (mdata_pending == 5'd1);
      end
      ST_BODY: begin
        // The body goes to the payload stream
        in_chdr_tready <= in_pyld_tready;
        in_ctxt_tuser  <= 4'h0;
        last_ctxt_line <= 1'b0;
      end
      ST_DROP: begin
        // Errant packets get dropped
        in_chdr_tready <= 1'b1;
        in_ctxt_tuser  <= 4'h0;
        last_ctxt_line <= 1'b0;
      end
      default: begin
        in_chdr_tready <= 1'b0;
        in_ctxt_tuser  <= 4'h0;
        last_ctxt_line <= 1'b0;
      end
    endcase
  end

  // ---------------------------------------------------
  //  Payload and Context FIFOs
  // ---------------------------------------------------
  wire [CHDR_W-1:0]       out_pyld_tdata;
  wire [CHDR_KEEP_W-1:0]  out_pyld_tkeep;
  wire                    out_pyld_tlast, out_pyld_tvalid, out_pyld_tready;

  wire tmp_ctxt_tvalid, tmp_ctxt_tready;

  wire [(ITEM_W*NIPC)-1:0] flush_pyld_tdata;
  wire [NIPC-1:0]          flush_pyld_tkeep;
  wire                     flush_pyld_tlast, flush_pyld_tvalid, flush_pyld_tready;
  wire [CHDR_W-1:0]        flush_ctxt_tdata;
  wire [3:0]               flush_ctxt_tuser;
  wire                     flush_ctxt_tlast, flush_ctxt_tvalid, flush_ctxt_tready;

  generate if (SYNC_CLKS) begin : gen_sync_fifo
    axi_fifo #(.WIDTH(CHDR_W+4+1), .SIZE(CONTEXT_FIFO_SIZE)) ctxt_fifo_i (
      .clk(axis_data_clk), .reset(axis_data_rst), .clear(1'b0),
      .i_tdata({in_ctxt_tlast, in_ctxt_tuser, in_ctxt_tdata}),
      .i_tvalid(in_ctxt_tvalid), .i_tready(in_ctxt_tready),
      .o_tdata({flush_ctxt_tlast, flush_ctxt_tuser, flush_ctxt_tdata}),
      .o_tvalid(tmp_ctxt_tvalid), .o_tready(tmp_ctxt_tready),
      .space(), .occupied()
    );
    axi_fifo #(.WIDTH(CHDR_W+CHDR_KEEP_W+1), .SIZE(PAYLOAD_FIFO_SIZE)) pyld_fifo_i (
      .clk(axis_data_clk), .reset(axis_data_rst), .clear(1'b0),
      .i_tdata({in_pyld_tlast, in_pyld_tkeep, in_pyld_tdata}),
      .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
      .o_tdata({out_pyld_tlast, out_pyld_tkeep, out_pyld_tdata}),
      .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready),
      .space(), .occupied()
    );
  end else begin : gen_async_fifo
    axi_fifo_2clk #(.WIDTH(CHDR_W+4+1), .SIZE(CONTEXT_FIFO_SIZE)) ctxt_fifo_i (
      .reset(axis_chdr_rst),
      .i_aclk(axis_chdr_clk),
      .i_tdata({in_ctxt_tlast, in_ctxt_tuser, in_ctxt_tdata}),
      .i_tvalid(in_ctxt_tvalid), .i_tready(in_ctxt_tready),
      .o_aclk(axis_data_clk),
      .o_tdata({flush_ctxt_tlast, flush_ctxt_tuser, flush_ctxt_tdata}),
      .o_tvalid(tmp_ctxt_tvalid), .o_tready(tmp_ctxt_tready)
    );
    axi_fifo_2clk #(.WIDTH(CHDR_W+CHDR_KEEP_W+1), .SIZE(PAYLOAD_FIFO_SIZE)) pyld_fifo_i (
      .reset(axis_chdr_rst),
      .i_aclk(axis_chdr_clk),
      .i_tdata({in_pyld_tlast, in_pyld_tkeep, in_pyld_tdata}),
      .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
      .o_aclk(axis_data_clk),
      .o_tdata({out_pyld_tlast, out_pyld_tkeep, out_pyld_tdata}),
      .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready)
    );
  end endgenerate

  // ---------------------------------------------------
  //  Data Width Converter: CHDR_W => ITEM_W*NIPC
  // ---------------------------------------------------
  wire tmp_pyld_tvalid, tmp_pyld_tready;

  axis_width_conv #(
    .WORD_W(ITEM_W), .IN_WORDS(CHDR_W/ITEM_W), .OUT_WORDS(NIPC),
    .SYNC_CLKS(1), .PIPELINE("NONE")
  ) payload_width_conv_i (
    .s_axis_aclk(axis_data_clk), .s_axis_rst(axis_data_rst),
    .s_axis_tdata(out_pyld_tdata), .s_axis_tkeep(out_pyld_tkeep),
    .s_axis_tlast(out_pyld_tlast), .s_axis_tvalid(out_pyld_tvalid),
    .s_axis_tready(out_pyld_tready),
    .m_axis_aclk(axis_data_clk), .m_axis_rst(axis_data_rst),
    .m_axis_tdata(flush_pyld_tdata), .m_axis_tkeep(flush_pyld_tkeep),
    .m_axis_tlast(flush_pyld_tlast), .m_axis_tvalid(tmp_pyld_tvalid),
    .m_axis_tready(tmp_pyld_tready)
  );

  // ---------------------------------------------------
  //  Output State Machine
  // ---------------------------------------------------
  reg [2:0] ctxt_pkt_cnt = 3'd0, pyld_pkt_cnt = 3'd0;

  // A payload packet can pass only if it is preceeded by a context packet
  wire pass_pyld = ((ctxt_pkt_cnt - pyld_pkt_cnt) > 3'd0);
  // A context packet has to be blocked if its corresponding payload packet hasn't passed except
  // when prefetching is enabled. In that case one additional context packet is allowed to pass
  wire pass_ctxt = ((ctxt_pkt_cnt - pyld_pkt_cnt) < (CONTEXT_PREFETCH_EN == 1 ? 3'd2 : 3'd1));

  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      ctxt_pkt_cnt <= 3'd0;
      pyld_pkt_cnt <= 3'd0;
    end else begin
      if (flush_ctxt_tvalid && flush_ctxt_tready && flush_ctxt_tlast)
        ctxt_pkt_cnt <= ctxt_pkt_cnt + 3'd1;
      if (flush_pyld_tvalid && flush_pyld_tready && flush_pyld_tlast)
        pyld_pkt_cnt <= pyld_pkt_cnt + 3'd1;
    end
  end

  assign flush_pyld_tvalid = tmp_pyld_tvalid && pass_pyld;
  assign tmp_pyld_tready = flush_pyld_tready && pass_pyld;

  assign flush_ctxt_tvalid = tmp_ctxt_tvalid && pass_ctxt;
  assign tmp_ctxt_tready = flush_ctxt_tready && pass_ctxt;

  // ---------------------------------------------------
  //  Flushing Logic
  // ---------------------------------------------------
  wire [31:0] flush_timeout_dclk;
  wire        flush_en_dclk;
  wire        flush_active_pyld_cclk, flush_active_ctxt_cclk;
  wire        flush_done_pyld_cclk, flush_done_ctxt_cclk;
  wire        flush_active_pyld, flush_active_ctxt;
  wire        flush_done_pyld, flush_done_ctxt;

  synchronizer #(.WIDTH(4), .INITIAL_VAL(4'd0)) flush_2clk_rb_i (
    .clk(axis_chdr_clk), .rst(1'b0),
    .in({flush_active_pyld, flush_done_pyld,
         flush_active_ctxt, flush_done_ctxt}),
    .out({flush_active_pyld_cclk, flush_done_pyld_cclk,
         flush_active_ctxt_cclk, flush_done_ctxt_cclk})
  );
  assign flush_active = flush_active_pyld_cclk | flush_active_ctxt_cclk;
  assign flush_done = flush_done_pyld_cclk & flush_done_ctxt_cclk;

  axi_fifo_2clk #(.WIDTH(33), .SIZE(1)) flush_2clk_ctrl_i (
    .reset(axis_chdr_rst),
    .i_aclk(axis_chdr_clk),
    .i_tdata({flush_en, flush_timeout}), .i_tvalid(1'b1), .i_tready(),
    .o_aclk(axis_data_clk),
    .o_tdata({flush_en_dclk, flush_timeout_dclk}), .o_tvalid(), .o_tready(1'b1)
  );

  axis_packet_flush #(
    .WIDTH((ITEM_W+1)*NIPC), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("OUT")
  ) pyld_flusher_i (
    .clk(axis_data_clk), .reset(axis_data_rst),
    .enable(flush_en_dclk), .timeout(flush_timeout_dclk),
    .flushing(flush_active_pyld), .done(flush_done_pyld),
    .s_axis_tdata({flush_pyld_tkeep, flush_pyld_tdata}),
    .s_axis_tlast(flush_pyld_tlast),
    .s_axis_tvalid(flush_pyld_tvalid),
    .s_axis_tready(flush_pyld_tready),
    .m_axis_tdata({m_axis_payload_tkeep, m_axis_payload_tdata}),
    .m_axis_tlast(m_axis_payload_tlast),
    .m_axis_tvalid(m_axis_payload_tvalid),
    .m_axis_tready(m_axis_payload_tready)
  );

  axis_packet_flush #(
    .WIDTH(CHDR_W+4), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("OUT")
  ) ctxt_flusher_i (
    .clk(axis_data_clk), .reset(axis_data_rst),
    .enable(flush_en_dclk), .timeout(flush_timeout_dclk),
    .flushing(flush_active_ctxt), .done(flush_done_ctxt),
    .s_axis_tdata({flush_ctxt_tuser, flush_ctxt_tdata}),
    .s_axis_tlast(flush_ctxt_tlast),
    .s_axis_tvalid(flush_ctxt_tvalid),
    .s_axis_tready(flush_ctxt_tready),
    .m_axis_tdata({m_axis_context_tuser, m_axis_context_tdata}),
    .m_axis_tlast(m_axis_context_tlast),
    .m_axis_tvalid(m_axis_context_tvalid),
    .m_axis_tready(m_axis_context_tready)
  );

endmodule // chdr_to_axis_pyld_ctxt
