//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_null_src_sink
//
// Description:
//
//   This block can source, sink, or loopback data. Each port is used for a
//   specific purpose. The RFNoC CHDR ports are mapped is as follows:
//
//    Input  Port 0 : Sink
//    Input  Port 1 : Loopback in
//    Output Port 0 : Source
//    Output Port 1 : Loopback out
//
// Parameters:
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   ITEM_W      : Item width
//   NIPC        : Items per Clock
//
// Signals:

module rfnoc_block_null_src_sink #(
  parameter  [9:0] THIS_PORTID = 10'd0,
  parameter        CHDR_W      = 64,
  parameter  [5:0] MTU         = 10,
  parameter        ITEM_W      = 32,
  parameter        NIPC        = CHDR_W/ITEM_W
)(
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // 2 CHDR Input Ports (from framework)
  input  wire [(CHDR_W*2)-1:0]  s_rfnoc_chdr_tdata,
  input  wire [1:0]             s_rfnoc_chdr_tlast,
  input  wire [1:0]             s_rfnoc_chdr_tvalid,
  output wire [1:0]             s_rfnoc_chdr_tready,
  // 2 CHDR Output Ports (to framework)
  output wire [(CHDR_W*2)-1:0]  m_rfnoc_chdr_tdata,
  output wire [1:0]             m_rfnoc_chdr_tlast,
  output wire [1:0]             m_rfnoc_chdr_tvalid,
  input  wire [1:0]             m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

  `include "../../core/rfnoc_chdr_utils.vh"

  localparam [19:0] REG_CTRL_STATUS       = 20'h00;
  localparam [19:0] REG_SRC_LINES_PER_PKT = 20'h04;
  localparam [19:0] REG_SRC_BYTES_PER_PKT = 20'h08;
  localparam [19:0] REG_SRC_THROTTLE_CYC  = 20'h0C;
  localparam [19:0] REG_SNK_LINE_CNT_LO   = 20'h10;
  localparam [19:0] REG_SNK_LINE_CNT_HI   = 20'h14;
  localparam [19:0] REG_SNK_PKT_CNT_LO    = 20'h18;
  localparam [19:0] REG_SNK_PKT_CNT_HI    = 20'h1C;
  localparam [19:0] REG_SRC_LINE_CNT_LO   = 20'h20;
  localparam [19:0] REG_SRC_LINE_CNT_HI   = 20'h24;
  localparam [19:0] REG_SRC_PKT_CNT_LO    = 20'h28;
  localparam [19:0] REG_SRC_PKT_CNT_HI    = 20'h2C;
  localparam [19:0] REG_LOOP_LINE_CNT_LO  = 20'h30;
  localparam [19:0] REG_LOOP_LINE_CNT_HI  = 20'h34;
  localparam [19:0] REG_LOOP_PKT_CNT_LO   = 20'h38;
  localparam [19:0] REG_LOOP_PKT_CNT_HI   = 20'h3C;

  wire                 rfnoc_chdr_rst;

  wire                 ctrlport_req_wr;
  wire                 ctrlport_req_rd;
  wire [19:0]          ctrlport_req_addr;
  wire [31:0]          ctrlport_req_data;
  reg                  ctrlport_resp_ack;
  reg  [31:0]          ctrlport_resp_data;

  wire [(ITEM_W*NIPC)-1:0] src_pyld_tdata ,                  loop_pyld_tdata ;
  wire [NIPC-1:0]          src_pyld_tkeep ,                  loop_pyld_tkeep ;
  wire                     src_pyld_tlast , snk_pyld_tlast , loop_pyld_tlast ;
  wire                     src_pyld_tvalid, snk_pyld_tvalid, loop_pyld_tvalid;
  wire                     src_pyld_tready, snk_pyld_tready, loop_pyld_tready;

  wire [CHDR_W-1:0]        src_ctxt_tdata ,                  loop_ctxt_tdata ;
  wire [3:0]               src_ctxt_tuser ,                  loop_ctxt_tuser ;
  wire                     src_ctxt_tlast ,                  loop_ctxt_tlast ;
  wire                     src_ctxt_tvalid,                  loop_ctxt_tvalid;
  wire                     src_ctxt_tready, snk_ctxt_tready, loop_ctxt_tready;

  // NoC Shell
  // ---------------------------
  noc_shell_null_src_sink #(
    .THIS_PORTID (THIS_PORTID),
    .NIPC        (NIPC),
    .ITEM_W      (ITEM_W),
    .CHDR_W      (CHDR_W),
    .MTU         (MTU)
  ) noc_shell_null_src_sink_i (
    .rfnoc_chdr_clk          (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk          (rfnoc_ctrl_clk),
    .rfnoc_chdr_rst          (rfnoc_chdr_rst),
    .rfnoc_ctrl_rst          (),
    .rfnoc_core_config       (rfnoc_core_config),
    .rfnoc_core_status       (rfnoc_core_status),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata      (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast      (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid     (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready     (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata      (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast      (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid     (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready     (m_rfnoc_ctrl_tready),
    .ctrlport_clk            (),
    .ctrlport_rst            (),
    .m_ctrlport_req_wr       (ctrlport_req_wr),
    .m_ctrlport_req_rd       (ctrlport_req_rd),
    .m_ctrlport_req_addr     (ctrlport_req_addr),
    .m_ctrlport_req_data     (ctrlport_req_data),
    .m_ctrlport_resp_ack     (ctrlport_resp_ack),
    .m_ctrlport_resp_data    (ctrlport_resp_data),
    .axis_data_clk           (),
    .axis_data_rst           (),
    .m_sink_payload_tdata    (),        // Sink data is dropped and not used
    .m_sink_payload_tkeep    (),
    .m_sink_payload_tlast    (snk_pyld_tlast),
    .m_sink_payload_tvalid   (snk_pyld_tvalid),
    .m_sink_payload_tready   (snk_pyld_tready),
    .m_sink_context_tdata    (),        // Sink context is dropped and not used
    .m_sink_context_tuser    (),
    .m_sink_context_tlast    (),
    .m_sink_context_tvalid   (),
    .m_sink_context_tready   (snk_ctxt_tready),
    .m_loop_payload_tdata    (loop_pyld_tdata),
    .m_loop_payload_tkeep    (loop_pyld_tkeep),
    .m_loop_payload_tlast    (loop_pyld_tlast),
    .m_loop_payload_tvalid   (loop_pyld_tvalid),
    .m_loop_payload_tready   (loop_pyld_tready),
    .m_loop_context_tdata    (loop_ctxt_tdata),
    .m_loop_context_tuser    (loop_ctxt_tuser),
    .m_loop_context_tlast    (loop_ctxt_tlast),
    .m_loop_context_tvalid   (loop_ctxt_tvalid),
    .m_loop_context_tready   (loop_ctxt_tready),
    .s_source_payload_tdata  (src_pyld_tdata),
    .s_source_payload_tkeep  (src_pyld_tkeep),
    .s_source_payload_tlast  (src_pyld_tlast),
    .s_source_payload_tvalid (src_pyld_tvalid),
    .s_source_payload_tready (src_pyld_tready),
    .s_source_context_tdata  (src_ctxt_tdata),
    .s_source_context_tuser  (src_ctxt_tuser),
    .s_source_context_tlast  (src_ctxt_tlast),
    .s_source_context_tvalid (src_ctxt_tvalid),
    .s_source_context_tready (src_ctxt_tready),
    .s_loop_payload_tdata    (loop_pyld_tdata),
    .s_loop_payload_tkeep    (loop_pyld_tkeep),
    .s_loop_payload_tlast    (loop_pyld_tlast),
    .s_loop_payload_tvalid   (loop_pyld_tvalid),
    .s_loop_payload_tready   (loop_pyld_tready),
    .s_loop_context_tdata    (loop_ctxt_tdata),
    .s_loop_context_tuser    (loop_ctxt_tuser),
    .s_loop_context_tlast    (loop_ctxt_tlast),
    .s_loop_context_tvalid   (loop_ctxt_tvalid),
    .s_loop_context_tready   (loop_ctxt_tready)
  );

  // Packet Counters
  // ---------------------------
  reg        reg_clear_cnts = 1'b0;
  reg [63:0] snk_line_cnt  = 64'd0, snk_pkt_cnt  = 64'd0;
  reg [63:0] src_line_cnt  = 64'd0, src_pkt_cnt  = 64'd0;
  reg [63:0] loop_line_cnt = 64'd0, loop_pkt_cnt = 64'd0;

  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst | reg_clear_cnts) begin
      snk_line_cnt  <= 64'd0;
      snk_pkt_cnt   <= 64'd0;
      src_line_cnt  <= 64'd0;
      src_pkt_cnt   <= 64'd0;
      loop_line_cnt <= 64'd0;
      loop_pkt_cnt  <= 64'd0;
    end else begin
      if (snk_pyld_tvalid & snk_pyld_tready) begin
        snk_line_cnt <= snk_line_cnt + 1;
        if (snk_pyld_tlast)
          snk_pkt_cnt <= snk_pkt_cnt + 1;
      end
      if (src_pyld_tvalid & src_pyld_tready) begin
        src_line_cnt <= src_line_cnt + 1;
        if (src_pyld_tlast)
          src_pkt_cnt <= src_pkt_cnt + 1;
      end
      if (loop_pyld_tvalid & loop_pyld_tready) begin
        loop_line_cnt <= loop_line_cnt + 1;
        if (loop_pyld_tlast)
          loop_pkt_cnt <= loop_pkt_cnt + 1;
      end
    end
  end

  // NULL Sink
  // ---------------------------
  assign snk_pyld_tready = 1'b1;
  assign snk_ctxt_tready = 1'b1;

  // NULL Source
  // ---------------------------
  reg        reg_src_en = 1'b0;
  reg        reg_src_eob = 1'b0;
  reg [11:0] reg_src_lpp = 12'd0;
  reg [15:0] reg_src_bpp = 16'd0;
  reg [9:0]  reg_throttle_cyc = 10'd0;

  localparam [1:0] ST_HDR  = 2'd0;
  localparam [1:0] ST_PYLD = 2'd1;
  localparam [1:0] ST_WAIT = 2'd2;

  reg  [1:0] state = ST_HDR;
  reg [11:0] lines_left = 12'd0;
  reg  [9:0] throttle_cntr = 10'd0;

  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      state <= ST_HDR;
    end else begin
      case (state)
        ST_HDR: begin
          if (src_ctxt_tvalid && src_ctxt_tready) begin
            state <= ST_PYLD;
            lines_left <= reg_src_lpp;
          end
        end
        ST_PYLD: begin
          if (src_pyld_tvalid && src_pyld_tready) begin
            if (src_pyld_tlast) begin
              if (reg_throttle_cyc == 10'd0) begin
                state <= ST_HDR;
                if (!reg_src_en && !reg_src_eob)
                  reg_src_eob  <=  1'b1;
                else
                  reg_src_eob  <=  1'b0;
              end else begin
                state <= ST_WAIT;
                throttle_cntr <= reg_throttle_cyc;
              end
            end else begin
              lines_left <= lines_left - 12'd1;
            end
          end
        end
        ST_WAIT: begin
          if (throttle_cntr == 10'd0) begin
            state <= ST_HDR;
            if (!reg_src_en && !reg_src_eob)
              reg_src_eob  <=  1'b1;
            else
              reg_src_eob  <=  1'b0;
          end else begin
            throttle_cntr <= throttle_cntr - 10'd1;
          end
        end
        default: begin
          state <= ST_HDR;
        end
      endcase
    end
  end

  assign src_pyld_tdata  = {NIPC{{~src_line_cnt[ITEM_W/2-1:0], src_line_cnt[ITEM_W/2-1:0]}}};
  assign src_pyld_tkeep  = {NIPC{1'b1}};
  assign src_pyld_tlast  = (lines_left == 12'd0);
  assign src_pyld_tvalid = (state == ST_PYLD);

  assign src_ctxt_tdata  = chdr_build_header(
    6'd0, reg_src_eob, 1'b0, CHDR_PKT_TYPE_DATA, CHDR_NO_MDATA, src_pkt_cnt[15:0], reg_src_bpp, 16'd0);
  assign src_ctxt_tuser  = CHDR_W > 64 ? CONTEXT_FIELD_HDR_TS : CONTEXT_FIELD_HDR;
  assign src_ctxt_tlast  = 1'b1;
  assign src_ctxt_tvalid = (state == ST_HDR && (reg_src_en || reg_src_eob));

  // Register Interface
  // ---------------------------
  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      ctrlport_resp_ack <= 1'b0;
    end else begin
      // All transactions finish in 1 cycle
      ctrlport_resp_ack <= ctrlport_req_wr | ctrlport_req_rd;
      // Handle register writes
      if (ctrlport_req_wr) begin
        case(ctrlport_req_addr)
          REG_CTRL_STATUS:
            {reg_src_en, reg_clear_cnts} <= ctrlport_req_data[1:0];
          REG_SRC_LINES_PER_PKT:
            reg_src_lpp <= ctrlport_req_data[11:0];
          REG_SRC_BYTES_PER_PKT:
            reg_src_bpp <= ctrlport_req_data[15:0];
          REG_SRC_THROTTLE_CYC:
            reg_throttle_cyc <= ctrlport_req_data[9:0];
        endcase
      end
      // Handle register reads
      if (ctrlport_req_rd) begin
        case(ctrlport_req_addr)
          REG_CTRL_STATUS:
            ctrlport_resp_data <= {NIPC[7:0],ITEM_W[7:0], state, 12'h0, reg_src_en, reg_clear_cnts};
          REG_SRC_LINES_PER_PKT:
            ctrlport_resp_data <= {20'h0, reg_src_lpp};
          REG_SRC_BYTES_PER_PKT:
            ctrlport_resp_data <= {16'h0, reg_src_bpp};
          REG_SRC_THROTTLE_CYC:
            ctrlport_resp_data <= {22'h0, reg_throttle_cyc};
          REG_SNK_LINE_CNT_LO:
            ctrlport_resp_data <= snk_line_cnt[31:0];
          REG_SNK_LINE_CNT_HI:
            ctrlport_resp_data <= snk_line_cnt[63:32];
          REG_SNK_PKT_CNT_LO:
            ctrlport_resp_data <= snk_pkt_cnt[31:0];
          REG_SNK_PKT_CNT_HI:
            ctrlport_resp_data <= snk_pkt_cnt[63:32];
          REG_SRC_LINE_CNT_LO:
            ctrlport_resp_data <= src_line_cnt[31:0];
          REG_SRC_LINE_CNT_HI:
            ctrlport_resp_data <= src_line_cnt[63:32];
          REG_SRC_PKT_CNT_LO:
            ctrlport_resp_data <= src_pkt_cnt[31:0];
          REG_SRC_PKT_CNT_HI:
            ctrlport_resp_data <= src_pkt_cnt[63:32];
          REG_LOOP_LINE_CNT_LO:
            ctrlport_resp_data <= loop_line_cnt[31:0];
          REG_LOOP_LINE_CNT_HI:
            ctrlport_resp_data <= loop_line_cnt[63:32];
          REG_LOOP_PKT_CNT_LO:
            ctrlport_resp_data <= loop_pkt_cnt[31:0];
          REG_LOOP_PKT_CNT_HI:
            ctrlport_resp_data <= loop_pkt_cnt[63:32];
          default:
            ctrlport_resp_data <= 32'h0;
        endcase
      end
    end
  end

endmodule // rfnoc_block_null_src_sink
