//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_packet_flush
// Description:
//   When this module is inserted in an AXI-Stream link, it allows
//   the client to flip a bit to make the stream lossy. When enable=1
//   all data coming through the input is dropped. This module can
//   start and stop flushing at packet boundaries to ensure no partial
//   packets are introduces into the stream. Set FLUSH_PARTIAL_PKTS = 1
//   to disable that behavior. An optional timeout can be set to 
//   determine if flushing was done (without turning it off).
//
// Parameters:
//   - WIDTH: The bitwidth of the AXI-Stream bus
//   - TIMEOUT_W: Width of the timeout counter
//   - FLUSH_PARTIAL_PKTS: Start flusing immediately even if a packet is in flight
//   - PIPELINE: Which ports to pipeline? {NONE, IN, OUT, INOUT}
//
// Signals:
//   - s_axis_*  : Input AXI-Stream
//   - m_axis_*  : Output AXI-Stream
//   - enable    : Enable flush mode
//   - timeout   : Flush timeout (# of cycles of inactivity until done)
//   - flushing  : The module is currently flushing
//   - done      : Finished flushing (but is still active)

module axis_packet_flush #(
  parameter WIDTH               = 64,
  parameter TIMEOUT_W           = 32,
  parameter FLUSH_PARTIAL_PKTS  = 0,
  parameter PIPELINE            = "NONE"
)(
  // Clock and reset
  input  wire                 clk,
  input  wire                 reset,
  // Control and status       
  input  wire                 enable,
  input  wire [TIMEOUT_W-1:0] timeout,
  output wire                 flushing,
  output reg                  done = 1'b0,
  // Input stream             
  input  wire [WIDTH-1:0]     s_axis_tdata,
  input  wire                 s_axis_tlast,
  input  wire                 s_axis_tvalid,
  output wire                 s_axis_tready,
  // Output stream            
  output wire [WIDTH-1:0]     m_axis_tdata,
  output wire                 m_axis_tlast,
  output wire                 m_axis_tvalid,
  input  wire                 m_axis_tready
);

  //----------------------------------------------
  // Pipeline Logic
  //----------------------------------------------

  wire [WIDTH-1:0] i_pipe_tdata,  o_pipe_tdata;
  wire             i_pipe_tlast,  o_pipe_tlast;
  wire             i_pipe_tvalid, o_pipe_tvalid;
  wire             i_pipe_tready, o_pipe_tready;

  generate
    if (PIPELINE == "IN" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH(WIDTH+1)) in_pipe_i (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({s_axis_tlast, s_axis_tdata}), .i_tvalid(s_axis_tvalid), .i_tready(s_axis_tready),
        .o_tdata({i_pipe_tlast, i_pipe_tdata}), .o_tvalid(i_pipe_tvalid), .o_tready(i_pipe_tready),
        .space(), .occupied()
      );
    end else begin
      assign {i_pipe_tlast, i_pipe_tdata, i_pipe_tvalid} = {s_axis_tlast, s_axis_tdata, s_axis_tvalid}; 
      assign s_axis_tready = i_pipe_tready;
    end

    if (PIPELINE == "OUT" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH(WIDTH+1)) out_pipe_i (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({o_pipe_tlast, o_pipe_tdata}), .i_tvalid(o_pipe_tvalid), .i_tready(o_pipe_tready),
        .o_tdata({m_axis_tlast, m_axis_tdata}), .o_tvalid(m_axis_tvalid), .o_tready(m_axis_tready),
        .space(), .occupied()
      );
    end else begin
      assign {m_axis_tlast, m_axis_tdata, m_axis_tvalid} = {o_pipe_tlast, o_pipe_tdata, o_pipe_tvalid};
      assign o_pipe_tready = m_axis_tready;
    end
  endgenerate

  //----------------------------------------------
  // Flushing Logic
  //----------------------------------------------

  // Shortcuts
  wire xfer_stb = i_pipe_tvalid & i_pipe_tready;
  wire pkt_stb  = xfer_stb & i_pipe_tlast;

  // Packet boundary detector
  reg mid_pkt = 1'b0;
  always @(posedge clk) begin
    if (reset) begin
      mid_pkt <= 1'b0;
    end else if (xfer_stb) begin
      mid_pkt <= ~pkt_stb;
    end
  end

  // Flush startup state machine
  reg active  = 1'b0;
  always @(posedge clk) begin
    if (reset) begin
      active <= 1'b0;
    end else begin
      if (enable & (pkt_stb | (~mid_pkt & ~xfer_stb))) begin
        active <= 1'b1;
      end else if (~enable) begin
        active <= 1'b0;
      end
    end
  end
  assign flushing = (FLUSH_PARTIAL_PKTS == 0) ? active : enable;

  // Flush done detector based on timeout
  reg [TIMEOUT_W-1:0] cyc_to_go = {TIMEOUT_W{1'b1}};
  wire done_tmp = (cyc_to_go == {TIMEOUT_W{1'b0}});
  always @(posedge clk) begin
    if (reset | ~enable) begin
      cyc_to_go <= {TIMEOUT_W{1'b1}};
      done <= 1'b0;
    end else if (enable & ~active) begin
      cyc_to_go <= timeout;
    end else begin
      if (~done_tmp) begin
        cyc_to_go <= xfer_stb ? timeout : (cyc_to_go - 1'b1);
      end
      done <= done_tmp;
    end
  end

  // When flushing, drop all input data and quiet output data
  // When no flushing, pass data without interruption
  assign o_pipe_tdata  = i_pipe_tdata;
  assign o_pipe_tlast  = i_pipe_tlast;
  assign o_pipe_tvalid = flushing ? 1'b0 : i_pipe_tvalid;
  assign i_pipe_tready = flushing ? 1'b1 : o_pipe_tready;

endmodule