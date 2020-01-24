//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_output
// Description:
//   Implements the CHDR output port for a stream endpoint.
//   The module generates stream command packets to setup
//   a downstream endpoint module (chdr_stream_input). Once
//   a stream is setup, the CHDR data on the axis_data port
//   can be sent downstream with full flow control. Stream
//   status messages are recieved from the downstream node
//   to update flow control state. This module has an external
//   configuration bus to initiate stream creation.
//
// Parameters:
//   - CHDR_W: Width of the CHDR bus in bits
//   - MTU: Log2 of the maximum number of lines in a packet
//
// Signals:
//   - m_axis_chdr_* : Output CHDR stream (AXI-Stream)
//   - s_axis_data_* : Input CHDR Data stream (AXI-Stream) before flow control
//   - s_axis_strs_* : Input stream status (AXI-Stream)

module chdr_stream_output #(
  parameter CHDR_W = 256,
  parameter MTU    = 10
)(
  // Clock, reset and settings
  input  wire              clk,
  input  wire              rst,
  // CHDR out (AXI-Stream)
  output wire [CHDR_W-1:0] m_axis_chdr_tdata,
  output wire              m_axis_chdr_tlast,
  output wire              m_axis_chdr_tvalid,
  input  wire              m_axis_chdr_tready,
  // Data packets in (AXI-Stream)
  input  wire [CHDR_W-1:0] s_axis_data_tdata,
  input  wire              s_axis_data_tlast,
  input  wire              s_axis_data_tvalid,
  output wire              s_axis_data_tready,
  // Stream status in (AXI-Stream)
  input  wire [CHDR_W-1:0] s_axis_strs_tdata,
  input  wire              s_axis_strs_tlast,
  input  wire              s_axis_strs_tvalid,
  output wire              s_axis_strs_tready,
  // Configuration port
  input  wire              cfg_start,
  output reg               cfg_pending = 1'b0,
  output reg               cfg_failed = 1'b0,
  input  wire              cfg_lossy_xport,
  input  wire [15:0]       cfg_dst_epid,
  input  wire [15:0]       cfg_this_epid,
  input  wire [39:0]       cfg_fc_freq_bytes,
  input  wire [23:0]       cfg_fc_freq_pkts,
  input  wire [15:0]       cfg_fc_headroom_bytes,
  input  wire [7:0]        cfg_fc_headroom_pkts,
  // Flow control status
  output reg               fc_enabled     = 1'b0,
  output reg  [39:0]       capacity_bytes = 40'd0,
  output reg  [23:0]       capacity_pkts  = 24'd0,
  // Stream status
  output wire              seq_err_stb,
  output reg  [31:0]       seq_err_cnt = 32'd0,
  output wire              data_err_stb,
  output reg  [31:0]       data_err_cnt = 32'd0,
  output wire              route_err_stb,
  output reg  [31:0]       route_err_cnt = 32'd0
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_chdr_internal_utils.vh"

  localparam CHDR_W_LOG2 = $clog2(CHDR_W);

  // ---------------------------------------------------
  //  Output packet gate
  // ---------------------------------------------------
  reg  [CHDR_W-1:0] chdr_out_tdata;
  reg               chdr_out_tlast, chdr_out_tvalid;
  wire              chdr_out_tready;

  axi_packet_gate #(
    .WIDTH(CHDR_W), .SIZE(MTU), .USE_AS_BUFF(0)
  ) chdr_pkt_gate_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata(chdr_out_tdata), .i_tlast(chdr_out_tlast), .i_terror(1'b0),
    .i_tvalid(chdr_out_tvalid), .i_tready(chdr_out_tready),
    .o_tdata(m_axis_chdr_tdata), .o_tlast(m_axis_chdr_tlast),
    .o_tvalid(m_axis_chdr_tvalid), .o_tready(m_axis_chdr_tready)
  );

  // ---------------------------------------------------
  //  Flow Control State
  // ---------------------------------------------------

  // send_cnt: Total transfer count at the sender (here)
  // recv_cnt: Total transfer count at the receiver
  // accum:    Transfer count since last FC resynchronization request
  // headroom: Total headroom to keep in the downstream buffer
  // adj_cap:  The adjusted capacity (after headroom) of the downstream buffer
  // strc_cnt: Saved count for the STRC packet (prevents mid-packet updates)
  reg [63:0] send_cnt_bytes = 64'd0;
  reg [39:0] send_cnt_pkts  = 40'd0;
  reg [63:0] recv_cnt_bytes = 64'd0;
  reg [39:0] recv_cnt_pkts  = 40'd0;
  reg [39:0] accum_bytes    = 40'd0;
  reg [23:0] accum_pkts     = 24'd0;
  reg [15:0] headroom_bytes = 16'd0;
  reg [ 7:0] headroom_pkts  =  8'd0;
  reg [39:0] adj_cap_bytes  = 40'd0;
  reg [23:0] adj_cap_pkts   = 24'd0;
  reg [63:0] strc_cnt_bytes = 64'd0;

  // Output transfer count
  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      send_cnt_bytes <= 64'd0;
      send_cnt_pkts  <= 40'd0;
    end else if (chdr_out_tvalid && chdr_out_tready) begin
      send_cnt_bytes <= send_cnt_bytes + (CHDR_W/8);
      if (chdr_out_tlast)
        send_cnt_pkts <= send_cnt_pkts + 40'd1;
    end
  end

  // Buffer occupied counts
  // TODO: Need better overflow handling
  wire signed [64:0] occupied_bytes = 
    $signed({1'b0, send_cnt_bytes}) - $signed({1'b0, recv_cnt_bytes});
  wire signed [40:0] occupied_pkts =
    $signed({1'b0, send_cnt_pkts}) - $signed({1'b0, recv_cnt_pkts});

  // OK-to-Send shift register
  // - Why a shift-register here?
  //   To allow the tools to re-time the wide comparators.
  // - We don't care about the latency here because stream
  //   status messages are asynchronous wrt the data
  reg  [3:0] ok_shreg = 4'b1111; // OK to send? (shift register)
  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      ok_shreg <= 4'b1111;
    end else begin
      ok_shreg <= {ok_shreg[2:0], (
        (occupied_bytes[40:0] < $signed({1'b0, adj_cap_bytes})) &&
        (occupied_pkts [24:0] < $signed({1'b0, adj_cap_pkts }))
      )};
    end
  end
  wire ok_to_send = ok_shreg[3];

  // Accumulated transfer count updater for FC resync
  reg lossy_xport = 1'b0;
  reg [3:0] fc_resync_req_shreg = 4'h0;
  wire fc_resync_req, fc_resync_ack;

  always @(posedge clk) begin
    if (rst || !fc_enabled || !lossy_xport || fc_resync_ack) begin
      // Reset
      accum_bytes <= 40'd0;
      accum_pkts <= 24'd0;
      fc_resync_req_shreg <= 4'b0000;
    end else begin 
      if (chdr_out_tvalid && chdr_out_tready) begin
        // Count
        accum_bytes <= accum_bytes + (CHDR_W/8);
        if (chdr_out_tlast)
          accum_pkts <= accum_pkts + 24'd1;
      end
      // FC resync request
      fc_resync_req_shreg <= {fc_resync_req_shreg[2:0],
        (accum_bytes > capacity_bytes) || (accum_pkts > capacity_pkts)};
    end
  end
  assign fc_resync_req = fc_resync_req_shreg[3];

  // ---------------------------------------------------
  //  Stream Status Parser
  // ---------------------------------------------------

  wire [3:0] msg_i_tdata,  msg_o_tdata;
  wire       msg_i_tvalid, msg_o_tvalid;
  wire       msg_i_tready, msg_o_tready;

  axi_fifo #(.WIDTH(4), .SIZE(1)) msg_fifo_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata(msg_i_tdata), .i_tvalid(msg_i_tvalid), .i_tready(msg_i_tready),
    .o_tdata(msg_o_tdata), .o_tvalid(msg_o_tvalid), .o_tready(msg_o_tready),
    .space(), .occupied()
  );

  localparam [2:0] ST_STRS_HDR   = 3'd0;  // Receiving the CHDR header of a stream status msg
  localparam [2:0] ST_STRS_W0    = 3'd1;  // Receiving the first word of a stream status msg
  localparam [2:0] ST_STRS_W1    = 3'd2;  // Receiving the second word of a stream status msg
  localparam [2:0] ST_STRS_W2    = 3'd3;  // Receiving the third word of a stream status msg
  localparam [2:0] ST_STRS_W3    = 3'd4;  // Receiving the fourth word of a stream status msg
  localparam [2:0] ST_STRS_LATCH = 3'd5;  // Atomically updating and posting the status msg
  localparam [2:0] ST_STRS_DROP  = 3'd6;  // Something went wrong dropping current packet

  reg [2:0]   strs_state = ST_STRS_HDR;
  reg         strs_too_long = 1'b0;
  reg [15:0]  cached_dst_epid = 16'd0;
  reg [255:0] cached_strs_msg;

  always @(posedge clk) begin
    if (rst) begin
      strs_state <= ST_STRS_HDR;
      strs_too_long <= 1'b0;
    end else begin
      case (strs_state)

        // ST_STRS_HDR
        // ------------------
        ST_STRS_HDR: begin
          if (s_axis_strs_tvalid) begin
            // Only accept stream status packets. Drop everything else
            if (chdr_get_pkt_type(s_axis_strs_tdata[63:0]) == CHDR_PKT_TYPE_STRS)
              strs_state <= ST_STRS_W0;
            else
              strs_state <= ST_STRS_DROP;
            strs_too_long <= 1'b0;
          end
        end

        // ST_STRS_W0
        // ------------------
        // - Cache the first word of the stream status
        // - For CHDR_W == 64, this is one of 4 words.
        // - For CHDR_W == 128, this is one of 2 words.
        // - For CHDR_W >= 256, this is the only word.
        ST_STRS_W0: begin
          if (s_axis_strs_tvalid) begin
            if (CHDR_W == 64) begin
              cached_strs_msg[63:0] <= s_axis_strs_tdata[63:0];
              strs_state <= !s_axis_strs_tlast ? ST_STRS_W1 : ST_STRS_HDR;
            end else if (CHDR_W == 128) begin
              cached_strs_msg[127:0] <= s_axis_strs_tdata[127:0];
              strs_state <= !s_axis_strs_tlast ? ST_STRS_W1 : ST_STRS_HDR;
            end else begin  //CHDR_W >= 256
              cached_strs_msg[255:0] <= s_axis_strs_tdata[255:0];
              strs_state <= ST_STRS_LATCH;
              strs_too_long <= !s_axis_strs_tlast;
            end
          end
        end

        // ST_STRS_W1
        // ------------------
        // - Cache the second word of the stream status
        ST_STRS_W1: begin
          if (s_axis_strs_tvalid) begin
            if (CHDR_W == 64) begin
              cached_strs_msg[127:64] <= s_axis_strs_tdata[63:0];
              strs_state <= !s_axis_strs_tlast ? ST_STRS_W2 : ST_STRS_HDR;
            end else begin  //CHDR_W >= 128
              cached_strs_msg[255:128] <= s_axis_strs_tdata[127:0];
              strs_state <= ST_STRS_LATCH;
              strs_too_long <= !s_axis_strs_tlast;
            end
          end
        end

        // ST_STRS_W2
        // ------------------
        // - Cache the third word of the stream status
        ST_STRS_W2: begin
          if (s_axis_strs_tvalid) begin
            cached_strs_msg[191:128] <= s_axis_strs_tdata[63:0];
              strs_state <= !s_axis_strs_tlast ? ST_STRS_W3 : ST_STRS_HDR;
          end
        end

        // ST_STRS_W3
        // ------------------
        // - Cache the fourth word of the stream status
        ST_STRS_W3: begin
          if (s_axis_strs_tvalid) begin
            cached_strs_msg[255:192] <= s_axis_strs_tdata[63:0];
            strs_state <= ST_STRS_LATCH;
            strs_too_long <= !s_axis_strs_tlast;
          end
        end

        // ST_STRS_LATCH
        // ------------------
        // - Act on the received stream status
        ST_STRS_LATCH: begin
          capacity_bytes <= chdr256_strs_get_capacity_bytes(cached_strs_msg);
          capacity_pkts  <= chdr256_strs_get_capacity_pkts(cached_strs_msg);
          recv_cnt_bytes <= chdr256_strs_get_xfercnt_bytes(cached_strs_msg);
          recv_cnt_pkts  <= chdr256_strs_get_xfercnt_pkts(cached_strs_msg);
          adj_cap_bytes  <= chdr256_strs_get_capacity_bytes(cached_strs_msg) - 
                            {24'd0, headroom_bytes[15:(CHDR_W_LOG2-3)], {(CHDR_W_LOG2-3){1'b0}}};
          adj_cap_pkts   <= chdr256_strs_get_capacity_pkts(cached_strs_msg) - 
                            {16'd0, headroom_pkts};
          if (msg_i_tready) begin
            strs_state <= strs_too_long ? ST_STRS_DROP : ST_STRS_HDR;
          end
        end

        // ST_STRS_DROP
        // ------------------
        ST_STRS_DROP: begin
          if (s_axis_strs_tvalid && s_axis_strs_tlast)
            strs_state <= ST_STRS_HDR;
        end
        default: begin
          // We should never get here
          strs_state <= ST_STRS_HDR;
        end
      endcase
    end
  end

  assign s_axis_strs_tready = (strs_state != ST_STRS_LATCH);

  assign msg_i_tvalid = (strs_state == ST_STRS_LATCH);
  assign msg_i_tdata = (chdr256_strs_get_src_epid(cached_strs_msg) != cached_dst_epid) ? 
    CHDR_STRS_STATUS_CMDERR : chdr256_strs_get_status(cached_strs_msg);


  // ---------------------------------------------------
  //  Main State Machine
  // ---------------------------------------------------

  localparam [2:0] ST_PASS_DATA = 3'd0;   // Passing input axis_data out
  localparam [2:0] ST_STRC_HDR  = 3'd1;   // Sending CHDR header for stream cmd
  localparam [2:0] ST_STRC_W0   = 3'd2;   // Sending first word of stream cmd
  localparam [2:0] ST_STRC_W1   = 3'd3;   // Sending second word of stream cmd
  localparam [2:0] ST_STRC_WAIT = 3'd4;   // Waiting for response (stream status)
  localparam [2:0] ST_INIT_DLY  = 3'd5;   // Finishing command execution

  reg [2:0]   state = ST_PASS_DATA;
  reg         mid_pkt = 1'b0;
  reg [15:0]  data_seq_num = 16'd0;
  reg [15:0]  strc_seq_num = 16'd0;
  reg [2:0]   cfg_delay = 3'd0;

  always @(posedge clk) begin
    if (rst) begin
      state <= ST_PASS_DATA;
      mid_pkt <= 1'b0;
      data_seq_num <= 16'd0;
      strc_seq_num <= 16'd0;
      cfg_pending <= 1'b0;
      cfg_failed <= 1'b0;
    end else begin
      case (state)

        // ST_PASS_DATA
        // ------------------
        // This is the default state where input data is passed to the
        // output port. Flow control is enforced in this state.
        // This state also serves as the launch state for a configuration
        // operation (using cfg_start) 
        ST_PASS_DATA: begin
          // Update the mid_pkt flag and sequence number
          if (chdr_out_tvalid && chdr_out_tready) begin
            mid_pkt <= !chdr_out_tlast;
            if (chdr_out_tlast)
              data_seq_num <= data_seq_num + 16'd1;
          end
          // Launch a configuration operation
          if (cfg_start) begin
            // Latch cfg command
            cfg_pending <= 1'b1;
            cfg_failed <= 1'b0;
            // Disable flow control
            fc_enabled <= 1'b0;
            // Cache relevant data from the cfg cmd
            lossy_xport <= cfg_lossy_xport;
            cached_dst_epid <= cfg_dst_epid;
            headroom_bytes <= cfg_fc_headroom_bytes;
            headroom_pkts <= cfg_fc_headroom_pkts;
          end
          // Wait for current packet to transfer then begin the
          // configuration process or stream command
          if (cfg_start || cfg_pending || fc_resync_req) begin
            if (mid_pkt) begin
              if (chdr_out_tvalid && chdr_out_tready && chdr_out_tlast)
                state <= ST_STRC_HDR;
            end else begin
              if (!(chdr_out_tvalid && chdr_out_tready))
                state <= ST_STRC_HDR;
            end
          end
        end

        // ST_STRC_HDR
        // ------------------
        // Send the CHDR header for a stream command
        ST_STRC_HDR: begin
          if (chdr_out_tvalid && chdr_out_tready) begin
            state <= ST_STRC_W0;
            // Update seqnum for the next packet
            strc_seq_num <= strc_seq_num + 16'd1;
          end
          // Update byte count for stream command
          strc_cnt_bytes <= send_cnt_bytes;
        end

        // ST_STRC_W0
        // ------------------
        // Send the first line of a stream command
        ST_STRC_W0: begin
          if (chdr_out_tvalid && chdr_out_tready)
            if (CHDR_W < 128)
              state <= ST_STRC_W1;
            else
              state <= ST_STRC_WAIT;
        end

        // ST_STRC_W1
        // ------------------
        // Send the second line of a stream command
        ST_STRC_W1: begin
          if (chdr_out_tvalid && chdr_out_tready)
            state <= fc_resync_req ? ST_PASS_DATA : ST_STRC_WAIT;
        end

        // ST_STRC_WAIT
        // ------------------
        // Done sending stream command. Wait for a response
        ST_STRC_WAIT: begin
          // Wait for a new response to arrive
          if (msg_o_tvalid) begin
            if (msg_o_tdata == CHDR_STRS_STATUS_OKAY) begin
              state <= ST_INIT_DLY;
              cfg_delay <= 3'd4;
              fc_enabled <= 1'b1;
              data_seq_num <= 16'd0;
              strc_seq_num <= 16'd0;
            end else begin
              state <= ST_PASS_DATA;
              cfg_failed <= 1'b1;
              cfg_pending <= 1'b0;
            end
          end
        end

        // ST_INIT_DLY
        // ------------------
        // Delay matching state for ok_shreg
        ST_INIT_DLY: begin
          if (cfg_delay == 3'd0) begin
            state <= ST_PASS_DATA;
            cfg_pending <= 1'b0;
          end else begin
            cfg_delay <= cfg_delay - 3'd1;
          end
        end

        // We should never get here
        default: begin
          state <= ST_PASS_DATA;
        end
      endcase
    end
  end

  // Header for output CHDR data
  wire [CHDR_W-1:0] data_header;
  assign data_header[63:0] = chdr_set_seq_num(
    chdr_set_dst_epid(s_axis_data_tdata[63:0], cached_dst_epid),
    data_seq_num);
  generate if (CHDR_W > 64)
    assign data_header[CHDR_W-1:64] = s_axis_data_tdata[CHDR_W-1:64];
  endgenerate

  // Header for stream command
  wire [CHDR_W-1:0] strc_header;
  assign strc_header[63:0] = chdr_build_header(
    /*VC*/ 6'd0, /*eob*/ 1'b0, /*eov*/ 1'b0, CHDR_PKT_TYPE_STRC, CHDR_NO_MDATA,
    strc_seq_num, 16'd16+(CHDR_W/8), cached_dst_epid);
  generate if (CHDR_W > 64)
    assign strc_header[CHDR_W-1:64] = {(CHDR_W-64){1'b0}};
  endgenerate

  // Payload for stream command
  wire [127:0] strc_init_payload = chdr128_strc_build(
    {24'h0, cfg_fc_freq_bytes}, {16'h0, cfg_fc_freq_pkts},
    /*op_data*/ 4'h0, CHDR_STRC_OPCODE_INIT, cfg_this_epid);
  wire [127:0] strc_resync_payload = chdr128_strc_build(
    strc_cnt_bytes, send_cnt_pkts,
    /*op_data*/ 4'h0, CHDR_STRC_OPCODE_RESYNC, cfg_this_epid);
  wire [127:0] strc_payload = fc_resync_req ? strc_resync_payload : strc_init_payload;

  always @(*) begin
    case (state)
      ST_PASS_DATA: begin
        chdr_out_tdata  = mid_pkt ? s_axis_data_tdata : data_header;
        chdr_out_tlast  = s_axis_data_tlast;
        chdr_out_tvalid = s_axis_data_tvalid && ok_to_send;
      end
      ST_STRC_HDR: begin
        chdr_out_tdata  = strc_header;
        chdr_out_tlast  = 1'b0;
        chdr_out_tvalid = ok_to_send;
      end
      ST_STRC_W0: begin
        chdr_out_tdata  = strc_payload;
        chdr_out_tlast  = (CHDR_W < 128) ? 1'b0 : 1'b1;
        chdr_out_tvalid = ok_to_send;
      end
      ST_STRC_W1: begin
        // We will enter this state only if CHDR_W = 64
        chdr_out_tdata  = strc_payload[127:64];
        chdr_out_tlast  = 1'b1;
        chdr_out_tvalid = ok_to_send;
      end
      default: begin
        chdr_out_tdata  = {CHDR_W{1'b0}};
        chdr_out_tlast  = 1'b0;
        chdr_out_tvalid = 1'b0;
      end
    endcase
  end
  assign s_axis_data_tready = (state == ST_PASS_DATA) && chdr_out_tready && ok_to_send;

  // Consume all messages when passing data forward. The flow control state is automatically
  // updated outside the message FIFO. When a stream command is issued, we wait for the
  // "wait" state to consume responses.
  assign msg_o_tready = msg_o_tvalid && (state == ST_PASS_DATA || state == ST_STRC_WAIT);

  // Acknowledge a flow control resync command
  assign fc_resync_ack = fc_resync_req && (state == ST_STRC_W1) && 
                         chdr_out_tvalid && chdr_out_tready && chdr_out_tlast;

  // ---------------------------------------------------
  //  Stream Status Reporting
  // ---------------------------------------------------

  wire runtime_err_stb = msg_o_tvalid && msg_o_tready && (state == ST_PASS_DATA);
  assign seq_err_stb = runtime_err_stb && (msg_o_tdata == CHDR_STRS_STATUS_SEQERR);
  assign data_err_stb = runtime_err_stb && (msg_o_tdata == CHDR_STRS_STATUS_DATAERR);
  assign route_err_stb = runtime_err_stb && (msg_o_tdata == CHDR_STRS_STATUS_RTERR);

  always @(posedge clk) begin
    if (rst || !fc_enabled) begin
      seq_err_cnt <= 32'd0;
      data_err_cnt <= 32'd0;
      route_err_cnt <= 32'd0;
    end else begin
      if (seq_err_stb)
        seq_err_cnt <= seq_err_cnt + 32'd1;
      if (data_err_stb)
        data_err_cnt <= data_err_cnt + 32'd1;
      if (route_err_stb)
        route_err_cnt <= route_err_cnt + 32'd1;
    end
  end

endmodule // chdr_stream_output
