//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_to_axis_data_mdata
//
// Description:
//
//   A deframer module for CHDR data packets. It accepts an input CHDR stream, 
//   and produces two output streams:
//
//     1) Payload data, which includes the payload of the packet, as well as 
//        timestamp and packet flags presented as sideband information.
//     2) Metadata (mdata), which contains only the metadata of the packet.
//
//   This module also performs an optional clock crossing and data width 
//   conversion from CHDR_W to a user requested width for the payload data bus.
//
//   The metadata and data packets are interleaved, i.e., a mdata packet will 
//   arrive before its corresponding data packet. However, if mdata prefetching 
//   is enabled, the mdata for the next packet might arrive before the data for 
//   the current packet has been consumed. In the case of a rate reduction, 
//   this allows the module to sustain a gapless stream of payload items and a 
//   bursty sideband mdata path. If there is no metadata in a packet, then an 
//   empty packet is output on m_axis_mdata_* (i.e., m_axis_mdata_tkeep will be 
//   set to 0).
//
// Parameters:
//
//   - CHDR_W            : Width of the input CHDR bus in bits
//   - ITEM_W            : Width of the output item bus in bits
//   - NIPC              : The number of output items delivered per cycle
//   - SYNC_CLKS         : Are the CHDR and data clocks synchronous to each other?
//   - MDATA_FIFO_SIZE   : FIFO size for the mdata path
//   - INFO_FIFO_SIZE    : FIFO size for the packet info path
//   - PAYLOAD_FIFO_SIZE : FIFO size for the payload path
//   - MDATA_PREFETCH_EN : Is mdata prefetching enabled?
//
// Signals:
//
//   - s_axis_chdr_*  : Input CHDR stream (AXI-Stream)
//   - m_axis_*       : Output payload data stream (AXI-Stream)
//   - m_axis_mdata_* : Output mdata stream (AXI-Stream)
//   - flush_*        : Signals for flush control and status
//

