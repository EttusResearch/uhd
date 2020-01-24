//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: context_parser
//
// Description:
//
// This module extracts the context information from the AXI-Stream Raw Data 
// (Simple Interface) in RFNoC and outputs it as sideband information for an 
// AXI-Stream data bus. This includes the timestamp, if present, and packet 
// flags (EOB, EOV).
//
// For each payload and context packet that is input, one data packet will be
// output along with the sideband data.
//
// Parameters:
//
//   CHDR_W : Width of the CHDR interface (width of context words)
//   ITEM_W : Width of each item/sample
//   NIPC   : Number of items/samples per clock cycle
//

module context_parser #(
  parameter CHDR_W = 64,
  parameter ITEM_W = 32,
  parameter NIPC   = 2
) (
  input axis_data_clk,
  input axis_data_rst,

  // AXI-Stream Raw Data (Simple Interface) input
  input  wire [(ITEM_W*NIPC)-1:0] s_axis_payload_tdata,
  input  wire [         NIPC-1:0] s_axis_payload_tkeep,
  input  wire                     s_axis_payload_tlast,
  input  wire                     s_axis_payload_tvalid,
  output wire                     s_axis_payload_tready,
  //
  input  wire [       CHDR_W-1:0] s_axis_context_tdata,
  input  wire                     s_axis_context_tlast,
  input  wire                     s_axis_context_tvalid,
  output wire                     s_axis_context_tready,

  // Data stream out (AXI-Stream)
  output wire [(ITEM_W*NIPC)-1:0] m_axis_tdata,
  output wire [         NIPC-1:0] m_axis_tkeep,
  output wire                     m_axis_tlast,
  output wire                     m_axis_tvalid,
  input  wire                     m_axis_tready,
  // Sideband information
  output wire [             63:0] m_axis_ttimestamp,
  output wire                     m_axis_thas_time,
  output wire [             15:0] m_axis_tlength,      // Payload length, in bytes
  output wire                     m_axis_teov,
  output wire                     m_axis_teob
);

  `include "../core/rfnoc_chdr_utils.vh"


  // Sideband-FIFO signals
  reg  sideband_i_tvalid = 1'b0;
  wire sideband_i_tready;
  wire sideband_o_tvalid;
  wire sideband_o_tready;

  // Sideband data for next packet
  reg [63:0] timestamp;
  reg        has_time;
  reg [15:0] length;
  reg        eov;
  reg        eob;


  //---------------------------------------------------------------------------
  // Context State Machine
  //---------------------------------------------------------------------------
  //
  // This state machine parses the context data so that it can be output as
  // sideband information on the AXI-Stream output.
  //
  // This state machine assumes that the context packet is always properly 
  // formed (i.e., it doesn't explicitly check for and drop malformed packets).
  //
  //---------------------------------------------------------------------------

  localparam ST_HEADER    = 0;
  localparam ST_TIMESTAMP = 1;
  localparam ST_METADATA  = 2;

  reg [1:0] state = ST_HEADER;

  always @(posedge axis_data_clk) begin
    if (axis_data_rst) begin
      state             <= ST_HEADER;
      sideband_i_tvalid <= 1'b0;
    end else begin
      sideband_i_tvalid <= 1'b0;

      case(state)
        ST_HEADER: begin
          // Grab header information
          eov      <= chdr_get_eov(s_axis_context_tdata[63:0]);
          eob      <= chdr_get_eob(s_axis_context_tdata[63:0]);
          has_time <= chdr_get_has_time(s_axis_context_tdata[63:0]);
          length   <= chdr_calc_payload_length(CHDR_W, s_axis_context_tdata[63:0]);

          if (s_axis_context_tvalid && s_axis_context_tready) begin
            if (CHDR_W > 64) begin
              // When CHDR_W > 64, the timestamp is a part of the header word
              if (chdr_get_has_time(s_axis_context_tdata[63:0])) begin
                timestamp <= s_axis_context_tdata[127:64];
              end

              // Load the sideband data into the FIFO
              sideband_i_tvalid <= 1'b1;

              // Check if there's more context packet to wait for
              if (!s_axis_context_tlast) begin
                state <= ST_METADATA;
              end

            end else begin
              // When CHDR_W == 64, the timestamp comes after the header word
              if (s_axis_context_tlast) begin
                // Context packet is ending. Load the sideband data into FIFO.
                sideband_i_tvalid <= 1'b1;
              end else begin
                // More context packet to come
                if (chdr_get_has_time(s_axis_context_tdata[63:0])) begin
                  state <= ST_TIMESTAMP;
                end else begin
                  // Load the sideband data into the FIFO
                  sideband_i_tvalid <= 1'b1;
                  state             <= ST_METADATA;
                end
              end
            end
          end
        end

        ST_TIMESTAMP: begin
          // This state only applies when CHDR_W == 64
          if (s_axis_context_tvalid && s_axis_context_tready) begin
            timestamp <= s_axis_context_tdata;

            // Load the sideband data into the FIFO
            sideband_i_tvalid <= 1'b1;

            // Check if there's more context packet to wait for
            if (s_axis_context_tlast) begin
              state <= ST_HEADER;
            end else begin
              state <= ST_METADATA;
            end
          end
        end

        ST_METADATA: begin
          // This module doesn't handle metadata currently, so just ignore it
          if (s_axis_context_tvalid && s_axis_context_tready) begin
            if (s_axis_context_tlast) begin
              state <= ST_HEADER;
            end
          end
        end

        default: state <= ST_HEADER;
      endcase
    end
  end


  //---------------------------------------------------------------------------
  // Sideband Data FIFO
  //---------------------------------------------------------------------------
  //
  // Here we buffer the sideband information into a FIFO. The information will 
  // be output coincident with the corresponding data packet.
  //
  //---------------------------------------------------------------------------

  axi_fifo_short #(
    .WIDTH (83)
  ) sideband_fifo (
    .clk      (axis_data_clk),
    .reset    (axis_data_rst),
    .clear    (1'b0),
    .i_tdata  ({length, eob, eov, has_time, timestamp}),
    .i_tvalid (sideband_i_tvalid),
    .i_tready (sideband_i_tready),
    .o_tdata  ({m_axis_tlength, m_axis_teob, m_axis_teov, 
                m_axis_thas_time, m_axis_ttimestamp}),
    .o_tvalid (sideband_o_tvalid),
    .o_tready (sideband_o_tready),
    .space    (),
    .occupied ()
  );


  //---------------------------------------------------------------------------
  // Payload Transfer Logic
  //---------------------------------------------------------------------------
  //
  // Here we handle the logic for AXI-Stream flow control. The data and 
  // sideband information are treated as a single AXI-Stream bus. The sideband 
  // information is output for the duration of the packet and is popped off of 
  // the sideband FIFO at the end of each packet.
  //
  //---------------------------------------------------------------------------

  // We can only accept context info when there's room in the sideband FIFO.
  assign s_axis_context_tready = sideband_i_tready;

  // Allow payload transfer whenever the sideband info is valid
  assign s_axis_payload_tready = (m_axis_tready         & sideband_o_tvalid);
  assign m_axis_tvalid         = (s_axis_payload_tvalid & sideband_o_tvalid);

  // Pop off the sideband info at the end of each packet
  assign sideband_o_tready = (s_axis_payload_tready &
                              s_axis_payload_tvalid &
                              s_axis_payload_tlast);

  // Other AXI-Stream signals pass through untouched
  assign m_axis_tdata = s_axis_payload_tdata;
  assign m_axis_tkeep = s_axis_payload_tkeep;
  assign m_axis_tlast = s_axis_payload_tlast;

endmodule
