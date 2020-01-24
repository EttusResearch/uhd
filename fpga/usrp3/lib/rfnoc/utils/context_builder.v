//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: context_builder
//
// Description:
//
// This module builds the payload and context data streams necessary for RFnoC 
// communication through an AXI-Stream Raw Data (Simple Interface). It takes as 
// input an AXI-Stream data bus and sideband buses containing the timestamp and 
// packet flags.
//
// For each AXI-Stream raw data packet that is input, the same data packet will 
// be output in the payload stream along with the context stream that's 
// necessary to create a CHDR packet for this data packet.
//
// The timestamp and flags must be input coincident with the AXI-Stream data 
// input. The timestamp and flag inputs will be sampled coincident with the 
// last word of data in the packet (i.e., when tlast is asserted).
//
// In order to determine the length of the packet, the entire packet is 
// buffered before the header in the context stream is generated. Therefore, 
// the internal FIFO size (configured by MTU) must be large enough to buffer 
// the maximum packet size.
//
// The maximum number of packets that can be simultaneously buffered in this 
// block is limited by INFO_FIFO_SIZE, where the maximum number of packets is 
// 2**INFO_FIFO_SIZE. This must be large enough to handle the expected worse 
// case, or data flow will stall.
//
// Parameters:
//
//   CHDR_W         : Width of the CHDR interface (width of context words)
//   ITEM_W         : Number of samples/items per data word
//   NIPC           : Number of samples/items per clock cycle
//   MTU            : Log2 of maximum transfer unit (maximum packet size) in CHDR_W sized words.
//   INFO_FIFO_SIZE : Size of the internal packet info FIFO is 2**INFO_FIFO_SIZE
//

