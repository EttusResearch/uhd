//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_data_to_chdr
//
// Description:
//
//   A framer module for CHDR data packets. It accepts an input data stream
//   with sideband information for packet flags and timestamp). A CHDR packet
//   will be generated for each data packet that is input.
//
//   The sideband information (e.g., timestamp, flags) must be input with the
//   AXI-Stream data input and will be sampled coincident with either the first
//   word or the last word of data in the packet (i.e., when tlast is
//   asserted), depending on the SIDEBAND_AT_END parameter setting.
//
//   If sideband information is sampled at the beginning of a packet then
//   tlength must be provided.
//
//   If sideband information is sampled at the end of the packet then tlength
//   will be ignored, and the entire packet will be buffered before the CHDR
//   output can begin. Also, in this mode, the payload FIFO size will be
//   coerced to be at least as big as the MTU. This mode is intended for cases
//   when the sideband information is not known until the end of the packet
//   (e.g., the length).
//
//   This module also performs an optional clock crossing and data width
//   conversion from a user requested width for the payload bus to CHDR_W.
//
// Parameters:
//
//   CHDR_W          : Width of the input CHDR bus in bits
//   ITEM_W          : Width of the output item bus in bits
//   NIPC            : The number of output items delivered per cycle
//   SYNC_CLKS       : Are the CHDR and data clocks synchronous to each other?
//   MTU             : Log2 of the maximum packet size in CHDR words
//   INFO_FIFO_SIZE  : Log2 of the info FIFO size. This determines the number
//                     of packets that can be simultaneously buffered in the
//                     payload FIFO.
//   PYLD_FIFO_SIZE  : Log2 of the payload FIFO size. The actual FIFO size will
//                     be the maximum of 2**MTU or 2**PYLD_FIFO_SIZE, since the
//                     FIFO must be at least one MTU so that we can calculate
//                     the packet length in the header.
//   SIDEBAND_AT_END : If 0 then the sideband information is sampled coincident
//                     with the first word of the input packet. If 1, then the
//                     sideband information is sampled with the last word of the
//                     input packet (the word in which tlast is asserted).
//
// Signals:
//
//   m_axis_chdr_* : Output CHDR stream
//   s_axis_*      : Input data stream (AXI-Stream)
//   flush_*       : Signals for flush control and status
//

