//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_to_axis_data
//
// Description:
//
//   A deframer module for CHDR data packets. It accepts an input CHDR stream 
//   and produces an output data stream that includes the payload of the 
//   packet, as well as timestamp and packet flags presented as sideband 
//   information.
//
//   This module also performs an optional clock crossing and data width 
//   conversion from CHDR_W to a user requested width for the payload data bus.
//
// Parameters:
//   - CHDR_W         : Width of the input CHDR bus in bits
//   - ITEM_W         : Width of the output item bus in bits
//   - NIPC           : The number of output items delivered per cycle
//   - SYNC_CLKS      : Are the CHDR and data clocks synchronous to each other?
//   - INFO_FIFO_SIZE : Log2 of the FIFO size for the packet info data path
//   - PYLD_FIFO_SIZE : Log2 of the FIFO size for the payload data path
//
// Signals:
//   - s_axis_chdr_*  : Input CHDR stream (AXI-Stream)
//   - m_axis_*       : Output payload data stream (AXI-Stream)
//   - m_axis_mdata_* : Output mdata stream (AXI-Stream)
//   - flush_*        : Signals for flush control and status
//

module chdr_to_axis_data #(
  parameter CHDR_W         = 256,
  parameter ITEM_W         = 32,
  parameter NIPC           = 2,
  parameter SYNC_CLKS      = 0,
  parameter INFO_FIFO_SIZE = 5,
  parameter PYLD_FIFO_SIZE = 5
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

  localparam [2:0] ST_HDR   = 3'd0;   // Processing the input CHDR header
  localparam [2:0] ST_TS    = 3'd1;   // Processing the input CHDR timestamp
  localparam [2:0] ST_MDATA = 3'd2;   // Processing the input CHDR metadata word
  localparam [2:0] ST_BODY  = 3'd3;   // Processing the input CHDR payload word
  localparam [2:0] ST_DROP  = 3'd4;   // Something went wrong... Dropping packet

  reg [2:0] state = ST_HDR;
  reg [4:0] mdata_pending = CHDR_NO_MDATA;

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

  // CHDR data goes to the payload stream only in the BODY state. Packets are
  // expected to have at least one payload word so the CHDR tlast can be used
  // as the payload tlast.
  assign in_pyld_tdata  = in_chdr_tdata;
  assign in_pyld_tkeep  = in_chdr_tkeep;
  assign in_pyld_tlast  = in_chdr_tlast;
  assign in_pyld_tvalid = in_chdr_tvalid && (state == ST_BODY);

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
      ST_HDR   : in_chdr_tready = in_info_tready;
      ST_TS    : in_chdr_tready = in_info_tready;
      ST_MDATA : in_chdr_tready = 1'b1;
      ST_BODY  : in_chdr_tready = in_pyld_tready;
      ST_DROP  : in_chdr_tready = 1'b1;
      default  : in_chdr_tready = 1'b0;
    endcase
  end

  // ---------------------------------------------------
  //  Payload and Sideband Data FIFOs
  // ---------------------------------------------------
  wire [CHDR_W-1:0]       out_pyld_tdata;
  wire [CHDR_KEEP_W-1:0]  out_pyld_tkeep;
  wire                    out_pyld_tlast, out_pyld_tvalid, out_pyld_tready;
  
  wire [INFO_W-1:0]       out_info_tdata;
  wire                    out_info_tvalid, out_info_tready;

  wire [(ITEM_W*NIPC)-1:0] conv_pyld_tdata;
  wire [NIPC-1:0]          conv_pyld_tkeep;
  wire                     conv_pyld_tlast, conv_pyld_tvalid, conv_pyld_tready;


  generate
    // Metadata FIFOs
    if (SYNC_CLKS) begin : gen_sync_info_fifo
      axi_fifo #(.WIDTH(INFO_W), .SIZE(INFO_FIFO_SIZE)) info_fifo_i (
        .clk(axis_data_clk), .reset(axis_data_rst), .clear(1'b0),
        .i_tdata(in_info_tdata),
        .i_tvalid(in_info_tvalid), .i_tready(in_info_tready),
        .o_tdata(out_info_tdata),
        .o_tvalid(out_info_tvalid), .o_tready(out_info_tready),
        .space(), .occupied()
      );
    end else begin : gen_async_info_fifo
      axi_fifo_2clk #(.WIDTH(INFO_W), .SIZE(INFO_FIFO_SIZE)) info_fifo_i (
        .reset(axis_chdr_rst),
        .i_aclk(axis_chdr_clk),
        .i_tdata(in_info_tdata),
        .i_tvalid(in_info_tvalid), .i_tready(in_info_tready),
        .o_aclk(axis_data_clk),
        .o_tdata(out_info_tdata),
        .o_tvalid(out_info_tvalid), .o_tready(out_info_tready)
      );
    end
  
    // Payload FIFOs
    if (CHDR_W != ITEM_W*NIPC) begin : gen_axis_width_conv
      axi_fifo #(.WIDTH(CHDR_W+CHDR_KEEP_W+1), .SIZE(PYLD_FIFO_SIZE)) pyld_fifo_i (
        .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(1'b0),
        .i_tdata({in_pyld_tlast, in_pyld_tkeep, in_pyld_tdata}),
        .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
        .o_tdata({out_pyld_tlast, out_pyld_tkeep, out_pyld_tdata}),
        .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready),
        .space(), .occupied()
      );

      // Do the width conversion and clock crossing in the axis_width_conv
      // module to ensure that the resize happens on the correct side of the
      // clock crossing.
      axis_width_conv #(
        .WORD_W(ITEM_W), .IN_WORDS(CHDR_W/ITEM_W), .OUT_WORDS(NIPC),
        .SYNC_CLKS(SYNC_CLKS), .PIPELINE("NONE")
      ) payload_width_conv_i (
        .s_axis_aclk(axis_chdr_clk), .s_axis_rst(axis_chdr_rst),
        .s_axis_tdata(out_pyld_tdata), .s_axis_tkeep(out_pyld_tkeep),
        .s_axis_tlast(out_pyld_tlast), .s_axis_tvalid(out_pyld_tvalid),
        .s_axis_tready(out_pyld_tready),
        .m_axis_aclk(axis_data_clk), .m_axis_rst(axis_data_rst),
        .m_axis_tdata(conv_pyld_tdata), .m_axis_tkeep(conv_pyld_tkeep),
        .m_axis_tlast(conv_pyld_tlast), .m_axis_tvalid(conv_pyld_tvalid),
        .m_axis_tready(conv_pyld_tready)
      );
    end else begin : no_gen_axis_width_conv
      if (SYNC_CLKS) begin : gen_sync_pyld_fifo
        axi_fifo #(.WIDTH(CHDR_W+CHDR_KEEP_W+1), .SIZE(PYLD_FIFO_SIZE)) pyld_fifo_i (
          .clk(axis_chdr_clk), .reset(axis_chdr_rst), .clear(1'b0),
          .i_tdata({in_pyld_tlast, in_pyld_tkeep, in_pyld_tdata}),
          .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
          .o_tdata({out_pyld_tlast, out_pyld_tkeep, out_pyld_tdata}),
          .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready),
          .space(), .occupied()
        );
      end else begin : gen_async_pyld_fifo
        axi_fifo_2clk #(.WIDTH(CHDR_W+CHDR_KEEP_W+1), .SIZE(PYLD_FIFO_SIZE)) pyld_fifo_i (
          .reset(axis_chdr_rst),
          .i_aclk(axis_chdr_clk),
          .i_tdata({in_pyld_tlast, in_pyld_tkeep, in_pyld_tdata}),
          .i_tvalid(in_pyld_tvalid), .i_tready(in_pyld_tready),
          .o_aclk(axis_data_clk),
          .o_tdata({out_pyld_tlast, out_pyld_tkeep, out_pyld_tdata}),
          .o_tvalid(out_pyld_tvalid), .o_tready(out_pyld_tready)
        );
      end

      // No width conversion needed
      assign conv_pyld_tdata  = out_pyld_tdata;
      assign conv_pyld_tkeep  = out_pyld_tkeep;
      assign conv_pyld_tlast  = out_pyld_tlast;
      assign conv_pyld_tvalid = out_pyld_tvalid;
      assign out_pyld_tready  = conv_pyld_tready;
    end
  endgenerate

  // ---------------------------------------------------
  //  Merge payload and info streams
  // ---------------------------------------------------
  // There should be one info word for each payload packet.
  wire [INFO_W+(ITEM_W+1)*NIPC-1:0] flush_tdata;
  wire                              flush_tlast;
  wire                              flush_tvalid;
  wire                              flush_tready;

  assign flush_tdata      = { out_info_tdata, conv_pyld_tkeep, conv_pyld_tdata };
  assign flush_tlast      = conv_pyld_tlast;
  assign flush_tvalid     = conv_pyld_tvalid && out_info_tvalid;
  assign conv_pyld_tready = flush_tready     && out_info_tvalid;
  assign out_info_tready  = conv_pyld_tready && conv_pyld_tlast && conv_pyld_tvalid;

  // ---------------------------------------------------
  //  Flushing Logic
  // ---------------------------------------------------
  wire [31:0] flush_timeout_dclk;
  wire        flush_en_dclk;
  wire        flush_active_pyld_cclk;
  wire        flush_done_pyld_cclk;
  wire        flush_active_pyld;
  wire        flush_done_pyld;

  synchronizer #(.WIDTH(2), .INITIAL_VAL(4'd0)) flush_2clk_rb_i (
    .clk(axis_chdr_clk), .rst(1'b0),
    .in({flush_active_pyld, flush_done_pyld}),
    .out({flush_active_pyld_cclk, flush_done_pyld_cclk})
  );
  assign flush_active = flush_active_pyld_cclk;
  assign flush_done = flush_done_pyld_cclk;

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
    .s_axis_tdata(flush_tdata),
    .s_axis_tlast(flush_tlast),
    .s_axis_tvalid(flush_tvalid),
    .s_axis_tready(flush_tready),
    .m_axis_tdata({m_axis_ttimestamp, m_axis_thas_time, m_axis_tlength, 
                   m_axis_teob, m_axis_teov, m_axis_tkeep, m_axis_tdata}),
    .m_axis_tlast(m_axis_tlast),
    .m_axis_tvalid(m_axis_tvalid),
    .m_axis_tready(m_axis_tready)
  );

endmodule
