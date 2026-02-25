//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_endpoint_tb
//

`default_nettype none


module ctrlport_endpoint_tb;

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  `include "rfnoc_axis_ctrl_utils.vh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgAxisCtrlBfm::*;
  import ctrlport_pkg::ctrlport_request_t;

  // Parameters
  localparam [9:0]  THIS_PORTID = 10'h17;
  localparam [15:0] THIS_EPID   = 16'hDEAD;

  localparam integer NUM_XACT_PER_TEST = 5000;
  localparam integer FAST_STALL_PROB   = 0;
  localparam integer SLOW_STALL_PROB   = 50;
  localparam bit     VERBOSE           = 0;

  // Clock and Reset Definition
  bit rfnoc_ctrl_clk, rfnoc_ctrl_rst;
  bit ctrlport_clk, ctrlport_rst;

  sim_clock_gen #(.PERIOD(6.0), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (rfnoc_ctrl_clk, rfnoc_ctrl_rst);  // 166.6 MHz
  sim_clock_gen #(.PERIOD(20.0), .AUTOSTART(0))
    ctrlport_clk_gen (ctrlport_clk, ctrlport_rst);  // 50 MHz

  // ----------------------------------------
  // Instantiate DUT
  // ----------------------------------------

  // AXIS-Ctrl master BFM
  AxiStreamIf #(32) mst_req  (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxiStreamIf #(32) mst_resp (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxisCtrlBfm axis_ctrl_mst_bfm;

  // AXIS-Ctrl slave BFM
  AxiStreamIf #(32) slv_req  (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxiStreamIf #(32) slv_resp (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxisCtrlBfm axis_ctrl_slv_bfm;

  wire  [31:0]  axis_mst_tdata,  axis_slv_tdata;
  wire          axis_mst_tlast,  axis_slv_tlast;
  wire          axis_mst_tvalid, axis_slv_tvalid;
  wire          axis_mst_tready, axis_slv_tready;

  wire          cp_slv_req_wr;
  wire          cp_slv_req_rd;
  wire  [19:0]  cp_slv_req_addr;
  wire  [31:0]  cp_slv_req_data;
  wire  [3:0]   cp_slv_req_byte_en;
  wire          cp_slv_req_has_time;
  wire  [63:0]  cp_slv_req_time;
  reg           cp_slv_resp_ack;
  reg   [1:0]   cp_slv_resp_status;
  reg   [31:0]  cp_slv_resp_data;

  // Controls dummy ctrlport slave status output:
  //   0 = Always OKAY
  //   1 = Status derived from address
  logic blk_status_mode = 1'b0;

  // Mailbox of expected ctrlport slave requests; populated by axis_ctrl_transact
  // and drained by the monitor initial block below.
  mailbox #(ctrlport_request_t) cp_req_mbox = new();

  logic         cp_mst_req_wr;
  logic         cp_mst_req_rd;
  logic [19:0]  cp_mst_req_addr;
  logic [9:0]   cp_mst_req_portid;
  logic [15:0]  cp_mst_req_rem_epid;
  logic [9:0]   cp_mst_req_rem_portid;
  logic [31:0]  cp_mst_req_data;
  logic [3:0]   cp_mst_req_byte_en;
  logic         cp_mst_req_has_time;
  logic [63:0]  cp_mst_req_time;
  wire          cp_mst_resp_ack;
  wire  [1:0]   cp_mst_resp_status;
  wire  [31:0]  cp_mst_resp_data;

  ctrlport_endpoint #(
    .THIS_PORTID              (THIS_PORTID),
    .SYNC_CLKS                (0),
    .AXIS_CTRL_MST_EN         (1),
    .AXIS_CTRL_SLV_EN         (1),
    .SLAVE_FIFO_SIZE          (5)
  ) dut (
    .rfnoc_ctrl_clk           (rfnoc_ctrl_clk      ),
    .rfnoc_ctrl_rst           (rfnoc_ctrl_rst      ),
    .ctrlport_clk             (ctrlport_clk        ),
    .ctrlport_rst             (ctrlport_rst        ),
    .s_rfnoc_ctrl_tdata       (axis_mst_tdata      ),
    .s_rfnoc_ctrl_tlast       (axis_mst_tlast      ),
    .s_rfnoc_ctrl_tvalid      (axis_mst_tvalid     ),
    .s_rfnoc_ctrl_tready      (axis_mst_tready     ),
    .m_rfnoc_ctrl_tdata       (axis_slv_tdata      ),
    .m_rfnoc_ctrl_tlast       (axis_slv_tlast      ),
    .m_rfnoc_ctrl_tvalid      (axis_slv_tvalid     ),
    .m_rfnoc_ctrl_tready      (axis_slv_tready     ),
    .m_ctrlport_req_wr        (cp_slv_req_wr       ),
    .m_ctrlport_req_rd        (cp_slv_req_rd       ),
    .m_ctrlport_req_addr      (cp_slv_req_addr     ),
    .m_ctrlport_req_data      (cp_slv_req_data     ),
    .m_ctrlport_req_byte_en   (cp_slv_req_byte_en  ),
    .m_ctrlport_req_has_time  (cp_slv_req_has_time ),
    .m_ctrlport_req_time      (cp_slv_req_time     ),
    .m_ctrlport_resp_ack      (cp_slv_resp_ack     ),
    .m_ctrlport_resp_status   (cp_slv_resp_status  ),
    .m_ctrlport_resp_data     (cp_slv_resp_data    ),
    .s_ctrlport_req_wr        (cp_mst_req_wr       ),
    .s_ctrlport_req_rd        (cp_mst_req_rd       ),
    .s_ctrlport_req_addr      (cp_mst_req_addr     ),
    .s_ctrlport_req_portid    (cp_mst_req_portid   ),
    .s_ctrlport_req_rem_epid  (cp_mst_req_rem_epid ),
    .s_ctrlport_req_rem_portid(cp_mst_req_rem_portid),
    .s_ctrlport_req_data      (cp_mst_req_data     ),
    .s_ctrlport_req_byte_en   (cp_mst_req_byte_en  ),
    .s_ctrlport_req_has_time  (cp_mst_req_has_time ),
    .s_ctrlport_req_time      (cp_mst_req_time     ),
    .s_ctrlport_resp_ack      (cp_mst_resp_ack     ),
    .s_ctrlport_resp_status   (cp_mst_resp_status  ),
    .s_ctrlport_resp_data     (cp_mst_resp_data    )
  );

  // ----------------------------------------
  // Test Helpers
  // ----------------------------------------

  wire [31:0] m_ctrl_tdata;
  wire        m_ctrl_tlast;
  wire        m_ctrl_tvalid;
  wire        m_ctrl_tready;

  assign m_ctrl_tdata   = mst_req.tdata;
  assign m_ctrl_tlast   = mst_req.tlast;
  assign m_ctrl_tvalid  = mst_req.tvalid;
  assign mst_req.tready = m_ctrl_tready;

  wire [31:0] axis_resp_tdata;
  wire        axis_resp_tlast;
  wire        axis_resp_tvalid;
  wire        axis_resp_tready;

  assign axis_resp_tdata  = slv_resp.tdata;
  assign axis_resp_tlast  = slv_resp.tlast;
  assign axis_resp_tvalid = slv_resp.tvalid;
  assign slv_resp.tready  = axis_resp_tready;

  // Add a MUX and DEMUX on the ctrlport logic to loop responses
  // back into the endpoint and to allow external access from the
  // master and slave BFM.
  axi_mux #(
    .WIDTH(32), .SIZE(2), .PRIO(0), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
  ) mux_i (
    .clk(rfnoc_ctrl_clk), .reset(rfnoc_ctrl_rst), .clear(1'b0),
    .i_tdata ({m_ctrl_tdata , axis_resp_tdata }),
    .i_tlast ({m_ctrl_tlast , axis_resp_tlast }),
    .i_tvalid({m_ctrl_tvalid, axis_resp_tvalid}),
    .i_tready({m_ctrl_tready, axis_resp_tready}),
    .o_tdata (axis_mst_tdata ),
    .o_tlast (axis_mst_tlast ),
    .o_tvalid(axis_mst_tvalid),
    .o_tready(axis_mst_tready)
  );

  wire [31:0] s_ctrl_tdata;
  wire        s_ctrl_tlast;
  wire        s_ctrl_tvalid;
  wire        s_ctrl_tready;

  assign mst_resp.tdata  = s_ctrl_tdata;
  assign mst_resp.tlast  = s_ctrl_tlast;
  assign mst_resp.tvalid = s_ctrl_tvalid;
  assign s_ctrl_tready   = mst_resp.tready;

  wire [31:0] axis_req_tdata;
  wire        axis_req_tlast;
  wire        axis_req_tvalid;
  wire        axis_req_tready;

  assign slv_req.tdata   = axis_req_tdata;
  assign slv_req.tlast   = axis_req_tlast;
  assign slv_req.tvalid  = axis_req_tvalid;
  assign axis_req_tready = slv_req.tready;

  wire [31:0] in_hdr;
  axi_demux #(
    .WIDTH(32), .SIZE(2), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
  ) demux_i (
    .clk(rfnoc_ctrl_clk), .reset(rfnoc_ctrl_rst), .clear(1'b0),
    .header(in_hdr), .dest(in_hdr[31]),
    .i_tdata (axis_slv_tdata ),
    .i_tlast (axis_slv_tlast ),
    .i_tvalid(axis_slv_tvalid),
    .i_tready(axis_slv_tready),
    .o_tdata ({s_ctrl_tdata , axis_req_tdata }),
    .o_tlast ({s_ctrl_tlast , axis_req_tlast }),
    .o_tvalid({s_ctrl_tvalid, axis_req_tvalid}),
    .o_tready({s_ctrl_tready, axis_req_tready})
  );

  // --------------------------
  // [Dummy Control Port Slave]
  // Slave Model:
  // - Respond in 1 clock cycle
  // - Status = addr[3:2] (when blk_status_mode=1) or always OKAY (when 0)
  // - Response Data = {~lower 16-bits of addr, lower 16-bits of addr}
  // - Verifies each request against the next expected entry in cp_req_mbox
  always @(posedge ctrlport_clk) begin : dummy_cp_slave
    ctrlport_request_t exp_req;
    if (ctrlport_rst) begin
      cp_slv_resp_ack <= 1'b0;
    end else begin
      // A new request (req_wr or req_rd) must not be issued on the same clock
      // cycle as an incoming ack.
      `ASSERT_ERROR(
        !(cp_slv_resp_ack && (cp_slv_req_wr || cp_slv_req_rd)),
        "ctrlport protocol violation: req_wr/req_rd asserted on same cycle as resp_ack"
      );
      cp_slv_resp_ack    <= cp_slv_req_wr | cp_slv_req_rd;
      cp_slv_resp_status <= blk_status_mode ? cp_slv_req_addr[3:2] : 2'b00;
      cp_slv_resp_data   <= {~cp_slv_req_addr[15:0], cp_slv_req_addr[15:0]};
      if (cp_slv_req_wr | cp_slv_req_rd) begin
        `ASSERT_ERROR(cp_req_mbox.try_get(exp_req),
          "Unexpected ctrlport slave request: mailbox was empty");
        `ASSERT_ERROR(cp_slv_req_wr == exp_req.wr,
          "ctrlport slave req: wrong wr strobe");
        `ASSERT_ERROR(cp_slv_req_rd == exp_req.rd,
          "ctrlport slave req: wrong rd strobe");
        `ASSERT_ERROR(cp_slv_req_addr == exp_req.addr,
          "ctrlport slave req: wrong address");
        if (exp_req.wr)
          `ASSERT_ERROR(cp_slv_req_data == exp_req.data,
            "ctrlport slave req: wrong data");
      end
    end
  end
  // --------------------------

  // ------------------------------------------------------------
  // [AXIS-Ctrl Packet Length Monitor]
  // Monitors all packets output from the DUT on axis_slv and
  // validates that the num_data field in the upper header matches
  // the actual number of data words in the packet. This covers
  // both AXIS-Ctrl slave responses and master requests.
  // ------------------------------------------------------------
  initial begin : axis_slv_length_monitor
    logic        has_time;
    logic [3:0]  hdr_hi_num_data;
    int          actual_data_words, expected_data_words;
    logic        is_ack;

    forever begin
      @(posedge rfnoc_ctrl_clk);
      if (!rfnoc_ctrl_rst && axis_slv_tvalid && axis_slv_tready) begin
        // Start of a new packet. Capture header low word.
        has_time = axis_ctrl_get_has_time(axis_slv_tdata);
        is_ack   = axis_ctrl_get_is_ack(axis_slv_tdata);

        // Wait for header high word
        @(posedge rfnoc_ctrl_clk);
        while (!axis_slv_tvalid || !axis_slv_tready) @(posedge rfnoc_ctrl_clk);
        hdr_hi_num_data = axis_ctrl_get_data_length(axis_slv_tdata);
        // Expected length is timestamp + op-word + num data words
        expected_data_words = (has_time ? 2 : 0) + 1 + hdr_hi_num_data;

        // Count remaining words until tlast
        actual_data_words = 0;
        while (!axis_slv_tlast) begin
          @(posedge rfnoc_ctrl_clk);
          while (!axis_slv_tvalid || !axis_slv_tready) @(posedge rfnoc_ctrl_clk);
          actual_data_words++;
        end

        // The remaining words should be the timestamp (2 words if present),
        // the op-word (1 word), and the data words (hdr_hi_num_data).
        `ASSERT_ERROR(
          actual_data_words == expected_data_words,
          $sformatf({"AXIS-Ctrl DUT output packet length mismatch: ",
            "expected=%0d, actual=%0d, is_ack=%0d"}, (has_time ? 2 : 0) + 1 +
            hdr_hi_num_data, actual_data_words, is_ack));
      end
    end
  end
  // ------------------------------------------------------------

  // ----------------------------
  // [Dummy AXIS-Ctrl Port Slave]
  // Receives each request packet, transforms it into a response, and
  // retransmits it:
  //   Write / BlockWrite / Sleep: response has no data words
  //   Read  / BlockRead:          response has num_data generated words
  //   WriteRead:                  response has 1 generated word
  // Status in ACK op-word = addr[19:18]
  // Generated read data   = {~addr[15:0], addr[15:0]}
  //   (BlockRead increments addr by 4 per word; Read repeats the same addr)
  // ----------------------------
  initial begin : axis_ctrl_slv
    AxisCtrlPacket pkt;
    logic [19:0]   base_addr;
    int            num_data;

    // Wait until the BFM is initialized by the main initial block.
    wait (axis_ctrl_slv_bfm != null);

    forever begin
      // Receive the next request packet.
      axis_ctrl_slv_bfm.get_ctrl(pkt);

      // Set is_ack and derive status from addr[19:18].
      pkt.header.is_ack  = 1'b1;
      pkt.op_word.status = ctrl_status_t'(pkt.op_word.address[19:18]);
      base_addr          = pkt.op_word.address;
      num_data           = int'(pkt.header.num_data);

      // Build response data words based on opcode.
      case (pkt.op_word.op_code)
        CTRL_OP_READ: begin
          pkt.data = '{};
          repeat (num_data)
            pkt.data.push_back({~base_addr[15:0], base_addr[15:0]});
        end
        CTRL_OP_BLOCK_READ: begin
          pkt.data = '{};
          for (int word_idx = 0; word_idx < num_data; word_idx++) begin
            pkt.data.push_back(
              {~(base_addr[15:0] + 16'(4 * word_idx)),
                 base_addr[15:0] + 16'(4 * word_idx)});
          end
        end
        CTRL_OP_READ_WRITE: begin
          pkt.data = '{};
          pkt.data.push_back({~base_addr[15:0], base_addr[15:0]});
        end
        default: begin  // WRITE, BLOCK_WRITE, SLEEP
          pkt.data = '{};
        end
      endcase

      // Retransmit as response.
      axis_ctrl_slv_bfm.put_ctrl(pkt);
    end
  end
  // ----------------------------

  // ----------------------------------------
  // Functional Coverage
  // ----------------------------------------
  logic [3:0] blk_status_mask    = 0;  // Track status values seen in a single
                                       // block op. blk_status_mask bit N is
                                       // set if status value N appeared as a
                                       // sub-word status.
  logic       blk_has_nonok      = 0;  // any sub-word status was non-OKAY
  int         blk_distinct_count = 0;  // number of distinct status values seen

  covergroup cg_block_status;
    // Track coverage of a getting a block op where at least one sub-word
    // returned a non-OKAY status.
    cp_nonok: coverpoint blk_has_nonok {
      bins has_nonok = {1'b1};
    }
    // Track coverage of 3 or more distinct status values appearing.
    cp_multi_status: coverpoint blk_distinct_count {
      bins three_or_more = {[3:4]};
    }
  endgroup : cg_block_status

  cg_block_status cg_blk = new();
  // ----------------------------------------


  // Task to send a ctrlport request and receive a response
  task ctrlport_transact(
    input         wr,
    input         rd,
    input  [19:0] addr,
    input  [9:0]  portid,
    input  [15:0] rem_epid,
    input  [9:0]  rem_portid,
    input  [31:0] data,
    input  [3:0]  byte_en,
    input         has_time,
    input  [63:0] timestamp,
    output [1:0]  resp_status,
    output [31:0] resp_data
  );
    if (rd | wr) begin
      cp_mst_req_wr         <= wr;
      cp_mst_req_rd         <= rd;
      cp_mst_req_addr       <= addr;
      cp_mst_req_portid     <= portid;
      cp_mst_req_rem_epid   <= rem_epid;
      cp_mst_req_rem_portid <= rem_portid;
      cp_mst_req_data       <= data;
      cp_mst_req_byte_en    <= byte_en;
      cp_mst_req_has_time   <= has_time;
      cp_mst_req_time       <= timestamp;
      @(posedge ctrlport_clk);
      cp_mst_req_wr         <= 0;
      cp_mst_req_rd         <= 0;
      while (~cp_mst_resp_ack) @(posedge ctrlport_clk);
      resp_status           = cp_mst_resp_status;
      resp_data             = cp_mst_resp_data;

      // Validate contents
      if (VERBOSE) begin
        $display("%s(addr=%0x, data=%0x, portid=%0x, has_time=%0b) = %0x (Status = %0d)",
          (rd&wr)?"WRRD":(rd?"RD":"WR"), addr, data, portid, has_time, resp_data, resp_status);
      end
      `ASSERT_ERROR(cp_mst_resp_status == addr[19:18],
        "Received Ctrlport response had the wrong status");
      if (rd) begin
        `ASSERT_ERROR(cp_mst_resp_data == {~addr[15:0], addr[15:0]},
          "Received Ctrlport response had the wrong data");
      end
    end
  endtask

  // Returns a numerical severity for priority-ordered status accumulation.
  function automatic int status_priority(ctrl_status_t status);
    case (status)
      CTRL_STS_CMDERR:  return 3;
      CTRL_STS_TSERR:   return 2;
      CTRL_STS_WARNING: return 1;
      default:          return 0;  // CTRL_STS_OKAY
    endcase
  endfunction

  // Returns whichever status has higher severity.
  function automatic ctrl_status_t accum_status(
    ctrl_status_t current,
    ctrl_status_t next_s
  );
    return (status_priority(next_s) > status_priority(current)) ? next_s : current;
  endfunction : accum_status

  // Task to send a AxisCtrl request and receive a response
  logic [5:0] cached_seq_num = 0;
  task axis_ctrl_transact(
    input  [3:0]  opcode,
    input  [19:0] addr,
    input  [9:0]  portid,
    input  [15:0] rem_epid,
    input  [9:0]  rem_portid,
    input  [31:0] data[$],
    input  [3:0]  byte_en,
    input         has_time,
    input  [63:0] timestamp,
    input  int    num_data,
    output [1:0]  resp_status,
    output [31:0] resp_data
  );
    automatic AxisCtrlPacket tx_pkt, rx_pkt = null, exp_pkt = null;
    automatic axis_ctrl_header_t header;
    automatic ctrl_op_word_t op_word;
    automatic ctrl_status_t  exp_status;
    automatic ctrl_word_t    exp_data[$];
    automatic ctrlport_request_t exp_cp_req;
    automatic ctrlport_request_t exp_cp_reqs[$];

    // Opcode specific logic
    case (ctrl_opcode_t'(opcode))
      CTRL_OP_SLEEP: begin
        if (num_data != 1) begin
          // Bad data count. Slave responds with CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else begin
          // data[0] = cycles of sleep. Limit its value to avoid long
          // simulations.
          if (data.size() > 0) data[0][31:5] = 0;
          exp_status = CTRL_STS_OKAY;
          exp_data   = '{};
        end
      end
      CTRL_OP_READ_WRITE: begin
        if (num_data != 1) begin
          // Bad num_data, slave responds with CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else begin
          exp_status = ctrl_status_t'(blk_status_mode ? addr[3:2] : 2'b00);
          exp_data = '{};
          exp_data.push_back({~addr[15:0], addr[15:0]});
          if (data.size() > 0) begin
            exp_cp_req = '{default: '0, wr: 1'b1, rd: 1'b1, addr: addr, data: data[0]};
            exp_cp_reqs.push_back(exp_cp_req);
          end
        end
      end
      CTRL_OP_WRITE: begin
        if (num_data == 0) begin
          // No write data. Slave responds with CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else if (num_data == 1) begin
          // Single write. One ctrlport request, no response data.
          exp_status = ctrl_status_t'(blk_status_mode ? addr[3:2] : 2'b00);
          exp_data   = '{};
          if (data.size() > 0) begin
            exp_cp_req = '{default: '0, wr: 1'b1, rd: 1'b0, addr: addr, data: data[0]};
            exp_cp_reqs.push_back(exp_cp_req);
          end
        end else begin
          // Multi-word write (num_data > 1). N writes at the same address (no
          // increment). Status is accumulated across all sub-writes.
          exp_status      = CTRL_STS_OKAY;
          exp_data        = '{};
          blk_status_mask = '0;
          for (int i = 0; i < data.size(); i++) begin
            automatic logic [1:0] wr_status =
              blk_status_mode ? addr[3:2] : 2'b00;
            exp_status = accum_status(exp_status, ctrl_status_t'(wr_status));
            blk_status_mask[wr_status] = 1'b1;
            exp_cp_req = '{default: '0, wr: 1'b1, rd: 1'b0, addr: addr, data: data[i]};
            exp_cp_reqs.push_back(exp_cp_req);
          end
          blk_has_nonok      = |blk_status_mask[3:1];
          blk_distinct_count = $countones(blk_status_mask);
          cg_blk.sample();
        end
      end
      CTRL_OP_READ: begin
        if (num_data == 0) begin
          // Invalid num_data. Slave will respond with CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else if (num_data == 1) begin
          // One ctrlport request, one response word.
          exp_status = ctrl_status_t'(blk_status_mode ? addr[3:2] : '0);
          exp_data = '{};
          exp_data.push_back({~addr[15:0], addr[15:0]});
          exp_cp_req = '{default: '0, wr: 1'b0, rd: 1'b1, addr: addr, data: '0};
          exp_cp_reqs.push_back(exp_cp_req);
        end else begin  // num_data > 1
          // Multi-word read. N reads at the same address (no increment).
          // Status word returns the value based on address.
          exp_status      = CTRL_STS_OKAY;
          exp_data        = '{};
          blk_status_mask = '0;
          for (int i = 0; i < num_data; i++) begin
            automatic logic [1:0] rd_status = blk_status_mode ? addr[3:2] : 2'b00;
            exp_status = accum_status(exp_status, ctrl_status_t'(rd_status));
            blk_status_mask[rd_status] = 1'b1;
            exp_data.push_back({~addr[15:0], addr[15:0]});
            exp_cp_req = '{default: '0, wr: 1'b0, rd: 1'b1, addr: addr, data: '0};
            exp_cp_reqs.push_back(exp_cp_req);
          end
          blk_has_nonok      = |blk_status_mask[3:1];
          blk_distinct_count = $countones(blk_status_mask);
          cg_blk.sample();
        end
      end
      CTRL_OP_BLOCK_WRITE: begin
        if (num_data == 0) begin
          // No write data. Slave responds with CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else begin
          // Accumulate status across all sub-writes in priority order. Write
          // responses carry no data.
          exp_status      = CTRL_STS_OKAY;
          exp_data        = '{};
          blk_status_mask = '0;
          foreach (data[idx]) begin
            automatic logic [19:0] sub_addr;
            automatic logic [ 1:0] sub_addr_status;
            sub_addr = addr + idx*4;
            sub_addr_status = blk_status_mode ? sub_addr[3:2] : '0;
            exp_status = accum_status(exp_status, ctrl_status_t'(sub_addr_status));
            blk_status_mask[sub_addr_status] = 1'b1;
            exp_cp_req = '{default: '0, wr: 1'b1, rd: 1'b0, addr: sub_addr, data: data[idx]};
            exp_cp_reqs.push_back(exp_cp_req);
          end
          blk_has_nonok      = |blk_status_mask[3:1];
          blk_distinct_count = $countones(blk_status_mask);
          cg_blk.sample();
        end
      end
      CTRL_OP_BLOCK_READ: begin
        // Accumulate status across all sub-reads in priority order. Response
        // data based on address.
        exp_data        = '{};
        blk_status_mask = '0;
        if (num_data == 0) begin
          // Invalid num_data. Slave responds with CMDERR.
          exp_status = CTRL_STS_CMDERR;
        end else begin
          exp_status = CTRL_STS_OKAY;
          for (int i = 0; i < num_data; i++) begin
            automatic logic [19:0] sa = addr + 20'(i*4);
            automatic logic [1:0] sa_status = blk_status_mode ? sa[3:2] : 2'b00;
            exp_status = accum_status(exp_status, ctrl_status_t'(sa_status));
            blk_status_mask[sa_status] = 1'b1;
            exp_data.push_back({~sa[15:0], sa[15:0]});
            exp_cp_req = '{default: '0, wr: 1'b0, rd: 1'b1, addr: sa, data: '0};
            exp_cp_reqs.push_back(exp_cp_req);
          end
          blk_has_nonok      = |blk_status_mask[3:1];
          blk_distinct_count = $countones(blk_status_mask);
          cg_blk.sample();
        end
      end
      CTRL_OP_POLL: begin
        if (num_data != 3) begin
          // Invalid num_data. Slave returns CMDERR.
          exp_status = CTRL_STS_CMDERR;
          exp_data   = '{};
        end else begin
          automatic logic [31:0] slave_val     = {~addr[15:0], addr[15:0]};
          automatic logic [31:0] poll_mask_v   = data[1];
          automatic logic        poll_hit      =
            ((slave_val & poll_mask_v) == (data[0] & poll_mask_v));
          automatic int          poll_timeout  = int'(data[2]);
          automatic int          num_reads     = poll_hit ? 1 : poll_timeout + 1;
          // The last-read register value is always returned as data[0].
          exp_data   = '{slave_val};
          exp_status = poll_hit ? CTRL_STS_OKAY : CTRL_STS_CMDERR;
          for (int i = 0; i < num_reads; i++) begin
            exp_cp_req = '{default: '0, wr: 1'b0, rd: 1'b1, addr: addr, data: '0};
            exp_cp_reqs.push_back(exp_cp_req);
          end
        end
      end
      default: begin
        // Unknown opcode results in no-op.
        exp_status = CTRL_STS_CMDERR;
        exp_data   = '{};
      end
    endcase

    // Push expected ctrlport requests to the mailbox before sending the packet
    // so the monitor can verify them as they arrive on the ctrlport bus.
    foreach (exp_cp_reqs[i]) cp_req_mbox.put(exp_cp_reqs[i]);

    // Build TX packet
    tx_pkt = new();
    header = '{
      default      : '0,
      rem_dst_port : rem_portid,
      rem_dst_epid : rem_epid,
      is_ack       : 1'b0,
      has_time     : has_time,
      seq_num      : cached_seq_num,
      num_data     : num_data,
      src_port     : THIS_PORTID,
      dst_port     : portid
    };
    op_word = '{
      default      : '0,
      status       : CTRL_STS_OKAY,
      op_code      : ctrl_opcode_t'(opcode),
      byte_enable  : byte_en,
      address      : addr
    };
    tx_pkt.write_ctrl(header, op_word, data, timestamp);

    // Always expect a response. The slave now responds with CMDERR for all
    // malformed requests (num_data=0, truncated, or unknown opcode) rather
    // than silently dropping them.
    exp_pkt = tx_pkt.copy();
    exp_pkt.header.is_ack  = 1'b1;
    exp_pkt.op_word.status = exp_status;
    exp_pkt.data.delete();
    for (int data_idx = 0; data_idx < exp_data.size(); data_idx++)
      exp_pkt.data.push_back(exp_data[data_idx]);
    exp_pkt.header.data_length = exp_data.size();
    exp_pkt.header.num_data = exp_data.size();

    if (VERBOSE) $display("*******************");
    fork
      // Send the packet
      begin
        axis_ctrl_mst_bfm.put_ctrl(tx_pkt.copy());
        if (VERBOSE) begin $display("[TRANSMITTED]"); tx_pkt.print(); end
      end
      // Wait for response only if we are expecting one
      if (exp_pkt != null) begin
        axis_ctrl_mst_bfm.get_ctrl(rx_pkt);
        if (VERBOSE) begin $display("[RECEIVED]"); rx_pkt.print(); end
      end
    join
    cached_seq_num = cached_seq_num + 1;

    // Validate contents
    if (exp_pkt != null) begin
      if (VERBOSE) begin $display("[EXPECTED]"); exp_pkt.print(); end
      `ASSERT_ERROR(exp_pkt.equal(rx_pkt),
        "Received AXIS-Ctrl packet was incorrect");
    end
  endtask


  // Generate and perform a random ctrlport transaction
  task rand_ctrlport_transact(
    output logic [1:0]  resp_status,
    output logic [31:0] resp_data
  );
    ctrlport_transact(
      $urandom_range(1),        // wr
      $urandom_range(1),        // rd
      $urandom(),               // addr
      THIS_PORTID,              // portid
      $urandom(),               // rem_epid
      $urandom(),               // rem_portid
      $urandom(),               // data
      $urandom_range(15),       // byte_en
      $urandom_range(1),        // has_time
      {$urandom(), $urandom()}, // timestamp
      resp_status,
      resp_data
    );
  endtask


  // Generate and perform a random AXIS-Ctrl transaction
  task rand_axis_ctrl_transact(
    output logic [1:0] resp_status,
    output logic [31:0] resp_data
  );
    logic [3:0]  rand_opcode;
    logic [19:0] rand_addr;
    int          rand_num_data;
    logic [31:0] data_vtr[$];
    logic [31:0] slave_val;
    logic        poll_hit;

    rand_opcode     = $urandom_range(AXIS_CTRL_OPCODE_POLL + 1); // Allow bad op-codes
    rand_addr       = $urandom();
    rand_num_data   = $urandom_range(15);
    blk_status_mode = $urandom_range(1); // 50% chance of non-OKAY statuses
    data_vtr.delete();

    if (rand_opcode == CTRL_OP_POLL) begin
      // POLL requires exactly 3 data words. Randomly choose between a
      // guaranteed match (OKAY) and a guaranteed mismatch (CMDERR).
      slave_val     = {~rand_addr[15:0], rand_addr[15:0]};
      poll_hit      = $urandom_range(1);
      rand_num_data = 3;
      data_vtr.push_back(poll_hit ? slave_val : ~slave_val); // data[0]: target
      data_vtr.push_back(32'hFFFFFFFF);                      // data[1]: mask
      data_vtr.push_back($urandom_range(10));                // data[2]: timeout
    end else if (rand_opcode == CTRL_OP_READ ||
                 rand_opcode == CTRL_OP_BLOCK_READ) begin
      // For reads, extra data words are ignored, so throw in a random number.
      repeat($urandom_range(rand_num_data)) data_vtr.push_back($urandom());
    end else begin
      // For writes, num_data should match number of words given.
      repeat(rand_num_data) data_vtr.push_back($urandom());
    end

    axis_ctrl_transact(
      rand_opcode,
      rand_addr,
      THIS_PORTID,              // portid
      $urandom(),               // rem_epid
      $urandom(),               // rem_portid
      data_vtr,
      $urandom_range(15),       // byte_en
      $urandom_range(1),        // has_time
      {$urandom(), $urandom()}, // timestamp
      rand_num_data,
      resp_status,
      resp_data
    );
  endtask


  // ----------------------------------------
  // Test Process
  // ----------------------------------------
  initial begin : main
    // Shared Variables
    // ----------------------------------------
    timeout_t    timeout;
    string       tc_label;
    logic [31:0] data_vtr[$];
    logic [1:0]  resp_status;
    logic [31:0] resp_data;

    // Initialize
    // ----------------------------------------
    test.start_tb("ctrlport_endpoint_tb", 100ms);

    // Start the clocks
    rfnoc_ctrl_clk_gen.start();
    ctrlport_clk_gen.start();

    // Start the BFMs
    axis_ctrl_mst_bfm = new(mst_req, mst_resp);
    axis_ctrl_mst_bfm.run();
    axis_ctrl_slv_bfm = new(slv_resp, slv_req);
    axis_ctrl_slv_bfm.run();

    // Reset
    // ----------------------------------------
    rfnoc_ctrl_clk_gen.reset();
    ctrlport_clk_gen.reset();

    test.start_test("Wait for reset");
    test.start_timeout(timeout, 1us, "Waiting for reset");
    while (rfnoc_ctrl_rst) @(posedge rfnoc_ctrl_clk);
    while (ctrlport_rst) @(posedge ctrlport_clk);
    test.end_timeout(timeout);
    `ASSERT_ERROR(!ctrlport_rst && !rfnoc_ctrl_rst, "Reset did not deassert");
    test.end_test();

    // AXIS-Ctrl Slave Test
    // ----------------------------------------
    // Send AXIS-Ctrl packets to the DUT and expect AXIS-Ctrl
    // responses. There is a ctrlport slave implemented above
    for (int cfg = 0; cfg < 4; cfg++) begin
      automatic logic mst_cfg = cfg[0];
      automatic logic slv_cfg = cfg[1];
      $sformat(tc_label,
        "AXIS-Ctrl Slave (%s Master, %s Slave)",
        (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
      test.start_test(tc_label);
      begin
        // Set bus stall probabilities based on configuration
        axis_ctrl_mst_bfm.set_master_stall_prob(mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        axis_ctrl_mst_bfm.set_slave_stall_prob(slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        // Test multiple transactions
        repeat(NUM_XACT_PER_TEST) begin
          test.start_timeout(timeout, 100us, "Waiting for AXIS-Ctrl transaction");
          rand_axis_ctrl_transact(resp_status, resp_data);
          test.end_timeout(timeout);
        end
      end
      test.end_test();
    end

    // AXIS-Ctrl Master Test
    // ----------------------------------------
    // Send Ctrlport packets to the DUT and expect Ctrlport
    // responses. There is a AXIS-Ctrl slave implemented above
    test.start_test("AXIS-Ctrl Master");
    // Test multiple transactions
    repeat (NUM_XACT_PER_TEST) begin
      test.start_timeout(timeout, 10us, "Waiting for Ctrlport transaction");
      rand_ctrlport_transact(resp_status, resp_data);
      test.end_timeout(timeout);
    end
    test.end_test();

    // AXIS-Ctrl Master+Slave Test
    // ----------------------------------------
    test.start_test("AXIS-Ctrl Master + Slave Simultaneously");
    begin
      axis_ctrl_mst_bfm.set_master_stall_prob(FAST_STALL_PROB);
      axis_ctrl_mst_bfm.set_slave_stall_prob(FAST_STALL_PROB);
      test.start_timeout(timeout, 100us * NUM_XACT_PER_TEST, "Waiting for test case");
      fork
        repeat (NUM_XACT_PER_TEST) begin
          rand_axis_ctrl_transact(resp_status, resp_data);
        end
        repeat (NUM_XACT_PER_TEST) begin
          rand_ctrlport_transact(resp_status, resp_data);
        end
      join
      test.end_timeout(timeout);
    end
    test.end_test();

    // Multi-word READ/WRITE Test
    // ----------------------------------------
    // Verify that READ and WRITE with num_data > 1 repeat at the same address
    // (no increment), unlike BLOCK_READ/BLOCK_WRITE which increment. The dummy
    // ctrlport slave verifies the exact address of every sub-request via
    // cp_req_mbox.
    test.start_test("Multi-word READ/WRITE (no address increment)");
    begin
      axis_ctrl_mst_bfm.set_master_stall_prob(FAST_STALL_PROB);
      axis_ctrl_mst_bfm.set_slave_stall_prob(FAST_STALL_PROB);
      blk_status_mode = 1'b0;

      // Multi-word write. 4 words to the same address. All 4 ctrlport requests
      // must arrive at addr 20'h00100 (no increment).
      data_vtr.delete();
      for (int i = 0; i < 4; i++) data_vtr[i] = 32'hA000_0000 | i;
      test.start_timeout(timeout, 100us, "Waiting for multi-word write");
      axis_ctrl_transact(
        CTRL_OP_WRITE, 20'h00100, THIS_PORTID,
        16'h0, 10'h0,
        data_vtr,
        4'hF, 1'b0, 64'h0,
        4, resp_status, resp_data
      );
      test.end_timeout(timeout);

      // Multi-word read. 7 words from the same address. All 7 ctrlport
      // requests must arrive at addr 20'h00200 (no increment). Response must
      // contain 7 copies of {~addr[15:0], addr[15:0]}.
      data_vtr.delete();
      test.start_timeout(timeout, 100us, "Waiting for multi-word read");
      axis_ctrl_transact(
        CTRL_OP_READ, 20'h00200, THIS_PORTID,
        16'h0, 10'h0,
        data_vtr,
        4'hF, 1'b0, 64'h0,
        7, resp_status, resp_data
      );
      test.end_timeout(timeout);

      // Verify BLOCK_WRITE still increments the address. 3 writes starting at
      // 20'h00300 should arrive at 20'h00300, 20'h00304, 20'h00308.
      data_vtr.delete();
      for (int i = 0; i < 3; i++) data_vtr[i] = 32'hB000_0000 | i;
      test.start_timeout(timeout, 100us, "Waiting for block write");
      axis_ctrl_transact(
        CTRL_OP_BLOCK_WRITE, 20'h00300, THIS_PORTID,
        16'h0, 10'h0,
        data_vtr,
        4'hF, 1'b0, 64'h0,
        3, resp_status, resp_data
      );
      test.end_timeout(timeout);

      // Verify BLOCK_READ still increments the address. 3 reads starting at
      // 20'h00400 should arrive at 20'h00400, 20'h00404, 20'h00408.
      data_vtr.delete();
      test.start_timeout(timeout, 100us, "Waiting for block read");
      axis_ctrl_transact(
        CTRL_OP_BLOCK_READ, 20'h00400, THIS_PORTID,
        16'h0, 10'h0,
        data_vtr,
        4'hF, 1'b0, 64'h0,
        3, resp_status, resp_data
      );
      test.end_timeout(timeout);
    end
    test.end_test();

    // Unwaited write + immediate burst read test
    // ----------------------------------------
    // Models use of burst_poke32 followed by burst_peek32 without SW waiting
    // for intervening ACK. That is, the SW collects the write ACK concurrently
    // with sending the read. This verifies axis_ctrl_slave processes both
    // packets in order and that the read response is correct.
    test.start_test("Unwaited write + immediate burst read");
    begin
      automatic AxisCtrlPacket wr_tx_pkt = new(), rd_tx_pkt = new();
      automatic AxisCtrlPacket wr_rx_pkt = null, rd_rx_pkt = null;
      automatic axis_ctrl_header_t hdr;
      automatic ctrl_op_word_t     op;
      automatic logic [31:0]       tmp_data[$];
      automatic logic [19:0]       addr = 20'h00600;
      automatic int                N    = 5;
      automatic ctrlport_request_t     req;

      axis_ctrl_mst_bfm.set_master_stall_prob(FAST_STALL_PROB);
      axis_ctrl_mst_bfm.set_slave_stall_prob(FAST_STALL_PROB);
      blk_status_mode = 1'b0;

      // Build write packet of N words, all to same addr
      tmp_data.delete();
      for (int i = 0; i < N; i++) tmp_data.push_back(32'hD000_0000 | i);
      hdr = '{ default: '0, is_ack: 1'b0, has_time: 1'b0,
               seq_num: cached_seq_num, num_data: N,
               src_port: THIS_PORTID, dst_port: THIS_PORTID };
      op  = '{ default: '0, status: CTRL_STS_OKAY, op_code: CTRL_OP_WRITE,
               byte_enable:4'hF, address:addr };
      wr_tx_pkt.write_ctrl(hdr, op, tmp_data, '0);
      cached_seq_num++;

      // Push expected ctrlport write requests
      for (int i = 0; i < N; i++) begin
        req.wr = 1'b1; req.rd = 1'b0; req.addr = addr; req.data = 32'hD000_0000 | i;
        cp_req_mbox.put(req);
      end

      // Build read packet. N words from addr.
      tmp_data.delete();  // Read request carries no data payload
      hdr = '{ default: '0, is_ack: 1'b0, has_time: 1'b0,
               seq_num: cached_seq_num, num_data: N,
               src_port: THIS_PORTID, dst_port: THIS_PORTID };
      op  = '{ default: '0, status: CTRL_STS_OKAY, op_code: CTRL_OP_READ,
               byte_enable: 4'hF, address: addr };
      rd_tx_pkt.write_ctrl(hdr, op, tmp_data, '0);
      cached_seq_num++;

      // Push expected ctrlport read requests.
      for (int i = 0; i < N; i++) begin
        req.wr = 1'b0; req.rd = 1'b1; req.addr = addr; req.data = '0;
        cp_req_mbox.put(req);
      end

      test.start_timeout(timeout, 200us, "Waiting for burst no-ack write + read");
      fork
        // Thread 1: send write then read back-to-back without waiting for write ACK
        begin
          axis_ctrl_mst_bfm.put_ctrl(wr_tx_pkt.copy());
          axis_ctrl_mst_bfm.put_ctrl(rd_tx_pkt.copy());
        end
        // Thread 2: collect write ACK then read ACK in order
        begin
          axis_ctrl_mst_bfm.get_ctrl(wr_rx_pkt);
          axis_ctrl_mst_bfm.get_ctrl(rd_rx_pkt);
        end
      join
      test.end_timeout(timeout);

      // Validate write response (multi-word write: op-word only, no data)
      `ASSERT_ERROR(wr_rx_pkt != null,
        "Burst write: ACK not received");
      `ASSERT_ERROR(wr_rx_pkt.header.is_ack == 1'b1,
        "Burst write response: is_ack not set");
      `ASSERT_ERROR(wr_rx_pkt.op_word.op_code == CTRL_OP_WRITE,
        "Burst write response: wrong opcode");
      `ASSERT_ERROR(wr_rx_pkt.op_word.status == CTRL_STS_OKAY,
        "Burst write response: wrong status");
      `ASSERT_ERROR(wr_rx_pkt.op_word.address == addr,
        "Burst write response: wrong address");
      `ASSERT_ERROR(wr_rx_pkt.data.size() == 0,
        "Burst write response: unexpected data words");

      // Validate read response
      `ASSERT_ERROR(rd_rx_pkt != null,
        "Burst read: ACK not received");
      `ASSERT_ERROR(rd_rx_pkt.header.is_ack == 1'b1,
        "Burst read response: is_ack not set");
      `ASSERT_ERROR(rd_rx_pkt.op_word.op_code == CTRL_OP_READ,
        "Burst read response: wrong opcode");
      `ASSERT_ERROR(rd_rx_pkt.op_word.status == CTRL_STS_OKAY,
        "Burst read response: wrong status");
      `ASSERT_ERROR(rd_rx_pkt.op_word.address == addr,
        "Burst read response: wrong address");
      `ASSERT_ERROR(rd_rx_pkt.data.size() == N,
        "Burst read response: wrong number of data words");
      for (int i = 0; i < N; i++) begin
        `ASSERT_ERROR(rd_rx_pkt.data[i] == {~addr[15:0], addr[15:0]},
          "Burst read response: wrong data value");
      end
    end
    test.end_test();

    // Make sure we covered all the block scenarios we expected
    `ASSERT_ERROR(cg_blk.get_coverage() == 100.0,
      "Block op functional coverage not met: not all block status scenarios were exercised");

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    rfnoc_ctrl_clk_gen.kill();
    ctrlport_clk_gen.kill();
    test.end_tb(0);

  end : main

endmodule
