//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_traffic_source_sim
// Description:
//   A traffic generator for CHDR traffic. Simulation only.
//   Supports multiple traffic pattern and injection rates.
//

`timescale 1ns/1ps

`include "sim_cvita_lib.svh"

module chdr_traffic_source_sim #(
  parameter        WIDTH        = 64,   // Width of the AXI-Stream data bus
  parameter        MTU          = 5,    // log2 of the max number of lines in a packet
  parameter [15:0] NODE_ID      = 'd0,  // Node ID for this generator
  parameter [15:0] NUM_NODES    = 'd16  // Total number of generators in the application
) (
  // Clocks and resets
  input               clk,              // AXI-Stream clock
  input               rst,              // AXI-Stream reset
  // Settings         
  input [63:0]        current_time,     // The current value of the global timebase (synch to clk)
  input               start_stb,        // A strobe that indicates the start of a generation session
  input [7:0]         injection_rate,   // The inject rate (in percent) to simulate
  input [15:0]        lines_per_pkt,    // Number of lines per packet to generate
  input [7:0]         traffic_patt,     // The traffic pattern (see localparams below for values)
  input [31:0]        num_pkts_to_send, // Number of packets to send
  // CHDR master interface
  output [WIDTH-1:0]  m_axis_tdata,     // AXI-Stream master tdata
  output              m_axis_tlast,     // AXI-Stream master tlast
  output              m_axis_tvalid,    // AXI-Stream master tvalid
  input               m_axis_tready,    // AXI-Stream master tready
  // Metrics          
  output              session_active,   // Signal indicating if generation session is active
  output [63:0]       session_duration, // Session duration (only valid after session ends)
  output [31:0]       xfer_count,       // Number of lines transferred (only valid after session ends)
  output [31:0]       pkt_count         // Number of packets transferred (only valid after session ends)
);
  // **** Supported Traffic Patters ****
  localparam [7:0] TRAFFIC_PATT_LOOPBACK       = 8'd76;  //L
  localparam [7:0] TRAFFIC_PATT_NEIGHBOR       = 8'd78;  //N
  localparam [7:0] TRAFFIC_PATT_BIT_COMPLEMENT = 8'd67;  //C
  localparam [7:0] TRAFFIC_PATT_SEQUENTIAL     = 8'd83;  //S
  localparam [7:0] TRAFFIC_PATT_UNIFORM        = 8'd85;  //U
  localparam [7:0] TRAFFIC_PATT_UNIFORM_OTHERS = 8'd79;  //O
  localparam [7:0] TRAFFIC_PATT_RANDOM_PERM    = 8'd82;  //R

  cvita_master  #(.DWIDTH(WIDTH)) m_chdr (.clk(clk));
  axis_t        #(.DWIDTH(WIDTH)) post_fifo (.clk(clk));
  axis_t        #(.DWIDTH(WIDTH)) pre_gate (.clk(clk));
  cvita_hdr_t  header;
  reg throttle = 1'b1;

  logic        running = 0;
  logic [31:0] curr_pkt_num = 'd0;
  logic [31:0] num_samps_xferd = 'd0;
  logic [63:0] start_time = 0;
  logic [63:0] stop_time = 0;
  logic [15:0] last_gen_sid = (NODE_ID - 16'd1);

  assign xfer_count = num_samps_xferd;
  assign pkt_count = curr_pkt_num;
  assign session_duration = (stop_time - start_time);
  assign session_active = running;

  // Utility function to assign SIDs based on traffic pattern
  function [15:0] gen_dst_sid;
    input [7:0] traffic_patt;
    input [15:0] last_sid;

    if (traffic_patt == TRAFFIC_PATT_UNIFORM) begin
      gen_dst_sid = $urandom_range('d0, NUM_NODES-'d1);
    end else if (traffic_patt == TRAFFIC_PATT_UNIFORM_OTHERS) begin
      logic [31:0] rnum = $urandom_range('d0, NUM_NODES-'d2);
      if (rnum < NODE_ID)
        gen_dst_sid = rnum[15:0];
      else
        gen_dst_sid = rnum[15:0] + 16'd1;
    end else if (traffic_patt == TRAFFIC_PATT_SEQUENTIAL) begin
      gen_dst_sid = (last_sid + 16'd1) % NUM_NODES;
    end else if (traffic_patt == TRAFFIC_PATT_NEIGHBOR) begin
      gen_dst_sid = (NODE_ID + 16'd1) % NUM_NODES;
    end else if (traffic_patt == TRAFFIC_PATT_LOOPBACK) begin
      gen_dst_sid = NODE_ID;
    end else if (traffic_patt == TRAFFIC_PATT_BIT_COMPLEMENT) begin
      gen_dst_sid = (NUM_NODES - NODE_ID - 1) % NUM_NODES;
    end else if (traffic_patt == TRAFFIC_PATT_RANDOM_PERM) begin
      //TODO: Implement me
      gen_dst_sid = 0;
    end else begin
      gen_dst_sid = 'd0;
    end
  endfunction

  // Generation loop. Push to m_chdr infinitely fast
  initial begin: gen_blk
    // Generate infinitely
    $srandom(NODE_ID + NUM_NODES);
    m_chdr.reset();
    while (1) begin
      // A generation session begins on the posedge of start_stb
      while (~start_stb) @(posedge clk);
      curr_pkt_num = 'd0;
      m_chdr.reset();
      num_samps_xferd = 'd0;
      start_time = current_time;
      running = 1;
      while (curr_pkt_num < num_pkts_to_send) begin
        header = '{
          pkt_type:DATA, has_time:1, eob:0,
          seqnum:curr_pkt_num[11:0], length:(lines_per_pkt*8),
          src_sid:NODE_ID, dst_sid:gen_dst_sid(traffic_patt, last_gen_sid),
          timestamp:0 //TS attached later
        };
        last_gen_sid = header.dst_sid;
        curr_pkt_num = curr_pkt_num + 'd1;
        m_chdr.push_ramp_pkt(lines_per_pkt-2, 'h0, 'h1, header);
        num_samps_xferd = num_samps_xferd + lines_per_pkt;
      end
      running = 0;
      stop_time = current_time;
    end
  end

  // Capture packets in a really short FIFO (for backpressure)
  axi_fifo #(
    .WIDTH(WIDTH+1), .SIZE(MTU + 1)
  ) fifo_i (
    .clk      (clk), 
    .reset    (rst), 
    .clear    (1'b0),
    .i_tdata  ({m_chdr.axis.tlast, m_chdr.axis.tdata}),
    .i_tvalid (m_chdr.axis.tvalid),
    .i_tready (m_chdr.axis.tready),
    .o_tdata  ({post_fifo.tlast, post_fifo.tdata}),
    .o_tvalid (post_fifo.tvalid),
    .o_tready (post_fifo.tready),
    .space    (),
    .occupied ()
  );

  // Attach timestamp after the packet leaves the FIFO after
  // throttling. 

  localparam [1:0] ST_HDR   = 2'd0;
  localparam [1:0] ST_TS    = 2'd1;
  localparam [1:0] ST_BODY  = 2'd2;

  reg [1:0] pkt_state = ST_HDR;
  always_ff @(posedge clk) begin
    if (rst) begin
      pkt_state <= ST_HDR;
    end else if (pre_gate.tvalid & pre_gate.tready) begin
      case (pkt_state)
        ST_HDR:
          if (~pre_gate.tlast)
            pkt_state <= pre_gate.tdata[61] ? ST_TS : ST_BODY;
        ST_TS:
          pkt_state <= pre_gate.tlast ? ST_HDR : ST_BODY;
        ST_BODY:
          pkt_state <= pre_gate.tlast ? ST_HDR : ST_BODY;
        default:
          pkt_state <= ST_HDR;
      endcase
    end
  end

  // Enforce injection rate by pulling from FIFO with a certain time probability
  always_ff @(posedge clk) begin
    throttle <= ($urandom_range(32'd99, 32'd0) > {24'h0, injection_rate});
  end

  // Insert timestamp + throttle logic
  assign pre_gate.tdata   = (pkt_state == ST_TS) ? current_time : post_fifo.tdata;
  assign pre_gate.tlast   = post_fifo.tlast;
  assign pre_gate.tvalid  = post_fifo.tvalid & ~throttle;
  assign post_fifo.tready = pre_gate.tready & ~throttle;

  // Gate the packet to smooth out throttle-related noise.
  // This also serves as a buffer for the packet in case things are backed up
  axi_packet_gate #(
    .WIDTH(WIDTH), .SIZE(MTU + 4), .USE_AS_BUFF(1)
  ) pkt_gate_i (
    .clk      (clk), 
    .reset    (rst), 
    .clear    (1'b0),
    .i_tdata  (pre_gate.tdata),
    .i_tlast  (pre_gate.tlast),
    .i_terror (1'b0),
    .i_tvalid (pre_gate.tvalid),
    .i_tready (pre_gate.tready),
    .o_tdata  (m_axis_tdata),
    .o_tlast  (m_axis_tlast),
    .o_tvalid (m_axis_tvalid),
    .o_tready (m_axis_tready)
  );

endmodule