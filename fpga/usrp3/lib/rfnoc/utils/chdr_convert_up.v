//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_convert_up
//
// Description:
//
//   Takes a CHDR packet data stream that was generated using a CHDR width
//   equal to the current bust width (DATA_W) and reformats the packet stream
//   to use a wider width (O_CHDR_W). It does not resize the bus, but rather
//   only changes the CHDR_W of the encoded packets.
//
//   The metadata might not be a nice multiple of O_CHDR_W sized words. This
//   module repacks the metadata into the new word size, little-endian ordered,
//   and pads the last metadata word with zeros if necessary.
//
// Parameters:
//
//   DATA_W  :  The width of the data bus and the input CHDR width for the
//              input data stream on i_chdr.
//   O_CHDR_W : CHDR_W for the output data stream on o_chdr. Must be larger
//              than DATA_W.
//   PIPELINE : Indicates whether to add pipeline stages to the input and/or
//              output. This can be: "NONE", "IN", "OUT", or "INOUT".
//

`default_nettype none


module chdr_convert_up #(
  parameter DATA_W   = 64,
  parameter O_CHDR_W = 512,
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
      // Must be up-sizing
      (DATA_W < O_CHDR_W) &&
      // CHDR widths must be valid (at least 64 and powers of 2)
      (DATA_W   >= 64) &&
      (O_CHDR_W >= 64) &&
      (2**$clog2(DATA_W)   == DATA_W)   &&
      (2**$clog2(O_CHDR_W) == O_CHDR_W) &&
      // O_CHDR_W must be a multiple of DATA_W
      (O_CHDR_W % DATA_W == 0)
    )) begin : gen_error
      ERROR__Invalid_CHDR_W_parameters();
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
  // Up-size State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine does the translation from the smaller CHDR_W to the
  // larger CHDR_W by updating the header and padding words as needed.
  //
  //---------------------------------------------------------------------------

  // States
  localparam [3:0] ST_HDR       = 4'd0;   // CHDR header
  localparam [3:0] ST_TS        = 4'd1;   // CHDR timestamp
  localparam [3:0] ST_HDR_PAD   = 4'd2;   // CHDR header padding
  localparam [3:0] ST_MDATA     = 4'd3;   // CHDR metadata words
  localparam [3:0] ST_MDATA_PAD = 4'd4;   // CHDR metadata padding
  localparam [3:0] ST_PYLD      = 4'd5;   // CHDR payload words
  localparam [3:0] ST_MGMT_HDR  = 4'd6;   // CHDR management header word
  localparam [3:0] ST_MGMT_PYLD = 4'd7;   // CHDR management payload words
  localparam [3:0] ST_MGMT_PAD  = 4'd8;   // CHDR management word padding
  localparam [3:0] ST_LAST_PAD  = 4'd9;   // Pad the last CHDR word

  reg [3:0] state = ST_HDR;

  // Number of input words per output word
  localparam NUM_WORDS = O_CHDR_W/DATA_W;

  // Determine the number of bits needed to represent a counter to track
  // which CHDR words are valid and which are padding.
  localparam COUNT_W = $clog2(NUM_WORDS);

  // Determine the maximum number DATA_W-sized payload words. The maximum
  // packet size is 2**16-1 bytes, then subtract one word for the smallest
  // possible header and convert that to a number of whole CHDR words.
  localparam NUM_PYLD_WORDS = `DIV_CEIL((2**16-1) - (DATA_W/8), DATA_W/8);

  // Determine the number of bits needed to represent a counter to track which
  // I_DATA_W payload word we are processing.
  localparam PYLD_COUNT_W = $clog2(NUM_PYLD_WORDS + 1);

  // Header info we need to save
  reg [4:0] num_mdata_reg;
  reg [2:0] pkt_type_reg;

  // Counters (number of DATA_W sized words processed on the input)
  reg [        4:0] mdata_count;
  reg [COUNT_W-1:0] word_count;     // Zero based (starts at 0)

  // Shortcuts for CHDR header info
  wire [2:0] pkt_type  = chdr_get_pkt_type(i_pipe_tdata[63:0]);
  wire [4:0] num_mdata = chdr_get_num_mdata(i_pipe_tdata[63:0]);

  // Calculate payload length in bytes
  wire [15:0] pyld_len_bytes = chdr_calc_payload_length(DATA_W, i_pipe_tdata[63:0]);

  // Calculate the payload length of a management packet in words (management
  // packets have the same number of payload words, regardless of CHDR width).
  wire [PYLD_COUNT_W-1:0] mgmt_pyld_len = `DIV_CEIL(pyld_len_bytes, DATA_W/8);

  // Determine the number of metadata words for the output packet
  wire [4:0] o_num_mdata = `DIV_CEIL(num_mdata, O_CHDR_W/DATA_W);

  // Generate packet headers with updated NumMData and Length fields
  reg [DATA_W-1:0] new_header;
  always @(*) begin
    // Pass through upper bits unchanged (e.g., timestamp)
    new_header = i_pipe_tdata;
    // Update NumMData
    new_header[63:0] = chdr_set_num_mdata(new_header, o_num_mdata);
    // Update packet length
    new_header[63:0] = chdr_update_length(O_CHDR_W, new_header,
      (pkt_type == CHDR_PKT_TYPE_MGMT) ? mgmt_pyld_len * (O_CHDR_W/8) : pyld_len_bytes);
  end

  reg [DATA_W-1:0] new_mgmt_header;
  always @(*) begin
    // Update the CHDRWidth field in the management header.
    new_mgmt_header = i_pipe_tdata;
    new_mgmt_header[63:0] =
      chdr_mgmt_set_chdr_w(i_pipe_tdata[63:0], chdr_w_to_enum(O_CHDR_W));
  end

  reg  [DATA_W-1:0] o_pipe_tdata;
  reg               o_pipe_tlast;
  reg               o_pipe_tvalid;
  wire              o_pipe_tready;

  always @(posedge clk) begin
    if (rst) begin
      state         <= ST_HDR;
      mdata_count   <= 'bX;
      word_count    <= 'bX;
      num_mdata_reg <= 'bX;
      pkt_type_reg  <= 'bX;
    end else if (o_pipe_tvalid & o_pipe_tready) begin
      // Default assignment
      word_count <= word_count + 1;

      case (state)

        // ST_HDR: CHDR Header
        ST_HDR: begin
          mdata_count   <= 1;    // The first metadata word will be word 1
          word_count    <= 1;    // Word 0 is the current word (header)
          pkt_type_reg  <= pkt_type;
          // Save the number of DATA_W sized metadata words
          num_mdata_reg <= num_mdata;
          if (DATA_W == 64) begin
            // When CHDR_W == 64, the timestamp comes after the header.
            if (pkt_type == CHDR_PKT_TYPE_DATA_TS) begin
              state <= ST_TS;
            end else begin
              // O_CHDR_W must be at least 128, so there must be at least one
              // word of header padding.
              state <= ST_HDR_PAD;
            end
          end else begin
            // If DATA_W > 64 then O_CHDR_W must be at least 256, so we know
            // there must be some header padding needed.
            state <= ST_HDR_PAD;
          end
        end

        // ST_TS: Timestamp (DATA_W == 64 only)
        ST_TS: begin
          if (O_CHDR_W > 128) begin
            state <= ST_HDR_PAD;
          end else begin
            if (num_mdata_reg != 0) begin
              state <= ST_MDATA;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_HDR_PAD: CHDR header padding to fill out the last O_CHDR_W
        ST_HDR_PAD: begin
          if (word_count == NUM_WORDS-1) begin
            if (num_mdata_reg != 0) begin
              state <= ST_MDATA;
            end else if (pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
              state <= ST_MGMT_HDR;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_MDATA: Metadata words
        ST_MDATA: begin
          mdata_count <= mdata_count + 1;
          if (mdata_count == num_mdata_reg) begin
            // If we've input a multiple of O_CHDR_W, then we're done with
            // metadata. Otherwise, we need to add some padding words.
            if (word_count == NUM_WORDS-1) begin
              if (pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
                state <= ST_MGMT_HDR;
              end else begin
                state <= ST_PYLD;
              end
            end else begin
              state <= ST_MDATA_PAD;
            end
          end
        end

        // ST_MDATA_PAD: Add metadata padding to fill out the last O_CHDR_W
        ST_MDATA_PAD: begin
          if (word_count == NUM_WORDS-1) begin
            if (pkt_type_reg == CHDR_PKT_TYPE_MGMT) begin
              state <= ST_MGMT_HDR;
            end else begin
              state <= ST_PYLD;
            end
          end
        end

        // ST_PYLD: Payload words
        ST_PYLD: begin
          if (i_pipe_tlast) begin
            // We don't pad data words because unused bytes are not sent or
            // expected on the transport.
            state <= ST_HDR;
          end
        end

        // ST_MGMT_HDR: Management header
        ST_MGMT_HDR: begin
          // Management packets are different from other packet types in that
          // the payload is not serialized. So we need to pad each word to make
          // it a full O_CHDR_W size.
          if (i_pipe_tlast) begin
            state <= ST_LAST_PAD;
          end else begin
            state <= ST_MGMT_PAD;
          end
        end

        // ST_MGMT_PYLD: Management operation words
        ST_MGMT_PYLD: begin
          if (i_pipe_tlast) begin
            state <= ST_LAST_PAD;
          end else begin
            state <= ST_MGMT_PAD;
          end
        end

        // ST_MGMT_PAD: Management word padding
        ST_MGMT_PAD: begin
          if (word_count == NUM_WORDS-1) begin
            state <= ST_MGMT_PYLD;
          end
        end

        // ST_LAST_PAD: Pad the last word so output is a multiple of O_CHDR_W
        ST_LAST_PAD : begin
          if (word_count == NUM_WORDS-1) begin
            state <= ST_HDR;
          end
        end

      endcase
    end
  end


  //-----------------------------
  // State machine output logic
  //-----------------------------

  always @(*) begin
    case (state)
      ST_HDR : begin
        o_pipe_tdata  = new_header;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = i_pipe_tlast;
        i_pipe_tready = o_pipe_tready;
      end
      ST_TS : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = i_pipe_tlast;
        i_pipe_tready = o_pipe_tready;
      end
      ST_HDR_PAD : begin
        o_pipe_tdata  = { DATA_W {1'b0} };
        o_pipe_tvalid = 1'b1;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = 1'b0;
      end
      ST_MDATA : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = o_pipe_tready;
      end
      ST_MDATA_PAD : begin
        o_pipe_tdata  = { DATA_W {1'b0} };
        o_pipe_tvalid = 1'b1;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = 1'b0;
      end
      ST_PYLD : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = i_pipe_tlast;
        i_pipe_tready = o_pipe_tready;
      end
      ST_MGMT_HDR : begin
        o_pipe_tdata  = new_mgmt_header;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = o_pipe_tready;
      end
      ST_MGMT_PYLD : begin
        o_pipe_tdata  = i_pipe_tdata;
        o_pipe_tvalid = i_pipe_tvalid;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = o_pipe_tready;
      end
      ST_MGMT_PAD : begin
        o_pipe_tdata  = { DATA_W {1'b0} };
        o_pipe_tvalid = 1'b1;
        o_pipe_tlast  = 1'b0;
        i_pipe_tready = 1'b0;
      end
      ST_LAST_PAD : begin
        o_pipe_tdata  = { DATA_W {1'b0} };
        o_pipe_tvalid = 1'b1;
        o_pipe_tlast  = (word_count == NUM_WORDS-1);
        i_pipe_tready = 1'b0;
      end
      default : begin
        o_pipe_tdata  = 'bX;
        o_pipe_tvalid = 1'bX;
        o_pipe_tlast  = 1'bX;
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
