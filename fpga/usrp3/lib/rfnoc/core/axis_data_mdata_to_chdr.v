//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_data_mdata_to_chdr
//
// Description:
//
//   A framer module for CHDR data packets. It accepts an input data stream 
//   (with sideband information for packet flags and timestamp) and a separate 
//   metadata stream. A data packet and a metadata packet are required to be 
//   input in order for a single CHDR packet to be generated. If no metadata is 
//   associated with the payload, then an empty metadata packet must be input 
//   along with the data packet (i.e., input a metadata packet with 
//   s_axis_mdata_tkeep set to 0).
//
//   The sideband information (e.g., timestamp, flags) must be input coincident 
//   with the AXI-Stream data input and will be sampled coincident with the 
//   last word of data in the packet (i.e., when tlast is asserted).
//
//   This module also performs an optional clock crossing and data width 
//   conversion from a user requested width for the payload bus to CHDR_W.
//
//   In order to guarantee a gapless CHDR data stream, the metadata packet 
//   should be input before the end of the data packet, although this is not 
//   required.
//
// Parameters:
//
//   CHDR_W         : Width of the input CHDR bus in bits
//   ITEM_W         : Width of the output item bus in bits
//   NIPC           : The number of output items delivered per cycle
//   SYNC_CLKS      : Are the CHDR and data clocks synchronous to each other?
//   MTU            : Log2 of the maximum packet size in CHDR words
//   INFO_FIFO_SIZE : Log2 of the info FIFO size. This determines the number of 
//                    packets that can be simultaneously buffered in the 
//                    payload FIFO.
//   PYLD_FIFO_SIZE : Log2 of the payload FIFO size. The actual FIFO size will 
//                    be the maximum of 2**MTU or 2**PYLD_FIFO_SIZE, since the 
//                    FIFO must be at least one MTU so that we can calculate 
//                    the packet length in the header.
//
// Signals:
//
//   m_axis_chdr_*  : Output CHDR stream
//   s_axis_*       : Input data stream (AXI-Stream)
//   s_axis_mdata_* : Input metadata stream (AXI-Stream)
//   flush_*        : Signals for flush control and status
//

