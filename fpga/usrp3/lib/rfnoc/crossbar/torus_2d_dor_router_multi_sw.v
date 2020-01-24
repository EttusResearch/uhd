//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: torus_2d_dor_router_multi_sw
// Description:
//   Alternate implementation for torus_2d_dor_router_single_sw with
//   multiple switches for independent paths between inputs and outputs
//   **NOTE**: This module has not been validated

module torus_2d_dor_router_multi_sw #(
  parameter                         WIDTH          = 64,
  parameter                         DIM_SIZE       = 4,
  parameter [$clog2(DIM_SIZE)-1:0]  XB_ADDR_X      = 0,
  parameter [$clog2(DIM_SIZE)-1:0]  XB_ADDR_Y      = 0,
  parameter                         TERM_BUFF_SIZE = 5,
  parameter                         XB_BUFF_SIZE   = 5,
  parameter                         ROUTING_ALLOC  = "WORMHOLE"
) (
  // Clocks and resets
  input  wire             clk,
  input  wire             reset,

  // Terminal connections
  input  wire [WIDTH-1:0] s_axis_term_tdata,
  input  wire             s_axis_term_tlast,
  input  wire             s_axis_term_tvalid,
  output wire             s_axis_term_tready,
  output wire [WIDTH-1:0] m_axis_term_tdata,
  output wire             m_axis_term_tlast,
  output wire             m_axis_term_tvalid,
  input  wire             m_axis_term_tready,

  // X-dimension inter-XB connections
  input  wire [WIDTH-1:0] s_axis_xdim_tdata,
  input  wire [0:0]       s_axis_xdim_tdest,
  input  wire             s_axis_xdim_tlast,
  input  wire             s_axis_xdim_tvalid,
  output wire             s_axis_xdim_tready,
  output wire [WIDTH-1:0] m_axis_xdim_tdata,
  output wire [0:0]       m_axis_xdim_tdest,
  output wire             m_axis_xdim_tlast,
  output wire             m_axis_xdim_tvalid,
  input  wire             m_axis_xdim_tready,

  // Y-dimension inter-XB connections
  input  wire [WIDTH-1:0] s_axis_ydim_tdata,
  input  wire [0:0]       s_axis_ydim_tdest,
  input  wire             s_axis_ydim_tlast,
  input  wire             s_axis_ydim_tvalid,
  output wire             s_axis_ydim_tready,
  output wire [WIDTH-1:0] m_axis_ydim_tdata,
  output wire [0:0]       m_axis_ydim_tdest,
  output wire             m_axis_ydim_tlast,
  output wire             m_axis_ydim_tvalid,
  input  wire             m_axis_ydim_tready
);

  // -------------------------------------------------
  // Routing functions
  // -------------------------------------------------
  `include "mesh_node_mapping.vh"

  function [2:0] term_in_route;
    input [WIDTH:0] header;
    reg [$clog2(DIM_SIZE)-1:0] xdst, ydst, xdiff, ydiff;
  begin
    xdst  = node_to_xdst(header);
    ydst  = node_to_ydst(header);
    xdiff = xdst - XB_ADDR_X;
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    // - MSB is the VC, 2 LSBs are the router destination
    // - Long journeys get VC = 1 to bypass local traffic
    if (xdst == XB_ADDR_X && ydst == XB_ADDR_Y) begin
      term_in_route = {1'b0 /* VC don't care */, 2'd2 /* term out */};
    end else if (xdst == XB_ADDR_X) begin
      term_in_route = {ydiff[$clog2(DIM_SIZE)-1], 2'd0 /* ydim out */};
    end else begin
      term_in_route = {xdiff[$clog2(DIM_SIZE)-1], 2'd1 /* xdim out */};
    end
  end
  endfunction

  function [2:0] xdim_in_route;
    input [WIDTH:0] header;
    reg [$clog2(DIM_SIZE)-1:0] xdst, ydst, xdiff, ydiff;
  begin
    xdst  = node_to_xdst(header);
    ydst  = node_to_ydst(header);
    xdiff = xdst - XB_ADDR_X;
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    // - MSB is the VC, 2 LSBs are the router destination
    // - Long journeys get VC = 1 to bypass local traffic
    if (xdst == XB_ADDR_X && ydst == XB_ADDR_Y) begin
      xdim_in_route = {1'b0 /* VC don't care */, 2'd2 /* term out */};
    end else if (xdst == XB_ADDR_X) begin
      xdim_in_route = {ydiff[$clog2(DIM_SIZE)-1], 2'd0 /* ydim out */};
    end else begin
      xdim_in_route = {xdiff[$clog2(DIM_SIZE)-1], 2'd1 /* xdim out */};
    end
  end
  endfunction

  function [1:0] ydim_in_route;
    input [WIDTH:0] header;
    reg [$clog2(DIM_SIZE)-1:0] ydst, ydiff;
  begin
    ydst  = node_to_ydst(header);
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    // - MSB is the VC, LSB is the router destination
    // - Long journeys get VC = 1 to bypass local traffic
    if (ydst == XB_ADDR_Y) begin
      ydim_in_route = {1'b0 /* VC don't care */, 1'd1 /* term out */};
    end else begin
      ydim_in_route = {ydiff[$clog2(DIM_SIZE)-1], 1'd0 /* ydim out */};
    end
  end
  endfunction

  // -------------------------------------------------
  // Input demuxes
  // -------------------------------------------------
  wire [WIDTH-1:0] ti_gt_tdata;
  wire             ti_gt_tdest; 
  wire             ti_gt_tlast; 
  wire             ti_gt_tvalid;
  wire             ti_gt_tready;
  wire [WIDTH-1:0] t2t_tdata,  t2x_tdata,  t2y_tdata;
  wire             t2t_tdest,  t2x_tdest,  t2y_tdest;
  wire             t2t_tlast,  t2x_tlast,  t2y_tlast;
  wire             t2t_tvalid, t2x_tvalid, t2y_tvalid;
  wire             t2t_tready, t2x_tready, t2y_tready;
  wire [WIDTH-1:0] term_in_hdr;
  wire [1:0]       term_in_port;

  assign {ti_gt_tdest, term_in_port} = term_in_route(term_in_hdr);

  axi_packet_gate #(
    .WIDTH(WIDTH), .SIZE(TERM_BUFF_SIZE)
  ) term_in_pkt_gate_i (
    .clk      (clk), 
    .reset    (reset), 
    .clear    (1'b0),
    .i_tdata  (s_axis_term_tdata),
    .i_tlast  (s_axis_term_tlast),
    .i_tvalid (s_axis_term_tvalid),
    .i_tready (s_axis_term_tready),
    .i_terror (1'b0),
    .o_tdata  (ti_gt_tdata),
    .o_tlast  (ti_gt_tlast),
    .o_tvalid (ti_gt_tvalid),
    .o_tready (ti_gt_tready)
  );

  axi_demux #(
    .WIDTH(WIDTH+1), .SIZE(3),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) term_in_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (term_in_hdr),
    .dest     (term_in_port),
    .i_tdata  ({ti_gt_tdest, ti_gt_tdata}),
    .i_tlast  (ti_gt_tlast),
    .i_tvalid (ti_gt_tvalid),
    .i_tready (ti_gt_tready),
    .o_tdata  ({t2t_tdest, t2t_tdata, t2x_tdest, t2x_tdata, t2y_tdest, t2y_tdata}),
    .o_tlast  ({t2t_tlast,  t2x_tlast,  t2y_tlast}),
    .o_tvalid ({t2t_tvalid, t2x_tvalid, t2y_tvalid}),
    .o_tready ({t2t_tready, t2x_tready, t2y_tready})
  );

  wire [WIDTH-1:0] xi_gt_tdata;
  wire             xi_gt_tdest; 
  wire             xi_gt_tlast; 
  wire             xi_gt_tvalid;
  wire             xi_gt_tready;
  wire [WIDTH-1:0] x2t_tdata,  x2x_tdata,  x2y_tdata;
  wire             x2t_tdest,  x2x_tdest,  x2y_tdest;
  wire             x2t_tlast,  x2x_tlast,  x2y_tlast;
  wire             x2t_tvalid, x2x_tvalid, x2y_tvalid;
  wire             x2t_tready, x2x_tready, x2y_tready;
  wire [WIDTH-1:0] xdim_in_hdr;
  wire [1:0]       xdim_in_port;

  assign {xi_gt_tdest, xdim_in_port} = xdim_in_route(xdim_in_hdr);

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(2),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) xdim_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_xdim_tdata),
    .s_axis_tdest  (s_axis_xdim_tdest),
    .s_axis_tlast  (s_axis_xdim_tlast),
    .s_axis_tvalid (s_axis_xdim_tvalid),
    .s_axis_tready (s_axis_xdim_tready),
    .m_axis_tdata  (xi_gt_tdata),
    .m_axis_tlast  (xi_gt_tlast),
    .m_axis_tvalid (xi_gt_tvalid),
    .m_axis_tready (xi_gt_tready)
  );

  axi_demux #(
    .WIDTH(WIDTH+1), .SIZE(3),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) xdim_in_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (xdim_in_hdr),
    .dest     (xdim_in_port),
    .i_tdata  ({xi_gt_tdest, xi_gt_tdata}),
    .i_tlast  (xi_gt_tlast),
    .i_tvalid (xi_gt_tvalid),
    .i_tready (xi_gt_tready),
    .o_tdata  ({x2t_tdest, x2t_tdata, x2x_tdest, x2x_tdata, x2y_tdest, x2y_tdata}),
    .o_tlast  ({x2t_tlast, x2x_tlast, x2y_tlast}),
    .o_tvalid ({x2t_tvalid, x2x_tvalid, x2y_tvalid}),
    .o_tready ({x2t_tready, x2x_tready, x2y_tready})
  );

  wire [WIDTH-1:0] yi_gt_tdata;
  wire             yi_gt_tdest; 
  wire             yi_gt_tlast; 
  wire             yi_gt_tvalid;
  wire             yi_gt_tready;
  wire [WIDTH-1:0] y2t_tdata,  y2y_tdata;
  wire             y2t_tdest,  y2y_tdest;
  wire             y2t_tlast,  y2y_tlast;
  wire             y2t_tvalid, y2y_tvalid;
  wire             y2t_tready, y2y_tready;
  wire [WIDTH-1:0] ydim_in_hdr;
  wire [0:0]       ydim_in_port;

  assign {yi_gt_tdest, ydim_in_port} = ydim_in_route(ydim_in_hdr);

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(2),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) ydim_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_ydim_tdata ),
    .s_axis_tdest  (s_axis_ydim_tdest ),
    .s_axis_tlast  (s_axis_ydim_tlast ),
    .s_axis_tvalid (s_axis_ydim_tvalid),
    .s_axis_tready (s_axis_ydim_tready),
    .m_axis_tdata  (yi_gt_tdata ),
    .m_axis_tlast  (yi_gt_tlast ),
    .m_axis_tvalid (yi_gt_tvalid),
    .m_axis_tready (yi_gt_tready)
  );

  axi_demux #(
    .WIDTH(WIDTH+1), .SIZE(2),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) ydim_in_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (ydim_in_hdr),
    .dest     (ydim_in_port),
    .i_tdata  ({yi_gt_tdest, yi_gt_tdata}),
    .i_tlast  (yi_gt_tlast),
    .i_tvalid (yi_gt_tvalid),
    .i_tready (yi_gt_tready),
    .o_tdata  ({y2t_tdest, y2t_tdata, y2y_tdest, y2y_tdata}),
    .o_tlast  ({y2t_tlast,  y2y_tlast}),
    .o_tvalid ({y2t_tvalid, y2y_tvalid}),
    .o_tready ({y2t_tready, y2y_tready})
  );

  // -------------------------------------------------
  // Output muxes
  // -------------------------------------------------
  wire term_tdest_discard;
  axi_mux #(
    .WIDTH(WIDTH+1), .SIZE(3),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) term_out_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2t_tdest, t2t_tdata, x2t_tdest, x2t_tdata, y2t_tdest, y2t_tdata}),
    .i_tlast  ({t2t_tlast, x2t_tlast, y2t_tlast }),
    .i_tvalid ({t2t_tvalid, x2t_tvalid, y2t_tvalid}),
    .i_tready ({t2t_tready, x2t_tready, y2t_tready}),
    .o_tdata  ({term_tdest_discard, m_axis_term_tdata}),
    .o_tlast  (m_axis_term_tlast),
    .o_tvalid (m_axis_term_tvalid),
    .o_tready (m_axis_term_tready)
  );

  axi_mux #(
    .WIDTH(WIDTH+1), .SIZE(2),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) xdim_out_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2x_tdest, t2x_tdata, x2x_tdest, x2x_tdata}),
    .i_tlast  ({t2x_tlast,  x2x_tlast}),
    .i_tvalid ({t2x_tvalid, x2x_tvalid}),
    .i_tready ({t2x_tready, x2x_tready}),
    .o_tdata  ({m_axis_xdim_tdest, m_axis_xdim_tdata}),
    .o_tlast  (m_axis_xdim_tlast ),
    .o_tvalid (m_axis_xdim_tvalid),
    .o_tready (m_axis_xdim_tready)
  );

  axi_mux #(
    .WIDTH(WIDTH+1), .SIZE(3),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) ydim_out_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2y_tdest, t2y_tdata, x2y_tdest, x2y_tdata, y2y_tdest, y2y_tdata}),
    .i_tlast  ({t2y_tlast, x2y_tlast, y2y_tlast }),
    .i_tvalid ({t2y_tvalid, x2y_tvalid, y2y_tvalid}),
    .i_tready ({t2y_tready, x2y_tready, y2y_tready}),
    .o_tdata  ({m_axis_ydim_tdest, m_axis_ydim_tdata}),
    .o_tlast  (m_axis_ydim_tlast),
    .o_tvalid (m_axis_ydim_tvalid),
    .o_tready (m_axis_ydim_tready)
  );

endmodule

