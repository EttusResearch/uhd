//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ingress_vc_buff
// Description:
//  A wrapper around a buffer to implement one or more virtual channels
//  Supports gate a packet for cut-through routing

module axis_ingress_vc_buff #(
  parameter WIDTH     = 64,         // Width of the datapath
  parameter NUM_VCS   = 2,          // Number of virtual channels
  parameter SIZE      = 5,          // Virtual channel buffer size
  parameter ROUTING   = "WORMHOLE", // Routing (switching) method {WORMHOLE, CUT-THROUGH}
  parameter DEST_W    = (NUM_VCS > 1) ? $clog2(NUM_VCS) : 1 // PRIVATE
) (
  input  wire               clk,
  input  wire               reset,
  input  wire [WIDTH-1:0]   s_axis_tdata,
  input  wire [DEST_W-1:0]  s_axis_tdest,
  input  wire               s_axis_tlast,
  input  wire               s_axis_tvalid,
  output wire               s_axis_tready,
  output wire [WIDTH-1:0]   m_axis_tdata,
  output wire               m_axis_tlast,
  output wire               m_axis_tvalid,
  input  wire               m_axis_tready
);

  generate if (NUM_VCS > 1) begin
    //----------------------------------------------------
    // Multiple virtual channels
    //----------------------------------------------------

    wire [(WIDTH*NUM_VCS)-1:0] bufin_tdata , bufout_tdata ;
    wire [NUM_VCS-1:0]         bufin_tlast , bufout_tlast ;
    wire [NUM_VCS-1:0]         bufin_tvalid, bufout_tvalid;
    wire [NUM_VCS-1:0]         bufin_tready, bufout_tready;
  
    axi_demux #(
      .WIDTH(WIDTH), .SIZE(NUM_VCS),
      .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
    ) vc_demux_i (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .header   (/* unused */),
      .dest     (s_axis_tdest ),
      .i_tdata  (s_axis_tdata ),
      .i_tlast  (s_axis_tlast ),
      .i_tvalid (s_axis_tvalid),
      .i_tready (s_axis_tready),
      .o_tdata  (bufin_tdata),
      .o_tlast  (bufin_tlast),
      .o_tvalid (bufin_tvalid),
      .o_tready (bufin_tready)
    );
  
    genvar vc;
    for (vc = 0; vc < NUM_VCS; vc = vc + 1) begin
      if (ROUTING == "WORMHOLE") begin
        axi_fifo #(
          .WIDTH(WIDTH+1), .SIZE(SIZE)
        ) buf_i (
          .clk      (clk), 
          .reset    (reset), 
          .clear    (1'b0),
          .i_tdata  ({bufin_tlast[vc], bufin_tdata  [(vc*WIDTH)+:WIDTH]}),
          .i_tvalid (bufin_tvalid [vc]),
          .i_tready (bufin_tready [vc]),
          .o_tdata  ({bufout_tlast[vc], bufout_tdata [(vc*WIDTH)+:WIDTH]}),
          .o_tvalid (bufout_tvalid[vc]),
          .o_tready (bufout_tready[vc]),
          .space    (),
          .occupied ()
        );
      end else begin
        axi_packet_gate #(
          .WIDTH(WIDTH), .SIZE(SIZE)
        ) buf_i (
          .clk      (clk), 
          .reset    (reset), 
          .clear    (1'b0),
          .i_tdata  (bufin_tdata[(vc*WIDTH)+:WIDTH]),
          .i_tlast  (bufin_tlast[vc]),
          .i_tvalid (bufin_tvalid[vc]),
          .i_tready (bufin_tready[vc]),
          .i_terror (1'b0),
          .o_tdata  (bufout_tdata[(vc*WIDTH)+:WIDTH]),
          .o_tlast  (bufout_tlast[vc]),
          .o_tvalid (bufout_tvalid[vc]),
          .o_tready (bufout_tready[vc])
        );
      end
    end
  
    axi_mux #(
      .WIDTH(WIDTH), .SIZE(NUM_VCS),
      .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
    ) vc_mux_i (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .i_tdata  (bufout_tdata ),
      .i_tlast  (bufout_tlast ),
      .i_tvalid (bufout_tvalid),
      .i_tready (bufout_tready),
      .o_tdata  (m_axis_tdata ),
      .o_tlast  (m_axis_tlast ),
      .o_tvalid (m_axis_tvalid),
      .o_tready (m_axis_tready)
    );

  end else begin
    //----------------------------------------------------
    // Single virtual channel
    //----------------------------------------------------
    wire [WIDTH-1:0] pipe_tdata;
    wire             pipe_tlast;
    wire             pipe_tvalid;
    wire             pipe_tready;

    if (ROUTING == "WORMHOLE") begin
      axi_fifo #(
        .WIDTH(WIDTH+1), .SIZE(SIZE)
      ) buf_i (
        .clk      (clk), 
        .reset    (reset), 
        .clear    (1'b0),
        .i_tdata  ({s_axis_tlast, s_axis_tdata}),
        .i_tvalid (s_axis_tvalid ),
        .i_tready (s_axis_tready ),
        .o_tdata  ({pipe_tlast, pipe_tdata}),
        .o_tvalid (pipe_tvalid),
        .o_tready (pipe_tready),
        .space    (),
        .occupied ()
      );
    end else begin
      axi_packet_gate #(
        .WIDTH(WIDTH), .SIZE(SIZE)
      ) buf_i (
        .clk      (clk), 
        .reset    (reset), 
        .clear    (1'b0),
        .i_tdata  (s_axis_tdata),
        .i_tlast  (s_axis_tlast),
        .i_tvalid (s_axis_tvalid),
        .i_tready (s_axis_tready),
        .i_terror (1'b0),
        .o_tdata  (pipe_tdata),
        .o_tlast  (pipe_tlast),
        .o_tvalid (pipe_tvalid),
        .o_tready (pipe_tready)
      );
    end

    axi_fifo #(
      .WIDTH(WIDTH+1), .SIZE(1)
    ) buf_i (
      .clk      (clk), 
      .reset    (reset), 
      .clear    (1'b0),
      .i_tdata  ({pipe_tlast, pipe_tdata}),
      .i_tvalid (pipe_tvalid ),
      .i_tready (pipe_tready ),
      .o_tdata  ({m_axis_tlast, m_axis_tdata}),
      .o_tvalid (m_axis_tvalid),
      .o_tready (m_axis_tready),
      .space    (),
      .occupied ()
    );

  end endgenerate

endmodule

