//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_to_axis_ctrl
// Description:
//  Converts from CHDR to AXIS-Control and vice versa.
//  This module has to handle remote control transactions
//  correctly. The CHDR frame has/needs the DstEPID, DstPort
//  SrcEPID and SrcPort and the AXIS-Ctrl frame has/needs
//  the DstPort, SrcPort, RemDstEPID and RemDstPort.
//
// Parameters:
//   - CHDR_W: Width of the CHDR bus in bits
//   - THIS_PORTID: The port number of the control xbar
//                  that this module is connected to.
//
// Signals:
//   - s_rfnoc_chdr_* : Input CHDR stream (AXI-Stream)
//   - m_rfnoc_chdr_* : Output CHDR stream (AXI-Stream)
//   - s_rfnoc_ctrl_* : Input control stream (AXI-Stream)
//   - m_rfnoc_ctrl_* : Output control stream (AXI-Stream)

module chdr_to_axis_ctrl #(
  parameter       CHDR_W      = 256,
  parameter [9:0] THIS_PORTID = 10'd0
)(
  // CHDR Bus (master and slave)
  input  wire              rfnoc_chdr_clk,
  input  wire              rfnoc_chdr_rst,
  input  wire [15:0]       this_epid,
  input  wire [CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire              s_rfnoc_chdr_tlast,
  input  wire              s_rfnoc_chdr_tvalid,
  output wire              s_rfnoc_chdr_tready,
  output wire [CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire              m_rfnoc_chdr_tlast,
  output wire              m_rfnoc_chdr_tvalid,
  input  wire              m_rfnoc_chdr_tready,
  // AXIS-Control Bus (master and slave)
  input  wire              rfnoc_ctrl_clk,
  input  wire              rfnoc_ctrl_rst,
  input  wire [31:0]       s_rfnoc_ctrl_tdata,
  input  wire              s_rfnoc_ctrl_tlast,
  input  wire              s_rfnoc_ctrl_tvalid,
  output wire              s_rfnoc_ctrl_tready,
  output wire [31:0]       m_rfnoc_ctrl_tdata,
  output wire              m_rfnoc_ctrl_tlast,
  output wire              m_rfnoc_ctrl_tvalid,
  input  wire              m_rfnoc_ctrl_tready
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  localparam [1:0] ST_CHDR_HDR   = 2'd0;  // Processing the CHDR header
  localparam [1:0] ST_CHDR_MDATA = 2'd1;  // Processing the CHDR metadata
  localparam [1:0] ST_CTRL_HDR   = 2'd2;  // Processing the CHDR control header
  localparam [1:0] ST_CTRL_BODY  = 2'd3;  // Processing the CHDR control body

  // ---------------------------------------------------
  //  Input/output register slices
  // ---------------------------------------------------
  // - ch2ct: CHDR to Ctrl
  // - ct2ch: Ctrl to CHDR

  wire [CHDR_W-1:0] ch2ct_tdata,  ct2ch_tdata;
  wire              ch2ct_tlast,  ct2ch_tlast;
  wire              ch2ct_tvalid, ct2ch_tvalid;
  wire              ch2ct_tready, ct2ch_tready;

  axi_fifo #(.WIDTH(CHDR_W+1), .SIZE(1)) ch2ct_reg_i (
    .clk(rfnoc_chdr_clk), .reset(rfnoc_chdr_rst), .clear(1'b0),
    .i_tdata({s_rfnoc_chdr_tlast, s_rfnoc_chdr_tdata}),
    .i_tvalid(s_rfnoc_chdr_tvalid), .i_tready(s_rfnoc_chdr_tready),
    .o_tdata({ch2ct_tlast, ch2ct_tdata}),
    .o_tvalid(ch2ct_tvalid), .o_tready(ch2ct_tready),
    .space(), .occupied()
  );

  axi_fifo #(.WIDTH(CHDR_W+1), .SIZE(1)) ct2ch_reg_i (
    .clk(rfnoc_chdr_clk), .reset(rfnoc_chdr_rst), .clear(1'b0),
    .i_tdata({ct2ch_tlast, ct2ch_tdata}),
    .i_tvalid(ct2ch_tvalid), .i_tready(ct2ch_tready),
    .o_tdata({m_rfnoc_chdr_tlast, m_rfnoc_chdr_tdata}),
    .o_tvalid(m_rfnoc_chdr_tvalid), .o_tready(m_rfnoc_chdr_tready),
    .space(), .occupied()
  );

  // ---------------------------------------------------
  //  CH2CT: CHDR => Ctrl path
  // ---------------------------------------------------
  // When converting CHDR => Ctrl we know we are dealing with
  // a remote control transaction so we need to perform
  // the following transformations to ensure that the packet
  // has all the info to route downstream and has enough info
  // to return to the master (of the transaction).
  // - Use the CHDR DstPort as the Ctrl DstPort (forward the master's request)
  // - Use THIS_PORTID as the Ctrl SrcPort (for the return path back here)
  // - Use the CHDR SrcEPID as the Ctrl RemDstEPID (return path for CHDR packet)
  // - Use the CHDR SrcPort as the Ctrl RemDstPort (return path in the downstream EP)
  // - Ignore the CHDR DstEPID because packet is already here

  reg [1:0]   ch2ct_state = ST_CHDR_HDR;
  reg [4:0]   ch2ct_nmdata = CHDR_NO_MDATA;

  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      ch2ct_state <= ST_CHDR_HDR;
    end else if (ch2ct_tvalid && ch2ct_tready) begin
      case (ch2ct_state)
        ST_CHDR_HDR: begin
          ch2ct_nmdata <= chdr_get_num_mdata(ch2ct_tdata[63:0]) - 5'd1;
          if (!ch2ct_tlast)
            ch2ct_state <= (chdr_get_num_mdata(ch2ct_tdata[63:0]) == 5'd0) ? 
              ST_CTRL_HDR : ST_CHDR_MDATA;
          else
            ch2ct_state <= ST_CHDR_HDR;  // Premature termination
        end
        ST_CHDR_MDATA: begin
          ch2ct_nmdata <= ch2ct_nmdata - 5'd1;
          if (!ch2ct_tlast)
            ch2ct_state <= (ch2ct_nmdata == CHDR_NO_MDATA) ? ST_CTRL_HDR : ST_CHDR_MDATA;
          else
            ch2ct_state <= ST_CHDR_HDR;  // Premature termination
        end
        ST_CTRL_HDR: begin
          ch2ct_state <= ch2ct_tlast ? ST_CHDR_HDR : ST_CTRL_BODY;
        end
        ST_CTRL_BODY: begin
          if (ch2ct_tlast)
            ch2ct_state <= ST_CHDR_HDR;
        end
        default: begin
          // We should never get here
          ch2ct_state <= ST_CHDR_HDR;
        end
      endcase
    end
  end

  wire [(CHDR_W/32)-1:0] ch2ct_tkeep;
  chdr_compute_tkeep #(.CHDR_W(CHDR_W), .ITEM_W(32)) chdr_tkeep_gen_i (
    .clk(rfnoc_chdr_clk), .rst(rfnoc_chdr_rst),
    .axis_tdata(ch2ct_tdata), .axis_tlast(ch2ct_tlast),
    .axis_tvalid(ch2ct_tvalid), .axis_tready(ch2ct_tready),
    .axis_tkeep(ch2ct_tkeep)
  );

  // Create the first two lines of the Ctrl word (wide)
  // using data from CHDR packet 
  wire [CHDR_W-1:0] ch2ct_new_ctrl_hdr;
  assign ch2ct_new_ctrl_hdr[63:0] = {
    axis_ctrl_build_hdr_hi(
      axis_ctrl_get_src_port(ch2ct_tdata[31:0]),
      axis_ctrl_get_rem_dst_epid(ch2ct_tdata[63:32])
    ),
    axis_ctrl_build_hdr_lo(
      axis_ctrl_get_is_ack  (ch2ct_tdata[31:0]),
      axis_ctrl_get_has_time(ch2ct_tdata[31:0]),
      axis_ctrl_get_seq_num (ch2ct_tdata[31:0]),
      axis_ctrl_get_num_data(ch2ct_tdata[31:0]),
      THIS_PORTID,
      axis_ctrl_get_dst_port(ch2ct_tdata[31:0])
    )
  };
  generate if (CHDR_W > 64) begin
    assign ch2ct_new_ctrl_hdr[CHDR_W-1:64] = ch2ct_tdata[CHDR_W-1:64];
  end endgenerate

  wire [CHDR_W-1:0] ch2ct_wctrl_tdata = 
    (ch2ct_state == ST_CTRL_HDR) ? ch2ct_new_ctrl_hdr : ch2ct_tdata;

  axis_width_conv #(
    .WORD_W(32), .IN_WORDS(CHDR_W/32), .OUT_WORDS(1),
    .SYNC_CLKS(0), .PIPELINE("OUT")
  ) ctrl_downsizer_i (
    .s_axis_aclk(rfnoc_chdr_clk), .s_axis_rst(rfnoc_chdr_rst),
    .s_axis_tdata(ch2ct_wctrl_tdata),
    .s_axis_tkeep(ch2ct_tkeep),
    .s_axis_tlast(ch2ct_tlast),
    .s_axis_tvalid(ch2ct_tvalid && (ch2ct_state == ST_CTRL_HDR || ch2ct_state == ST_CTRL_BODY)),
    .s_axis_tready(ch2ct_tready),
    .m_axis_aclk(rfnoc_ctrl_clk), .m_axis_rst(rfnoc_ctrl_rst),
    .m_axis_tdata(m_rfnoc_ctrl_tdata),
    .m_axis_tkeep(/* Unused: OUT_WORDS=1 */),
    .m_axis_tlast(m_rfnoc_ctrl_tlast),
    .m_axis_tvalid(m_rfnoc_ctrl_tvalid),
    .m_axis_tready(m_rfnoc_ctrl_tready)
  );

  // ---------------------------------------------------
  //  CT2CH: Ctrl => CHDR path
  // ---------------------------------------------------
  // When converting Ctrl => CHDR we know we are dealing with
  // a remote control transaction so we need to perform
  // the following transformations to ensure that the packet
  // has all the info to route downstream and has enough info
  // to return to the initiator of the transaction.
  // - Use the Ctrl RemDstEPID as the CHDR DstEPID (forward the master's request)
  // - Use the Ctrl RemDstPort as the CHDR DstPort (forward the master's request)
  // - Use the this_epid as CHDR SrcEPID (return path for the CHDR packet)
  // - Use the Ctrl SrcPort as the CHDR SrcPort (return path to the master)
  // - Ignore the Ctrl DstPort because the packet has already been routed 

  wire [CHDR_W-1:0] ct2ch_wctrl_tdata;
  wire              ct2ch_wctrl_tlast, ct2ch_wctrl_tvalid, ct2ch_wctrl_tready;

  axis_width_conv #(
    .WORD_W(32), .IN_WORDS(1), .OUT_WORDS(CHDR_W/32),
    .SYNC_CLKS(0), .PIPELINE("IN")
  ) ctrl_upsizer_i (
    .s_axis_aclk(rfnoc_ctrl_clk), .s_axis_rst(rfnoc_ctrl_rst),
    .s_axis_tdata(s_rfnoc_ctrl_tdata),
    .s_axis_tkeep(/* Unused: IN_WORDS=1 */),
    .s_axis_tlast(s_rfnoc_ctrl_tlast),
    .s_axis_tvalid(s_rfnoc_ctrl_tvalid),
    .s_axis_tready(s_rfnoc_ctrl_tready),
    .m_axis_aclk(rfnoc_chdr_clk), .m_axis_rst(rfnoc_chdr_rst),
    .m_axis_tdata(ct2ch_wctrl_tdata),
    .m_axis_tkeep(/* Unused: We are updating the CHDR length */),
    .m_axis_tlast(ct2ch_wctrl_tlast),
    .m_axis_tvalid(ct2ch_wctrl_tvalid),
    .m_axis_tready(ct2ch_wctrl_tready)
  );

  reg [1:0] ct2ch_state = ST_CHDR_HDR;
  reg [15:0] ct2ch_seqnum = 16'd0;

  always @(posedge rfnoc_chdr_clk) begin
    if (rfnoc_chdr_rst) begin
      ct2ch_state <= ST_CHDR_HDR;
      ct2ch_seqnum <= 16'd0;
    end else if (ct2ch_tvalid && ct2ch_tready) begin
      case (ct2ch_state)
        ST_CHDR_HDR: begin
          if (!ct2ch_tlast)
            ct2ch_state <= ST_CTRL_HDR;
        end
        ST_CTRL_HDR: begin
          if (ct2ch_tlast)
            ct2ch_state <= ST_CHDR_HDR;
          else
            ct2ch_state <= ST_CTRL_BODY;
        end
        ST_CTRL_BODY: begin
          if (ct2ch_tlast)
            ct2ch_state <= ST_CHDR_HDR;
        end
        default: begin
          // We should never get here
          ct2ch_state <= ST_CHDR_HDR;
        end
      endcase
      if (ct2ch_tlast)
        ct2ch_seqnum <= ct2ch_seqnum + 16'd1;
    end
  end

  // Hold the first line to generate info for the outgoing CHDR header
  assign ct2ch_wctrl_tready = (ct2ch_state == ST_CTRL_HDR || ct2ch_state == ST_CTRL_BODY) ? ct2ch_tready : 1'b0;

  wire [7:0] ct2ch_32bit_lines = 8'd3 +                               // Header + OpWord
    (axis_ctrl_get_has_time(ct2ch_wctrl_tdata[31:0]) ? 8'd2 : 8'd0) + // Timestamp
    ({4'h0, axis_ctrl_get_num_data(ct2ch_wctrl_tdata[31:0])});        // Data words

  wire [15:0] ct2ch_chdr_lines = 16'd1 +            // CHDR header
    ct2ch_32bit_lines[7:$clog2(CHDR_W/32)] +        // Convert 32-bit lines to CHDR_W
    (|ct2ch_32bit_lines[$clog2(CHDR_W/32)-1:0]);    // Residue

  reg [63:0] ct2ch_chdr_tdata;
  always @(*) begin
    case (ct2ch_state)
      ST_CHDR_HDR: begin
        ct2ch_chdr_tdata = chdr_build_header(
          6'd0, /* VC */
          1'b0, 1'b0, /* eob, eov */
          CHDR_PKT_TYPE_CTRL,
          CHDR_NO_MDATA,
          ct2ch_seqnum,
          (ct2ch_chdr_lines << $clog2(CHDR_W/8)), /* length in bytes */
          axis_ctrl_get_rem_dst_epid(ct2ch_wctrl_tdata[63:32])
        );
      end
      ST_CTRL_HDR: begin
        ct2ch_chdr_tdata = {
          axis_ctrl_build_hdr_hi(
            10'd0,        /* Unused in CHDR Control payload */
            this_epid     /* This is the SrcEPID */
          ),
          axis_ctrl_build_hdr_lo(
            axis_ctrl_get_is_ack  (ct2ch_wctrl_tdata[31:0]),
            axis_ctrl_get_has_time(ct2ch_wctrl_tdata[31:0]),
            axis_ctrl_get_seq_num (ct2ch_wctrl_tdata[31:0]),
            axis_ctrl_get_num_data(ct2ch_wctrl_tdata[31:0]),
            axis_ctrl_get_src_port(ct2ch_wctrl_tdata[31:0]),
            axis_ctrl_get_rem_dst_port(ct2ch_wctrl_tdata[63:32])
          )
        };
      end
      default: begin
        ct2ch_chdr_tdata = ct2ch_wctrl_tdata[63:0];
      end
    endcase
  end

  // Output signals
  assign ct2ch_tdata[63:0] = ct2ch_chdr_tdata;
  assign ct2ch_tlast       = ct2ch_wctrl_tlast;
  assign ct2ch_tvalid      = ct2ch_wctrl_tvalid;
  generate if (CHDR_W > 64) begin
    assign ct2ch_tdata[CHDR_W-1:64] = ct2ch_wctrl_tdata[CHDR_W-1:64];
  end endgenerate

endmodule // chdr_to_axis_ctrl