module axis_data_mdata_to_chdr #(
  parameter CHDR_W         = 256,
  parameter ITEM_W         = 32,
  parameter NIPC           = 2,
  parameter SYNC_CLKS      = 0,
  parameter MTU            = 10,
  parameter INFO_FIFO_SIZE = 4,
  parameter PYLD_FIFO_SIZE = MTU
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
  input  wire                     s_axis_teov,
  input  wire                     s_axis_teob,
  // Metadata stream in (AXI-Stream)
  input  wire [CHDR_W-1:0]        s_axis_mdata_tdata,
  input  wire                     s_axis_mdata_tlast,
  input  wire                     s_axis_mdata_tkeep,
  input  wire                     s_axis_mdata_tvalid,
  output wire                     s_axis_mdata_tready,
  // Flush signals
  input  wire                     flush_en,
  input  wire [31:0]              flush_timeout,
  output wire                     flush_active,
  output wire                     flush_done
);

  // Make sure the metadata FIFO is large enough to store an entire packet's 
  // worth of metadata (32 words).
  localparam MDATA_FIFO_SIZE = 5;

  // Make sure the payload FIFO is large enough to store an entire packet's 
  // worth of payload data. This will ensure that we can buffer the entire 
  // packet to calculate its length.
  localparam PAYLOAD_FIFO_SIZE = PYLD_FIFO_SIZE > MTU ? 
                                 PYLD_FIFO_SIZE : MTU;


  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------

  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"


  //---------------------------------------------------------------------------
  // Timestamp and Flags Capture
  //---------------------------------------------------------------------------
  //
  // The timestamp and flags that we use for each packet is that of the last 
  // data word. Here, we capture this information at the end of the packet. 
  //
  //---------------------------------------------------------------------------

  reg [63:0] packet_timestamp;
  reg        packet_has_time;
  reg        packet_eov;
  reg        packet_eob;

  always @(posedge axis_data_clk) begin
    if (s_axis_tvalid & s_axis_tready & s_axis_tlast) begin
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
  // and mdata length.
  //
  //---------------------------------------------------------------------------

  localparam HDR_LEN = CHDR_W/8;      // Length of CHDR header word in bytes

  reg [15:0] packet_length;
  reg [15:0] length_count       = HDR_LEN;
  reg        in_pkt_info_tvalid = 0;
  wire       in_pkt_info_tready;

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


  reg [4:0] num_mdata            = 0;
  reg [4:0] mdata_count          = 0;
  reg       in_mdata_info_tvalid = 0;
  wire      in_mdata_info_tready;

  always @(posedge axis_data_clk) begin : num_mdata_counter
    if (axis_data_rst) begin
      mdata_count          <= 0;
      num_mdata            <= 0;
      in_mdata_info_tvalid <= 1'b0;
    end else begin : num_mdata_counter_main
      // Update the mdata length if the word is accepted
      in_mdata_info_tvalid <= 1'b0;
      if (s_axis_mdata_tvalid && s_axis_mdata_tready) begin
        if (s_axis_mdata_tlast) begin
          mdata_count          <= 0;
          num_mdata            <= mdata_count + s_axis_mdata_tkeep;
          in_mdata_info_tvalid <= 1'b1;
        end else begin
          mdata_count <= mdata_count + s_axis_mdata_tkeep;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  //  Data Width Converter (ITEM_W*NIPC => CHDR_W)
  //---------------------------------------------------------------------------

  wire [CHDR_W-1:0] in_pyld_tdata;
  wire              in_pyld_tlast;
  wire              in_pyld_tvalid;
  wire              in_pyld_tready;
  wire              width_conv_tready;

  assign width_conv_tready = in_pyld_tready & in_pkt_info_tready;

  generate
    if (NIPC != CHDR_W/ITEM_W) begin : gen_axis_width_conv
      axis_width_conv #(
        .WORD_W    (ITEM_W),
        .IN_WORDS  (NIPC),
        .OUT_WORDS (CHDR_W/ITEM_W),
        .SYNC_CLKS (1),
        .PIPELINE  ("IN")
      ) payload_width_conv_i (
        .s_axis_aclk   (axis_data_clk),
        .s_axis_rst    (axis_data_rst),
        .s_axis_tdata  (s_axis_tdata),
        .s_axis_tkeep  ({NIPC{1'b1}}),
        .s_axis_tlast  (s_axis_tlast),
        .s_axis_tvalid (s_axis_tvalid),
        .s_axis_tready (s_axis_tready),
        .m_axis_aclk   (axis_data_clk),
        .m_axis_rst    (axis_data_rst),
        .m_axis_tdata  (in_pyld_tdata),
        .m_axis_tkeep  (),
        .m_axis_tlast  (in_pyld_tlast),
        .m_axis_tvalid (in_pyld_tvalid),
        .m_axis_tready (width_conv_tready)
      );
    end else begin : no_gen_axis_width_conv
      assign in_pyld_tdata  = s_axis_tdata;
      assign in_pyld_tlast  = s_axis_tlast;
      assign in_pyld_tvalid = s_axis_tvalid;
      assign s_axis_tready  = width_conv_tready;
    end
  endgenerate


  //---------------------------------------------------------------------------
  // Input FIFOs
  //---------------------------------------------------------------------------
  //
  // Buffer the data, packet info, metadata, and cross it into the CHDR clock 
  // domain, if needed. The payload FIFO is sized to match the MTU so that an 
  // entire packet can be buffered while the length is calculated.
  //
  //---------------------------------------------------------------------------

  wire [CHDR_W-1:0] out_mdata_tdata,  out_pyld_tdata;
  wire              out_mdata_tlast,  out_pyld_tlast;
  wire              out_mdata_tvalid, out_pyld_tvalid;
  reg               out_mdata_tready, out_pyld_tready;

  wire        out_pkt_info_tvalid;
  reg         out_pkt_info_tready;
  wire        out_eob, out_eov, out_has_time;
  wire [63:0] out_timestamp;
  wire [15:0] out_length;

  wire [4:0] out_num_mdata;
  reg        out_mdata_info_tready;
  wire       out_mdata_info_tvalid;

  wire in_mdata_tready;


  assign s_axis_mdata_tready = in_mdata_tready & in_mdata_info_tready;

  generate if (SYNC_CLKS) begin : gen_sync_fifo
    axi_fifo #(
      .WIDTH (CHDR_W+1),
      .SIZE  (PAYLOAD_FIFO_SIZE)
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
    axi_fifo #(
      .WIDTH (CHDR_W + 1),
      .SIZE  (MDATA_FIFO_SIZE)
    ) mdata_fifo (
      .clk      (axis_chdr_clk),
      .reset    (axis_chdr_rst),
      .clear    (1'b0),
      .i_tdata  ({s_axis_mdata_tlast, s_axis_mdata_tdata}),
      .i_tvalid (s_axis_mdata_tvalid),
      .i_tready (in_mdata_tready),
      .o_tdata  ({out_mdata_tlast, out_mdata_tdata}),
      .o_tvalid (out_mdata_tvalid),
      .o_tready (out_mdata_tready),
      .space    (),
      .occupied ()
    );
    axi_fifo #(
      .WIDTH (3 + 64 + 16),
      .SIZE  (INFO_FIFO_SIZE)
    ) pkt_info_fifo (
      .clk      (axis_chdr_clk),
      .reset    (axis_chdr_rst),
      .clear    (1'b0),
      .i_tdata  ({packet_eob, packet_eov, packet_has_time,packet_timestamp, packet_length}),
      .i_tvalid (in_pkt_info_tvalid),
      .i_tready (in_pkt_info_tready),
      .o_tdata  ({out_eob, out_eov, out_has_time, out_timestamp, out_length}),
      .o_tvalid (out_pkt_info_tvalid),
      .o_tready (out_pkt_info_tready),
      .space    (),
      .occupied ()
    );
    axi_fifo #(
      .WIDTH (5),
      .SIZE  (INFO_FIFO_SIZE)
    ) mdata_info_fifo (
      .clk      (axis_chdr_clk),
      .reset    (axis_chdr_rst),
      .clear    (1'b0),
      .i_tdata  (num_mdata),
      .i_tvalid (in_mdata_info_tvalid),
      .i_tready (in_mdata_info_tready),
      .o_tdata  (out_num_mdata),
      .o_tvalid (out_mdata_info_tvalid),
      .o_tready (out_mdata_info_tready),
      .space    (),
      .occupied ()
    );

  end else begin : gen_async_fifo
    axi_fifo_2clk #(
      .WIDTH (CHDR_W + 1),
      .SIZE  (PAYLOAD_FIFO_SIZE)
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
    axi_fifo_2clk #(
      .WIDTH (CHDR_W + 1),
      .SIZE  (MDATA_FIFO_SIZE)
    ) mdata_fifo (
      .reset    (axis_data_rst),
      .i_aclk   (axis_data_clk),
      .i_tdata  ({s_axis_mdata_tlast, s_axis_mdata_tdata}),
      .i_tvalid (s_axis_mdata_tvalid),
      .i_tready (in_mdata_tready),
      .o_aclk   (axis_chdr_clk),
      .o_tdata  ({out_mdata_tlast, out_mdata_tdata}),
      .o_tvalid (out_mdata_tvalid),
      .o_tready (out_mdata_tready)
    );
    axi_fifo_2clk #(
      .WIDTH (3 + 64 + 16),
      .SIZE  (INFO_FIFO_SIZE)
    ) pkt_info_fifo (
      .reset    (axis_data_rst),
      .i_aclk   (axis_data_clk),
      .i_tdata  ({packet_eob, packet_eov, packet_has_time,packet_timestamp, packet_length}),
      .i_tvalid (in_pkt_info_tvalid),
      .i_tready (in_pkt_info_tready),
      .o_aclk   (axis_chdr_clk),
      .o_tdata  ({out_eob, out_eov, out_has_time, out_timestamp, out_length}),
      .o_tvalid (out_pkt_info_tvalid),
      .o_tready (out_pkt_info_tready)
    );
    axi_fifo_2clk #(
      .WIDTH (5),
      .SIZE  (INFO_FIFO_SIZE)
    ) mdata_info_fifo (
      .reset    (axis_data_rst),
      .i_aclk   (axis_data_clk),
      .i_tdata  (num_mdata),
      .i_tvalid (in_mdata_info_tvalid),
      .i_tready (in_mdata_info_tready),
      .o_aclk   (axis_chdr_clk),
      .o_tdata  (out_num_mdata),
      .o_tvalid (out_mdata_info_tvalid),
      .o_tready (out_mdata_info_tready)
    );
  end endgenerate


  //---------------------------------------------------------------------------
  // Output State Machine
  //---------------------------------------------------------------------------

  reg  [CHDR_W-1:0] chdr_pf_tdata;
  reg               chdr_pf_tlast, chdr_pf_tvalid;
  wire              chdr_pf_tready;

  localparam [1:0] ST_HDR   = 0;   // Processing the output CHDR header       
  localparam [1:0] ST_TS    = 1;   // Processing the output CHDR timestamp    
  localparam [1:0] ST_MDATA = 2;   // Processing the output CHDR metadata word
  localparam [1:0] ST_PYLD  = 3;   // Processing the output CHDR payload word 

  reg [1:0] state = ST_HDR;

  reg [15:0] seq_num = 0;

  wire [63:0] header;
  reg  [63:0] timestamp;
  wire [15:0] length;
  reg         has_mdata;

  // Some the payload, metadata, and timestamp lengths (out_length already 
  // includes the header).
  assign length = (CHDR_W > 64) ?
    out_length + out_num_mdata * (CHDR_W/8) :
    out_length + out_num_mdata * (CHDR_W/8) + 8*out_has_time;

  // Build the header word
  assign header = chdr_build_header(
    6'b0,                                  // vc
    out_eob,                               // eob
    out_eov,                               // eov
    out_has_time ? CHDR_PKT_TYPE_DATA_TS :
                   CHDR_PKT_TYPE_DATA,     // pkt_type
    out_num_mdata,                         // num_mdata
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
          has_mdata <= (out_num_mdata != CHDR_NO_MDATA);

          if (out_pkt_info_tvalid && out_mdata_info_tvalid && chdr_pf_tready) begin
            if (CHDR_W > 64) begin
              // When CHDR_W > 64, the timestamp is a part of the header word. 
              // If this is a data packet (with or without a TS), we skip the 
              // timestamp state move directly to metadata/body.
              if (out_num_mdata == CHDR_NO_MDATA) begin
                state <= ST_PYLD;
              end else begin
                state <= ST_MDATA;
              end
            end else begin
              // When CHDR_W == 64, the timestamp comes after the header. Check 
              // if this is a data packet with a timestamp or metadata to 
              // figure out the next state.
              if (out_has_time) begin
                state <= ST_TS;
              end else if (out_num_mdata != CHDR_NO_MDATA) begin
                state <= ST_MDATA;
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
            state <= has_mdata ? ST_MDATA : ST_PYLD;
          end
        end

        // ST_MDATA: Metadata word
        // -----------------------
        ST_MDATA: begin
          if (out_mdata_tvalid && out_mdata_tready && out_mdata_tlast) begin
            state <= ST_PYLD;
          end
        end

        // ST_PYLD: Payload word
        // ---------------------
        ST_PYLD: begin
          if (out_pyld_tvalid && out_pyld_tready && out_pyld_tlast) begin
            state   <= ST_HDR;
            seq_num <= seq_num + 1;
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
        // Insert header word
        chdr_pf_tdata         = (CHDR_W > 64) ? { out_timestamp, header } : header;
        chdr_pf_tvalid        = out_pkt_info_tvalid & out_mdata_info_tvalid;
        chdr_pf_tlast         = 1'b0;
        out_mdata_tready      = chdr_pf_tready &  // Remove empty mdata packet from FIFO
                                (out_num_mdata == CHDR_NO_MDATA);
        out_mdata_info_tready = chdr_pf_tready;   // Remove mdata info word from FIFO
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = chdr_pf_tready;   // Remove packet info word from FIFO
      end
      ST_TS: begin
        // Insert timestamp
        chdr_pf_tdata[63:0]   = timestamp;
        chdr_pf_tvalid        = 1'b1;  // Timestamp register is always valid in this state
        chdr_pf_tlast         = 1'b0;
        out_mdata_tready      = 1'b0;
        out_mdata_info_tready = 1'b0;
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = 1'b0;
      end
      ST_MDATA: begin
        // Insert mdata words
        chdr_pf_tdata         = out_mdata_tdata;
        chdr_pf_tvalid        = out_mdata_tvalid;
        chdr_pf_tlast         = 1'b0;
        out_mdata_tready      = chdr_pf_tready;
        out_mdata_info_tready = 1'b0;
        out_pyld_tready       = 1'b0;
        out_pkt_info_tready   = 1'b0;
      end
      ST_PYLD: begin
        // Insert payload words
        chdr_pf_tdata         = out_pyld_tdata;
        chdr_pf_tvalid        = out_pyld_tvalid;
        chdr_pf_tlast         = out_pyld_tlast;
        out_mdata_tready      = 1'b0;
        out_mdata_info_tready = 1'b0;
        out_pyld_tready       = chdr_pf_tready;
        out_pkt_info_tready   = 1'b0;
      end
      default: begin
        chdr_pf_tdata         = out_pyld_tdata;
        chdr_pf_tvalid        = 1'b0;
        chdr_pf_tlast         = 1'b0;
        out_mdata_tready      = 1'b0;
        out_mdata_info_tready = 1'b0;
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