module axis_data_to_chdr #(
  parameter CHDR_W          = 256,
  parameter ITEM_W          = 32,
  parameter NIPC            = 2,
  parameter SYNC_CLKS       = 0,
  parameter MTU             = 10,
  parameter INFO_FIFO_SIZE  = 5,
  parameter PYLD_FIFO_SIZE  = MTU,
  parameter SIDEBAND_AT_END = 1
)(
  // Clock, reset and settings
  input  wire                     axis_chdr_clk,
  input  wire                     axis_chdr_rst,
  input  wire                     axis_data_clk,
  input  wire                     axis_data_rst,
  // CHDR out (AXI-Stream)
  output wire [CHDR_W-1:0]        m_axis_chdr_tdata,
  output wire                     m_axis_chdr_tlast,
  output wire                     m_axis_chdr_tvalid,
  input  wire                     m_axis_chdr_tready,
  // Payload data stream in (AXI-Stream)
  input  wire [(ITEM_W*NIPC)-1:0] s_axis_tdata,
  input  wire [NIPC-1:0]          s_axis_tkeep,
  input  wire                     s_axis_tlast,
  input  wire                     s_axis_tvalid,
  output wire                     s_axis_tready,
  // Payload sideband info
  input  wire [63:0]              s_axis_ttimestamp,
  input  wire                     s_axis_thas_time,
  input  wire [15:0]              s_axis_tlength,
  input  wire                     s_axis_teov,
  input  wire                     s_axis_teob,
  // Flush signals
  input  wire                     flush_en,
  input  wire [31:0]              flush_timeout,
  output wire                     flush_active,
  output wire                     flush_done
);

  // If we are sampling the sideband information at the end of the packet then
  // the payload FIFO must be large enough to store an entire packet's worth of
  // payload data. We coerce the size here if needed.
  localparam PAYLOAD_FIFO_SIZE =
   (!SIDEBAND_AT_END || PYLD_FIFO_SIZE > MTU) ? PYLD_FIFO_SIZE : MTU;


  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------

  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"


  //---------------------------------------------------------------------------
  // Start of Packet Register
  //---------------------------------------------------------------------------

  // The sop register will indicate if the next word on s_axis_t* is the first
  // word of a packet.
  reg start_of_packet = 1'b1;

  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      start_of_packet <= 1'b1;
    end else if (s_axis_tvalid && s_axis_tready) begin
      start_of_packet <= s_axis_tlast;
    end
  end


  //---------------------------------------------------------------------------
  // Timestamp and Flags Capture
  //---------------------------------------------------------------------------

  reg [63:0] packet_timestamp;
  reg        packet_has_time;
  reg        packet_eov;
  reg        packet_eob;

  always @(posedge axis_data_clk) begin
    if (s_axis_tvalid && s_axis_tready &&
      (( SIDEBAND_AT_END && s_axis_tlast) ||
       (!SIDEBAND_AT_END && start_of_packet))
    ) begin
      packet_timestamp <= s_axis_ttimestamp;
      packet_has_time  <= s_axis_thas_time;
      packet_eov       <= s_axis_teov;
      packet_eob       <= s_axis_teob;
    end
  end


  //---------------------------------------------------------------------------
  // Length Counters
  //---------------------------------------------------------------------------
  //
  // Here We track the state of the incoming packet to determine the payload
  // length, if needed.
  //
  //---------------------------------------------------------------------------

  localparam HDR_LEN = CHDR_W/8;     // Length of CHDR header word in bytes
  localparam INFO_W  = 3 + 64 + 16;  // Length of pkt_info data

  wire [INFO_W-1:0] in_pkt_info_tdata;
  reg               in_pkt_info_tvalid = 1'b0;
  wire              in_pkt_info_tready;
  reg  [      15:0] packet_length;

  assign in_pkt_info_tdata = { packet_eob, packet_eov, packet_has_time,
                               packet_timestamp, packet_length };

  generate
    if (!SIDEBAND_AT_END) begin : gen_sample_sop
      //-------------------------------
      //  Sample at Start of Packet
      //-------------------------------

      always @(posedge axis_data_clk) begin
        if (axis_data_rst) begin
          in_pkt_info_tvalid <= 1'b0;
        end else begin
          in_pkt_info_tvalid <= 1'b0;
          if (s_axis_tvalid && s_axis_tready && start_of_packet) begin
            packet_length      <= s_axis_tlength + HDR_LEN;
            in_pkt_info_tvalid <= 1'b1;
          end
        end
      end

    end else begin : gen_sample_eop
      //-------------------------------
      //  Sample at End of Packet
      //-------------------------------

      reg [15:0] length_count = HDR_LEN;

      always @(posedge axis_data_clk) begin : pkt_length_counter
        if (axis_data_rst) begin
          length_count       <= HDR_LEN;
          in_pkt_info_tvalid <= 1'b0;
        end else begin : pkt_length_counter_main
          // Calculate the length of this word in bytes, taking tkeep into account
          integer i;
          integer num_bytes;
          num_bytes = 0;
          for (i = 0; i < NIPC; i = i + 1) begin
            num_bytes = num_bytes + (s_axis_tkeep[i]*(ITEM_W/8));
          end

          // Update the packet length if the word is accepted
          in_pkt_info_tvalid <= 1'b0;
          if (s_axis_tvalid && s_axis_tready) begin
            if (s_axis_tlast) begin
              length_count       <= HDR_LEN;
              packet_length      <= length_count + num_bytes;
              in_pkt_info_tvalid <= 1'b1;
            end else begin
              length_count <= length_count + num_bytes;
            end
          end
        end
      end
    end
  endgenerate


  //---------------------------------------------------------------------------
  // Data Width Conversion and Input FIFOs
  //---------------------------------------------------------------------------
  //
  // Convert the data width and cross the data into the CHDR clock domain, as
  // needed. Buffer the data and packet info.
  //
  //---------------------------------------------------------------------------

  wire [(ITEM_W*NIPC)-1:0] in_pyld_tdata;
  wire                     in_pyld_tlast;
  wire                     in_pyld_tvalid;
  wire                     in_pyld_tready;

  wire [CHDR_W-1:0] out_pyld_tdata;
  wire              out_pyld_tlast;
  wire              out_pyld_tvalid;
  reg               out_pyld_tready;

  wire        out_pkt_info_tvalid;
  reg         out_pkt_info_tready;
  wire        out_eob, out_eov, out_has_time;
  wire [63:0] out_timestamp;
  wire [15:0] out_length;

  reg gating = 0;

  // This state machine prevents data from transferring when the pkt_info_fifo
  // is stalled. This ensures that we don't overflow the pkt_info_fifo.
  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      gating <= 0;
    end else begin
      if (gating) begin
        if (in_pkt_info_tready) begin
          gating <= 0;
        end
      end else begin
        // whenever the pkt_info_fifo fills (i.e., in_pkt_info_tready
        // deasserts), we want to assert "gating" to stop data transfer as soon
        // as it's safe to do so.
        //
        // It's safe to gate (i.e., block) data transfer on in_pyld_* if we're
        // not asserting tvalid or we're completing a transfer this cycle.
        if (!in_pkt_info_tready && (!in_pyld_tvalid || (in_pyld_tvalid && in_pyld_tready))) begin
          gating <= 1;
        end
      end
    end
  end

  // Generate in_pyld_* from the s_axis_* data inputs. But block data input
  // when "gating" asserts.
  assign in_pyld_tdata  = s_axis_tdata;
  assign in_pyld_tlast  = s_axis_tlast;
  assign in_pyld_tvalid = s_axis_tvalid  & ~gating;
  assign s_axis_tready  = in_pyld_tready & ~gating;

  generate
    // Transfer packet info between clock domains:
    // in_pkt_info_* (axis_data_clk) to out_pkt_info_* (axis_chdr_clk).
    if (SYNC_CLKS) begin : gen_sync_info_fifo
      axi_fifo #(
        .WIDTH    (INFO_W),
        .SIZE     (INFO_FIFO_SIZE)
      ) pkt_info_fifo (
        .clk      (axis_chdr_clk),
        .reset    (axis_chdr_rst),
        .clear    (1'b0),
        .i_tdata  (in_pkt_info_tdata),
        .i_tvalid (in_pkt_info_tvalid),
        .i_tready (in_pkt_info_tready),
        .o_tdata  ({out_eob, out_eov, out_has_time, out_timestamp, out_length}),
        .o_tvalid (out_pkt_info_tvalid),
        .o_tready (out_pkt_info_tready),
        .space    (),
        .occupied ()
      );
    end else begin : gen_async_info_fifo
      axi_fifo_2clk #(
        .WIDTH    (INFO_W),
        .SIZE     (INFO_FIFO_SIZE)
      ) pkt_info_fifo (
        .reset    (axis_data_rst),
        .i_aclk   (axis_data_clk),
        .i_tdata  (in_pkt_info_tdata),
        .i_tvalid (in_pkt_info_tvalid),
        .i_tready (in_pkt_info_tready),
        .o_aclk   (axis_chdr_clk),
        .o_tdata  ({out_eob, out_eov, out_has_time, out_timestamp, out_length}),
        .o_tvalid (out_pkt_info_tvalid),
        .o_tready (out_pkt_info_tready)
      );
    end

    // Transfer packet payload between clock domains:
    // in_pyld_* (axis_data_clk) to out_pyld_* (axis_chdr_clk)
    if (NIPC != CHDR_W/ITEM_W) begin : gen_axis_width_conv
      wire [CHDR_W-1:0] pyld_resize_tdata;
      wire              pyld_resize_tlast;
      wire              pyld_resize_tvalid;
      wire              pyld_resize_tready;

      // Do the width conversion and clock crossing in the axis_width_conv
      // module to ensure that the resize happens on the correct side of the
      // clock crossing.
      axis_width_conv #(
        .WORD_W    (ITEM_W),
        .IN_WORDS  (NIPC),
        .OUT_WORDS (CHDR_W/ITEM_W),
        .SYNC_CLKS (SYNC_CLKS),
        .PIPELINE  ("IN")
      ) payload_width_conv_i (
        .s_axis_aclk   (axis_data_clk),
        .s_axis_rst    (axis_data_rst),
        .s_axis_tdata  (in_pyld_tdata),
        .s_axis_tkeep  ({NIPC{1'b1}}),
        .s_axis_tlast  (in_pyld_tlast),
        .s_axis_tvalid (in_pyld_tvalid),
        .s_axis_tready (in_pyld_tready),
        .m_axis_aclk   (axis_chdr_clk),
        .m_axis_rst    (axis_chdr_rst),
        .m_axis_tdata  (pyld_resize_tdata),
        .m_axis_tkeep  (),
        .m_axis_tlast  (pyld_resize_tlast),
        .m_axis_tvalid (pyld_resize_tvalid),
        .m_axis_tready (pyld_resize_tready)
      );

      axi_fifo #(
        .WIDTH    (CHDR_W+1),
        .SIZE     (PAYLOAD_FIFO_SIZE)
      ) pyld_fifo (
        .clk      (axis_chdr_clk),
        .reset    (axis_chdr_rst),
        .clear    (1'b0),
        .i_tdata  ({pyld_resize_tlast, pyld_resize_tdata}),
        .i_tvalid (pyld_resize_tvalid),
        .i_tready (pyld_resize_tready),
        .o_tdata  ({out_pyld_tlast, out_pyld_tdata}),
        .o_tvalid (out_pyld_tvalid),
        .o_tready (out_pyld_tready),
        .space    (),
        .occupied ()
      );
    end else begin : no_gen_axis_width_conv
      if (SYNC_CLKS) begin : gen_sync_pyld_fifo
        axi_fifo #(
          .WIDTH    (CHDR_W+1),
          .SIZE     (PAYLOAD_FIFO_SIZE)
        ) pyld_fifo (
          .clk      (axis_chdr_clk),
          .reset    (axis_chdr_rst),
          .clear    (1'b0),
          .i_tdata  ({in_pyld_tlast, in_pyld_tdata}),
          .i_tvalid (in_pyld_tvalid),
          .i_tready (in_pyld_tready),
          .o_tdata  ({out_pyld_tlast, out_pyld_tdata}),
          .o_tvalid (out_pyld_tvalid),
          .o_tready (out_pyld_tready),
          .space    (),
          .occupied ()
        );
      end else begin : gen_async_pyld_fifo
        axi_fifo_2clk #(
          .WIDTH    (CHDR_W+1),
          .SIZE     (PAYLOAD_FIFO_SIZE)
        ) pyld_fifo (
          .reset    (axis_data_rst),
          .i_aclk   (axis_data_clk),
          .i_tdata  ({in_pyld_tlast, in_pyld_tdata}),
          .i_tvalid (in_pyld_tvalid),
          .i_tready (in_pyld_tready),
          .o_aclk   (axis_chdr_clk),
          .o_tdata  ({out_pyld_tlast, out_pyld_tdata}),
          .o_tvalid (out_pyld_tvalid),
          .o_tready (out_pyld_tready)
        );
      end
    end
  endgenerate


  //---------------------------------------------------------------------------
  // Output State Machine
  //---------------------------------------------------------------------------

  reg  [CHDR_W-1:0] chdr_pf_tdata;
  reg               chdr_pf_tlast, chdr_pf_tvalid;
  wire              chdr_pf_tready;

  localparam [1:0] ST_HDR   = 0;   // Processing the output CHDR header
  localparam [1:0] ST_TS    = 1;   // Processing the output CHDR timestamp
  localparam [1:0] ST_PYLD  = 2;   // Processing the output CHDR payload word

  reg [1:0] state = ST_HDR;

  reg [15:0] seq_num = 0;

  wire [63:0] header;
  reg  [63:0] timestamp;
  wire [15:0] length;

  // Some the payload, metadata, and timestamp lengths (out_length already
  // includes the header).
  assign length = (CHDR_W > 64) ? out_length : out_length + 8*out_has_time;

  // Build the header word
  assign header = chdr_build_header(
    6'b0,                                  // vc
    out_eob,                               // eob
    out_eov,                               // eov
    out_has_time ? CHDR_PKT_TYPE_DATA_TS :
                   CHDR_PKT_TYPE_DATA,     // pkt_type
    0,                                     // num_mdata
    seq_num,                               // seq_num
    length,                                // length
    16'b0                                  // dst_epid
  );

  always @(posedge axis_chdr_clk) begin
    if (axis_chdr_rst) begin
      state   <= ST_HDR;
      seq_num <= 0;
    end else begin
      case (state)

        // ST_HDR: CHDR Header
        // -------------------
        ST_HDR: begin
          timestamp <= out_timestamp;

          if (out_pkt_info_tvalid && chdr_pf_tready) begin
            seq_num <= seq_num + 1;

            if (CHDR_W > 64) begin
              // When CHDR_W > 64, the timestamp is a part of the header word.
              // If this is a data packet (with or without a TS), we skip the
              // timestamp state move directly to the payload.
              state <= ST_PYLD;
            end else begin
              // When CHDR_W == 64, the timestamp comes after the header. Check
              // if this is a data packet with a timestamp to figure out the
              // next state.
              if (out_has_time) begin
                state <= ST_TS;
              end else begin
                state <= ST_PYLD;
              end
            end
          end
        end

        // ST_TS: Timestamp (CHDR_W == 64 only)
        // ------------------------------------
        ST_TS: begin
          if (chdr_pf_tready) begin
            state <= ST_PYLD;
          end
        end

        // ST_PYLD: Payload word
        // ---------------------
        ST_PYLD: begin
          if (out_pyld_tvalid && out_pyld_tready && out_pyld_tlast) begin
            state   <= ST_HDR;
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
    // Initialize to 0 to avoid latch on unassigned bits
    chdr_pf_tdata = {CHDR_W{1'b0}};
    case (state)
      ST_HDR: begin
        // Insert header word
        chdr_pf_tdata         = (CHDR_W > 64) ? { out_timestamp, header } : header;
        chdr_pf_tvalid        = out_pkt_info_tvalid;
        chdr_pf_tlast         = 1'b0;
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = chdr_pf_tready;   // Remove packet info word from FIFO
      end
      ST_TS: begin
        // Insert timestamp
        chdr_pf_tdata[63:0]   = timestamp;
        chdr_pf_tvalid        = 1'b1;  // Timestamp register is always valid in this state
        chdr_pf_tlast         = 1'b0;
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = 1'b0;
      end
      ST_PYLD: begin
        // Insert payload words
        chdr_pf_tdata         = out_pyld_tdata;
        chdr_pf_tvalid        = out_pyld_tvalid;
        chdr_pf_tlast         = out_pyld_tlast;
        out_pyld_tready       = chdr_pf_tready;
        out_pkt_info_tready   = 1'b0;
      end
      default: begin
        chdr_pf_tdata         = out_pyld_tdata;
        chdr_pf_tvalid        = 1'b0;
        chdr_pf_tlast         = 1'b0;
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = 1'b0;
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Flushing Logic
  //---------------------------------------------------------------------------

  axis_packet_flush #(
    .WIDTH              (CHDR_W),
    .FLUSH_PARTIAL_PKTS (0),
    .TIMEOUT_W          (32),
    .PIPELINE           ("IN")
  ) chdr_flusher_i (
    .clk           (axis_chdr_clk),
    .reset         (axis_chdr_rst),
    .enable        (flush_en),
    .timeout       (flush_timeout),
    .flushing      (flush_active),
    .done          (flush_done),
    .s_axis_tdata  (chdr_pf_tdata),
    .s_axis_tlast  (chdr_pf_tlast),
    .s_axis_tvalid (chdr_pf_tvalid),
    .s_axis_tready (chdr_pf_tready),
    .m_axis_tdata  (m_axis_chdr_tdata),
    .m_axis_tlast  (m_axis_chdr_tlast),
    .m_axis_tvalid (m_axis_chdr_tvalid),
    .m_axis_tready (m_axis_chdr_tready)
  );

endmodule
