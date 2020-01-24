//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_slave
// Description:
//  This module implements an AXIS-Control slave (and a Control-Port
//  master). Requests are accepted on the slave axis port and responses
//  are sent out on the master axis port. This module implements the
//  following operations: {SLEEP, READ, WRITE}. All other operations
//  will be treated as a nop and the output will throw a CMDERR.
//
// Parameters:
//   None
//
// Signals:
//   - s_axis_ctrl_*   : Input control stream (AXI-Stream) for requests
//   - m_axis_ctrl_*   : Output control stream (AXI-Stream) for responses
//   - ctrlport_req_*  : Control-port master request port
//   - ctrlport_resp_* : Control-port master response port

module axis_ctrl_slave (
  // CHDR Bus (master and slave)
  input  wire         clk,
  input  wire         rst,
  // AXIS-Control Bus (Request)
  input  wire [31:0]  s_axis_ctrl_tdata,
  input  wire         s_axis_ctrl_tlast,
  input  wire         s_axis_ctrl_tvalid,
  output wire         s_axis_ctrl_tready,
  // AXIS-Control Bus (Response)
  output wire [31:0]  m_axis_ctrl_tdata,
  output wire         m_axis_ctrl_tlast,
  output wire         m_axis_ctrl_tvalid,
  input  wire         m_axis_ctrl_tready,
  // Control Port Endpoint (Request)
  output wire         ctrlport_req_wr,
  output wire         ctrlport_req_rd,
  output wire [19:0]  ctrlport_req_addr,
  output wire [31:0]  ctrlport_req_data,
  output wire [3:0]   ctrlport_req_byte_en,
  output wire         ctrlport_req_has_time,
  output wire [63:0]  ctrlport_req_time,
  // Control Port Endpoint (Response)
  input  wire         ctrlport_resp_ack,
  input  wire [1:0]   ctrlport_resp_status,
  input  wire [31:0]  ctrlport_resp_data
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  // ---------------------------------------------------
  //  Width converters
  // ---------------------------------------------------
  // Convert 32-bit messages to 64 bits for ease of handling
  // and buffering. Convert back to 32 bits.

  wire [63:0] in64_tdata;
  wire [1:0]  in64_tkeep;
  wire        in64_tlast, in64_tvalid;
  reg         in64_tready;

  axis_width_conv #(
    .WORD_W(32), .IN_WORDS(1), .OUT_WORDS(2),
    .SYNC_CLKS(1), .PIPELINE("OUT")
  ) upsizer_i (
    .s_axis_aclk(clk), .s_axis_rst(rst),
    .s_axis_tdata(s_axis_ctrl_tdata), .s_axis_tkeep(1'b1),
    .s_axis_tlast(s_axis_ctrl_tlast),
    .s_axis_tvalid(s_axis_ctrl_tvalid), .s_axis_tready(s_axis_ctrl_tready),
    .m_axis_aclk(clk), .m_axis_rst(rst),
    .m_axis_tdata(in64_tdata), .m_axis_tkeep(in64_tkeep),
    .m_axis_tlast(in64_tlast),
    .m_axis_tvalid(in64_tvalid), .m_axis_tready(in64_tready)
  );

  reg  [63:0] out64_tdata;
  wire [1:0]  out64_tkeep;
  reg         out64_tvalid;
  wire        out64_tlast, out64_terror, out64_tready;

  wire [63:0] out64_gt_tdata;
  wire [1:0]  out64_gt_tkeep;
  wire        out64_gt_tlast, out64_gt_tvalid, out64_gt_tready;

  // The header of the response packet is generated
  // immediately when a request is received but the data
  // comes much later. The packet gate will smooth out the
  // outgoing responses.

  axi_packet_gate #(
    .WIDTH(66), .SIZE(4)
  ) gate_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({out64_tkeep, out64_tdata}), .i_tlast(out64_tlast),
    .i_terror(out64_terror),
    .i_tvalid(out64_tvalid), .i_tready(out64_tready),
    .o_tdata({out64_gt_tkeep, out64_gt_tdata}), .o_tlast(out64_gt_tlast),
    .o_tvalid(out64_gt_tvalid), .o_tready(out64_gt_tready)
  );

  axis_width_conv #(
    .WORD_W(32), .IN_WORDS(2), .OUT_WORDS(1),
    .SYNC_CLKS(1), .PIPELINE("IN")
  ) downsizer_i (
    .s_axis_aclk(clk), .s_axis_rst(rst),
    .s_axis_tdata(out64_gt_tdata), .s_axis_tkeep(out64_gt_tkeep),
    .s_axis_tlast(out64_gt_tlast),
    .s_axis_tvalid(out64_gt_tvalid), .s_axis_tready(out64_gt_tready),
    .m_axis_aclk(clk), .m_axis_rst(rst),
    .m_axis_tdata(m_axis_ctrl_tdata), .m_axis_tkeep(/*unused*/),
    .m_axis_tlast(m_axis_ctrl_tlast),
    .m_axis_tvalid(m_axis_ctrl_tvalid), .m_axis_tready(m_axis_ctrl_tready)
  );

  // ---------------------------------------------------
  //  Transaction Processor
  // ---------------------------------------------------

  localparam [2:0] ST_IN_HDR        = 3'd0;   // Transferring input header to output
  localparam [2:0] ST_IN_TS         = 3'd1;   // Transferring input timestamp to output
  localparam [2:0] ST_IN_OP_WORD    = 3'd2;   // Processing input control word 
  localparam [2:0] ST_WAIT_FOR_ACK  = 3'd3;   // Waiting for a ctrlport response
  localparam [2:0] ST_SLEEP         = 3'd4;   // Idle state for sleep operation
  localparam [2:0] ST_OUT_OP_WORD   = 3'd5;   // Outputing control word after respose receipt
  localparam [2:0] ST_MORE_DATA     = 3'd6;   // Control word is too long. Passing extra data forward
  localparam [2:0] ST_DROP          = 3'd7;   // Something went wrong. Drop the current packet

  // State variables
  reg [2:0]   state = ST_IN_HDR;              // Current state of FSM
  reg [31:0]  sleep_cntr = 32'd0;             // Counter to count sleep cycles
  reg         cached_has_time = 1'b0;         // Cached "has_time" bit for input transaction request
  reg [63:0]  cached_time;                    // Cached timestamp for input transaction request
  reg [1:0]   resp_status;                    // Status for outgoing response
  reg [31:0]  resp_data;                      // Data for outgoing response

  // Sleep is an internal operation
  wire ctrlport_req_sleep;

  // Shortcuts (transaction request header)
  wire        is_ack       = axis_ctrl_get_is_ack      (in64_tdata[31:0] );
  wire        has_time     = axis_ctrl_get_has_time    (in64_tdata[31:0] );
  wire [5:0]  seq_num      = axis_ctrl_get_seq_num     (in64_tdata[31:0] );
  wire [3:0]  num_data     = axis_ctrl_get_num_data    (in64_tdata[31:0] );
  wire [9:0]  src_port     = axis_ctrl_get_src_port    (in64_tdata[31:0] );
  wire [9:0]  dst_port     = axis_ctrl_get_dst_port    (in64_tdata[31:0] );
  wire [9:0]  rem_dst_port = axis_ctrl_get_rem_dst_port(in64_tdata[63:32]);
  wire [15:0] rem_dst_epid = axis_ctrl_get_rem_dst_epid(in64_tdata[63:32]);
  wire        malformed    = (is_ack || num_data == 4'd0);
  // Shortcuts (transaction request op-word)
  wire [19:0] xact_address = axis_ctrl_get_address(in64_tdata[31:0]);
  wire [3:0]  xact_byte_en = axis_ctrl_get_byte_en(in64_tdata[31:0]);
  wire [3:0]  xact_opcode  = axis_ctrl_get_opcode (in64_tdata[31:0]);
  wire [31:0] xact_data    = in64_tdata[63:32];

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_IN_HDR;
    end else begin
      case (state)

        // Receive an AXIS-Control request
        // (a state for each stage in the packet)
        // Except for the OP_WORD stage, the appropriate response
        // line is also pushed to the output
        // ------------------------------------
        ST_IN_HDR: begin
          if (in64_tvalid && in64_tready) begin
            cached_has_time <= has_time;
            if (!in64_tlast) begin
              if (malformed)        // Malformed packet. Drop.
                state <= ST_DROP;
              else if (has_time)    // Pkt has a timestamp
                state <= ST_IN_TS;
              else                  // Pkt has no timestamp
                state <= ST_IN_OP_WORD;
            end else begin
              // Premature termination
              // out64_terror will be asserted to cancel the outgoing response
              state <= ST_IN_HDR;
            end
          end
        end
        ST_IN_TS: begin
          if (in64_tvalid && in64_tready) begin
            cached_time <= in64_tdata;
            if (!in64_tlast) begin
              state <= ST_IN_OP_WORD;
            end else begin
              // Premature termination
              // out64_terror will be asserted to cancel the outgoing response
              state <= ST_IN_HDR;
            end
          end
        end
        ST_IN_OP_WORD: begin
          if (in64_tvalid) begin
            if (ctrlport_req_sleep) begin
              state <= ST_SLEEP;
              sleep_cntr <= xact_data;
            end else if (ctrlport_req_rd | ctrlport_req_wr) begin
              state <= ST_WAIT_FOR_ACK;
            end else begin
              // Treat all other operations as a NOP (1 cycle sleep)
              state <= ST_SLEEP;
              sleep_cntr <= 32'd0;
              resp_status <= AXIS_CTRL_STS_CMDERR;
            end
          end
        end

        // Hold the input bus to implement a sleep
        // ------------------------------------
        ST_SLEEP: begin
          if (sleep_cntr == 32'd0) begin
            state <= ST_OUT_OP_WORD;
            resp_data <= xact_data;
            // We could get to this state for an invalid opcode so
            // only update the status if this is a legit sleep op
            if (xact_opcode == AXIS_CTRL_OPCODE_SLEEP)
              resp_status <= AXIS_CTRL_STS_OKAY;
          end else begin
            sleep_cntr <= sleep_cntr - 32'd1;
          end
        end

        // Wait for a response on the master ctrlport
        // ------------------------------------
        ST_WAIT_FOR_ACK: begin
          if (ctrlport_resp_ack) begin
            resp_status <= ctrlport_resp_status;
            if (xact_opcode == AXIS_CTRL_OPCODE_READ ||
                xact_opcode == AXIS_CTRL_OPCODE_WRITE_READ)
              resp_data <= ctrlport_resp_data;
            else
              resp_data <= xact_data;
            state <= ST_OUT_OP_WORD;
          end
        end

        // Send the AXIS-Control response data
        // ------------------------------------
        ST_OUT_OP_WORD: begin
          if (in64_tvalid && in64_tready) begin
            state <= in64_tlast ? ST_IN_HDR : ST_MORE_DATA;
          end
        end

        // Framing error handlers
        // ------------------------------------
        ST_MORE_DATA: begin
          if (in64_tvalid && in64_tready && in64_tlast)
            state <= ST_IN_HDR;
        end
        ST_DROP: begin
          if (in64_tvalid && in64_tready && in64_tlast)
            state <= ST_IN_HDR;
        end

        default: begin
          // We should never get here
          state <= ST_IN_HDR;
        end
      endcase
    end
  end

  always @(*) begin
    case (state)
      ST_IN_HDR: begin      // Swap src/dst and add resp flag when passing header
        in64_tready  = out64_tready;
        out64_tdata  = {
          axis_ctrl_build_hdr_hi(rem_dst_port, rem_dst_epid),
          axis_ctrl_build_hdr_lo(1'b1, has_time, seq_num, num_data, dst_port, src_port)
        };
        out64_tvalid = in64_tvalid && !malformed;
      end
      ST_IN_TS: begin       // Pass input to the output without modification
        in64_tready  = out64_tready;
        out64_tdata  = in64_tdata;
        out64_tvalid = in64_tvalid;
      end
      ST_OUT_OP_WORD: begin // Update status and data when passing op-word
        in64_tready  = out64_tready;
        out64_tdata  = {
          resp_data, 
          axis_ctrl_build_op_word(resp_status, xact_opcode, xact_byte_en, xact_address)
        };
        out64_tvalid = in64_tvalid;
      end
      ST_MORE_DATA: begin   // Pass input to the output without modification
        in64_tready  = out64_tready;
        out64_tdata  = in64_tdata;
        out64_tvalid = in64_tvalid;
      end
      ST_DROP: begin        // Consume input but don't produce output
        in64_tready  = 1'b1;
        out64_tdata  = 64'h0;
        out64_tvalid = 1'b0;
      end
      default: begin        // State machine is waiting. Don't produce output
        in64_tready  = 1'b0;
        out64_tdata  = 64'h0;
        out64_tvalid = 1'b0;
      end
    endcase
  end

  assign out64_tlast  = in64_tlast;
  assign out64_tkeep  = in64_tkeep;
  assign out64_terror = (state == ST_IN_HDR || state == ST_IN_TS) && in64_tlast; //Premature termination

  // Control-port request signals
  assign ctrlport_req_sleep    = in64_tvalid && (state == ST_IN_OP_WORD) &&
                                 (xact_opcode == AXIS_CTRL_OPCODE_SLEEP);
  assign ctrlport_req_wr       = in64_tvalid && (state == ST_IN_OP_WORD) &&
                                 (xact_opcode == AXIS_CTRL_OPCODE_WRITE ||
                                  xact_opcode == AXIS_CTRL_OPCODE_WRITE_READ);
  assign ctrlport_req_rd       = in64_tvalid && (state == ST_IN_OP_WORD) &&
                                 (xact_opcode == AXIS_CTRL_OPCODE_READ ||
                                  xact_opcode == AXIS_CTRL_OPCODE_WRITE_READ);
  assign ctrlport_req_addr     = xact_address;
  assign ctrlport_req_byte_en  = xact_byte_en;
  assign ctrlport_req_data     = xact_data;
  assign ctrlport_req_has_time = cached_has_time;
  assign ctrlport_req_time     = cached_time;

endmodule // axis_ctrl_slave
