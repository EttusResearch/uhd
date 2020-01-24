//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_pyld_ctxt_to_chdr
// Description:
//   A header framer module for CHDR data packets.
//   Accepts an input payload and context stream, and produces an
//   output CHDR stream.
//   This module also performs an optional clock crossing and data
//   width convertion from a user requested width for the
//   payload bus to CHDR_W.
//   Context and data packets must be interleaved i.e. a context packet
//   must arrive before its corresponding data packet. However, if
//   context prefetching is enabled, the context for the next packet
//   may arrive before the data for the current packet has been
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
//   - MTU: Log2 of the maximum packet size in words
//   - CONTEXT_PREFETCH_EN: Is context prefetching enabled?
//
// Signals:
//   - s_axis_payload_* : Input payload stream (AXI-Stream)
//   - s_axis_context_* : Input context stream (AXI-Stream)
//   - s_axis_chdr_* : Output CHDR stream (AXI-Stream)
//   - framer_errors : Number of framer errors (dropped packets)
//   - flush_* : Signals for flush control and status
//

module axis_pyld_ctxt_to_chdr #(
  parameter CHDR_W              = 256,
  parameter ITEM_W              = 32,
  parameter NIPC                = 2,
  parameter SYNC_CLKS           = 0,
  parameter CONTEXT_FIFO_SIZE   = 1,
  parameter PAYLOAD_FIFO_SIZE   = 1,
  parameter MTU                 = 9,
  parameter CONTEXT_PREFETCH_EN = 1
)(
  // Clock, reset and settings
  input  wire                     axis_chdr_clk,
  input  wire                     axis_chdr_rst,
  input  wire                     axis_data_clk,
  input  wire                     axis_data_rst,
  // CHDR in (AXI-Stream)
  output wire [CHDR_W-1:0]        m_axis_chdr_tdata,
  output wire                     m_axis_chdr_tlast,
  output wire                     m_axis_chdr_tvalid,
  input  wire                     m_axis_chdr_tready,
  // Payload stream out (AXI-Stream)
  input  wire [(ITEM_W*NIPC)-1:0] s_axis_payload_tdata,
  input  wire [NIPC-1:0]          s_axis_payload_tkeep,
  input  wire                     s_axis_payload_tlast,
  input  wire                     s_axis_payload_tvalid,
  output wire                     s_axis_payload_tready,
  // Context stream out (AXI-Stream)
  input  wire [CHDR_W-1:0]        s_axis_context_tdata,
  input  wire [3:0]               s_axis_context_tuser,
  input  wire                     s_axis_context_tlast,
  input  wire                     s_axis_context_tvalid,
  output wire                     s_axis_context_tready,
  // Status
  output reg  [31:0]              framer_errors,
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
  //  Intput State Machine
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
      if (s_axis_context_tvalid && s_axis_context_tready && s_axis_context_tlast)
        ctxt_pkt_cnt <= ctxt_pkt_cnt + 3'd1;
      if (s_axis_payload_tvalid && s_axis_payload_tready && s_axis_payload_tlast)
        pyld_pkt_cnt <= pyld_pkt_cnt + 3'd1;
    end
  end

  wire tmp_ctxt_tvalid, tmp_ctxt_tready;
  wire tmp_pyld_tvalid, tmp_pyld_tready;

  assign tmp_ctxt_tvalid       = s_axis_context_tvalid && pass_ctxt;
  assign tmp_pyld_tvalid       = s_axis_payload_tvalid && pass_pyld;
  assign s_axis_context_tready = tmp_ctxt_tready       && pass_ctxt;
  assign s_axis_payload_tready = tmp_pyld_tready       && pass_pyld;

  // ---------------------------------------------------
  //  Data Width Converter: ITEM_W*NIPC => CHDR_W
  // ---------------------------------------------------
  wire [CHDR_W-1:0] in_pyld_tdata;
  wire              in_pyld_tlast;
  wire              in_pyld_tvalid;
  wire              in_pyld_tready;

  axis_width_conv #(
    .WORD_W(ITEM_W), .IN_WORDS(NIPC), .OUT_WORDS(CHDR_W/ITEM_W),
    .SYNC_CLKS(1), .PIPELINE("IN")
  ) payload_width_conv_i (
    .s_axis_aclk(axis_data_clk), .s_axis_rst(axis_data_rst),
    .s_axis_tdata(s_axis_payload_tdata),
    .s_axis_tkeep(s_axis_payload_tkeep),
    .s_axis_tlast(s_axis_payload_tlast),
    .s_axis_tvalid(tmp_pyld_tvalid),
    .s_axis_tready(tmp_pyld_tready),
    .m_axis_aclk(axis_data_clk), .m_axis_rst(axis_data_rst),
    .m_axis_tdata(in_pyld_tdata),
    .m_axis_tkeep(/* unused */),
    .m_axis_tlast(in_pyld_tlast),
    .m_axis_tvalid(in_pyld_tvalid),
    .m_axis_tready(in_pyld_tready)
  );

  // ---------------------------------------------------
  //  Payload and Context FIFOs
  // ---------------------------------------------------
  wire [CHDR_W-1:0] out_ctxt_tdata , out_pyld_tdata ;
  wire [3:0]        out_ctxt_tuser;
  wire              out_ctxt_tlast , out_pyld_tlast ;
  wire              out_ctxt_tvalid, out_pyld_tvalid;
  reg               out_ctxt_tready, out_pyld_tready;

  generate if (SYNC_CLKS) begin
    axi_fifo #(.WIDTH(CHDR_W+4+1), .SIZE(CONTEXT_FIFO_SIZE)) ctxt_fifo_i (
      .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(1'b0),
      .i_tdata({s_axis_context_tlast, s_axis_context_tuser, s_axis_context_tdata}),
      .i_tvalid(tmp_ctxt_tvalid), .i_tready(tmp_ctxt_tready),
      .o_tdata({out_ctxt_tlast, out_ctxt_tuser, out_ctxt_tdata}),
      .o_tvalid(out_ctxt_tvalid), .o_tready(out_ctxt_tready),
      .space(), .occupied()
    );
    axi_fifo #(.WIDTH(CHDR_W+1), .SIZE(PAYLOAD_FIFO_SIZE)) pyld_fifo_i (
      .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(1'b0),
      .i_tdata({in_pyld_tlast, in_pyld_tdata}),
      .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
      .o_tdata({out_pyld_tlast, out_pyld_tdata}),
      .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready),
      .space(), .occupied()
    );
  end else begin
    axi_fifo_2clk #(.WIDTH(CHDR_W+4+1), .SIZE(CONTEXT_FIFO_SIZE)) ctxt_fifo_i (
      .reset(axis_data_rst),
      .i_aclk(axis_data_clk),
      .i_tdata({s_axis_context_tlast, s_axis_context_tuser, s_axis_context_tdata}),
      .i_tvalid(tmp_ctxt_tvalid), .i_tready(tmp_ctxt_tready),
      .o_aclk(axis_chdr_clk),
      .o_tdata({out_ctxt_tlast, out_ctxt_tuser, out_ctxt_tdata}),
      .o_tvalid(out_ctxt_tvalid), .o_tready(out_ctxt_tready)
    );
    axi_fifo_2clk #(.WIDTH(CHDR_W+1), .SIZE(PAYLOAD_FIFO_SIZE)) pyld_fifo_i (
      .reset(axis_data_rst),
      .i_aclk(axis_data_clk),
      .i_tdata({in_pyld_tlast, in_pyld_tdata}),
      .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
      .o_aclk(axis_chdr_clk),
      .o_tdata({out_pyld_tlast, out_pyld_tdata}),
      .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready)
    );
  end endgenerate

  // ---------------------------------------------------
  //  Output State Machine
  // ---------------------------------------------------
  wire [CHDR_W-1:0] chdr_pg_tdata;
  reg               chdr_pg_tlast, chdr_pg_tvalid;
  wire              chdr_pg_terror, chdr_pg_tready;

  localparam [2:0] ST_HDR       = 3'd0;   // Processing the output CHDR header       
  localparam [2:0] ST_TS        = 3'd1;   // Processing the output CHDR timestamp    
  localparam [2:0] ST_MDATA     = 3'd2;   // Processing the output CHDR metadata word
  localparam [2:0] ST_BODY      = 3'd3;   // Processing the output CHDR payload word 
  localparam [2:0] ST_DROP_CTXT = 3'd4;   // Something went wrong... Dropping context packet
  localparam [2:0] ST_DROP_PYLD = 3'd5;   // Something went wrong... Dropping payload packet
  localparam [2:0] ST_TERMINATE = 3'd6;   // Something went wrong... Rejecting output packet

  reg [2:0] state = ST_HDR;
  reg [4:0] mdata_pending = 5'd0;

  // Shortcuts: CHDR header
  wire [2:0] out_pkt_type = chdr_get_pkt_type(out_ctxt_tdata[63:0]);
  wire [4:0] out_num_mdata = chdr_get_num_mdata(out_ctxt_tdata[63:0]);

  always @(posedge axis_chdr_clk) begin
    if (axis_chdr_rst) begin
      state <= ST_HDR;
      framer_errors <= 32'd0;
    end else begin
      case (state)

        // ST_HDR: CHDR Header
        // -------------------
        ST_HDR: begin
          if (out_ctxt_tvalid && out_ctxt_tready) begin
            mdata_pending <= out_num_mdata;
            if (CHDR_W > 64) begin
              // When CHDR_W > 64, the timestamp is a part of the header word.
              // If this is a data packet (with/without a TS), we skip the TS state
              // move directly to metadata/body
              if (out_num_mdata != CHDR_NO_MDATA) begin
                if (!out_ctxt_tlast)
                  if (out_ctxt_tuser == CONTEXT_FIELD_HDR_TS)
                    state <= ST_MDATA;        // tlast should be low. Move to metadata.
                  else
                    state <= ST_DROP_CTXT;    // Malformed packet: Wrong tuser. Drop ctxt+pyld
                else
                  state <= ST_DROP_PYLD;      // Premature tlast. Drop pyld
              end else begin
                if (out_ctxt_tlast)
                  if (out_ctxt_tuser == CONTEXT_FIELD_HDR_TS)
                    state <= ST_BODY;         // tlast should be high. Move to payload.
                  else
                    state <= ST_DROP_PYLD;    // Malformed packet: Wrong tuser. Drop pyld
                else
                  state <= ST_DROP_CTXT;      // Malformed packet: extra context lines. Drop ctxt+pyld
              end
            end else begin
              // When CHDR_W == 64, the timestamp comes after the header. Check if this is a data
              // packet with a TS to figure out the next state. If no TS, then check for metadata
              // to move to the next state. Drop any non-data packets.
              if (out_pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
                if (!out_ctxt_tlast)
                  if (out_ctxt_tuser == CONTEXT_FIELD_HDR)
                    state <= ST_TS;           // tlast should be low. Move to timestamp.
                  else
                    state <= ST_DROP_CTXT;    // Malformed packet: Wrong tuser. Drop ctxt+pyld
                else
                  state <= ST_DROP_PYLD;      // Premature tlast. Drop pyld
              end else begin
                if (out_num_mdata != CHDR_NO_MDATA) begin
                  if (!out_ctxt_tlast)
                    if (out_ctxt_tuser == CONTEXT_FIELD_HDR)
                      state <= ST_MDATA;      // tlast should be low. Move to metadata.
                    else
                      state <= ST_DROP_CTXT;  // Malformed packet: Wrong tuser. Drop ctxt+pyld
                  else
                    state <= ST_DROP_PYLD;    // Premature tlast. Drop pyld
                end else begin
                  if (out_ctxt_tlast)
                    if (out_ctxt_tuser == CONTEXT_FIELD_HDR)
                      state <= ST_BODY;       // tlast should be high. Move to payload.
                    else
                      state <= ST_DROP_PYLD;  // Malformed packet: Wrong tuser. Drop pyld
                  else
                    state <= ST_DROP_CTXT;    // Malformed packet: extra context lines. Drop ctxt+pyld
                end
              end
            end
          end
        end

        // ST_TS: Timestamp (CHDR_W == 64 only)
        // ------------------------------------
        ST_TS: begin
          if (out_ctxt_tvalid && out_ctxt_tready) begin
            if (mdata_pending != CHDR_NO_MDATA) begin
              if (!out_ctxt_tlast)
                if (out_ctxt_tuser == CONTEXT_FIELD_TS)
                  state <= ST_MDATA;        // tlast should be low. Move to metadata.
                else
                  state <= ST_DROP_CTXT;    // Malformed packet: Wrong tuser. Drop ctxt+pyld
              else
                state <= ST_DROP_PYLD;      // Premature tlast. Drop pyld
            end else begin
              if (out_ctxt_tlast)
                if (out_ctxt_tuser == CONTEXT_FIELD_TS)
                  state <= ST_BODY;         // tlast should be high. Move to payload.
                else
                  state <= ST_DROP_PYLD;    // Malformed packet: Wrong tuser. Drop pyld
              else
                state <= ST_DROP_CTXT;      // Malformed packet: extra context lines. Drop ctxt+pyld
            end
          end
        end

        // ST_MDATA: Metadata word
        // -----------------------
        ST_MDATA: begin
          if (out_ctxt_tvalid && out_ctxt_tready) begin
            if (mdata_pending != 5'd1) begin
              mdata_pending <= mdata_pending - 'd1;
              if (!out_ctxt_tlast)
                if (out_ctxt_tuser == CONTEXT_FIELD_MDATA)
                  state <= ST_MDATA;        // tlast should be low. Continue processing metadata.
                else
                  state <= ST_DROP_CTXT;    // Malformed packet: Wrong tuser. Drop ctxt+pyld
              else
                state <= ST_DROP_PYLD;      // Premature tlast. Drop pyld
            end else begin
              if (out_ctxt_tlast)
                if (out_ctxt_tuser == CONTEXT_FIELD_MDATA)
                  state <= ST_BODY;         // tlast should be high. Move to payload.
                else
                  state <= ST_DROP_PYLD;    // Malformed packet: Wrong tuser. Drop pyld
              else
                state <= ST_DROP_CTXT;      // Malformed packet: extra context lines. Drop ctxt+pyld
            end
          end
        end

        // ST_BODY: Payload word
        // ---------------------
        ST_BODY: begin
          if (out_pyld_tvalid && out_pyld_tready) begin
            state <= out_pyld_tlast ? ST_HDR : ST_BODY;
          end
        end

        // ST_DROP_CTXT: Drop current context packet
        // -----------------------------------------
        ST_DROP_CTXT: begin
          if (out_ctxt_tvalid && out_ctxt_tready) begin
            state <= out_ctxt_tlast ? ST_DROP_PYLD : ST_DROP_CTXT;
          end
        end

        // ST_DROP_PYLD: Drop current payload packet
        // -----------------------------------------
        ST_DROP_PYLD: begin
          if (out_pyld_tvalid && out_pyld_tready) begin
            state <= out_pyld_tlast ? ST_TERMINATE : ST_DROP_PYLD;
          end
        end

        // ST_TERMINATE: Drop partial output packet
        // ----------------------------------------
        ST_TERMINATE: begin
          if (chdr_pg_tready) begin
            state <= ST_HDR;
            framer_errors <= framer_errors + 32'd1;
          end
        end

        default: begin
          // We should never get here
          state <= ST_HDR;
        end
      endcase
    end
  end


  always @(*) begin
    case (state)
      ST_HDR: begin
        // A context word passes fwd to the CHDR output
        chdr_pg_tvalid  = out_ctxt_tvalid;
        chdr_pg_tlast   = 1'b0;   // tlast is inherited from the data stream
        out_ctxt_tready = chdr_pg_tready;
        out_pyld_tready = 1'b0;
      end
      ST_TS: begin
        // A context word passes fwd to the CHDR output
        chdr_pg_tvalid  = out_ctxt_tvalid;
        chdr_pg_tlast   = 1'b0;   // tlast is inherited from the data stream
        out_ctxt_tready = chdr_pg_tready;
        out_pyld_tready = 1'b0;
      end
      ST_MDATA: begin
        // A context word passes fwd to the CHDR output
        chdr_pg_tvalid  = out_ctxt_tvalid;
        chdr_pg_tlast   = 1'b0;   // tlast is inherited from the data stream
        out_ctxt_tready = chdr_pg_tready;
        out_pyld_tready = 1'b0;
      end
      ST_BODY: begin
        // A payload word passes fwd to the CHDR output
        chdr_pg_tvalid  = out_pyld_tvalid;
        chdr_pg_tlast   = out_pyld_tlast;
        out_ctxt_tready = 1'b0;
        out_pyld_tready = chdr_pg_tready;
      end
      ST_DROP_CTXT: begin
        // A context word is consumed without passing fwd
        chdr_pg_tvalid  = 1'b0;
        chdr_pg_tlast   = 1'b0;
        out_ctxt_tready = 1'b1;
        out_pyld_tready = 1'b0;
      end
      ST_DROP_PYLD: begin
        // A payload word is consumed without passing fwd
        chdr_pg_tvalid  = 1'b0;
        chdr_pg_tlast   = 1'b0;
        out_ctxt_tready = 1'b0;
        out_pyld_tready = 1'b1;
      end
      ST_TERMINATE: begin
        // A dummy word with a tlast and terror is passed fwd
        // to evacuate the current packet from the packet_gate
        chdr_pg_tvalid  = 1'b1;
        chdr_pg_tlast   = 1'b1;
        out_ctxt_tready = 1'b0;
        out_pyld_tready = 1'b0;
      end
      default: begin
        chdr_pg_tvalid  = 1'b0;
        chdr_pg_tlast   = 1'b0;
        out_ctxt_tready = 1'b0;
        out_pyld_tready = 1'b0;
      end
    endcase
  end

  assign chdr_pg_tdata  = (state == ST_BODY) ? out_pyld_tdata : out_ctxt_tdata;
  assign chdr_pg_terror = (state == ST_TERMINATE);

  // ---------------------------------------------------
  //  Packet gate
  // ---------------------------------------------------

  wire [CHDR_W-1:0] chdr_flush_tdata;
  wire              chdr_flush_tlast, chdr_flush_tvalid;
  wire              chdr_flush_terror, chdr_flush_tready;

  axis_packet_flush #(
    .WIDTH(CHDR_W+1), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("IN")
  ) chdr_flusher_i (
    .clk(axis_chdr_clk), .reset(axis_chdr_rst),
    .enable(flush_en), .timeout(flush_timeout),
    .flushing(flush_active), .done(flush_done),
    .s_axis_tdata({chdr_pg_terror, chdr_pg_tdata}), .s_axis_tlast(chdr_pg_tlast),
    .s_axis_tvalid(chdr_pg_tvalid), .s_axis_tready(chdr_pg_tready),
    .m_axis_tdata({chdr_flush_terror, chdr_flush_tdata}), .m_axis_tlast(chdr_flush_tlast),
    .m_axis_tvalid(chdr_flush_tvalid), .m_axis_tready(chdr_flush_tready)
  );

  axi_packet_gate #( .WIDTH(CHDR_W), .SIZE(MTU), .USE_AS_BUFF(0) ) out_gate_i (
    .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(flush_active),
    .i_tdata(chdr_flush_tdata), .i_tlast(chdr_flush_tlast), .i_terror(chdr_flush_terror),
    .i_tvalid(chdr_flush_tvalid), .i_tready(chdr_flush_tready),
    .o_tdata(m_axis_chdr_tdata), .o_tlast(m_axis_chdr_tlast),
    .o_tvalid(m_axis_chdr_tvalid), .o_tready(m_axis_chdr_tready)
  );

endmodule // axis_pyld_ctxt_to_chdr
