//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_convert_down
//
// Description:
//
//   Takes a CHDR packet data stream that was generated using a CHDR width
//   (I_CHDR_W) that is wider than the current bus width (DATA_W) and reformats
//   the packet stream to use the CHDR_W equal to that of the current bus width
//   (DATA_W). It does not resize the bus, but rather only changes the CHDR_W
//   of the encoded packets.
//
//   Packets with different CHDR width have a different maximum number of
//   metadata bytes. This module repacks the the metadata into the new word
//   size, little-endian ordered. If there is too much metadata for the smaller
//   DATA_W packet, then the excess metadata will be discarded.
//
// Parameters:
//
//   I_CHDR_W : CHDR_W for the input data stream on i_chdr. Must be larger than
//              DATA_W.
//   DATA_W   : Width of the data bus, and the new CHDR_W for the output data
//              stream on o_chdr.
//   PIPELINE : Indicates whether to add pipeline stages to the input and/or
//              output. This can be: "NONE", "IN", "OUT", or "INOUT".

`default_nettype none


module chdr_convert_down #(
  parameter I_CHDR_W = 512,
  parameter DATA_W   = 64,
  parameter PIPELINE = "NONE"
) (
  input wire clk,
  input wire rst,

  // Input
  input  wire [DATA_W-1:0] i_chdr_tdata,
  input  wire              i_chdr_tlast,
  input  wire              i_chdr_tvalid,
  output wire              i_chdr_tready,

  // Output
  output wire [DATA_W-1:0] o_chdr_tdata,
  output wire              o_chdr_tlast,
  output wire              o_chdr_tvalid,
  input  wire              o_chdr_tready
);

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"

  // Calculate ceiling(N/D)
  `define DIV_CEIL(N,D) (((N)+(D)-1)/(D))


  //---------------------------------------------------------------------------
  // Check Parameters
  //---------------------------------------------------------------------------

  generate
    if (!(
      // Must be reducing the CHDR width
      (I_CHDR_W > DATA_W) &&
      // CHDR widths must be valid (at least 64 and powers of 2)
      (I_CHDR_W >= 64) &&
      (DATA_W   >= 64) &&
      (2**$clog2(I_CHDR_W) == I_CHDR_W) &&
      (2**$clog2(DATA_W)   == DATA_W)   &&
      // I_CHDR_W must be a multiple of DATA_W
      (I_CHDR_W % DATA_W == 0)
    )) begin : gen_error
      ERROR__Invalid_CHDR_or_data_width_parameters();
    end
  endgenerate


  //---------------------------------------------------------------------------
  // Input Register
  //---------------------------------------------------------------------------

  wire [DATA_W-1:0] i_pipe_tdata;
  wire              i_pipe_tlast;
  wire              i_pipe_tvalid;
  reg               i_pipe_tready;

  if (PIPELINE == "IN" || PIPELINE == "INOUT") begin : gen_in_pipeline
    // Add a pipeline stage
    axi_fifo_flop2 #(
      .WIDTH (1 + DATA_W)
    ) axi_fifo_flop2_i (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .i_tdata  ({i_chdr_tlast, i_chdr_tdata}),
      .i_tvalid (i_chdr_tvalid),
      .i_tready (i_chdr_tready),
      .o_tdata  ({i_pipe_tlast, i_pipe_tdata}),
      .o_tvalid (i_pipe_tvalid),
      .o_tready (i_pipe_tready),
      .space    (),
      .occupied ()
    );
  end else begin : gen_no_in_pipeline
    assign i_pipe_tdata  = i_chdr_tdata;
    assign i_pipe_tlast  = i_chdr_tlast;
    assign i_pipe_tvalid = i_chdr_tvalid;
    assign i_chdr_tready = i_pipe_tready;
  end


  //---------------------------------------------------------------------------
  // Downsize State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine does the translation from the larger CHDR_W to the
  // smaller CHDR_W by updating the header and dropping empty words.
  //
  //---------------------------------------------------------------------------

  // States
  localparam [2:0] ST_HDR        = 3'd0;   // CHDR header
  localparam [2:0] ST_TS         = 3'd1;   // CHDR timestamp
  localparam [2:0] ST_HDR_DROP   = 3'd2;   // CHDR header, drop unused words
  localparam [2:0] ST_MDATA      = 3'd3;   // CHDR metadata words
  localparam [2:0] ST_MDATA_DROP = 3'd4;   // CHDR metadata, drop unused words
  localparam [2:0] ST_PYLD       = 3'd5;   // CHDR payload words
  localparam [2:0] ST_PYLD_DROP  = 3'd6;   // CHDR payload, drop unused words
  localparam [2:0] ST_MGMT_PYLD  = 3'd7;   // CHDR management payload words

  reg [2:0] state = ST_HDR;

  // Determine the number of bits needed to represent the new number of
  // metadata words, which might be bigger than the allowed value of 31.
  localparam NUM_MDATA_W = $clog2(31*I_CHDR_W/DATA_W + 1);

  // Number of output words per input word
  localparam NUM_WORDS = I_CHDR_W/DATA_W;

  // Determine the number of bits needed to represent a counter to track which
  // CHDR words are valid and which are unused and need to be dropped.
  localparam COUNT_W = $clog2(NUM_WORDS);

  // Determine the maximum number DATA_W-sized payload words. The maximum
  // packet size is 2**16-1 bytes, then subtract one word for the smallest
  // possible header and convert that to a number of whole CHDR words.
  localparam NUM_PYLD_WORDS = `DIV_CEIL((2**16-1) - (DATA_W/8), DATA_W/8);

  // Determine the number of bits needed to represent a counter to track which
  // O_DATA_W payload word we are processing.
  localparam PYLD_COUNT_W = $clog2(NUM_PYLD_WORDS + 1);

  // Header info we need to save
  reg [ NUM_MDATA_W-1:0] i_num_mdata_reg;   // Input packet NumMData in terms of DATA_W words
  reg [             4:0] o_num_mdata_reg;   // Output packet NumMData to keep
  reg [             2:0] pkt_type_reg;      // Packet type
  reg [PYLD_COUNT_W-1:0] pyld_len_reg;      // Packet payload length in DATA_W words
  reg [PYLD_COUNT_W-1:0] mgmt_pyld_len_reg; // Management payload length in DATA_W words

  // Counters (number of DATA_W sized words processed on the input)
  reg [ NUM_MDATA_W-1:0] mdata_count;
  reg [PYLD_COUNT_W-1:0] pyld_count;
  reg [     COUNT_W-1:0] word_count;        // Zero based (starts at 0)

  // Shortcuts for CHDR header info
  wire [ 2:0] pkt_type       = chdr_get_pkt_type(i_pipe_tdata[63:0]);
  wire [15:0] pyld_len_bytes = chdr_calc_payload_length(I_CHDR_W, i_pipe_tdata[63:0]);

  // Calculate the payload length in DATA_W words
  wire [PYLD_COUNT_W-1:0] pyld_len = `DIV_CEIL(pyld_len_bytes, DATA_W/8);

  // Calculate the payload length of a management packet in words (management
  // packets have the same number of payload words, regardless of CHDR width).
  wire [PYLD_COUNT_W-1:0] mgmt_pyld_len =
    `DIV_CEIL(chdr_calc_payload_length(I_CHDR_W, i_pipe_tdata), I_CHDR_W/8);

  // Calculate NumMData from input packet in terms of DATA_W words
  wire [NUM_MDATA_W-1:0] i_num_mdata =
    chdr_get_num_mdata(i_pipe_tdata[63:0]) * (I_CHDR_W/DATA_W);
  // Calculate NumMData for output packet (limit to max of 31)
  wire [4:0] o_num_mdata = (i_num_mdata <= 31) ? i_num_mdata : 31;

  // Generate packet headers with updated NumMData and Length fields
  reg [DATA_W-1:0] new_header;
  always @(*) begin
    new_header = i_pipe_tdata;
    // Update NumMData
    new_header[63:0] = chdr_set_num_mdata(new_header, o_num_mdata);
    // Update packet length
    new_header[63:0] = chdr_update_length(DATA_W, new_header,
      (pkt_type == CHDR_PKT_TYPE_MGMT) ? mgmt_pyld_len * (DATA_W/8) : pyld_len_bytes);
  end

  reg [DATA_W-1:0] new_mgmt_header;
  always @(*) begin
    // Update the CHDRWidth field in the management header.
    new_mgmt_header = i_pipe_tdata;
    new_mgmt_header[63:0] =
      chdr_mgmt_set_chdr_w(i_pipe_tdata[63:0], chdr_w_to_enum(DATA_W));
  end


  always @(posedge clk) begin
    if (rst) begin
      state             <= ST_HDR;
      mdata_count       <= 'bX;
      pyld_count        <= 'bX;
      word_count        <= 'bX;
      pkt_type_reg      <= 'bX;
      pyld_len_reg      <= 'bX;
      mgmt_pyld_len_reg <= 'bX;
      i_num_mdata_reg   <= 'bX;
      o_num_mdata_reg   <= 'bX;
    end else if (i_pipe_tvalid & i_pipe_tready) begin
      // Default assignment
      word_count <= word_count + 1;

      case (state)

        // ST_HDR: CHDR Header
        ST_HDR: begin
          mdata_count       <= 1;    // The first metadata word will be word 1
          pyld_count        <= 1;    // The first payload word will be word 1
          word_count        <= 1;    // Word 0 is the current word (header)
          pkt_type_reg      <= pkt_type;
          pyld_len_reg      <= pyld_len;
          mgmt_pyld_len_reg <= mgmt_pyld_len;
          // Save number of DATA_W words of mdata we expect
          i_num_mdata_reg <= i_num_mdata;
          // Save the number of DATA_W words of mdata we can keep
          o_num_mdata_reg <= o_num_mdata;
          if (DATA_W == 64) begin
            if (pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
              // Next word must be the timestamp
              state <= ST_TS;
            end else begin
              // Next word(s) must be empty, so drop it
              state <= ST_HDR_DROP;
            end
          end else begin
            // DATA_W >= 128. We should have received the header word and
            // timestamp (if present) this clock cycle. Since I_CHDR_W >
            // DATA_W, there must be extra words with the header that we need
            // to drop.
            state <= ST_HDR_DROP;
          end
        end

        // ST_TS: Timestamp (DATA_W == 64 only)
        ST_TS: begin
          if (I_CHDR_W > 128) begin
            state <= ST_HDR_DROP;
          end else if (o_num_mdata_reg != 0) begin
            state <= ST_MDATA;
          end else begin
            state <= ST_PYLD;
          end
        end

        // ST_HDR_DROP: CHDR header, drop unused words
        ST_HDR_DROP: begin
          if (word_count == NUM_WORDS-1) begin
            if (o_num_mdata_reg != 0) begin
              state <= ST_MDATA;
            end else if(pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
              state <= ST_MGMT_PYLD;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_MDATA: Metadata words
        ST_MDATA: begin
          mdata_count <= mdata_count + 1;
          if (mdata_count == o_num_mdata_reg) begin
            if (mdata_count < i_num_mdata_reg) begin
              // There are more MDATA words to deal with than we can fit, so we
              // need to drop the rest.
              state <= ST_MDATA_DROP;
            end else if (pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
              state <= ST_MGMT_PYLD;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_MDATA_DROP: Drop excess metadata words
        ST_MDATA_DROP: begin
          mdata_count <= mdata_count + 1;
          if (mdata_count == i_num_mdata_reg) begin
            if (pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
              state <= ST_MGMT_PYLD;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_PYLD: Payload words
        ST_PYLD: begin
          pyld_count <= pyld_count + 1;
          if (i_pipe_tlast) begin
            state <= ST_HDR;
          end else if (pyld_count == pyld_len_reg) begin
            state <= ST_PYLD_DROP;
          end
        end

        // ST_PYLD_DROP: Payload, drop unused words
        ST_PYLD_DROP: begin
          // The input packet may have had empty words at the end if the
          // payload didn't fill the last CHDR word. We remove those here.
          if (i_pipe_tlast) begin
            state <= ST_HDR;
          end
        end

        // ST_MGMT_PYLD: Management words
        ST_MGMT_PYLD: begin
          // Management packets are different from other packet types in that
          // the payload is not serialized. In the new DATA_W, we'll have empty
          // words we need to discard. When word_count is zero, that's when we
          // have a valid word. For all other counts, we want to discard words.
          if (word_count == 0) begin
            pyld_count <= pyld_count + 1;
          end
          if (i_pipe_tlast) begin
            state <= ST_HDR;
          end
        end

      endcase
    end
  end


  //-----------------------------
  // State machine output logic
  //-----------------------------

  reg  [DATA_W-1:0] o_pipe_tdata;
  reg               o_pipe_tlast;
  reg               o_pipe_tvalid;
  wire              o_pipe_tready;

  always @(*) begin
    case (state)
      ST_HDR : begin
        o_pipe_tdata  = new_header;
        o_pipe_tlast  = i_pipe_tlast;
        o_pipe_tvalid = i_pipe_tvalid;
        i_pipe_tready = o_pipe_tready;
      end
      ST_TS : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tlast  = i_pipe_tlast;
        o_pipe_tvalid = i_pipe_tvalid;
        i_pipe_tready = o_pipe_tready;
      end
      ST_HDR_DROP : begin
        o_pipe_tdata  = { DATA_W {1'bX} };
        o_pipe_tlast  = 1'bX;
        o_pipe_tvalid = 1'b0;
        i_pipe_tready = 1'b1;
      end
      ST_MDATA : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tlast  = i_pipe_tlast;
        o_pipe_tvalid = i_pipe_tvalid;
        i_pipe_tready = o_pipe_tready;
      end
      ST_MDATA_DROP : begin
        o_pipe_tdata  = { DATA_W {1'bX} };
        o_pipe_tlast  = 1'bX;
        o_pipe_tvalid = 1'b0;
        i_pipe_tready = 1'b1;
      end
      ST_PYLD : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tlast  = (pyld_count == pyld_len_reg);
        o_pipe_tvalid = i_pipe_tvalid;
        i_pipe_tready = o_pipe_tready;
      end
      ST_PYLD_DROP : begin
        o_pipe_tdata  = { DATA_W {1'bX} };
        o_pipe_tlast  = 1'bX;
        o_pipe_tvalid = 1'b0;
        i_pipe_tready = 1'b1;
      end
      ST_MGMT_PYLD : begin
        if (word_count == 0) begin
          o_pipe_tdata  = (pyld_count == 1) ? new_mgmt_header : i_pipe_tdata;
          o_pipe_tlast  = (pyld_count == mgmt_pyld_len_reg);
          o_pipe_tvalid = i_pipe_tvalid;
          i_pipe_tready = o_pipe_tready;
        end else begin
          // Drop unused management payload words
          o_pipe_tdata  = { DATA_W {1'bX} };
          o_pipe_tlast  = 1'bX;
          o_pipe_tvalid = 1'b0;
          i_pipe_tready = 1'b1;
        end
      end
      default : begin
        o_pipe_tdata  = { DATA_W {1'bX} };
        o_pipe_tlast  = 1'bX;
        o_pipe_tvalid = 1'bX;
        i_pipe_tready = 1'bX;
      end
    endcase
  end


  //---------------------------------------------------------------------------
  // Output Register
  //---------------------------------------------------------------------------

  if (PIPELINE == "OUT" || PIPELINE == "INOUT") begin : gen_out_pipeline
    // Add a pipeline stage
    axi_fifo_flop2 #(
      .WIDTH (1 + DATA_W)
    ) axi_fifo_flop2_i (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .i_tdata  ({ o_pipe_tlast, o_pipe_tdata }),
      .i_tvalid (o_pipe_tvalid),
      .i_tready (o_pipe_tready),
      .o_tdata  ({ o_chdr_tlast, o_chdr_tdata }),
      .o_tvalid (o_chdr_tvalid),
      .o_tready (o_chdr_tready),
      .space    (),
      .occupied ()
    );
  end else begin : gen_no_out_pipeline
    assign o_chdr_tdata  = o_pipe_tdata;
    assign o_chdr_tlast  = o_pipe_tlast;
    assign o_chdr_tvalid = o_pipe_tvalid;
    assign o_pipe_tready = o_chdr_tready;
  end

endmodule


`default_nettype wire
