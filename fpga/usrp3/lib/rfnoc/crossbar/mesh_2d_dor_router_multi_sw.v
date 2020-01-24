//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: mesh_2d_dor_router_multi_sw
// Description:
//   Alternate implementation for mesh_2d_dor_router_single_sw with
//   multiple switches for independent paths between inputs and outputs
//   **NOTE**: This module has not been validated

module mesh_2d_dor_router_multi_sw #(
  parameter                        WIDTH          = 64,
  parameter                        DIM_SIZE       = 4,
  parameter [$clog2(DIM_SIZE)-1:0] XB_ADDR_X      = 0,
  parameter [$clog2(DIM_SIZE)-1:0] XB_ADDR_Y      = 0,
  parameter                        TERM_BUFF_SIZE = 5,
  parameter                        XB_BUFF_SIZE   = 5,
  parameter                        ROUTING_ALLOC  = "WORMHOLE",  // Routing (switching) method {WORMHOLE, CUT-THROUGH}
  parameter                        SWITCH_ALLOC   = "PRIO"       // Switch allocation algorithm {ROUND-ROBIN, PRIO}
) (
  // Clocks and resets
  input  wire             clk,
  input  wire             reset,

  // Terminal connections
  input  wire [WIDTH-1:0] s_axis_ter_tdata,
  input  wire             s_axis_ter_tlast,
  input  wire             s_axis_ter_tvalid,
  output wire             s_axis_ter_tready,
  output wire [WIDTH-1:0] m_axis_ter_tdata,
  output wire             m_axis_ter_tlast,
  output wire             m_axis_ter_tvalid,
  input  wire             m_axis_ter_tready,

  // West inter-router connections
  input  wire [WIDTH-1:0] s_axis_wst_tdata,
  input  wire [0:0]       s_axis_wst_tdest,
  input  wire             s_axis_wst_tlast,
  input  wire             s_axis_wst_tvalid,
  output wire             s_axis_wst_tready,
  output wire [WIDTH-1:0] m_axis_wst_tdata,
  output wire [0:0]       m_axis_wst_tdest,
  output wire             m_axis_wst_tlast,
  output wire             m_axis_wst_tvalid,
  input  wire             m_axis_wst_tready,

  // East inter-router connections
  input  wire [WIDTH-1:0] s_axis_est_tdata,
  input  wire [0:0]       s_axis_est_tdest,
  input  wire             s_axis_est_tlast,
  input  wire             s_axis_est_tvalid,
  output wire             s_axis_est_tready,
  output wire [WIDTH-1:0] m_axis_est_tdata,
  output wire [0:0]       m_axis_est_tdest,
  output wire             m_axis_est_tlast,
  output wire             m_axis_est_tvalid,
  input  wire             m_axis_est_tready,

  // North inter-router connections
  input  wire [WIDTH-1:0] s_axis_nor_tdata,
  input  wire [0:0]       s_axis_nor_tdest,
  input  wire             s_axis_nor_tlast,
  input  wire             s_axis_nor_tvalid,
  output wire             s_axis_nor_tready,
  output wire [WIDTH-1:0] m_axis_nor_tdata,
  output wire [0:0]       m_axis_nor_tdest,
  output wire             m_axis_nor_tlast,
  output wire             m_axis_nor_tvalid,
  input  wire             m_axis_nor_tready,

  // South inter-router connections
  input  wire [WIDTH-1:0] s_axis_sou_tdata,
  input  wire [0:0]       s_axis_sou_tdest,
  input  wire             s_axis_sou_tlast,
  input  wire             s_axis_sou_tvalid,
  output wire             s_axis_sou_tready,
  output wire [WIDTH-1:0] m_axis_sou_tdata,
  output wire [0:0]       m_axis_sou_tdest,
  output wire             m_axis_sou_tlast,
  output wire             m_axis_sou_tvalid,
  input  wire             m_axis_sou_tready
);
  // -------------------------------------------------
  // Routing functions
  // -------------------------------------------------
  `include "mesh_node_mapping.vh"

  function [2:0] term_route;
    input [WIDTH-1:0] header;
    reg [$clog2(DIM_SIZE)-1:0] xdst, ydst;
    reg signed [$clog2(DIM_SIZE):0] xdiff, ydiff;
  begin
    xdst  = node_to_xdst(header);
    ydst  = node_to_ydst(header);
    xdiff = xdst - XB_ADDR_X;
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    if (xdst == XB_ADDR_X && ydst == XB_ADDR_Y) begin
      term_route = 3'd0; //TER
    end else if (xdst == XB_ADDR_X) begin
      if (ydiff < 0)
        term_route = 3'd3; //NOR
      else
        term_route = 3'd4; //SOU
    end else begin
      if (xdiff < 0)
        term_route = 3'd1; //WST
      else
        term_route = 3'd2; //EST
    end
  end
  endfunction

  function [1:0] xdim_route;
    input [WIDTH-1:0] header;
    reg [$clog2(DIM_SIZE)-1:0] xdst, ydst;
    reg signed [$clog2(DIM_SIZE):0] xdiff, ydiff;
  begin
    xdst  = node_to_xdst(header);
    ydst  = node_to_ydst(header);
    xdiff = xdst - XB_ADDR_X;
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    if (xdst == XB_ADDR_X && ydst == XB_ADDR_Y) begin
      xdim_route = 2'd0; //TER
    end else if (xdst == XB_ADDR_X) begin
      if (ydiff < 0)
        xdim_route = 2'd2; //NOR
      else
        xdim_route = 2'd3; //SOU
    end else begin
      xdim_route = 2'd1; //Forward
    end
  end
  endfunction

  function [0:0] ydim_route;
    input [WIDTH-1:0] header;
    reg [$clog2(DIM_SIZE)-1:0] xdst, ydst;
    reg signed [$clog2(DIM_SIZE):0] xdiff, ydiff;
  begin
    xdst  = node_to_xdst(header);
    ydst  = node_to_ydst(header);
    xdiff = xdst - XB_ADDR_X;
    ydiff = ydst - XB_ADDR_Y;
    // Routing logic
    if (xdst == XB_ADDR_X && ydst == XB_ADDR_Y) begin
      ydim_route = 1'd0; //TER
    end else if (xdst == XB_ADDR_X) begin
      ydim_route = 1'd1; //Forward
    end
  end
  endfunction


  // -------------------------------------------------
  // Input buffers
  // -------------------------------------------------
  wire [WIDTH-1:0] ter_i_tdata;
  wire             ter_i_tlast;
  wire             ter_i_tvalid;
  wire             ter_i_tready;

  axi_packet_gate #(
    .WIDTH(WIDTH), .SIZE(TERM_BUFF_SIZE)
  ) term_in_pkt_gate_i (
    .clk      (clk), 
    .reset    (reset), 
    .clear    (1'b0),
    .i_tdata  (s_axis_ter_tdata),
    .i_tlast  (s_axis_ter_tlast),
    .i_tvalid (s_axis_ter_tvalid),
    .i_tready (s_axis_ter_tready),
    .i_terror (1'b0),
    .o_tdata  (ter_i_tdata),
    .o_tlast  (ter_i_tlast),
    .o_tvalid (ter_i_tvalid),
    .o_tready (ter_i_tready)
  );

  wire [WIDTH-1:0] wst_i_tdata,  est_i_tdata,  nor_i_tdata,  sou_i_tdata;
  wire             wst_i_tlast,  est_i_tlast,  nor_i_tlast,  sou_i_tlast;
  wire             wst_i_tvalid, est_i_tvalid, nor_i_tvalid, sou_i_tvalid;
  wire             wst_i_tready, est_i_tready, nor_i_tready, sou_i_tready;

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(1),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) wst_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_wst_tdata),
    .s_axis_tdest  (s_axis_wst_tdest),
    .s_axis_tlast  (s_axis_wst_tlast),
    .s_axis_tvalid (s_axis_wst_tvalid),
    .s_axis_tready (s_axis_wst_tready),
    .m_axis_tdata  (wst_i_tdata),
    .m_axis_tlast  (wst_i_tlast),
    .m_axis_tvalid (wst_i_tvalid),
    .m_axis_tready (wst_i_tready)
  );

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(1),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) est_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_est_tdata),
    .s_axis_tdest  (s_axis_est_tdest),
    .s_axis_tlast  (s_axis_est_tlast),
    .s_axis_tvalid (s_axis_est_tvalid),
    .s_axis_tready (s_axis_est_tready),
    .m_axis_tdata  (est_i_tdata),
    .m_axis_tlast  (est_i_tlast),
    .m_axis_tvalid (est_i_tvalid),
    .m_axis_tready (est_i_tready)
  );

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(1),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) nor_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_nor_tdata),
    .s_axis_tdest  (s_axis_nor_tdest),
    .s_axis_tlast  (s_axis_nor_tlast),
    .s_axis_tvalid (s_axis_nor_tvalid),
    .s_axis_tready (s_axis_nor_tready),
    .m_axis_tdata  (nor_i_tdata),
    .m_axis_tlast  (nor_i_tlast),
    .m_axis_tvalid (nor_i_tvalid),
    .m_axis_tready (nor_i_tready)
  );

  axis_ingress_vc_buff #(
    .WIDTH(WIDTH), .NUM_VCS(1),
    .SIZE(XB_BUFF_SIZE),
    .ROUTING(ROUTING_ALLOC)
  ) sou_in_vc_buf_i (
    .clk           (clk), 
    .reset         (reset), 
    .s_axis_tdata  (s_axis_sou_tdata),
    .s_axis_tdest  (s_axis_sou_tdest),
    .s_axis_tlast  (s_axis_sou_tlast),
    .s_axis_tvalid (s_axis_sou_tvalid),
    .s_axis_tready (s_axis_sou_tready),
    .m_axis_tdata  (sou_i_tdata),
    .m_axis_tlast  (sou_i_tlast),
    .m_axis_tvalid (sou_i_tvalid),
    .m_axis_tready (sou_i_tready)
  );

  // -------------------------------------------------
  // Input demuxes
  // -------------------------------------------------

  wire [WIDTH-1:0] t2t_tdata,  t2w_tdata,  t2e_tdata,  t2n_tdata,  t2s_tdata;
  wire             t2t_tlast,  t2w_tlast,  t2e_tlast,  t2n_tlast,  t2s_tlast;
  wire             t2t_tvalid, t2w_tvalid, t2e_tvalid, t2n_tvalid, t2s_tvalid;
  wire             t2t_tready, t2w_tready, t2e_tready, t2n_tready, t2s_tready;

  wire [WIDTH-1:0] w2t_tdata,  w2e_tdata,  w2n_tdata,  w2s_tdata;
  wire             w2t_tlast,  w2e_tlast,  w2n_tlast,  w2s_tlast;
  wire             w2t_tvalid, w2e_tvalid, w2n_tvalid, w2s_tvalid;
  wire             w2t_tready, w2e_tready, w2n_tready, w2s_tready;

  wire [WIDTH-1:0] e2t_tdata,  e2w_tdata,  e2n_tdata,  e2s_tdata;
  wire             e2t_tlast,  e2w_tlast,  e2n_tlast,  e2s_tlast;
  wire             e2t_tvalid, e2w_tvalid, e2n_tvalid, e2s_tvalid;
  wire             e2t_tready, e2w_tready, e2n_tready, e2s_tready;

  wire [WIDTH-1:0] n2t_tdata,  n2s_tdata;
  wire             n2t_tlast,  n2s_tlast;
  wire             n2t_tvalid, n2s_tvalid;
  wire             n2t_tready, n2s_tready;

  wire [WIDTH-1:0] s2t_tdata,  s2n_tdata;
  wire             s2t_tlast,  s2n_tlast;
  wire             s2t_tvalid, s2n_tvalid;
  wire             s2t_tready, s2n_tready;

  wire [WIDTH-1:0] ter_i_hdr, wst_i_hdr, est_i_hdr, nor_i_hdr, sou_i_hdr;

  axi_demux #(
    .WIDTH(WIDTH), .SIZE(5),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) ter_i_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (ter_i_hdr),
    .dest     (term_route(ter_i_hdr)),
    .i_tdata  (ter_i_tdata),
    .i_tlast  (ter_i_tlast),
    .i_tvalid (ter_i_tvalid),
    .i_tready (ter_i_tready),
    .o_tdata  ({t2s_tdata,  t2n_tdata,  t2e_tdata,  t2w_tdata,  t2t_tdata}),
    .o_tlast  ({t2s_tlast,  t2n_tlast,  t2e_tlast,  t2w_tlast,  t2t_tlast}),
    .o_tvalid ({t2s_tvalid, t2n_tvalid, t2e_tvalid, t2w_tvalid, t2t_tvalid}),
    .o_tready ({t2s_tready, t2n_tready, t2e_tready, t2w_tready, t2t_tready})
  );

  axi_demux #(
    .WIDTH(WIDTH), .SIZE(4),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) wst_i_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (wst_i_hdr),
    .dest     (xdim_route(wst_i_hdr)),
    .i_tdata  (wst_i_tdata),
    .i_tlast  (wst_i_tlast),
    .i_tvalid (wst_i_tvalid),
    .i_tready (wst_i_tready),
    .o_tdata  ({w2s_tdata,  w2n_tdata,  w2e_tdata,  w2t_tdata}),
    .o_tlast  ({w2s_tlast,  w2n_tlast,  w2e_tlast,  w2t_tlast}),
    .o_tvalid ({w2s_tvalid, w2n_tvalid, w2e_tvalid, w2t_tvalid}),
    .o_tready ({w2s_tready, w2n_tready, w2e_tready, w2t_tready})
  );

  axi_demux #(
    .WIDTH(WIDTH), .SIZE(4),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) est_i_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (est_i_hdr),
    .dest     (xdim_route(est_i_hdr)),
    .i_tdata  (est_i_tdata),
    .i_tlast  (est_i_tlast),
    .i_tvalid (est_i_tvalid),
    .i_tready (est_i_tready),
    .o_tdata  ({e2s_tdata,  e2n_tdata,  e2w_tdata,  e2t_tdata}),
    .o_tlast  ({e2s_tlast,  e2n_tlast,  e2w_tlast,  e2t_tlast}),
    .o_tvalid ({e2s_tvalid, e2n_tvalid, e2w_tvalid, e2t_tvalid}),
    .o_tready ({e2s_tready, e2n_tready, e2w_tready, e2t_tready})
  );

  axi_demux #(
    .WIDTH(WIDTH), .SIZE(2),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) nor_i_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (nor_i_hdr),
    .dest     (ydim_route(nor_i_hdr)),
    .i_tdata  (nor_i_tdata),
    .i_tlast  (nor_i_tlast),
    .i_tvalid (nor_i_tvalid),
    .i_tready (nor_i_tready),
    .o_tdata  ({n2t_tdata,  n2s_tdata}),
    .o_tlast  ({n2t_tlast,  n2s_tlast}),
    .o_tvalid ({n2t_tvalid, n2s_tvalid}),
    .o_tready ({n2t_tready, n2s_tready})
  );

  axi_demux #(
    .WIDTH(WIDTH), .SIZE(2),
    .PRE_FIFO_SIZE(0 /* must be 0 */), .POST_FIFO_SIZE(0)
  ) sou_i_demux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .header   (sou_i_hdr),
    .dest     (ydim_route(sou_i_hdr)),
    .i_tdata  (sou_i_tdata),
    .i_tlast  (sou_i_tlast),
    .i_tvalid (sou_i_tvalid),
    .i_tready (sou_i_tready),
    .o_tdata  ({s2t_tdata,  s2n_tdata}),
    .o_tlast  ({s2t_tlast,  s2n_tlast}),
    .o_tvalid ({s2t_tvalid, s2n_tvalid}),
    .o_tready ({s2t_tready, s2n_tready})
  );

  // -------------------------------------------------
  // Output muxes
  // -------------------------------------------------

  axi_mux #(
    .WIDTH(WIDTH), .SIZE(5),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) ter_o_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2t_tdata,  w2t_tdata,  e2t_tdata,  n2t_tdata,  s2t_tdata}),
    .i_tlast  ({t2t_tlast,  w2t_tlast,  e2t_tlast,  n2t_tlast,  s2t_tlast}),
    .i_tvalid ({t2t_tvalid, w2t_tvalid, e2t_tvalid, n2t_tvalid, s2t_tvalid}),
    .i_tready ({t2t_tready, w2t_tready, e2t_tready, n2t_tready, s2t_tready}),
    .o_tdata  (m_axis_ter_tdata),
    .o_tlast  (m_axis_ter_tlast),
    .o_tvalid (m_axis_ter_tvalid),
    .o_tready (m_axis_ter_tready)
  );

  axi_mux #(
    .WIDTH(WIDTH), .SIZE(2),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) wst_o_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2w_tdata,  e2w_tdata}),
    .i_tlast  ({t2w_tlast,  e2w_tlast}),
    .i_tvalid ({t2w_tvalid, e2w_tvalid}),
    .i_tready ({t2w_tready, e2w_tready}),
    .o_tdata  (m_axis_wst_tdata),
    .o_tlast  (m_axis_wst_tlast),
    .o_tvalid (m_axis_wst_tvalid),
    .o_tready (m_axis_wst_tready)
  );
  assign m_axis_wst_tdest = 1'b0;

  axi_mux #(
    .WIDTH(WIDTH), .SIZE(2),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) est_o_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2e_tdata,  w2e_tdata}),
    .i_tlast  ({t2e_tlast,  w2e_tlast}),
    .i_tvalid ({t2e_tvalid, w2e_tvalid}),
    .i_tready ({t2e_tready, w2e_tready}),
    .o_tdata  (m_axis_est_tdata),
    .o_tlast  (m_axis_est_tlast),
    .o_tvalid (m_axis_est_tvalid),
    
    
    .o_tready (m_axis_est_tready)
  );
  assign m_axis_est_tdest = 1'b0;

  axi_mux #(
    .WIDTH(WIDTH), .SIZE(4),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) nor_o_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2n_tdata,  w2n_tdata,  e2n_tdata,  s2n_tdata}),
    .i_tlast  ({t2n_tlast,  w2n_tlast,  e2n_tlast,  s2n_tlast}),
    .i_tvalid ({t2n_tvalid, w2n_tvalid, e2n_tvalid, s2n_tvalid}),
    .i_tready ({t2n_tready, w2n_tready, e2n_tready, s2n_tready}),
    .o_tdata  (m_axis_nor_tdata),
    .o_tlast  (m_axis_nor_tlast),
    .o_tvalid (m_axis_nor_tvalid),
    .o_tready (m_axis_nor_tready)
  );
  assign m_axis_nor_tdest = 1'b0;

  axi_mux #(
    .WIDTH(WIDTH), .SIZE(4),
    .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) sou_o_mux_i (
    .clk      (clk),
    .reset    (reset),
    .clear    (1'b0),
    .i_tdata  ({t2s_tdata,  w2s_tdata,  e2s_tdata,  n2s_tdata}),
    .i_tlast  ({t2s_tlast,  w2s_tlast,  e2s_tlast,  n2s_tlast}),
    .i_tvalid ({t2s_tvalid, w2s_tvalid, e2s_tvalid, n2s_tvalid}),
    .i_tready ({t2s_tready, w2s_tready, e2s_tready, n2s_tready}),
    .o_tdata  (m_axis_sou_tdata),
    .o_tlast  (m_axis_sou_tlast),
    .o_tvalid (m_axis_sou_tvalid),
    .o_tready (m_axis_sou_tready)
  );
  assign m_axis_sou_tdest = 1'b0;

endmodule