module chdr_to_axis_data_mdata #(
  parameter CHDR_W            = 256,
  parameter ITEM_W            = 32,
  parameter NIPC              = 2,
  parameter SYNC_CLKS         = 0,
  parameter MDATA_FIFO_SIZE   = 1,
  parameter INFO_FIFO_SIZE    = 1,
  parameter PAYLOAD_FIFO_SIZE = 1,
  parameter MDATA_PREFETCH_EN = 1
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
  // Payload data stream out (AXI-Stream)
  output wire [(ITEM_W*NIPC)-1:0] m_axis_tdata,
  output wire [NIPC-1:0]          m_axis_tkeep,
  output wire                     m_axis_tlast,
  output wire                     m_axis_tvalid,
  input  wire                     m_axis_tready,
  // Payload sideband information
  output wire [63:0]              m_axis_ttimestamp,
  output wire                     m_axis_thas_time,
  output wire [15:0]              m_axis_tlength,
  output wire                     m_axis_teob,
  output wire                     m_axis_teov,
  // Metadata stream out (AXI-Stream)
  output wire [CHDR_W-1:0]        m_axis_mdata_tdata,
  output wire                     m_axis_mdata_tlast,
  output wire                     m_axis_mdata_tkeep,
  output wire                     m_axis_mdata_tvalid,
  input  wire                     m_axis_mdata_tready,
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
  localparam INFO_W = 64+1+16+1+1;   // timestamp, has_time, length, eob, eov

  wire [CHDR_W-1:0]       in_pyld_tdata;
  wire [CHDR_KEEP_W-1:0]  in_pyld_tkeep;
  wire                    in_pyld_tlast, in_pyld_tvalid, in_pyld_tready;

  reg [INFO_W-1:0] in_info_tdata;
  reg              in_info_tvalid;
  wire             in_info_tready;

  wire [CHDR_W-1:0] in_mdata_tdata;
  wire              in_mdata_tkeep;
  wire              in_mdata_tlast, in_mdata_tvalid, in_mdata_tready;


  localparam [2:0] ST_HDR   = 3'd0;   // Processing the input CHDR header
  localparam [2:0] ST_TS    = 3'd1;   // Processing the input CHDR timestamp
  localparam [2:0] ST_MDATA = 3'd2;   // Processing the input CHDR metadata word
  localparam [2:0] ST_BODY  = 3'd3;   // Processing the input CHDR payload word
  localparam [2:0] ST_DROP  = 3'd4;   // Something went wrong... Dropping packet

  reg [2:0] state = ST_HDR;
  reg [4:0] mdata_pending = CHDR_NO_MDATA;
  reg       last_mdata_line;

  reg [15:0] chdr_length_reg;
  reg        chdr_eob_reg, chdr_eov_reg;

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
              chdr_length_reg <= chdr_calc_payload_length(CHDR_W, in_chdr_tdata);
              chdr_eob_reg    <= chdr_get_eob(in_chdr_tdata);
              chdr_eov_reg    <= chdr_get_eov(in_chdr_tdata);
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

  // Only metadata goes into the mdata FIFO. However, if there is no metadata,
  // then we want an empty packet to go into the mdata FIFO. We check the
  // packet type because non-data packets will be discarded.
  assign in_mdata_tdata  = in_chdr_tdata;
  assign in_mdata_tlast  = in_chdr_tlast || last_mdata_line;
  assign in_mdata_tkeep  = (state == ST_MDATA);
  assign in_mdata_tvalid = in_chdr_tvalid && (
    (state == ST_MDATA) ||
    (state == ST_HDR && in_num_mdata  == CHDR_NO_MDATA && 
      (in_pkt_type == CHDR_PKT_TYPE_DATA || in_pkt_type == CHDR_PKT_TYPE_DATA_TS)));

  always @(*) begin
    // Packet timestamp and flags go into the info FIFO, but only if it's a 
    // data packet since non-data packets will be discarded.
    if (CHDR_W > 64) begin
      // When CHDR_W > 64, all info will be in the first word of the CHDR packet
      in_info_tdata =  { in_chdr_tdata[127:64], 
                         chdr_get_has_time(in_chdr_tdata),
                         chdr_calc_payload_length(CHDR_W, in_chdr_tdata),
                         chdr_get_eob(in_chdr_tdata),
                         chdr_get_eov(in_chdr_tdata) };
      in_info_tvalid = in_chdr_tvalid && (state == ST_HDR && 
        (in_pkt_type == CHDR_PKT_TYPE_DATA || in_pkt_type == CHDR_PKT_TYPE_DATA_TS));
    end else begin
      // When CHDR_W == 64, the flags will be in the first word of the packet, 
      // but the timestamp will be in the second word, if there is a timestamp.
      if (state == ST_HDR && in_pkt_type == CHDR_PKT_TYPE_DATA) begin
        // No timestamp in this case
        in_info_tdata  = { in_chdr_tdata[63:0], 1'b0, 
                           chdr_calc_payload_length(CHDR_W, in_chdr_tdata),
                           chdr_get_eob(in_chdr_tdata), chdr_get_eov(in_chdr_tdata) };
        in_info_tvalid = in_chdr_tvalid;
      end else begin
        // Assuming timestamp is present, so use flags from previous clock cycle
        in_info_tdata  = { in_chdr_tdata[63:0], 1'b1, chdr_length_reg,
                           chdr_eob_reg, chdr_eov_reg };
        in_info_tvalid = in_chdr_tvalid && (state == ST_TS);
      end
    end

    case (state)
      ST_HDR: begin
        in_chdr_tready  = in_info_tready && in_mdata_tready;
        last_mdata_line = (in_num_mdata == CHDR_NO_MDATA);
      end
      ST_TS: begin
        in_chdr_tready  = in_info_tready && in_mdata_tready;
        last_mdata_line = 1'b0;
      end
      ST_MDATA: begin
        in_chdr_tready  = in_mdata_tready;
        last_mdata_line = (mdata_pending == 5'd1);
      end
      ST_BODY: begin
        in_chdr_tready  = in_pyld_tready;
        last_mdata_line = 1'b0;
      end
      ST_DROP: begin
        in_chdr_tready  = 1'b1;
        last_mdata_line = 1'b0;
      end
      default: begin
        in_chdr_tready  = 1'b0;
        last_mdata_line = 1'b0;
      end
    endcase
  end

  // ---------------------------------------------------
  //  Payload and mdata FIFOs
  // ---------------------------------------------------
  wire [CHDR_W-1:0]       out_pyld_tdata;
  wire [CHDR_KEEP_W-1:0]  out_pyld_tkeep;
  wire                    out_pyld_tlast, out_pyld_tvalid, out_pyld_tready;

  wire tmp_mdata_tvalid, tmp_mdata_tready;
  wire tmp_info_tready;

  wire [(ITEM_W*NIPC)-1:0] flush_pyld_tdata;
  wire [NIPC-1:0]          flush_pyld_tkeep;
  wire                     flush_pyld_tlast, flush_pyld_tvalid, flush_pyld_tready;
  wire [INFO_W-1:0]        flush_info_tdata;
  wire [CHDR_W-1:0]        flush_mdata_tdata;
  wire                     flush_mdata_tkeep;
  wire                     flush_mdata_tlast, flush_mdata_tvalid, flush_mdata_tready;

  generate if (SYNC_CLKS) begin : gen_sync_fifo
    axi_fifo #(.WIDTH(CHDR_W+2), .SIZE(MDATA_FIFO_SIZE)) mdata_fifo_i (
      .clk(axis_data_clk), .reset(axis_data_rst), .clear(1'b0),
      .i_tdata({in_mdata_tkeep, in_mdata_tlast, in_mdata_tdata}),
      .i_tvalid(in_mdata_tvalid), .i_tready(in_mdata_tready),
      .o_tdata({flush_mdata_tkeep, flush_mdata_tlast, flush_mdata_tdata}),
      .o_tvalid(tmp_mdata_tvalid), .o_tready(tmp_mdata_tready),
      .space(), .occupied()
    );
    axi_fifo #(.WIDTH(INFO_W), .SIZE(INFO_FIFO_SIZE)) info_fifo_i (
      .clk(axis_data_clk), .reset(axis_data_rst), .clear(1'b0),
      .i_tdata(in_info_tdata),
      .i_tvalid(in_info_tvalid), .i_tready(in_info_tready),
      .o_tdata(flush_info_tdata),
      .o_tvalid(), .o_tready(tmp_info_tready),
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
    axi_fifo_2clk #(.WIDTH(CHDR_W+2), .SIZE(MDATA_FIFO_SIZE)) mdata_fifo_i (
      .reset(axis_chdr_rst),
      .i_aclk(axis_chdr_clk),
      .i_tdata({in_mdata_tkeep, in_mdata_tlast, in_mdata_tdata}),
      .i_tvalid(in_mdata_tvalid), .i_tready(in_mdata_tready),
      .o_aclk(axis_data_clk),
      .o_tdata({flush_mdata_tkeep, flush_mdata_tlast, flush_mdata_tdata}),
      .o_tvalid(tmp_mdata_tvalid), .o_tready(tmp_mdata_tready)
    );
    axi_fifo_2clk #(.WIDTH(INFO_W), .SIZE(INFO_FIFO_SIZE)) info_fifo_i (
      .reset(axis_chdr_rst),
      .i_aclk(axis_chdr_clk),
      .i_tdata(in_info_tdata),
      .i_tvalid(in_info_tvalid), .i_tready(in_info_tready),
      .o_aclk(axis_data_clk),
      .o_tdata(flush_info_tdata),
      .o_tvalid(), .o_tready(tmp_info_tready)
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

  generate
    if (CHDR_W != ITEM_W*NIPC) begin : gen_axis_width_conv
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
    end else begin : no_gen_axis_width_conv
      assign flush_pyld_tdata = out_pyld_tdata;
      assign flush_pyld_tkeep = out_pyld_tkeep;
      assign flush_pyld_tlast = out_pyld_tlast;
      assign tmp_pyld_tvalid  = out_pyld_tvalid;
      assign out_pyld_tready  = tmp_pyld_tready;
    end
  endgenerate


  // ---------------------------------------------------
  //  Output State Machine
  // ---------------------------------------------------
  reg [2:0] mdata_pkt_cnt = 3'd0, pyld_pkt_cnt = 3'd0;

  // A payload packet can pass only if it is preceded by a mdata packet
  wire pass_pyld = ((mdata_pkt_cnt - pyld_pkt_cnt) > 3'd0);
  // A mdata packet has to be blocked if its corresponding payload packet hasn't passed except
  // when prefetching is enabled. In that case one additional mdata packet is allowed to pass
  wire pass_mdata = ((mdata_pkt_cnt - pyld_pkt_cnt) < (MDATA_PREFETCH_EN == 1 ? 3'd2 : 3'd1));

  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      mdata_pkt_cnt <= 3'd0;
      pyld_pkt_cnt <= 3'd0;
    end else begin
      if (flush_mdata_tvalid && flush_mdata_tready && flush_mdata_tlast)
        mdata_pkt_cnt <= mdata_pkt_cnt + 3'd1;
      if (flush_pyld_tvalid && flush_pyld_tready && flush_pyld_tlast)
        pyld_pkt_cnt <= pyld_pkt_cnt + 3'd1;
    end
  end

  assign flush_pyld_tvalid = tmp_pyld_tvalid && pass_pyld;
  assign tmp_pyld_tready = flush_pyld_tready && pass_pyld;

  // Only read the info FIFO once per packet
  assign tmp_info_tready = tmp_pyld_tready && flush_pyld_tlast && tmp_pyld_tvalid;

  assign flush_mdata_tvalid = tmp_mdata_tvalid && pass_mdata;
  assign tmp_mdata_tready = flush_mdata_tready && pass_mdata;

  // ---------------------------------------------------
  //  Flushing Logic
  // ---------------------------------------------------
  wire [31:0] flush_timeout_dclk;
  wire        flush_en_dclk;
  wire        flush_active_pyld_cclk, flush_active_mdata_cclk;
  wire        flush_done_pyld_cclk, flush_done_mdata_cclk;
  wire        flush_active_pyld, flush_active_mdata;
  wire        flush_done_pyld, flush_done_mdata;

  synchronizer #(.WIDTH(4), .INITIAL_VAL(4'd0)) flush_2clk_rb_i (
    .clk(axis_chdr_clk), .rst(1'b0),
    .in({flush_active_pyld, flush_done_pyld,
         flush_active_mdata, flush_done_mdata}),
    .out({flush_active_pyld_cclk, flush_done_pyld_cclk,
         flush_active_mdata_cclk, flush_done_mdata_cclk})
  );
  assign flush_active = flush_active_pyld_cclk | flush_active_mdata_cclk;
  assign flush_done = flush_done_pyld_cclk & flush_done_mdata_cclk;

  axi_fifo_2clk #(.WIDTH(33), .SIZE(1)) flush_2clk_ctrl_i (
    .reset(axis_chdr_rst),
    .i_aclk(axis_chdr_clk),
    .i_tdata({flush_en, flush_timeout}), .i_tvalid(1'b1), .i_tready(),
    .o_aclk(axis_data_clk),
    .o_tdata({flush_en_dclk, flush_timeout_dclk}), .o_tvalid(), .o_tready(1'b1)
  );

  axis_packet_flush #(
    .WIDTH(INFO_W+(ITEM_W+1)*NIPC), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("OUT")
  ) pyld_flusher_i (
    .clk(axis_data_clk), .reset(axis_data_rst),
    .enable(flush_en_dclk), .timeout(flush_timeout_dclk),
    .flushing(flush_active_pyld), .done(flush_done_pyld),
    .s_axis_tdata({flush_info_tdata, flush_pyld_tkeep, flush_pyld_tdata}),
    .s_axis_tlast(flush_pyld_tlast),
    .s_axis_tvalid(flush_pyld_tvalid),
    .s_axis_tready(flush_pyld_tready),
    .m_axis_tdata({m_axis_ttimestamp, m_axis_thas_time, m_axis_tlength, 
                   m_axis_teob, m_axis_teov, m_axis_tkeep, m_axis_tdata}),
    .m_axis_tlast(m_axis_tlast),
    .m_axis_tvalid(m_axis_tvalid),
    .m_axis_tready(m_axis_tready)
  );

  axis_packet_flush #(
    .WIDTH(CHDR_W+1), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("OUT")
  ) mdata_flusher_i (
    .clk(axis_data_clk), .reset(axis_data_rst),
    .enable(flush_en_dclk), .timeout(flush_timeout_dclk),
    .flushing(flush_active_mdata), .done(flush_done_mdata),
    .s_axis_tdata({flush_mdata_tkeep, flush_mdata_tdata}),
    .s_axis_tlast(flush_mdata_tlast),
    .s_axis_tvalid(flush_mdata_tvalid),
    .s_axis_tready(flush_mdata_tready),
    .m_axis_tdata({m_axis_mdata_tkeep, m_axis_mdata_tdata}),
    .m_axis_tlast(m_axis_mdata_tlast),
    .m_axis_tvalid(m_axis_mdata_tvalid),
    .m_axis_tready(m_axis_mdata_tready)
  );

endmodule
