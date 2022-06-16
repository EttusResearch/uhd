//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_endpoint_tb
//

`default_nettype none

module chdr_stream_endpoint_tb#(
  parameter TEST_NAME  = "",
  parameter CHDR_W     = 64
)(
 /* no IO */
);
  // ----------------------------------------
  // Global settings
  // ----------------------------------------

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgChdrBfm::*;

  // Clocks and resets
  bit rfnoc_chdr_clk, rfnoc_chdr_rst;
  bit rfnoc_ctrl_clk, rfnoc_ctrl_rst;
  sim_clock_gen #(.PERIOD(6.0), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (rfnoc_chdr_clk, rfnoc_chdr_rst); // 166.6 MHz
  sim_clock_gen #(.PERIOD(20.0), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (rfnoc_ctrl_clk, rfnoc_ctrl_rst); // 50 MHz

  // Parameters
  localparam bit    VERBOSE           = 0;
  localparam int    NUM_PKTS_PER_TEST = 200;
  localparam int    FAST_STALL_PROB   = 0;
  localparam int    SLOW_STALL_PROB   = 35;

  localparam int    MTU      = 7;
  localparam [15:0] PROTOVER = {8'd1, 8'd0};
  localparam [15:0] DEV_ID   = 16'hBEEF;
  localparam [15:0] EPID_TB  = 16'h1001;
  localparam [15:0] EPID_A   = 16'h1002;
  localparam [15:0] EPID_B   = 16'h1003;
  localparam [9:0]  PORT_TB  = 10'd0;
  localparam [9:0]  PORT_A   = 10'd1;
  localparam [9:0]  PORT_B   = 10'd2;

  // Create a formatted string with the test name for reporting output
  string test_name = TEST_NAME == "" ? "" : {TEST_NAME, ": "};

  typedef ChdrData #(CHDR_W)::chdr_word_t chdr_word_t;

  // ----------------------------------------
  // DUT (and Crossbar) Instantiations
  // ----------------------------------------
  wire [CHDR_W-1:0] c2ae_chdr_tdata , c2ax_chdr_tdata , a2c_chdr_tdata ;
  wire              c2ae_chdr_tlast , c2ax_chdr_tlast , a2c_chdr_tlast ;
  wire              c2ae_chdr_tvalid, c2ax_chdr_tvalid, a2c_chdr_tvalid;
  wire              c2ae_chdr_tready, c2ax_chdr_tready, a2c_chdr_tready;
  wire [CHDR_W-1:0] c2be_chdr_tdata , c2bx_chdr_tdata , b2c_chdr_tdata ;
  wire              c2be_chdr_tlast , c2bx_chdr_tlast , b2c_chdr_tlast ;
  wire              c2be_chdr_tvalid, c2bx_chdr_tvalid, b2c_chdr_tvalid;
  wire              c2be_chdr_tready, c2bx_chdr_tready, b2c_chdr_tready;

  wire [31:0] a_ctrl_in_tdata, a_ctrl_out_tdata, b_ctrl_in_tdata, b_ctrl_out_tdata;
  wire        a_ctrl_loop_tlast , b_ctrl_loop_tlast ;
  wire        a_ctrl_loop_tvalid, b_ctrl_loop_tvalid;
  wire        a_ctrl_loop_tready, b_ctrl_loop_tready;

  logic       a_signal_data_err, b_signal_data_err;
  logic       a_lossy_input, b_lossy_input;
  logic [7:0] a_seqerr_prob, b_seqerr_prob;
  logic [7:0] a_rterr_prob, b_rterr_prob;

  AxiStreamIf #(CHDR_W) m_tb_chdr (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) s_tb_chdr (rfnoc_chdr_clk, rfnoc_chdr_rst);

  AxiStreamIf #(CHDR_W) m_a0_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) s_a0_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) m_a1_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) s_a1_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) m_b0_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) s_b0_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) m_b1_data (rfnoc_chdr_clk, rfnoc_chdr_rst);
  AxiStreamIf #(CHDR_W) s_b1_data (rfnoc_chdr_clk, rfnoc_chdr_rst);

  chdr_stream_endpoint #(
    .PROTOVER           (PROTOVER),
    .CHDR_W             (CHDR_W),
    .AXIS_CTRL_EN       (1),
    .AXIS_DATA_EN       (1),
    .INST_NUM           (0),
    .NUM_DATA_I         (2),
    .NUM_DATA_O         (2),
    .CTRL_XBAR_PORT     (PORT_A),
    .INGRESS_BUFF_SIZE  (MTU+1),
    .MTU                (MTU),
    .REPORT_STRM_ERRS   (1),
    .SIM_SPEEDUP        (1)
  ) sep_a (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk                                    ),
    .rfnoc_chdr_rst     (rfnoc_chdr_rst                                    ),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk                                    ),
    .rfnoc_ctrl_rst     (rfnoc_ctrl_rst                                    ),
    .device_id          (DEV_ID                                            ),
    .s_axis_chdr_tdata  (c2ae_chdr_tdata                                   ),
    .s_axis_chdr_tlast  (c2ae_chdr_tlast                                   ),
    .s_axis_chdr_tvalid (c2ae_chdr_tvalid                                  ),
    .s_axis_chdr_tready (c2ae_chdr_tready                                  ),
    .m_axis_chdr_tdata  (a2c_chdr_tdata                                    ),
    .m_axis_chdr_tlast  (a2c_chdr_tlast                                    ),
    .m_axis_chdr_tvalid (a2c_chdr_tvalid                                   ),
    .m_axis_chdr_tready (a2c_chdr_tready                                   ),
    .s_axis_data_tdata  ({m_a1_data.tdata  , m_a0_data.tdata  }),
    .s_axis_data_tlast  ({m_a1_data.tlast  , m_a0_data.tlast  }),
    .s_axis_data_tvalid ({m_a1_data.tvalid , m_a0_data.tvalid }),
    .s_axis_data_tready ({m_a1_data.tready , m_a0_data.tready }),
    .m_axis_data_tdata  ({s_a1_data.tdata , s_a0_data.tdata }),
    .m_axis_data_tlast  ({s_a1_data.tlast , s_a0_data.tlast }),
    .m_axis_data_tvalid ({s_a1_data.tvalid, s_a0_data.tvalid}),
    .m_axis_data_tready ({s_a1_data.tready, s_a0_data.tready}),
    .s_axis_ctrl_tdata  (a_ctrl_out_tdata                                  ),
    .s_axis_ctrl_tlast  (a_ctrl_loop_tlast                                 ),
    .s_axis_ctrl_tvalid (a_ctrl_loop_tvalid                                ),
    .s_axis_ctrl_tready (a_ctrl_loop_tready                                ),
    .m_axis_ctrl_tdata  (a_ctrl_in_tdata                                   ),
    .m_axis_ctrl_tlast  (a_ctrl_loop_tlast                                 ),
    .m_axis_ctrl_tvalid (a_ctrl_loop_tvalid                                ),
    .m_axis_ctrl_tready (a_ctrl_loop_tready                                ),
    .strm_seq_err_stb   (                                                  ),
    .strm_data_err_stb  (                                                  ),
    .strm_route_err_stb (                                                  ),
    .signal_data_err    (a_signal_data_err                                 )
  );

  chdr_stream_endpoint #(
    .PROTOVER           (PROTOVER),
    .CHDR_W             (CHDR_W),
    .AXIS_CTRL_EN       (1),
    .AXIS_DATA_EN       (1),
    .INST_NUM           (1),
    .NUM_DATA_I         (2),
    .NUM_DATA_O         (2),
    .CTRL_XBAR_PORT     (PORT_B),
    .INGRESS_BUFF_SIZE  (MTU+1),
    .MTU                (MTU),
    .REPORT_STRM_ERRS   (1),
    .SIM_SPEEDUP        (1)
  ) sep_b (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk                                    ),
    .rfnoc_chdr_rst     (rfnoc_chdr_rst                                    ),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk                                    ),
    .rfnoc_ctrl_rst     (rfnoc_ctrl_rst                                    ),
    .device_id          (DEV_ID                                            ),
    .s_axis_chdr_tdata  (c2be_chdr_tdata                                   ),
    .s_axis_chdr_tlast  (c2be_chdr_tlast                                   ),
    .s_axis_chdr_tvalid (c2be_chdr_tvalid                                  ),
    .s_axis_chdr_tready (c2be_chdr_tready                                  ),
    .m_axis_chdr_tdata  (b2c_chdr_tdata                                    ),
    .m_axis_chdr_tlast  (b2c_chdr_tlast                                    ),
    .m_axis_chdr_tvalid (b2c_chdr_tvalid                                   ),
    .m_axis_chdr_tready (b2c_chdr_tready                                   ),
    .s_axis_data_tdata  ({m_b1_data.tdata  , m_b0_data.tdata  }),
    .s_axis_data_tlast  ({m_b1_data.tlast  , m_b0_data.tlast  }),
    .s_axis_data_tvalid ({m_b1_data.tvalid , m_b0_data.tvalid }),
    .s_axis_data_tready ({m_b1_data.tready , m_b0_data.tready }),
    .m_axis_data_tdata  ({s_b1_data.tdata , s_b0_data.tdata }),
    .m_axis_data_tlast  ({s_b1_data.tlast , s_b0_data.tlast }),
    .m_axis_data_tvalid ({s_b1_data.tvalid, s_b0_data.tvalid}),
    .m_axis_data_tready ({s_b1_data.tready, s_b0_data.tready}),
    .s_axis_ctrl_tdata  (b_ctrl_out_tdata                                  ),
    .s_axis_ctrl_tlast  (b_ctrl_loop_tlast                                 ),
    .s_axis_ctrl_tvalid (b_ctrl_loop_tvalid                                ),
    .s_axis_ctrl_tready (b_ctrl_loop_tready                                ),
    .m_axis_ctrl_tdata  (b_ctrl_in_tdata                                   ),
    .m_axis_ctrl_tlast  (b_ctrl_loop_tlast                                 ),
    .m_axis_ctrl_tvalid (b_ctrl_loop_tvalid                                ),
    .m_axis_ctrl_tready (b_ctrl_loop_tready                                ),
    .strm_seq_err_stb   (                                                  ),
    .strm_data_err_stb  (                                                  ),
    .strm_route_err_stb (                                                  ),
    .signal_data_err    (b_signal_data_err                                 )
  );

  chdr_crossbar_nxn #(
    .CHDR_W         (CHDR_W),
    .NPORTS         (3),
    .DEFAULT_PORT   (0),
    .MTU            (MTU),
    .ROUTE_TBL_SIZE (6),
    .MUX_ALLOC      ("ROUND-ROBIN"),
    .OPTIMIZE       ("AREA"),
    .NPORTS_MGMT    (1),
    .EXT_RTCFG_PORT (0),
    .PROTOVER       (PROTOVER)
  ) xbar_c (
    .clk            (rfnoc_chdr_clk),
    .reset          (rfnoc_chdr_rst),
    .device_id      (DEV_ID),
    .s_axis_tdata   ({b2c_chdr_tdata,   a2c_chdr_tdata,   m_tb_chdr.tdata  }),
    .s_axis_tlast   ({b2c_chdr_tlast,   a2c_chdr_tlast,   m_tb_chdr.tlast  }),
    .s_axis_tvalid  ({b2c_chdr_tvalid,  a2c_chdr_tvalid,  m_tb_chdr.tvalid }),
    .s_axis_tready  ({b2c_chdr_tready,  a2c_chdr_tready,  m_tb_chdr.tready }),
    .m_axis_tdata   ({c2bx_chdr_tdata,  c2ax_chdr_tdata,  s_tb_chdr.tdata }),
    .m_axis_tlast   ({c2bx_chdr_tlast,  c2ax_chdr_tlast,  s_tb_chdr.tlast }),
    .m_axis_tvalid  ({c2bx_chdr_tvalid, c2ax_chdr_tvalid, s_tb_chdr.tvalid}),
    .m_axis_tready  ({c2bx_chdr_tready, c2ax_chdr_tready, s_tb_chdr.tready}),
    .ext_rtcfg_stb  ('0),
    .ext_rtcfg_addr ('0),
    .ext_rtcfg_data ('0),
    .ext_rtcfg_ack  ()
  );

  lossy_xport_model #( .CHDR_W(CHDR_W) ) xport_a (
    .clk           (rfnoc_chdr_clk  ),
    .rst           (rfnoc_chdr_rst  ),
    .s_axis_tdata  (c2ax_chdr_tdata ),
    .s_axis_tlast  (c2ax_chdr_tlast ),
    .s_axis_tvalid (c2ax_chdr_tvalid),
    .s_axis_tready (c2ax_chdr_tready),
    .m_axis_tdata  (c2ae_chdr_tdata ),
    .m_axis_tlast  (c2ae_chdr_tlast ),
    .m_axis_tvalid (c2ae_chdr_tvalid),
    .m_axis_tready (c2ae_chdr_tready),
    .seqerr_prob   (a_seqerr_prob   ),
    .rterr_prob    (a_rterr_prob    ),
    .lossy         (a_lossy_input   )
  );

  lossy_xport_model #( .CHDR_W(CHDR_W) ) xport_b (
    .clk           (rfnoc_chdr_clk  ),
    .rst           (rfnoc_chdr_rst  ),
    .s_axis_tdata  (c2bx_chdr_tdata ),
    .s_axis_tlast  (c2bx_chdr_tlast ),
    .s_axis_tvalid (c2bx_chdr_tvalid),
    .s_axis_tready (c2bx_chdr_tready),
    .m_axis_tdata  (c2be_chdr_tdata ),
    .m_axis_tlast  (c2be_chdr_tlast ),
    .m_axis_tvalid (c2be_chdr_tvalid),
    .m_axis_tready (c2be_chdr_tready),
    .seqerr_prob   (b_seqerr_prob   ),
    .rterr_prob    (b_rterr_prob    ),
    .lossy         (b_lossy_input   )
  );

  // ----------------------------------------
  // BFMs and Test Models
  // ----------------------------------------

  ChdrBfm #(CHDR_W) a0_data_bfm = new(m_a0_data, s_a0_data);
  ChdrBfm #(CHDR_W) b0_data_bfm = new(m_b0_data, s_b0_data);
  ChdrBfm #(CHDR_W) a1_data_bfm = new(m_a1_data, s_a1_data);
  ChdrBfm #(CHDR_W) b1_data_bfm = new(m_b1_data, s_b1_data);
  ChdrBfm #(CHDR_W) tb_chdr_bfm = new(m_tb_chdr, s_tb_chdr);

  // Simple responders for AXIS-Ctrl transactions
  reg a_first = 1'b1, b_first = 1'b1;
  always @(posedge rfnoc_ctrl_clk) begin
    if (rfnoc_ctrl_rst) begin
      a_first <= 1'd1;
      b_first <= 1'd1;
    end else begin
      if (a_ctrl_loop_tvalid & a_ctrl_loop_tready)
        a_first <= a_ctrl_loop_tlast;
      if (b_ctrl_loop_tvalid & b_ctrl_loop_tready)
        b_first <= b_ctrl_loop_tlast;
    end
  end
  // Respond with an ACK and the source and destination ports swapped
  assign a_ctrl_out_tdata =
    a_first ? {1'b1, a_ctrl_in_tdata[30:20], a_ctrl_in_tdata[9:0], a_ctrl_in_tdata[19:10]} : a_ctrl_in_tdata;
  assign b_ctrl_out_tdata =
    b_first ? {1'b1, b_ctrl_in_tdata[30:20], b_ctrl_in_tdata[9:0], b_ctrl_in_tdata[19:10]} : b_ctrl_in_tdata;

  // ----------------------------------------
  // Test Utilities
  // ----------------------------------------

  integer cached_mgmt_seqnum = 0;
  integer cached_ctrl_seqnum = 0;
  integer cached_data_seqnum = 0;

  // Test stream command packets and their stream status response packets
  task automatic send_recv_stream_packets(
    input [15:0] dst_epid
  );
    ChdrPacket         #(CHDR_W) tx_chdr, rx_chdr, exp_chdr;
    chdr_header_t      chdr_hdr;
    chdr_str_command_t str_cmd;
    chdr_str_status_t  str_sts;
    chdr_seq_num_t     seq_num, resp_seq_num;
    int                xfer_count_bytes;
    int                xfer_count_pkts;
    chdr_strc_opcode_t op[] = {
      STRC_INIT,    // Test once with zero init values
      STRC_PING,
      STRC_RESYNC,
      STRC_INIT     // Test again with non-zero init values
    };

    seq_num = 0;
    resp_seq_num = 0;
    xfer_count_bytes = 0;
    xfer_count_pkts = 0;

    // Iterate over stream commands to test
    foreach (op[i]) begin
      chdr_hdr = '{
        pkt_type : CHDR_STRM_CMD,
        dst_epid : dst_epid,
        seq_num  : seq_num,
        default  : 0
      };
      str_cmd = '{
        src_epid  : EPID_TB,
        op_code   : op[i],
        num_bytes : { $urandom(), $urandom() },  // 64-bit
        num_pkts  : { $urandom(), $urandom() },  // 40-bit
        default   : 0
      };

      // On the first STRC_INIT, test the init values of 0, which we use to get
      // the buffer sizes. We should only get one status update in response.
      if (i == 0) begin
        assert (op[i] == STRC_INIT) else $fatal(1, "Expecting STRC_INIT op");
        str_cmd.num_bytes = 0;
        str_cmd.num_pkts  = 0;
      end

      // Send the stream command packet
      tx_chdr = new();
      tx_chdr.write_stream_cmd(chdr_hdr, str_cmd);
      if (VERBOSE) begin $write("Tx"); tx_chdr.print(); end
      tb_chdr_bfm.put_chdr(tx_chdr);

      // Get the stream status packet response
      tb_chdr_bfm.get_chdr(rx_chdr);
      if (VERBOSE) begin $write("Rx"); rx_chdr.print(); end

      // Build up the expected response
      if (op[i] == STRC_INIT) begin
        // STRC_INIT resets the counters in the stream endpoint
        resp_seq_num     = 0;
        xfer_count_pkts  = 0;
        xfer_count_bytes = 0;
      end
      chdr_hdr.dst_epid = EPID_TB;  // Response should come back to TB endpoint
      chdr_hdr.seq_num  = resp_seq_num;
      str_sts = '{
        src_epid         : dst_epid,
        status           : STRS_OKAY,
        capacity_bytes   : 2**(MTU+1) * (CHDR_W/8) - 1,
        capacity_pkts    : 'hFFFFFF,
        xfer_count_pkts  : xfer_count_pkts,
        xfer_count_bytes : xfer_count_bytes,
        default          : 0
      };
      $write(""); str_sts.status_info = 0; // Work around Vivado bug :(
      if (op[i] == STRC_RESYNC) begin
        // The STRC_RESYNC op should update the xfer_count_bytes in the SEP, so
        // we should expect the values we sent to be echoed in the response.
        str_sts.xfer_count_pkts  = str_cmd.num_pkts;
        str_sts.xfer_count_bytes = str_cmd.num_bytes;
      end
      exp_chdr = new();
      exp_chdr.write_stream_status(chdr_hdr, str_sts);
      if (VERBOSE) begin $write("ExpRx"); exp_chdr.print(); end

      // Validate contents of the response
      `ASSERT_ERROR(exp_chdr.equal(rx_chdr),
        "Received CHDR stream status packet was incorrect");

      // Update counters. Note that xfer_count_bytes counts whole CHDR words
      // because that's the width of the internal buffers.
      xfer_count_bytes += $ceil(tx_chdr.header.length / (CHDR_W/8.0)) * (CHDR_W/8);
      xfer_count_pkts  += 1;
      resp_seq_num     += 1;
      seq_num          += 1;
    end
  endtask : send_recv_stream_packets

  task automatic send_recv_mgmt_packet(
    input  chdr_header_t tx_mgmt_hdr,
    input  chdr_mgmt_t   tx_mgmt_pl,
    output chdr_header_t rx_mgmt_hdr,
    output chdr_mgmt_t   rx_mgmt_pl
  );
    automatic timeout_t mgmt_timeout;
    automatic ChdrPacket #(CHDR_W) tx_chdr = new(), rx_chdr;
    tx_chdr.write_mgmt(tx_mgmt_hdr, tx_mgmt_pl);
    test.start_timeout(mgmt_timeout, 2us, "Waiting for management transaction");
    if (VERBOSE) begin $write("Tx"); tx_chdr.print(); end
    tb_chdr_bfm.put_chdr(tx_chdr.copy());
    tb_chdr_bfm.get_chdr(rx_chdr);
    test.end_timeout(mgmt_timeout);
    rx_chdr.read_mgmt(rx_mgmt_hdr, rx_mgmt_pl);
    if (VERBOSE) begin $write("Rx"); rx_chdr.print(); end
  endtask

  task automatic mgmt_read_err_counts(
    input [15:0] dst_epid,
    output [31:0] seq_err_count,
    output [31:0] route_err_count,
    output [31:0] data_err_count
  );
    automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
    automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
    automatic chdr_mgmt_op_t exp_mgmt_op;

    // Generic management header
    tx_mgmt_pl.header = '{
      default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
    };
    // Read error counts
    tx_mgmt_pl.header.num_hops = 3;
    tx_mgmt_pl.ops.delete();

    tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
      op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
    tx_mgmt_pl.ops[1] = '{  // Hop 2: Read status
      op_payload:{32'h0, sep_a.REG_OSTRM_SEQ_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd3};
    tx_mgmt_pl.ops[2] = '{  // Hop 2: Read status
      op_payload:{32'h0, sep_a.REG_OSTRM_DATA_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd2};
    tx_mgmt_pl.ops[3] = '{  // Hop 2: Read status
      op_payload:{32'h0, sep_a.REG_OSTRM_ROUTE_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd1};
    tx_mgmt_pl.ops[4] = '{  // Hop 2: Stream Endpoint: Return
      op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
    tx_mgmt_pl.ops[5] = '{  // Hop 3: Nop for return
      op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
    tx_mgmt_hdr = '{
      pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:dst_epid, default:'0};

    // Send the packet and ensure that error counts are zero
    send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
    `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
      "Check Errs: Mgmt header was incorrect");
    seq_err_count = 32'hx;
    route_err_count = 32'hx;
    data_err_count = 32'hx;
    for (int i = 1; i <= 3; i++) begin
      if (rx_mgmt_pl.ops[i].op_payload[15:0] == sep_a.REG_OSTRM_SEQ_ERR_CNT)
        seq_err_count = rx_mgmt_pl.ops[i].op_payload[47:16];
      else if (rx_mgmt_pl.ops[i].op_payload[15:0] == sep_a.REG_OSTRM_DATA_ERR_CNT)
        data_err_count = rx_mgmt_pl.ops[i].op_payload[47:16];
      else if (rx_mgmt_pl.ops[i].op_payload[15:0] == sep_a.REG_OSTRM_ROUTE_ERR_CNT)
        route_err_count = rx_mgmt_pl.ops[i].op_payload[47:16];
    end
  endtask

  task automatic send_recv_ctrl_packets(
    input [15:0] dst_epid,
    input [15:0] num_pkts,
    input [15:0] seq_num_start
  );
    for (int n = 0; n < num_pkts; n=n+1) begin
      automatic timeout_t ctrl_timeout;
      automatic ChdrPacket #(CHDR_W) tx_chdr = new(), rx_chdr, exp_chdr = new();
      automatic chdr_header_t chdr_hdr;
      automatic chdr_ctrl_header_t ctrl_hdr, exp_ctrl_hdr;
      automatic ctrl_op_word_t ctrl_op;
      automatic ctrl_word_t ctrl_data[$];
      automatic chdr_word_t ctrl_ts;

      ctrl_data.delete();
      for (int i = 0; i < $urandom_range(15,1); i++)
        ctrl_data[i] = $urandom();
      ctrl_hdr = '{
        default  : '0,
        src_epid : EPID_TB,
        is_ack   : 1'b0,
        has_time : $urandom_range(1),
        seq_num  : seq_num_start[5:0],
        num_data : ctrl_data.size(),
        src_port : $urandom(),
        dst_port : $urandom()
      };
      ctrl_ts = $urandom();
      ctrl_op = '{
        default      : '0,
        status       : CTRL_STS_OKAY,
        op_code      : ctrl_opcode_t'($urandom_range(9)),
        byte_enable  : $urandom_range(15),
        address      : $urandom()
      };
      chdr_hdr = '{
        dst_epid  : dst_epid,
        seq_num   : seq_num_start + n[15:0],
        pkt_type  : CHDR_CONTROL,
        default   : 0
      };
      tx_chdr.write_ctrl(chdr_hdr, ctrl_hdr, ctrl_op, ctrl_data, ctrl_ts);

      test.start_timeout(ctrl_timeout, 2us, "Waiting for control transaction");
      if (VERBOSE) begin $write("Tx"); tx_chdr.print(); end
      tb_chdr_bfm.put_chdr(tx_chdr.copy());
      tb_chdr_bfm.get_chdr(rx_chdr);
      test.end_timeout(ctrl_timeout);
      if (VERBOSE) begin $write("Rx"); rx_chdr.print(); end

      exp_ctrl_hdr = ctrl_hdr;
      exp_ctrl_hdr.dst_port = ctrl_hdr.src_port;
      exp_ctrl_hdr.src_port = ctrl_hdr.dst_port;
      exp_ctrl_hdr.src_epid = dst_epid;
      exp_ctrl_hdr.is_ack = 1'b1;

      exp_chdr.write_ctrl(chdr_hdr, exp_ctrl_hdr, ctrl_op, ctrl_data, ctrl_ts);
      exp_chdr.header.dst_epid = EPID_TB;


      if (VERBOSE) begin $write("ExpRx"); exp_chdr.print(); end

      // Validate contents
      `ASSERT_ERROR(exp_chdr.equal(rx_chdr),
        "Received CHDR control packet was incorrect");
    end
  endtask

  task automatic send_recv_data_packets(
    input [15:0] src_epid,
    input [15:0] dst_epid,
    input [15:0] num_pkts,
    input [15:0] seq_num_start,
    input bit    ignore_seq_route_errs = 0
  );
    // Pick a VC for this run randomly
    logic [5:0] vc = $urandom_range(1);
    fork
      begin: tx_loop
        for (int txi = 0; txi < num_pkts; txi=txi+1) begin
          automatic timeout_t tx_timeout;
          automatic ChdrPacket #(CHDR_W) tx_chdr = new();
          automatic chdr_header_t tx_hdr;
          automatic chdr_word_t tx_ts;
          automatic chdr_word_t tx_mdata[$];
          automatic chdr_word_t tx_data[$];
          // Fill data in the packet
          tx_hdr = '{
            vc        : vc,
            dst_epid  : dst_epid,
            seq_num   : seq_num_start + txi[15:0],
            pkt_type  : (txi%4==0) ? CHDR_DATA_WITH_TS : CHDR_DATA_NO_TS,
            num_mdata : $urandom_range(5),
            default   : 0
          };
          tx_ts = txi;
          tx_mdata.delete();
          for (int i = 0; i < tx_hdr.num_mdata; i++)
            tx_mdata[i] = $urandom();
          tx_data.delete();
          for (int i = 0; i < $urandom_range((1<<MTU)-10); i++)
            tx_data[i] = {txi << 16, i[15:0]};
          tx_chdr.write_raw(tx_hdr, tx_data, tx_mdata, tx_ts);
          if (VERBOSE) $display("%s%0d:Tx:%0d:",(src_epid == EPID_A)?"A":"B", vc, txi, tx_chdr.sprint());
          // Send the packet
          test.start_timeout(tx_timeout, 2us, "Waiting to send data packet");
          if (src_epid == EPID_A)
            if (vc == 0)
              a0_data_bfm.put_chdr(tx_chdr.copy());
            else
              a1_data_bfm.put_chdr(tx_chdr.copy());
          else
            if (vc == 0)
              b0_data_bfm.put_chdr(tx_chdr.copy());
            else
              b1_data_bfm.put_chdr(tx_chdr.copy());
          test.end_timeout(tx_timeout);
        end
      end
      begin: rx_loop
        for (int rxi = 0; rxi < num_pkts; rxi=rxi+1) begin
          automatic timeout_t rx_timeout;
          automatic ChdrPacket #(CHDR_W) rx_chdr;
          // Receive a packet
          test.start_timeout(rx_timeout, 2us, "Waiting to recv data packet");
          if (dst_epid == EPID_A)
            if (vc == 0)
              a0_data_bfm.get_chdr(rx_chdr);
            else
              a1_data_bfm.get_chdr(rx_chdr);
          else
            if (vc == 0)
              b0_data_bfm.get_chdr(rx_chdr);
            else
              b1_data_bfm.get_chdr(rx_chdr);
          test.end_timeout(rx_timeout);
          // Validate the packet
          if (VERBOSE) $display("%s:Rx%0d:%0d:",(src_epid == EPID_A)?"A":"B", vc, rxi, rx_chdr.sprint());
          `ASSERT_ERROR(ignore_seq_route_errs || rx_chdr.header.dst_epid == dst_epid, "Data Pkt: dst_epid was incorrect");
          `ASSERT_ERROR(ignore_seq_route_errs || (rx_chdr.header.seq_num  == rxi + seq_num_start), "Data Pkt: seq_num was incorrect");
          if (rx_chdr.header.pkt_type == CHDR_DATA_WITH_TS)
            `ASSERT_ERROR(rx_chdr.timestamp  == rxi, "Data Pkt: timestamp was incorrect");
          foreach (rx_chdr.data[i]) begin
            `ASSERT_ERROR(rx_chdr.data[i] == {rxi << 16, i[15:0]}, "Data Pkt: payload was incorrect");
          end
        end
      end
    join
  endtask

  task automatic set_unidir_stall_prob(
    input [15:0] src_epid,
    input [15:0] dst_epid,
    int          src_stall_prob,
    int          dst_stall_prob
  );
    if (src_epid == EPID_A) begin
      a0_data_bfm.set_master_stall_prob(src_stall_prob);
      a1_data_bfm.set_master_stall_prob(src_stall_prob);
      b0_data_bfm.set_slave_stall_prob (dst_stall_prob);
      b1_data_bfm.set_slave_stall_prob (dst_stall_prob);
    end else begin
      b0_data_bfm.set_master_stall_prob(src_stall_prob);
      b1_data_bfm.set_master_stall_prob(src_stall_prob);
      a0_data_bfm.set_slave_stall_prob (dst_stall_prob);
      a1_data_bfm.set_slave_stall_prob (dst_stall_prob);
    end
  endtask

  task automatic set_bidir_stall_prob(
    int src_stall_prob,
    int dst_stall_prob
  );
    set_unidir_stall_prob(EPID_A, EPID_B, src_stall_prob, dst_stall_prob);
    set_unidir_stall_prob(EPID_B, EPID_A, src_stall_prob, dst_stall_prob);
  endtask

  // ----------------------------------------
  // Test Process
  // ----------------------------------------
  initial begin : tb_main

    // Shared Variables
    // ----------------------------------------
    timeout_t     timeout;
    string        tc_label;
    automatic bit stop_responder = 0;
    logic [31:0]  seq_err_count;
    logic [31:0]  route_err_count;
    logic [31:0]  data_err_count;

    a_signal_data_err = 0;
    b_signal_data_err = 0;
    a_seqerr_prob     = 0;
    a_rterr_prob      = 0;
    a_lossy_input     = 0;
    b_seqerr_prob     = 0;
    b_rterr_prob      = 0;
    b_lossy_input     = 0;


    // Initialize
    // ----------------------------------------
    test.start_tb({test_name, "chdr_stream_endpoint_tb"});

    // Start the BFMs
    a0_data_bfm.run();
    b0_data_bfm.run();
    a1_data_bfm.run();
    b1_data_bfm.run();
    tb_chdr_bfm.run();

    tb_chdr_bfm.set_master_stall_prob(0);
    tb_chdr_bfm.set_slave_stall_prob(0);

    // Start the clocks
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();

    // Reset
    // ----------------------------------------
    rfnoc_ctrl_clk_gen.reset();
    rfnoc_chdr_clk_gen.reset();

    test.start_test({test_name, "Wait for reset"});
    test.start_timeout(timeout, 1us, "Waiting for reset");
    while (rfnoc_ctrl_rst) @(posedge rfnoc_ctrl_clk);
    while (rfnoc_chdr_rst) @(posedge rfnoc_chdr_clk);
    test.end_timeout(timeout);
    `ASSERT_ERROR(!rfnoc_chdr_rst && !rfnoc_ctrl_rst, "Reset did not deassert");
    test.end_test();

    // Discover Topology
    // ----------------------------------------
    test.start_test({test_name, "Discover Topology"});
    begin
      automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
      automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
      automatic chdr_mgmt_op_t exp_mgmt_op;

      // *Status* We know nothing about the network. Need to discover stuff.

      // Generic management header
      tx_mgmt_pl.header = '{
        default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
      };
      // Send a node info request to the crossbar
      tx_mgmt_pl.header.num_hops = 2;
      tx_mgmt_pl.ops.delete();
      tx_mgmt_pl.ops[0] = '{  // Hop 1: Send node info
        op_payload:48'h0, op_code:MGMT_OP_INFO_REQ, ops_pending:8'd1};
      tx_mgmt_pl.ops[1] = '{  // Hop 1: Return
        op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
      tx_mgmt_pl.ops[2] = '{  // Hop 2: Nop for return
        op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
      tx_mgmt_hdr = '{
        pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:16'h0, default:'0};

      // Send the packet and check the response
      send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
      `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
        "Discover XB: Mgmt header was incorrect");
      exp_mgmt_op = '{op_payload:{2'h0, 8'd1/*ports_mgmt*/, 8'd3 /*ports*/, 10'd0 /*inst*/, 4'd1 /*type*/, DEV_ID},
        op_code:MGMT_OP_INFO_RESP, ops_pending:8'd0};
      `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op,
        "Discover XB: Mgmt response ops were incorrect");

      // *Status* We just discovered a crossbar with 3 ports!

      // Configure the crossbar routing table with our (TB) address
      //  then send node info request on the other two ports
      tx_mgmt_pl.header.num_hops = 3;
      tx_mgmt_pl.ops.delete();
      tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Config router to return packet to dest
        op_payload:{22'h0, PORT_TB, EPID_TB}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd1};
      tx_mgmt_pl.ops[1] = '{  // Hop 1: Crossbar: Config router
        op_payload:{38'h0, PORT_A}, op_code:MGMT_OP_SEL_DEST, ops_pending:8'd0};
      tx_mgmt_pl.ops[2] = '{  // Hop 2: Stream Endpoint: Send node info
        op_payload:48'h0, op_code:MGMT_OP_INFO_REQ, ops_pending:8'd1};
      tx_mgmt_pl.ops[3] = '{  // Hop 2: Return
        op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
      tx_mgmt_pl.ops[4] = '{  // Hop 3: TB: Nop for return
        op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
      tx_mgmt_hdr = '{
        pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:16'h0, default:'0};

      // Send the packet and check the response
      send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
      `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
        "Discover SEP A: Mgmt header was incorrect");
      exp_mgmt_op = '{op_payload:{{4'd1, 6'd2, 6'd2, 2'b11} /*ext_info*/, 10'd0 /*inst*/, 4'd2 /*type*/, DEV_ID},
        op_code:MGMT_OP_INFO_RESP, ops_pending:8'd0};
      `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op,
        "Discover SEP A: Mgmt response ops were incorrect");

      // *Status* We just discovered a stream endpoint on crossbar port 1

      // Send node info request on the last port
      tx_mgmt_pl.header.num_hops = 3;
      tx_mgmt_pl.ops.delete();
      tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Config router
        op_payload:{38'h0, PORT_B}, op_code:MGMT_OP_SEL_DEST, ops_pending:8'd0};
      tx_mgmt_pl.ops[1] = '{  // Hop 2: Stream Endpoint: Send node info
        op_payload:48'h0, op_code:MGMT_OP_INFO_REQ, ops_pending:8'd1};
      tx_mgmt_pl.ops[2] = '{  // Hop 2: Return
        op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
      tx_mgmt_pl.ops[3] = '{  // Hop 3: TB: Nop for return
        op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
      tx_mgmt_hdr = '{
        pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:16'h0, default:'0};

      // Send the packet and check the response
      send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
      `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
        "Discover SEP B: Mgmt header was incorrect");
      exp_mgmt_op = '{op_payload:{{4'd1, 6'd2, 6'd2, 2'b11} /*ext_info*/, 10'd1 /*inst*/, 4'd2 /*type*/, DEV_ID},
        op_code:MGMT_OP_INFO_RESP, ops_pending:8'd0};
      `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op,
        "Discover SEP B: Mgmt response ops were incorrect");

      // *Status* We just discovered a stream endpoint on crossbar port 2
    end
    test.end_test();

    // Configure Routes to Stream Endpoints A and B
    // ----------------------------------------
    test.start_test({test_name, "Configure Routes"});
    begin
      automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
      automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
      automatic chdr_mgmt_op_t exp_mgmt_op;

      // Generic management header
      tx_mgmt_pl.header = '{
        default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
      };
      // Send a node info request to the crossbar
      tx_mgmt_pl.header.num_hops = 2;
      tx_mgmt_pl.ops.delete();

      tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Config path to EP A
        op_payload:{22'h0, PORT_A, EPID_A}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd2};
      tx_mgmt_pl.ops[1] = '{  // Hop 1: Crossbar: Config path to EP B
        op_payload:{22'h0, PORT_B, EPID_B}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd1};
      tx_mgmt_pl.ops[2] = '{  // Hop 1: Request node info to make the packet come back
        op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
      tx_mgmt_pl.ops[3] = '{  // Hop 2: Nop for return
        op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
      tx_mgmt_hdr = '{
        pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:16'h0, default:'0};

      // Send the packet and check the response
      send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
      `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
        "Config Routes: Mgmt header was incorrect");
      exp_mgmt_op = '{op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
      `ASSERT_ERROR(rx_mgmt_pl.ops[0] == exp_mgmt_op,
        "Config Routes: Mgmt response ops were incorrect");
    end
    test.end_test();

    // Configure Stream Endpoints
    // ----------------------------------------
    test.start_test({test_name, "Configure Stream Endpoints"});
    begin
      automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
      automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
      automatic chdr_mgmt_op_t exp_mgmt_op;

      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        // Generic management header
        tx_mgmt_pl.header = '{
          default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
        };
        // Send a node info request to the crossbar
        tx_mgmt_pl.header.num_hops = 3;
        tx_mgmt_pl.ops.delete();

        tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_pl.ops[1] = '{  // Hop 2: Reset
          op_payload:{32'b111, sep_a.REG_RESET_AND_FLUSH}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd4};
        tx_mgmt_pl.ops[2] = '{  // Hop 2: Write EPID
          op_payload:{16'h0, epids[i], sep_a.REG_EPID_SELF}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd3};
        tx_mgmt_pl.ops[3] = '{  // Hop 2: Read EPID
          op_payload:{32'h0, sep_a.REG_EPID_SELF}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd2};
        tx_mgmt_pl.ops[4] = '{  // Hop 2: Read EPID
          op_payload:{32'h0, sep_a.REG_OSTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd1};
        tx_mgmt_pl.ops[5] = '{  // Hop 2: Stream Endpoint: Return
          op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
        tx_mgmt_pl.ops[6] = '{  // Hop 3: Nop for return
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_hdr = '{
          pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:epids[i], default:'0};

        // Send the packet and check the response
        send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
        `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
          "Config SEP: Mgmt header was incorrect");
        exp_mgmt_op = '{op_payload:{16'h0, epids[i], sep_a.REG_EPID_SELF},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd1};
        `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op,
          "Config SEP: Mgmt response ops were incorrect");
        exp_mgmt_op = '{op_payload:{32'h0, sep_a.REG_OSTRM_CTRL_STATUS},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd0};
        `ASSERT_ERROR(rx_mgmt_pl.ops[2] == exp_mgmt_op,
          "Config SEP: Mgmt response ops were incorrect");
      end
    end
    test.end_test();

    // Test Stream Commands and Responses
    // ----------------------------------------
    test.start_test({test_name, "Test Stream Commands"}, 1ms);
    begin
      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        send_recv_stream_packets(epids[i]);
      end
    end
    test.end_test();

    // Setup a stream between Endpoint A and B
    // ----------------------------------------
    test.start_test({test_name, "Setup bidirectional stream between endpoints A and B"});
    begin
      automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
      automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
      automatic chdr_mgmt_op_t exp_mgmt_op;

      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        // Generic management header
        tx_mgmt_pl.header = '{
          default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
        };
        // Configure FC on streams
        tx_mgmt_pl.header.num_hops = 3;
        tx_mgmt_pl.ops.delete();

        tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_pl.ops[1] = '{  // Hop 2: Write destination EPID
          op_payload:{16'h0, epids[1-i], sep_a.REG_OSTRM_DST_EPID}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd7};
        tx_mgmt_pl.ops[2] = '{  // Hop 2: Configure flow ack control freq
          op_payload:{32'd50, sep_a.REG_OSTRM_FC_FREQ_BYTES_LO}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd6};
        tx_mgmt_pl.ops[3] = '{  // Hop 2: Configure flow ack control freq
          op_payload:{32'd0, sep_a.REG_OSTRM_FC_FREQ_BYTES_HI}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd5};
        tx_mgmt_pl.ops[4] = '{  // Hop 2: Configure flow ack control freq
          op_payload:{32'd1000, sep_a.REG_OSTRM_FC_FREQ_PKTS}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd4};
        tx_mgmt_pl.ops[5] = '{  // Hop 2: Configure flow headroom
          op_payload:{32'd0, sep_a.REG_OSTRM_FC_HEADROOM}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd3};
        tx_mgmt_pl.ops[6] = '{  // Hop 2: Configure word swapping
          op_payload:{32'h44, sep_a.REG_ISTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd2}; // Swap 32-bit words, endianness
        tx_mgmt_pl.ops[7] = '{  // Hop 2: Configure lossy and start config
          op_payload:{32'h47, sep_a.REG_OSTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd1}; // Swap 32-bit words, endianness, lossy and reset
        tx_mgmt_pl.ops[8] = '{  // Hop 2: Stream Endpoint: Return
          op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
        tx_mgmt_pl.ops[9] = '{  // Hop 3: Nop for return
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_hdr = '{
          pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:epids[i], default:'0};

        // Send the packet and check the response
        send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);

        // Wait for some time for node to flush and reset
        // Typically we would poll in SW but we just wait to keep the code simple
        repeat (256) @(posedge rfnoc_chdr_clk);

        // Read back FC status
        tx_mgmt_pl.header.num_hops = 3;
        tx_mgmt_pl.ops.delete();

        tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_pl.ops[1] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd7};
        tx_mgmt_pl.ops[2] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_BUFF_CAP_BYTES_LO}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd6};
        tx_mgmt_pl.ops[3] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_BUFF_CAP_BYTES_HI}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd5};
        tx_mgmt_pl.ops[4] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_BUFF_CAP_PKTS}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd4};
        tx_mgmt_pl.ops[5] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_SEQ_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd3};
        tx_mgmt_pl.ops[6] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_DATA_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd2};
        tx_mgmt_pl.ops[7] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_ROUTE_ERR_CNT}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd1};
        tx_mgmt_pl.ops[8] = '{  // Hop 2: Stream Endpoint: Return
          op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
        tx_mgmt_pl.ops[9] = '{  // Hop 3: Nop for return
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_hdr = '{
          pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:epids[i], default:'0};

        // Send the packet and check the response
        send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
        `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
          "Config SEP: Mgmt header was incorrect");
        exp_mgmt_op = '{op_payload:{32'h80000006, sep_a.REG_OSTRM_CTRL_STATUS},   // FC on, no errors and lossy
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd6};
        `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{((1<<(MTU+1))*(CHDR_W/8)-1), sep_a.REG_OSTRM_BUFF_CAP_BYTES_LO},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd5};
        `ASSERT_ERROR(rx_mgmt_pl.ops[2] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{32'h0, sep_a.REG_OSTRM_BUFF_CAP_BYTES_HI},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd4};
        `ASSERT_ERROR(rx_mgmt_pl.ops[3] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{32'h00ffffff, sep_a.REG_OSTRM_BUFF_CAP_PKTS},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd3};
        `ASSERT_ERROR(rx_mgmt_pl.ops[4] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{32'h0, sep_a.REG_OSTRM_SEQ_ERR_CNT},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd2};
        `ASSERT_ERROR(rx_mgmt_pl.ops[5] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{32'h0, sep_a.REG_OSTRM_DATA_ERR_CNT},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd1};
        `ASSERT_ERROR(rx_mgmt_pl.ops[6] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
        exp_mgmt_op = '{op_payload:{32'h0, sep_a.REG_OSTRM_ROUTE_ERR_CNT},
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd0};
        `ASSERT_ERROR(rx_mgmt_pl.ops[7] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
      end
    end
    test.end_test();

    // Control transactions to Endpoint A
    // ----------------------------------------
    cached_ctrl_seqnum = 0;
    for (int cfg = 0; cfg < 2; cfg++) begin
      $sformat(tc_label, "Control Xact to A (%s)", (cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        tb_chdr_bfm.set_master_stall_prob(cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        tb_chdr_bfm.set_slave_stall_prob(cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        send_recv_ctrl_packets(EPID_A, NUM_PKTS_PER_TEST, cached_ctrl_seqnum);
      end
      test.end_test();
      cached_ctrl_seqnum += NUM_PKTS_PER_TEST;
    end

    // Control transactions to Endpoint B
    // ----------------------------------------
    cached_ctrl_seqnum = 0;
    for (int cfg = 0; cfg < 2; cfg++) begin
      $sformat(tc_label, "Control Xact to B (%s)", (cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        tb_chdr_bfm.set_master_stall_prob(cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        tb_chdr_bfm.set_slave_stall_prob(cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        send_recv_ctrl_packets(EPID_B, NUM_PKTS_PER_TEST, cached_ctrl_seqnum);
      end
      test.end_test();
      cached_ctrl_seqnum += NUM_PKTS_PER_TEST;
    end

    // Stream data from A to B
    // ----------------------------------------
    cached_data_seqnum = 0;
    for (int cfg = 0; cfg < 4; cfg++) begin
      automatic logic mst_cfg = cfg[0];
      automatic logic slv_cfg = cfg[1];
      $sformat(tc_label, "Stream Data from A to B (%s Mst, %s Slv)",
        (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        set_unidir_stall_prob(EPID_A, EPID_B,
          mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB,
          slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        send_recv_data_packets(EPID_A, EPID_B, NUM_PKTS_PER_TEST, cached_data_seqnum);
      end
      test.end_test();
      cached_data_seqnum += NUM_PKTS_PER_TEST;
    end

    // Stream data from B to A
    // ----------------------------------------
    cached_data_seqnum = 0;
    for (int cfg = 0; cfg < 4; cfg++) begin
      automatic logic mst_cfg = cfg[0];
      automatic logic slv_cfg = cfg[1];
      $sformat(tc_label, "Stream Data from B to A (%s Mst, %s Slv)",
        (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        set_unidir_stall_prob(EPID_B, EPID_A,
          mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB,
          slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        send_recv_data_packets(EPID_B, EPID_A, NUM_PKTS_PER_TEST, cached_data_seqnum);
      end
      test.end_test();
      cached_data_seqnum += NUM_PKTS_PER_TEST;
    end

    // Stream data between A <=> B simultaneously
    // ----------------------------------------
    for (int cfg = 0; cfg < 4; cfg++) begin
      automatic logic mst_cfg = cfg[0];
      automatic logic slv_cfg = cfg[1];
      $sformat(tc_label, "Stream Data between A <=> B simultaneously (%s Mst, %s Slv)",
        (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        set_bidir_stall_prob(
          mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB,
          slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        fork
          send_recv_data_packets(EPID_B, EPID_A, NUM_PKTS_PER_TEST, cached_data_seqnum);
          send_recv_data_packets(EPID_A, EPID_B, NUM_PKTS_PER_TEST, cached_data_seqnum);
        join
      end
      test.end_test();
      cached_data_seqnum += NUM_PKTS_PER_TEST;
    end

    // Stream data and control between A <=> B simultaneously
    // ----------------------------------------
    for (int cfg = 0; cfg < 4; cfg++) begin
      automatic logic mst_cfg = cfg[0];
      automatic logic slv_cfg = cfg[1];
      $sformat(tc_label, "Stream Data and Control between A <=> B (%s Mst, %s Slv)",
        (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
      test.start_test({test_name, tc_label});
      begin
        tb_chdr_bfm.set_master_stall_prob(mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        tb_chdr_bfm.set_slave_stall_prob(slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        set_bidir_stall_prob(
          mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB,
          slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        fork
          send_recv_data_packets(EPID_B, EPID_A, NUM_PKTS_PER_TEST/2, cached_data_seqnum);
          send_recv_data_packets(EPID_A, EPID_B, NUM_PKTS_PER_TEST/2, cached_data_seqnum);
          send_recv_ctrl_packets(EPID_A, NUM_PKTS_PER_TEST, cached_ctrl_seqnum);
        join
        cached_data_seqnum += NUM_PKTS_PER_TEST/2;
        fork
          send_recv_data_packets(EPID_B, EPID_A, NUM_PKTS_PER_TEST/2, cached_data_seqnum);
          send_recv_data_packets(EPID_A, EPID_B, NUM_PKTS_PER_TEST/2, cached_data_seqnum);
          send_recv_ctrl_packets(EPID_B, NUM_PKTS_PER_TEST, cached_ctrl_seqnum);
        join
        cached_data_seqnum += NUM_PKTS_PER_TEST/2;
        cached_ctrl_seqnum += NUM_PKTS_PER_TEST;
      end
      test.end_test();
    end

    // Check zero sequence errors after streaming
    // ----------------------------------------
    test.start_test({test_name, "Check zero sequence errors after streaming"});
    begin
      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        mgmt_read_err_counts(epids[i], seq_err_count, route_err_count, data_err_count);
        `ASSERT_ERROR(seq_err_count == 32'd0, "Check NoErrs: Incorrect seq error count");
        `ASSERT_ERROR(route_err_count == 32'd0, "Check NoErrs: Incorrect route error count");
        `ASSERT_ERROR(data_err_count == 32'd0, "Check NoErrs: Incorrect data error count");
      end
    end
    test.end_test();


    // Force sequence error
    // Note: Occasional StreamCommand to resynch occur, which
    //   can cause the count to be greater than just the number data packets
    //   that are sent, so the comparisons are to > instead of ==
    // ----------------------------------------
    test.start_test({test_name, "Force sequence error"});
    begin
      // First sequence error
      send_recv_data_packets(EPID_A, EPID_B, 1, cached_data_seqnum++, 1);
      b_seqerr_prob = 100;  // Simulate a dropped packet
      send_recv_data_packets(EPID_A, EPID_B, 1, cached_data_seqnum++, 1);
      b_seqerr_prob = 0;
      repeat (100) @(posedge rfnoc_chdr_clk);  // Wait for sequence error to reach the upstream port
      mgmt_read_err_counts(EPID_A, seq_err_count, route_err_count, data_err_count);
      `ASSERT_ERROR(seq_err_count > 32'd0, "Force SeqErr: Incorrect seq error count");
      `ASSERT_ERROR(route_err_count == 32'd0, "Force SeqErr: Incorrect route error count");
      `ASSERT_ERROR(data_err_count == 32'd0, "Force SeqErr: Incorrect data error count");

      // Second and third sequence error
      send_recv_data_packets(EPID_A, EPID_B, 1, cached_data_seqnum++, 1);
      b_seqerr_prob = 100;  // Simulate another dropped packet
      send_recv_data_packets(EPID_A, EPID_B, 1, cached_data_seqnum++, 1);
      b_seqerr_prob = 0;
      repeat (100) @(posedge rfnoc_chdr_clk);  // Wait for sequence error to reach the upstream port
      mgmt_read_err_counts(EPID_A, seq_err_count, route_err_count, data_err_count);
      `ASSERT_ERROR(seq_err_count > 32'd2, "Force SeqErr: Incorrect seq error count");
      `ASSERT_ERROR(route_err_count == 32'd0, "Force SeqErr: Incorrect route error count");
      `ASSERT_ERROR(data_err_count == 32'd0, "Force SeqErr: Incorrect data error count");
    end
    test.end_test();

    // Force routing error
    // ----------------------------------------
    test.start_test({test_name, "Force routing error"});
    begin
      logic [31:0] old_route_err_count;
      // First sequence error
      send_recv_data_packets(EPID_B, EPID_A, 1, cached_data_seqnum++, 1);
      a_rterr_prob = 100;  // Simulate a routing error
      send_recv_data_packets(EPID_B, EPID_A, 1, cached_data_seqnum++, 1);
      a_rterr_prob = 0;
      repeat (100) @(posedge rfnoc_chdr_clk);  // Wait for sequence error to reach the upstream port
      mgmt_read_err_counts(EPID_B, seq_err_count, route_err_count, data_err_count);
      `ASSERT_ERROR(seq_err_count == 32'd0, "Force RouteErr 1: Incorrect seq error count");
      `ASSERT_ERROR(route_err_count > 32'd0, "Force RouteErr 1: Incorrect route error count");
      `ASSERT_ERROR(data_err_count == 32'd0, "Force RouteErr 1: Incorrect data error count");
      old_route_err_count = route_err_count;

      // Second routing error
      send_recv_data_packets(EPID_B, EPID_A, 1, cached_data_seqnum++, 1);
      a_rterr_prob = 100;  // Simulate a routing error
      send_recv_data_packets(EPID_B, EPID_A, 1, cached_data_seqnum++, 1);
      a_rterr_prob = 0;
      repeat (100) @(posedge rfnoc_chdr_clk);  // Wait for sequence error to reach the upstream port
      mgmt_read_err_counts(EPID_B, seq_err_count, route_err_count, data_err_count);
      `ASSERT_ERROR(seq_err_count == 32'd0, "Force RouteErr 2: Incorrect seq error count");
      `ASSERT_ERROR(route_err_count > old_route_err_count, "Force RouteErr 2: Incorrect route error count");
      `ASSERT_ERROR(data_err_count == 32'd0, "Force RouteErr 2: Incorrect data error count");
    end
    test.end_test();

    // Setup a stream between Endpoint A and B
    // ----------------------------------------
    test.start_test({test_name, "Reconfigure flow control (reset state)"});
    begin
      automatic chdr_header_t tx_mgmt_hdr, rx_mgmt_hdr;
      automatic chdr_mgmt_t tx_mgmt_pl, rx_mgmt_pl;
      automatic chdr_mgmt_op_t exp_mgmt_op;

      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        // Generic management header
        tx_mgmt_pl.header = '{
          default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:EPID_TB
        };
        // Configure FC on streams
        tx_mgmt_pl.header.num_hops = 3;
        tx_mgmt_pl.ops.delete();

        tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_pl.ops[1] = '{  // Hop 2: Disable swapping
          op_payload:{32'd0, sep_a.REG_ISTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd2};
        tx_mgmt_pl.ops[2] = '{  // Hop 2: Configure lossy and start config
          op_payload:{32'd3, sep_a.REG_OSTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_WR_REQ, ops_pending:8'd1};
        tx_mgmt_pl.ops[3] = '{  // Hop 2: Stream Endpoint: Return
          op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
        tx_mgmt_pl.ops[4] = '{  // Hop 3: Nop for return
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_hdr = '{
          pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:epids[i], default:'0};

        // Send the packet and check the response
        send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);

        // Wait for some time for node to flush and reset
        // Typically we would poll in SW but we just wait to keep the code simple
        repeat (256) @(posedge rfnoc_chdr_clk);

        // Read back FC status
        tx_mgmt_pl.header.num_hops = 3;
        tx_mgmt_pl.ops.delete();

        tx_mgmt_pl.ops[0] = '{  // Hop 1: Crossbar: Nop
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_pl.ops[1] = '{  // Hop 2: Read status
          op_payload:{32'h0, sep_a.REG_OSTRM_CTRL_STATUS}, op_code:MGMT_OP_CFG_RD_REQ, ops_pending:8'd1};
        tx_mgmt_pl.ops[2] = '{  // Hop 2: Stream Endpoint: Return
          op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
        tx_mgmt_pl.ops[3] = '{  // Hop 3: Nop for return
          op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
        tx_mgmt_hdr = '{
          pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:epids[i], default:'0};

        // Send the packet and check the response
        send_recv_mgmt_packet(tx_mgmt_hdr, tx_mgmt_pl, rx_mgmt_hdr, rx_mgmt_pl);
        `ASSERT_ERROR(rx_mgmt_pl.header.num_hops == 1,
          "Config SEP: Mgmt header was incorrect");
        exp_mgmt_op = '{op_payload:{32'h80000002, sep_a.REG_OSTRM_CTRL_STATUS},   // FC on, no errors and lossy
          op_code:MGMT_OP_CFG_RD_RESP, ops_pending:8'd0};
        `ASSERT_ERROR(rx_mgmt_pl.ops[1] == exp_mgmt_op, "Config SEP: Mgmt response was incorrect");
      end
    end
    test.end_test();

    // Check zero errors after reinit
    // ----------------------------------------
    test.start_test({test_name, "Check zero errors after reinit"});
    begin
      automatic logic [15:0] epids[2] = {EPID_A, EPID_B};
      foreach (epids[i]) begin
        mgmt_read_err_counts(epids[i], seq_err_count, route_err_count, data_err_count);
        `ASSERT_ERROR(seq_err_count == 32'd0, "Check NoErrs: Incorrect seq error count");
        `ASSERT_ERROR(route_err_count == 32'd0, "Check NoErrs: Incorrect route error count");
        `ASSERT_ERROR(data_err_count == 32'd0, "Check NoErrs: Incorrect data error count");
      end
    end
    test.end_test();

    // Stream data between A <=> B simultaneously
    // ----------------------------------------
    test.start_test({test_name, "Stream Data between A <=> B with a lossy link"});
    begin
      cached_data_seqnum = 0;
      set_bidir_stall_prob(FAST_STALL_PROB, SLOW_STALL_PROB);
      a_lossy_input = 1;
      b_lossy_input = 1;
      fork
        send_recv_data_packets(EPID_B, EPID_A, NUM_PKTS_PER_TEST * 10, cached_data_seqnum);
        send_recv_data_packets(EPID_A, EPID_B, NUM_PKTS_PER_TEST * 10, cached_data_seqnum);
      join
      a_lossy_input = 0;
      b_lossy_input = 0;
    end
    test.end_test();
    cached_data_seqnum += NUM_PKTS_PER_TEST*10;

    // Finish Up
    // ----------------------------------------
    // Stop the clocks
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    // Display final statistics and results
    test.end_tb(.finish(0));
  end : tb_main

endmodule

`default_nettype wire