module context_builder #(
  parameter CHDR_W         = 64,
  parameter ITEM_W         = 32,
  parameter NIPC           = 2,
  parameter MTU            = 10,
  parameter INFO_FIFO_SIZE = 5
) (
  input axis_data_clk,
  input axis_data_rst,

  // Data stream in (AXI-Stream)
  input  wire [(ITEM_W*NIPC)-1:0] s_axis_tdata,
  input  wire [         NIPC-1:0] s_axis_tkeep,
  input  wire                     s_axis_tlast,
  input  wire                     s_axis_tvalid,
  output wire                     s_axis_tready,
  // Sideband info (sampled on the first cycle of the packet)
  input  wire [             63:0] s_axis_ttimestamp,
  input  wire                     s_axis_thas_time,
  input  wire                     s_axis_teov,
  input  wire                     s_axis_teob,

  // Data stream out (AXI-Stream Payload)
  output wire [(ITEM_W*NIPC)-1:0] m_axis_payload_tdata,
  output wire [         NIPC-1:0] m_axis_payload_tkeep,
  output wire                     m_axis_payload_tlast,
  output wire                     m_axis_payload_tvalid,
  input  wire                     m_axis_payload_tready,

  // Data stream out (AXI-Stream Context)
  output reg  [CHDR_W-1:0] m_axis_context_tdata,
  output reg  [       3:0] m_axis_context_tuser,
  output reg               m_axis_context_tlast,
  output reg               m_axis_context_tvalid = 1'b0,
  input  wire              m_axis_context_tready
);
  `include "../core/rfnoc_chdr_utils.vh"


  reg packet_info_fifo_full;


  //---------------------------------------------------------------------------
  // Data FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO buffers packet data while we calculate each packet's length.
  //
  //---------------------------------------------------------------------------

  wire s_axis_tvalid_df;
  wire s_axis_tready_df;

  // Compute MTU (maximum packet) size in data words from the CHDR word MTU.
  localparam DATA_FIFO_SIZE = MTU + $clog2(CHDR_W) - $clog2(ITEM_W*NIPC);

  axi_fifo #(
    .WIDTH (NIPC + 1 + ITEM_W*NIPC),
    .SIZE  (DATA_FIFO_SIZE)
  ) data_fifo (
    .clk      (axis_data_clk),
    .reset    (axis_data_rst),
    .clear    (1'b0),
    .i_tdata  ({s_axis_tkeep, s_axis_tlast, s_axis_tdata}),
    .i_tvalid (s_axis_tvalid_df),
    .i_tready (s_axis_tready_df),
    .o_tdata  ({m_axis_payload_tkeep, m_axis_payload_tlast, m_axis_payload_tdata}),
    .o_tvalid (m_axis_payload_tvalid),
    .o_tready (m_axis_payload_tready),
    .space    (),
    .occupied ()
  );

  // To prevent the packet info FIFO from overflowing, we block the input of
  // new packets to the data FIFO whenever the packet info FIFO fills up.
  assign s_axis_tready    = s_axis_tready_df & ~packet_info_fifo_full;
  assign s_axis_tvalid_df = s_axis_tvalid    & ~packet_info_fifo_full;


  //---------------------------------------------------------------------------
  // Timestamp and Flags Capture
  //---------------------------------------------------------------------------
  //
  // The timestamp and flags that we use for each packet is that of the last 
  // data word. This maintains compatibility with how tuser was used on old 
  // RFnoC. Here, we capture this information at the start of the packet. At 
  // the end of the packet, when the length is known, this value will be 
  // inserted into the packet info FIFO.
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
  // Length Counter
  //---------------------------------------------------------------------------
  //
  // Here We track the state of the incoming packet to determine its length.
  //
  //---------------------------------------------------------------------------

  reg [15:0] packet_length, length_count;
  reg        packet_length_valid;

  always @(posedge axis_data_clk) begin : length_counter
    if (axis_data_rst) begin
      length_count        <= 0;
      packet_length       <= 0;
      packet_length_valid <= 1'b0;
    end else begin : length_counter_main
      // Calculate the length of this word in bytes, taking tkeep into account
      integer i;
      integer num_bytes;
      num_bytes = 0;
      for (i = 0; i < NIPC; i = i + 1) begin
        num_bytes = num_bytes + (s_axis_tkeep[i]*(ITEM_W/8));
      end

      // Update the packet length if the word is accepted
      packet_length_valid <= 1'b0;
      if (s_axis_tvalid & s_axis_tready) begin
        length_count  <= length_count + num_bytes;

        if (s_axis_tlast) begin
          length_count        <= 0;
          packet_length       <= length_count + num_bytes;
          packet_length_valid <= 1'b1;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // Packet Info FIFO
  //---------------------------------------------------------------------------
  //
  // This FIFO stores the packet info (length, timestamp, flags) for each fully 
  // received packet. Due to AXI-Stream flow control, we may end up with 
  // multiple packets being buffered in the data_fifo. The packet_info_fifo 
  // here stores each packet's info until the packet is ready to go out.
  //
  //---------------------------------------------------------------------------

  wire [63:0] next_packet_timestamp;
  wire        next_packet_has_time;
  wire        next_packet_eob;
  wire        next_packet_eov;
  wire [15:0] next_packet_length;
  wire [15:0] packet_info_space;
  wire        packet_info_valid;
  reg         packet_info_ready = 1'b0;

  axi_fifo #(
    .WIDTH (3 + 64 + 16),
    .SIZE  (INFO_FIFO_SIZE)
  ) packet_info_fifo (
    .clk      (axis_data_clk),
    .reset    (axis_data_rst),
    .clear    (1'b0),
    .i_tdata  ({packet_eov,
                packet_eob,
                packet_has_time,
                packet_timestamp,
                packet_length}),
    .i_tvalid (packet_length_valid),
    .i_tready (),
    .o_tdata  ({next_packet_eov,
                next_packet_eob,
                next_packet_has_time,
                next_packet_timestamp,
                next_packet_length}),
    .o_tvalid (packet_info_valid),
    .o_tready (packet_info_ready),
    .space    (packet_info_space),
    .occupied ()
  );


  // Create a register to indicate when the FIFO is (almost) full. We leave 
  // some space so that we can accept a new packet during the delay before data 
  // transfer gets blocked.
  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      packet_info_fifo_full <= 1'b0;
    end else begin
      if (packet_info_space < 4) begin
        packet_info_fifo_full <= 1'b1;
      end else begin
        packet_info_fifo_full <= 1'b0;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Context State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine controls generation of the context packets (containing 
  // the header and timestamp) that are output on m_axis_context, which will be 
  // needed to create the CHDR packet.
  //
  //---------------------------------------------------------------------------

  localparam ST_IDLE         = 0;
  localparam ST_HEADER       = 1;
  localparam ST_TIMESTAMP    = 2;

  reg [ 1:0] state   = ST_IDLE; // Current context FSM state
  reg [15:0] seq_num = 0;       // CHDR sequence number

  reg [15:0] chdr_length;
  reg [ 2:0] chdr_pkt_type;
  reg [63:0] chdr_header;


  always @(*) begin : calc_chdr_header
    // Calculate byte length of the CHDR packet by adding the header and 
    // timestamp length to the length of the payload.
    if (CHDR_W == 64) begin
      // If CHDR_W is 64-bit, timestamp is in a separate word
      if (next_packet_has_time) begin
        chdr_length = next_packet_length + 16;      // Add two 64-bit CHDR words
      end else begin
        chdr_length = next_packet_length + 8;       // Add one 64-bit CHDR word
      end
    end else begin
      // If CHDR_W is 128-bit or larger, timestamp is in the same word as the header
      chdr_length = next_packet_length + CHDR_W/8;  // Add one CHDR word
    end

    // Determine the packet type
    if (next_packet_has_time) begin
      chdr_pkt_type = CHDR_PKT_TYPE_DATA_TS;
    end else begin
      chdr_pkt_type = CHDR_PKT_TYPE_DATA;
    end

    // Build up header
    chdr_header = chdr_build_header(
      6'b0,                // vc
      next_packet_eob,     // eob
      next_packet_eov,     // eov
      chdr_pkt_type,       // pkt_type
      0,                   // num_mdata
      seq_num,             // seq_num
      chdr_length,         // length of CHDR packet in bytes
      0                    // dst_epid
    );
  end


  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      state                 <= ST_IDLE;
      seq_num               <= 'd0;
      packet_info_ready     <= 1'b0;
      m_axis_context_tvalid <= 1'b0;
    end else begin
      packet_info_ready <= 1'b0;

      if (CHDR_W == 64) begin : gen_ctx_fsm_64
        // For 64-bit CHDR_W, we require two words, one for the header and one 
        // for the timestamp.
        case (state)
          ST_IDLE: begin
            m_axis_context_tdata <= chdr_header;
            m_axis_context_tuser <= CONTEXT_FIELD_HDR;
            m_axis_context_tlast <= !next_packet_has_time;
            if (packet_info_valid && !packet_info_ready) begin
              m_axis_context_tvalid <= 1'b1;
              seq_num               <= seq_num + 1;
              state                 <= ST_HEADER;
            end
          end

          ST_HEADER : begin
            // Wait for header to be accepted
            if (m_axis_context_tready) begin
              packet_info_ready    <= 1'b1;
              m_axis_context_tdata <= next_packet_timestamp;
              if (next_packet_has_time) begin
                m_axis_context_tlast <= 1'b1;
                m_axis_context_tuser <= CONTEXT_FIELD_TS;
                state                <= ST_TIMESTAMP;
              end else begin
                m_axis_context_tlast  <= 1'b0;
                m_axis_context_tvalid <= 1'b0;
                state                 <= ST_IDLE;
              end
            end
          end

          ST_TIMESTAMP : begin
            // Wait for timestamp to be accepted
            if (m_axis_context_tready) begin
              m_axis_context_tvalid <= 1'b0;
              state                 <= ST_IDLE;
            end
          end

          default: state <= ST_IDLE;
        endcase

      end else begin : gen_ctx_fsm_128
        // For 128-bit and larger CHDR_W, we need the header and timestamp in
        // the same word.
        case (state)
          ST_IDLE: begin
            m_axis_context_tdata <= { next_packet_timestamp, chdr_header };
            m_axis_context_tuser <= next_packet_has_time ? CONTEXT_FIELD_HDR_TS :
                                                           CONTEXT_FIELD_HDR;
            m_axis_context_tlast <= 1'b1;
            if (packet_info_valid) begin
              m_axis_context_tvalid <= 1'b1;
              seq_num               <= seq_num + 1;
              packet_info_ready     <= 1'b1;
              state                 <= ST_HEADER;
            end
          end

          ST_HEADER : begin
            // Wait for header to be accepted
            if (m_axis_context_tready) begin
              m_axis_context_tvalid <= 1'b0;
              state                 <= ST_IDLE;
            end
          end

          default : state <= ST_IDLE;
        endcase

      end
    end
  end

endmodule
