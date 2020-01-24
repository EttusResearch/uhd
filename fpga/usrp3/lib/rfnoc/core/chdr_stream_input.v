//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_input
// Description:
//   Implements the CHDR input port for a stream endpoint.
//   The module accepts stream command and data packets and 
//   emits stream status packets. Flow control and error state
//   is communicated using stream status packets. There are no
//   external config interfaces because all configuration is done
//   using stream command packets.
//
// Parameters:
//   - CHDR_W: Width of the CHDR bus in bits
//   - BUFF_SIZE: Buffer size in log2 of the number of words in the
//                ingress buffer for the stream
//   - FLUSH_TIMEOUT_W: log2 of the number of cycles to wait in order
//                      to flush the input stream
//   - SIGNAL_ERRS: If set to 1 then all stream errors will be notified
//                  upstream, otherwise ALL errors are ignored
//
// Signals:
//   - s_axis_chdr_* : Input CHDR stream (AXI-Stream)
//   - m_axis_chdr_* : Output flow-controlled CHDR stream (AXI-Stream)
//   - m_axis_strs_* : Output stream status (AXI-Stream)
//   - data_err_stb  : If asserted, a data error notification is sent upstream
//

module chdr_stream_input #(
  parameter CHDR_W          = 256,
  parameter BUFF_SIZE       = 14,
  parameter FLUSH_TIMEOUT_W = 14,
  parameter MONITOR_EN      = 1,
  parameter SIGNAL_ERRS     = 1
)(
  // Clock, reset and settings
  input  wire              clk,
  input  wire              rst,
  // CHDR in (AXI-Stream)
  input  wire [CHDR_W-1:0] s_axis_chdr_tdata,
  input  wire              s_axis_chdr_tlast,
  input  wire              s_axis_chdr_tvalid,
  output wire              s_axis_chdr_tready,
  // Flow controlled data out (AXI-Stream)
  output wire [CHDR_W-1:0] m_axis_data_tdata,
  output wire              m_axis_data_tlast,
  output wire              m_axis_data_tvalid,
  input  wire              m_axis_data_tready,
  // Stream status out (AXI-Stream)
  output reg  [CHDR_W-1:0] m_axis_strs_tdata,
  output wire              m_axis_strs_tlast,
  output wire              m_axis_strs_tvalid,
  input  wire              m_axis_strs_tready,
  // External stream error signal
  input  wire              data_err_stb
);

  // The buffer size depends on the BUFF_SIZE parameter
  localparam [40:0] BUFF_SIZE_BYTES = ((41'h1 << BUFF_SIZE) * (CHDR_W / 8)) - 41'h1;
  // This is a flit-buffer. No packet limits
  localparam [23:0] BUFF_SIZE_PKTS  = 24'hFFFFFF;

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_chdr_internal_utils.vh"

  // ---------------------------------------------------
  //  Ingress Buffer and Flow Control Logic
  // ---------------------------------------------------
  wire [CHDR_W-1:0] buff_tdata;
  wire              buff_tlast, buff_tvalid;
  reg               buff_tready;
  wire [15:0]       buff_info;

  chdr_ingress_fifo #(
    .WIDTH(CHDR_W), .SIZE(BUFF_SIZE)
  ) ingress_fifo_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata(s_axis_chdr_tdata), .i_tlast(s_axis_chdr_tlast),
    .i_tvalid(s_axis_chdr_tvalid), .i_tready(s_axis_chdr_tready),
    .o_tdata(buff_tdata), .o_tlast(buff_tlast),
    .o_tvalid(buff_tvalid), .o_tready(buff_tready)
  );

  generate if (MONITOR_EN) begin
    wire [BUFF_SIZE:0] occ_lines;
    axis_fifo_monitor #( .COUNT_W(BUFF_SIZE+1) ) fifo_mon_i (
      .clk(clk), .reset(rst),
      .i_tlast(s_axis_chdr_tlast), .i_tvalid(s_axis_chdr_tvalid), .i_tready(s_axis_chdr_tready),
      .o_tlast(buff_tlast), .o_tvalid(buff_tvalid), .o_tready(buff_tready),
      .i_sop(), .i_eop(), .o_sop(), .o_eop(),
      .occupied(occ_lines), .occupied_pkts()
    );
    // buff_info represents a fraction of the fullness of the buffer
    // fullness percentage = (buff_info / 32768) * 100
    if (BUFF_SIZE + 1 >= 16)
      assign buff_info = occ_lines[BUFF_SIZE:(BUFF_SIZE-15)];
    else
      assign buff_info = {occ_lines, {(15-BUFF_SIZE){1'b0}}};
  end else begin
    assign buff_info = 16'd0;
  end endgenerate

  // Flow Control State
  // xfer_cnt: Total transfer count since fc_enabled = 1
  // accum: Transfer count since last FC response
  // fc_freq: The threshold for sending an FC response
  reg [63:0] xfer_cnt_bytes = 64'd0;
  reg [39:0] xfer_cnt_pkts  = 40'd0;
  reg [63:0] accum_bytes    = 64'd0;
  reg [39:0] accum_pkts     = 40'd0;
  reg [63:0] fc_freq_bytes  = 64'd0;
  reg [39:0] fc_freq_pkts   = 40'd0;

  // State machine transition signals info
  reg        fc_enabled = 1'b0;   // Is flow control enabled?
  wire       fc_ping;             // A flow control response was requested
  wire       fc_first_resp;       // Send the first flow control response
  wire       fc_refresh;          // Refresh accumulated values
  wire       fc_override;         // Override total xfer counts
  reg        fc_override_del = 1'b0;
  reg  [3:0] fc_due_shreg = 4'h0; // Is a response due? (shift register)

  // Endpoint IDs of this endpoint and the stream source
  reg [15:0] this_epid = 16'd0, return_epid = 16'd0;

  // Cached values from a stream command
  reg [63:0] strc_num_bytes;
  reg [39:0] strc_num_pkts;
  reg [3:0]  strc_op_data;  // Unused for now
  reg [3:0]  strc_op_code;

  // Total transfer count updater
  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      // Reset
      xfer_cnt_bytes <= 64'd0;
      xfer_cnt_pkts  <= 40'd0;
    end else if (fc_override) begin
      // Override
      xfer_cnt_bytes <= strc_num_bytes;
      xfer_cnt_pkts  <= strc_num_pkts;
    end else if (buff_tvalid && buff_tready) begin
      // Count
      xfer_cnt_bytes <= xfer_cnt_bytes + (CHDR_W/8);
      if (buff_tlast)
        xfer_cnt_pkts <= xfer_cnt_pkts + 40'd1;
    end
  end

  // Accumulated transfer count updater
  always @(posedge clk) begin
    if (rst || !fc_enabled || fc_refresh) begin
      // Reset
      accum_bytes <= 64'd0;
      accum_pkts  <= 40'd0;
    end else if (buff_tvalid && buff_tready) begin
      // Count
      accum_bytes <= accum_bytes + (CHDR_W/8);
      if (buff_tlast)
        accum_pkts <= accum_pkts + 40'd1;
    end
  end

  // Flow control trigger
  // Why a shift-register here?
  // 1. For edge detection
  // 2. To allow the tools to re-time the wide comparators.
  //    We don't care about the latency here because stream
  //    status messages are asynchronous wrt the input.
  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      fc_due_shreg <= 4'h0;
    end else begin
      fc_due_shreg <= {
        fc_due_shreg[2:0],
        (accum_bytes >= fc_freq_bytes) || (accum_pkts >= fc_freq_pkts)
      };
    end
  end
  wire fc_resp_due = fc_due_shreg[2] && !fc_due_shreg[3];

   // ---------------------------------------------------
  //  Stream Command Handler
  // ---------------------------------------------------
  localparam [2:0] ST_IN_HDR    = 3'd0;   // The CHDR header of an input pkt
  localparam [2:0] ST_IN_DATA   = 3'd1;   // The CHDR body (incl. mdata) of an input pkt
  localparam [2:0] ST_STRC_W0   = 3'd2;   // The first word of a stream command
  localparam [2:0] ST_STRC_W1   = 3'd3;   // The second word of a stream command
  localparam [2:0] ST_STRC_EXEC = 3'd4;   // A stream command is executing
  localparam [2:0] ST_FLUSH     = 3'd5;   // Input is flushing
  localparam [2:0] ST_DROP      = 3'd6;   // Current packet is being dropped

  reg [2:0]   state = ST_IN_HDR;          // State of the input state machine
  reg         pkt_too_long = 1'b0;        // Error case. Packet is too long
  reg         is_first_data_pkt = 1'b1;   // Is this the first data pkt after fc_enabled = 1?
  reg         is_first_strc_pkt = 1'b1;   // Is this the strm cmd data pkt after fc_enabled = 1?
  reg [15:0]  exp_data_seq_num = 16'd0;   // Expected sequence number for the next data pkt
  reg [15:0]  exp_strc_seq_num = 16'd0;   // Expected sequence number for the next stream cmd pkt
  reg [15:0]  strc_dst_epid = 16'd0;      // EPID in CHDR header of STRC packet 

  reg [FLUSH_TIMEOUT_W-1:0] flush_counter = {FLUSH_TIMEOUT_W{1'b0}};

  // Shortcuts
  wire is_data_pkt = 
    chdr_get_pkt_type(buff_tdata[63:0]) == CHDR_PKT_TYPE_DATA ||
    chdr_get_pkt_type(buff_tdata[63:0]) == CHDR_PKT_TYPE_DATA_TS;
  wire is_strc_pkt =
    chdr_get_pkt_type(buff_tdata[63:0]) == CHDR_PKT_TYPE_STRC;

  // Error Logic
  wire data_seq_err_stb = (state == ST_IN_HDR) && is_data_pkt && !is_first_data_pkt &&
    (chdr_get_seq_num(buff_tdata[63:0]) != exp_data_seq_num);
  wire strc_seq_err_stb = (state == ST_IN_HDR) && is_strc_pkt && !is_first_strc_pkt &&
    (chdr_get_seq_num(buff_tdata[63:0]) != exp_strc_seq_num);
  wire seq_err_stb = (data_seq_err_stb || strc_seq_err_stb) && buff_tvalid && buff_tready;

  wire route_err_stb = buff_tvalid && buff_tready && (state == ST_IN_HDR) && 
    (chdr_get_dst_epid(buff_tdata[63:0]) != this_epid);

  // Break critical paths to response FIFO
  reg [47:0] stream_err_info = 48'h0;
  reg        stream_err_stb = 1'b0;
  reg [3:0]  stream_err_status = CHDR_STRS_STATUS_OKAY;

  always @(posedge clk) begin
    if (rst || (SIGNAL_ERRS == 0)) begin
      stream_err_stb <= 1'b0;
    end else begin
      stream_err_stb <= seq_err_stb | route_err_stb | data_err_stb;
      if (seq_err_stb) begin
        stream_err_status <= CHDR_STRS_STATUS_SEQERR;
        // The extended info has the packet type (to detect which stream
        // had an error), the expected and actual sequence number.
        stream_err_info <= {13'h0, chdr_get_pkt_type(buff_tdata[63:0]),
            data_seq_err_stb ? exp_data_seq_num : exp_strc_seq_num, 
            chdr_get_seq_num(buff_tdata[63:0])};
      end else if (route_err_stb) begin
        stream_err_status <= CHDR_STRS_STATUS_RTERR;
        // The extended info has the expected and actual destination EPID.
        stream_err_info <= {16'd0, this_epid, chdr_get_dst_epid(buff_tdata[63:0])};
      end else begin
        stream_err_status <= CHDR_STRS_STATUS_DATAERR;
        // The extended info has the expected and actual destination EPID.
        stream_err_info <= {16'd0, this_epid, chdr_get_dst_epid(buff_tdata[63:0])};
      end
    end
  end

  // Input State Machine
  // - Pass data packets forward
  // - Consume stream cmd packets
  always @(posedge clk) begin
    if (rst) begin
      state <= ST_IN_HDR;
      pkt_too_long <= 1'b0;
      fc_enabled <= 1'b0;
    end else begin
      case (state)
        ST_IN_HDR: begin
          if (buff_tvalid && buff_tready) begin
            if (!buff_tlast) begin
              // Classify packet and...
              if (is_strc_pkt) begin
                // ...consume if it is a stream command or...
                state <= ST_STRC_W0;
              end else if (is_data_pkt) begin
                // ...pass to output if it is a data packet...
                state <= ST_IN_DATA;
              end else begin
                // ... otherwise drop.
                state <= ST_DROP;
              end
            end
            // Update other state vars
            pkt_too_long <= 1'b0;
            if (is_strc_pkt) begin
              is_first_strc_pkt <= 1'b0;
              strc_dst_epid <= chdr_get_dst_epid(buff_tdata[63:0]);
              exp_strc_seq_num <= chdr_get_seq_num(buff_tdata[63:0]) + 16'd1;
            end else if (is_data_pkt) begin
              is_first_data_pkt <= 1'b0;
              exp_data_seq_num <= chdr_get_seq_num(buff_tdata[63:0]) + 16'd1;
            end
          end
        end
        ST_IN_DATA: begin
          // Pass the data packet forward
          if (buff_tvalid && buff_tready && buff_tlast)
            state <= ST_IN_HDR;
        end
        ST_STRC_W0: begin
          if (buff_tvalid && buff_tready) begin
            // Consume the first word of a stream command packet
            if (CHDR_W > 64) begin
              strc_num_bytes <= chdr128_strc_get_num_bytes(buff_tdata[127:0]);
              strc_num_pkts  <= chdr128_strc_get_num_pkts (buff_tdata[127:0]);
              strc_op_data   <= chdr128_strc_get_op_data  (buff_tdata[127:0]);
              strc_op_code   <= chdr128_strc_get_op_code  (buff_tdata[127:0]);
              return_epid    <= chdr128_strs_get_src_epid (buff_tdata[127:0]);
              state <= ST_STRC_EXEC;
              pkt_too_long <= ~buff_tlast;
            end else begin
              strc_num_pkts <= chdr64_strc_get_num_pkts(buff_tdata[63:0]);
              strc_op_data  <= chdr64_strc_get_op_data (buff_tdata[63:0]);
              strc_op_code  <= chdr64_strc_get_op_code (buff_tdata[63:0]);
              return_epid   <= chdr64_strs_get_src_epid(buff_tdata[63:0]);
              state <= ST_STRC_W1;
            end
          end
        end
        ST_STRC_W1: begin
          if (buff_tvalid && buff_tready) begin
            // Consume the second word of a stream command packet
            strc_num_bytes <= chdr64_strc_get_num_bytes(buff_tdata[63:0]);
            state <= ST_STRC_EXEC;
            pkt_too_long <= ~buff_tlast;
          end
        end
        ST_STRC_EXEC: begin
          case (strc_op_code)
            CHDR_STRC_OPCODE_INIT: begin
              // Configure FC but disable it temporarily
              fc_freq_bytes <= strc_num_bytes;
              fc_freq_pkts <= strc_num_pkts;
              this_epid <= strc_dst_epid;
              fc_enabled <= 1'b0;
              // Flush the input
              state <= ST_FLUSH;
              flush_counter <= {FLUSH_TIMEOUT_W{1'b1}};
            end
            CHDR_STRC_OPCODE_PING: begin
              // Ping can complete in 1 cycle
              state <= pkt_too_long ? ST_DROP : ST_IN_HDR;
            end
            CHDR_STRC_OPCODE_RESYNC: begin
              // Resync can complete in 1 cycle
              state <= pkt_too_long ? ST_DROP : ST_IN_HDR;
            end
            default: begin
              state <= pkt_too_long ? ST_DROP : ST_IN_HDR;
            end
          endcase
        end
        ST_FLUSH: begin
          // Drop until the next packet arrives
          if (buff_tvalid && buff_tready) begin
            flush_counter <= {FLUSH_TIMEOUT_W{1'b1}};
          end else begin
            flush_counter <= flush_counter - 'd1;
            if (flush_counter == {FLUSH_TIMEOUT_W{1'b0}}) begin
              // Done flushing. Re-arm flow control and reset packet
              // sequence check info.
              fc_enabled <= 1'b1;
              is_first_data_pkt <= 1'b1;
              is_first_strc_pkt <= 1'b1;
              state <= ST_IN_HDR;
            end
          end
        end
        ST_DROP: begin
          // Drop until the next packet arrives
          if (buff_tvalid && buff_tready && buff_tlast)
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
      ST_IN_HDR:
        buff_tready = m_axis_data_tready || !is_data_pkt;
      ST_IN_DATA:
        buff_tready = m_axis_data_tready;
      ST_STRC_W0:
        buff_tready = 1'b1;
      ST_STRC_W1:
        buff_tready = 1'b1;
      ST_FLUSH:
        buff_tready = 1'b1;
      ST_DROP:
        buff_tready = 1'b1;
      default:
        buff_tready = 1'b0;
    endcase
  end

  // Logic to drive output port
  assign m_axis_data_tdata  = buff_tdata;
  assign m_axis_data_tlast  = buff_tlast;
  assign m_axis_data_tvalid = buff_tvalid && 
    ((state == ST_IN_HDR && is_data_pkt) || state == ST_IN_DATA);

  // Logic to drive triggers
  assign fc_ping = (state == ST_STRC_EXEC) && (strc_op_code == CHDR_STRC_OPCODE_PING);
  assign fc_first_resp = (state == ST_FLUSH) && (flush_counter == {FLUSH_TIMEOUT_W{1'b0}});
  assign fc_override = (state == ST_STRC_EXEC) && (strc_op_code == CHDR_STRC_OPCODE_RESYNC);
  always @(posedge clk) fc_override_del <= fc_override;

  wire [51:0] resp_o_tdata;
  wire        resp_o_tvalid;
  reg  [51:0] resp_i_tdata;
  reg         resp_i_tvalid = 1'b0;

  // Send a stream status packet for the following cases:
  // - Immediately after initialization 
  // - If a response is explicitly requested (ping)
  // - If a response is due i.e. we have exceeded the frequency
  // - If FC is resynchronized via a stream cmd
  // - If an error is detected in the stream
  always @(posedge clk) begin
    if (rst) begin
      resp_i_tvalid <= 1'b0;
      resp_i_tdata  <= 52'h0;
    end else begin
      resp_i_tvalid <= fc_first_resp || fc_ping || fc_resp_due || fc_override_del || stream_err_stb;
      resp_i_tdata  <= stream_err_stb ? {stream_err_info, stream_err_status} : {48'h0, CHDR_STRS_STATUS_OKAY};
    end
  end

  // ---------------------------------------------------
  //  Stream Status Responder
  // ---------------------------------------------------
  localparam [2:0] ST_STRS_IDLE = 3'd0;   // Waiting for response to post
  localparam [2:0] ST_STRS_HDR  = 3'd1;   // Sending response CHDR header
  localparam [2:0] ST_STRS_W0   = 3'd2;   // Sending first response word
  localparam [2:0] ST_STRS_W1   = 3'd3;   // Sending second response word
  localparam [2:0] ST_STRS_W2   = 3'd4;   // Sending third response word
  localparam [2:0] ST_STRS_W3   = 3'd5;   // Sending fourth response word
  localparam [2:0] ST_STRS_DONE = 3'd6;   // Consuming response

  reg [2:0]  resp_state = ST_STRS_IDLE;   // State of the responder
  reg [15:0] resp_seq_num = 16'd0;        // Current sequence number of response

  assign fc_refresh = (resp_state == ST_STRS_DONE);

  // A FIFO that holds up to 32 posted responses and status information
  // NOTE: This is a lossy FIFO. If the downstream response port is clogged
  // then we will drop responses. That should never happen in a normal operating
  // scenario.
  axi_fifo #(.WIDTH(48 + 4), .SIZE(5)) resp_fifo_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata(resp_i_tdata), .i_tvalid(resp_i_tvalid), .i_tready(/* Lossy FIFO */),
    .o_tdata(resp_o_tdata), .o_tvalid(resp_o_tvalid), .o_tready(resp_state == ST_STRS_DONE || !fc_enabled),
    .space(), .occupied()
  );

  // Responder State Machine
  // - Wait for response to appear in FIFO
  // - Output a full packet (different # of xfers depending on CHDR_W)
  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      resp_state <= ST_STRS_IDLE;
      resp_seq_num <= 16'd0;
    end else begin
      case (resp_state)
        ST_STRS_IDLE: begin
          if (resp_o_tvalid)
            resp_state <= ST_STRS_HDR;
        end
        ST_STRS_HDR: begin
          if (m_axis_strs_tready)
            resp_state <= ST_STRS_W0;
        end
        ST_STRS_W0: begin
          if (m_axis_strs_tready)
            if (CHDR_W < 256)
              resp_state <= ST_STRS_W1;
            else
              resp_state <= ST_STRS_DONE;
        end
        ST_STRS_W1: begin
          if (m_axis_strs_tready)
            if (CHDR_W < 128)
              resp_state <= ST_STRS_W2;
            else
              resp_state <= ST_STRS_DONE;
        end
        ST_STRS_W2: begin
          if (m_axis_strs_tready)
            resp_state <= ST_STRS_W3;
        end
        ST_STRS_W3: begin
          if (m_axis_strs_tready)
            resp_state <= ST_STRS_DONE;
        end
        ST_STRS_DONE: begin
          resp_state <= ST_STRS_IDLE;
          resp_seq_num <= resp_seq_num + 16'd1;
        end
        default: begin
          // We should never get here
          resp_state <= ST_STRS_IDLE;
        end
      endcase
    end
  end

  // Output data. Header and Payload
  wire [63:0] strs_header = chdr_build_header(
    /*VC*/ 6'd0, /*eob*/ 1'b0, /*eov*/ 1'b0, CHDR_PKT_TYPE_STRS, CHDR_NO_MDATA,
    resp_seq_num, 16'd32+(CHDR_W/8), return_epid);
  wire [255:0] strs_payload = chdr256_strs_build(
    /*statusinfo*/ resp_o_tdata[51:4], buff_info,
    xfer_cnt_bytes, xfer_cnt_pkts,
    BUFF_SIZE_PKTS[23:0], BUFF_SIZE_BYTES[39:0],
    resp_o_tdata[3:0], this_epid);

  // m_axis_strs_* signal values depend on CHDR_W
  generate
    if (CHDR_W == 64) begin
      // Response spans 5 transfers (header + 4 words)
      assign m_axis_strs_tlast = (resp_state == ST_STRS_W3);
      always @(*) begin
        case (resp_state)
          ST_STRS_W0:
            m_axis_strs_tdata = strs_payload[63:0];
          ST_STRS_W1:
            m_axis_strs_tdata = strs_payload[127:64];
          ST_STRS_W2:
            m_axis_strs_tdata = strs_payload[191:128];
          ST_STRS_W3:
            m_axis_strs_tdata = strs_payload[255:192];
          default:
            m_axis_strs_tdata = strs_header;
        endcase
      end
    end else if (CHDR_W == 128) begin
      // Response spans 3 transfers (header + 2 words)
      assign m_axis_strs_tlast = (resp_state == ST_STRS_W1);
      always @(*) begin
        case (resp_state)
          ST_STRS_W0:
            m_axis_strs_tdata = strs_payload[127:0];
          ST_STRS_W1:
            m_axis_strs_tdata = strs_payload[255:128];
          default:
            m_axis_strs_tdata = {64'h0, strs_header};
        endcase
      end
    end else begin
      // Response spans 2 transfers (header + word)
      assign m_axis_strs_tlast = (resp_state == ST_STRS_W0);
      always @(*) begin
        case (resp_state)
          ST_STRS_W0:
            m_axis_strs_tdata[255:0] = strs_payload;
          default:
            m_axis_strs_tdata[255:0] = {192'h0, strs_header};
        endcase
        if (CHDR_W > 256) begin
          m_axis_strs_tdata[CHDR_W-1:256] = 'h0;
        end
      end
    end
  endgenerate

  assign m_axis_strs_tvalid = (resp_state != ST_STRS_IDLE) && (resp_state != ST_STRS_DONE);

endmodule // chdr_stream_input
