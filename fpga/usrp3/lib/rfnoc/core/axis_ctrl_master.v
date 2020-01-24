//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_master
// Description:
//  This module implements an AXIS-Control master (and a Control-Port
//  slave). Requests are accepted on the slave Control-Port, converted
//  to AXIS-Control requests, then sent over the master AXI-Stream port.
//  Responses are received on the AXI-Stream slave port, and converted
//  to Control-Port responses.
//  NOTE: Transactions are not buffered so there is no need for flow 
//        control or throttling.
//
// Parameters:
//   - THIS_PORTID     : The local port-ID of this control port
//
// Signals:
//   - s_axis_ctrl_*   : Input control stream (AXI-Stream) for responses
//   - m_axis_ctrl_*   : Output control stream (AXI-Stream) for requests
//   - ctrlport_req_*  : Control-port master request port
//   - ctrlport_resp_* : Control-port master response port

module axis_ctrl_master #(
  parameter [9:0] THIS_PORTID = 10'd0
)(
  // Clock and reset
  input  wire         clk,
  input  wire         rst,
  // AXIS-Control Bus (Response)
  input  wire [31:0]  s_axis_ctrl_tdata,
  input  wire         s_axis_ctrl_tlast,
  input  wire         s_axis_ctrl_tvalid,
  output wire         s_axis_ctrl_tready,
  // AXIS-Control Bus (Request)
  output reg  [31:0]  m_axis_ctrl_tdata,
  output wire         m_axis_ctrl_tlast,
  output wire         m_axis_ctrl_tvalid,
  input  wire         m_axis_ctrl_tready,
  // Control Port Endpoint (Request)
  input  wire         ctrlport_req_wr,
  input  wire         ctrlport_req_rd,
  input  wire [19:0]  ctrlport_req_addr,
  input  wire [9:0]   ctrlport_req_portid,
  input  wire [15:0]  ctrlport_req_rem_epid,
  input  wire [9:0]   ctrlport_req_rem_portid,
  input  wire [31:0]  ctrlport_req_data,
  input  wire [3:0]   ctrlport_req_byte_en,
  input  wire         ctrlport_req_has_time,
  input  wire [63:0]  ctrlport_req_time,
  // Control Port Endpoint (Response)
  output wire         ctrlport_resp_ack,
  output wire [1:0]   ctrlport_resp_status,
  output wire [31:0]  ctrlport_resp_data
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  // ---------------------------------------------------
  //  State Machine
  // ---------------------------------------------------
  localparam [3:0] ST_IDLE          = 4'd0;   // Waiting for a request on slave ctrlport
  localparam [3:0] ST_REQ_HDR_LO    = 4'd1;   // Sending AXIS-Control request header (low bits)
  localparam [3:0] ST_REQ_HDR_HI    = 4'd2;   // Sending AXIS-Control request header (high bits)
  localparam [3:0] ST_REQ_TS_LO     = 4'd3;   // Sending AXIS-Control request timestamp (low bits)
  localparam [3:0] ST_REQ_TS_HI     = 4'd4;   // Sending AXIS-Control request timestamp (high bits)
  localparam [3:0] ST_REQ_OP_WORD   = 4'd5;   // Sending AXIS-Control request operation word
  localparam [3:0] ST_REQ_OP_DATA   = 4'd6;   // Sending AXIS-Control request data word
  localparam [3:0] ST_RESP_HDR_LO   = 4'd7;   // Receiving AXIS-Control response header (low bits)
  localparam [3:0] ST_RESP_HDR_HI   = 4'd8;   // Receiving AXIS-Control response header (high bits)
  localparam [3:0] ST_RESP_TS_LO    = 4'd9;   // Receiving AXIS-Control response timestamp (low bits)
  localparam [3:0] ST_RESP_TS_HI    = 4'd10;  // Receiving AXIS-Control response timestamp (high bits)
  localparam [3:0] ST_RESP_OP_WORD  = 4'd11;  // Receiving AXIS-Control response operation word
  localparam [3:0] ST_RESP_OP_DATA  = 4'd12;  // Receiving AXIS-Control response data word     
  localparam [3:0] ST_SHORT_PKT_ERR = 4'd13;  // Response was too short. Send a dummy response on ctrlport
  localparam [3:0] ST_DROP_LONG_PKT = 4'd14;  // Response was too long. Dump the rest of the packet

  // State variables
  reg [3:0]   state = ST_IDLE;    // Current state for FSM
  reg [5:0]   seq_num = 6'd0;     // Expected seqnum for response
  // Request state
  reg [3:0]   req_opcode;         // Cached opcode for transaction request
  reg [19:0]  req_addr;           // Cached address for transaction request
  reg [9:0]   req_portid;         // Cached port ID for transaction request
  reg [15:0]  req_rem_epid;       // Cached remote endpoint ID for transaction request
  reg [9:0]   req_rem_portid;     // Cached remote port ID for transaction request
  reg [31:0]  req_data;           // Cached data word for transaction request
  reg [3:0]   req_byte_en;        // Cached byte enable for transaction request
  reg         req_has_time;       // Cached has_time bit for transaction request
  reg [63:0]  req_time;           // Cached timestamp for transaction request
  // Response state
  reg         resp_has_time;      // Does the response have a timestamp?
  reg [1:0]   resp_status;        // The status in the response
  reg         resp_seq_err, resp_cmd_err; // Error bits for the response

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_IDLE;
      seq_num <= 6'd0;
    end else begin
      case (state)

        // Ready to receive a request on ctrlport
        // ------------------------------------
        ST_IDLE: begin
          if (ctrlport_req_wr | ctrlport_req_rd) begin
            // A transaction was posted on the slave ctrlport...
            // Cache the opcode
            if (ctrlport_req_wr & ctrlport_req_rd)
              req_opcode   <= AXIS_CTRL_OPCODE_WRITE_READ;
            else if (ctrlport_req_rd)
              req_opcode   <= AXIS_CTRL_OPCODE_READ;
            else
              req_opcode   <= AXIS_CTRL_OPCODE_WRITE;
            // Cache transaction info
            req_addr       <= ctrlport_req_addr;
            req_portid     <= ctrlport_req_portid;
            req_rem_epid   <= ctrlport_req_rem_epid;
            req_rem_portid <= ctrlport_req_rem_portid;
            req_data       <= ctrlport_req_data;
            req_byte_en    <= ctrlport_req_byte_en;
            req_has_time   <= ctrlport_req_has_time;
            req_time       <= ctrlport_req_time;
            // Start sending out AXIS-Ctrl packet
            state <= ST_REQ_HDR_LO;
          end
        end

        // Send a request AXIS comand
        // (a state for each stage in the packet)
        // ------------------------------------
        ST_REQ_HDR_LO: begin
          if (m_axis_ctrl_tready)
            state <= ST_REQ_HDR_HI;
        end
        ST_REQ_HDR_HI: begin
          if (m_axis_ctrl_tready)
            state <= req_has_time ? ST_REQ_TS_LO : ST_REQ_OP_WORD;
        end
        ST_REQ_TS_LO: begin
          if (m_axis_ctrl_tready)
            state <= ST_REQ_TS_HI;
        end
        ST_REQ_TS_HI: begin
          if (m_axis_ctrl_tready)
            state <= ST_REQ_OP_WORD;
        end
        ST_REQ_OP_WORD: begin
          if (m_axis_ctrl_tready)
            state <= ST_REQ_OP_DATA;
        end
        ST_REQ_OP_DATA: begin
          if (m_axis_ctrl_tready)
            state <= ST_RESP_HDR_LO;
        end

        // Receive a response AXIS comand
        // (a state for each stage in the packet)
        // ------------------------------------
        ST_RESP_HDR_LO: begin
          if (s_axis_ctrl_tvalid) begin
            // Remeber if the packet is supposed to have a timestamp
            resp_has_time <= axis_ctrl_get_has_time(s_axis_ctrl_tdata);
            // Check for a sequence error
            resp_seq_err <= (axis_ctrl_get_seq_num(s_axis_ctrl_tdata) != seq_num);
            // Assert a command error if:
            // - The port ID does not match
            // - The response was too short (the next check)
            resp_cmd_err <= (axis_ctrl_get_dst_port(s_axis_ctrl_tdata) != THIS_PORTID);
            if (!s_axis_ctrl_tlast) begin
              state <= ST_RESP_HDR_HI;
            end else begin
              // Response was too short
              resp_cmd_err <= 1'b1;
              state <= ST_SHORT_PKT_ERR;
            end
          end
        end
        ST_RESP_HDR_HI: begin
          if (s_axis_ctrl_tvalid) begin
            if (!s_axis_ctrl_tlast) begin
              state <= resp_has_time ? ST_RESP_TS_LO : ST_RESP_OP_WORD;
            end else begin
              // Response was too short
              resp_cmd_err <= 1'b1;
              state <= ST_SHORT_PKT_ERR;
            end
          end
        end
        ST_RESP_TS_LO: begin
          if (s_axis_ctrl_tvalid) begin
            if (!s_axis_ctrl_tlast) begin
              state <= ST_RESP_TS_HI;
            end else begin
              // Response was too short
              resp_cmd_err <= 1'b1;
              state <= ST_SHORT_PKT_ERR;
            end
          end
        end
        ST_RESP_TS_HI: begin
          if (s_axis_ctrl_tvalid) begin
            if (!s_axis_ctrl_tlast) begin
              state <= ST_RESP_OP_WORD;
            end else begin
              // Response was too short
              resp_cmd_err <= 1'b1;
              state <= ST_SHORT_PKT_ERR;
            end
          end
        end
        ST_RESP_OP_WORD: begin
          if (s_axis_ctrl_tvalid) begin
            if (!s_axis_ctrl_tlast) begin
              // Assert a command error if opcode and addr in request does not match response
              resp_cmd_err <= resp_cmd_err ||
                              (axis_ctrl_get_opcode(s_axis_ctrl_tdata) != req_opcode) ||
                              (axis_ctrl_get_address(s_axis_ctrl_tdata) != req_addr);
              resp_status <= axis_ctrl_get_status(s_axis_ctrl_tdata);
              state <= ST_RESP_OP_DATA;
            end else begin
              // Response was too short
              resp_cmd_err <= 1'b1;
              state <= ST_SHORT_PKT_ERR;
            end
          end
        end
        ST_RESP_OP_DATA: begin
          if (s_axis_ctrl_tvalid) begin
            // If the packet was too long then just drop the rest without complaining
            state <= s_axis_ctrl_tlast ? ST_IDLE : ST_DROP_LONG_PKT;
            seq_num <= seq_num + 6'd1;
          end
        end

        // Error handling states
        // ------------------------------------
        ST_SHORT_PKT_ERR: begin
          state <= ST_IDLE;
        end
        ST_DROP_LONG_PKT: begin
          if (s_axis_ctrl_tvalid && s_axis_ctrl_tlast)
            state <= ST_IDLE;
        end

        default: begin
          // We should never get here
          state <= ST_IDLE;
        end
      endcase
    end
  end

  // Logic to drive m_axis_ctrl_*
  // ------------------------------------
  always @(*) begin
    case (state)
      ST_REQ_HDR_LO: begin
        m_axis_ctrl_tdata = axis_ctrl_build_hdr_lo(
          1'b0 /* is_ack*/, req_has_time, seq_num,
          4'd1 /* num_data */, THIS_PORTID, req_portid);
      end
      ST_REQ_HDR_HI: begin
        m_axis_ctrl_tdata = axis_ctrl_build_hdr_hi(
          req_rem_portid, req_rem_epid);
      end
      ST_REQ_TS_LO: begin
        m_axis_ctrl_tdata = req_time[31:0];
      end
      ST_REQ_TS_HI: begin
        m_axis_ctrl_tdata = req_time[63:32];
      end
      ST_REQ_OP_WORD: begin
        m_axis_ctrl_tdata = axis_ctrl_build_op_word(
          AXIS_CTRL_STS_OKAY, req_opcode, req_byte_en, req_addr);
      end
      ST_REQ_OP_DATA: begin
        m_axis_ctrl_tdata = req_data;
      end
      default: begin
        m_axis_ctrl_tdata = 32'h0;
      end
    endcase
  end
  assign m_axis_ctrl_tvalid = (state == ST_REQ_HDR_LO)  ||
                              (state == ST_REQ_HDR_HI)  ||
                              (state == ST_REQ_TS_LO)   ||
                              (state == ST_REQ_TS_HI)   ||
                              (state == ST_REQ_OP_WORD) ||
                              (state == ST_REQ_OP_DATA);
  assign m_axis_ctrl_tlast  = (state == ST_REQ_OP_DATA);

  // Logic to backpressure responses
  // ------------------------------------
  assign s_axis_ctrl_tready = (state == ST_RESP_HDR_LO)  ||
                              (state == ST_RESP_HDR_HI)  ||
                              (state == ST_RESP_TS_LO)   ||
                              (state == ST_RESP_TS_HI)   ||
                              (state == ST_RESP_OP_WORD) ||
                              (state == ST_RESP_OP_DATA) ||
                              (state == ST_DROP_LONG_PKT);

  // Logic to drive Control-port response
  // ------------------------------------
  assign ctrlport_resp_ack    = (state == ST_RESP_OP_DATA && s_axis_ctrl_tvalid) || 
                                (state == ST_SHORT_PKT_ERR); 
  assign ctrlport_resp_status = resp_cmd_err ? AXIS_CTRL_STS_CMDERR : 
                                  (resp_seq_err ? AXIS_CTRL_STS_WARNING : resp_status);
  assign ctrlport_resp_data   = (state == ST_SHORT_PKT_ERR) ? 32'h0 : s_axis_ctrl_tdata;

endmodule // axis_ctrl_master
