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

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgAxisCtrlBfm::*;

  // Parameters
  localparam [9:0]  THIS_PORTID = 10'h17;
  localparam [15:0] THIS_EPID   = 16'hDEAD;

  localparam integer NUM_XACT_PER_TEST = 300;
  localparam integer FAST_STALL_PROB   = 0;
  localparam integer SLOW_STALL_PROB   = 50;
  localparam bit     VERBOSE           = 0;

  // Clock and Reset Definition
  bit rfnoc_ctrl_clk, rfnoc_ctrl_rst;
  bit ctrlport_clk, ctrlport_rst;

  sim_clock_gen #(6.0) rfnoc_ctrl_clk_gen (rfnoc_ctrl_clk, rfnoc_ctrl_rst); // 166.6 MHz
  sim_clock_gen #(20.0) ctrlport_clk_gen (ctrlport_clk, ctrlport_rst); // 50 MHz

  // ----------------------------------------
  // Instantiate DUT
  // ----------------------------------------
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, rfnoc_ctrl_rst);
  AxisCtrlBfm axis_ctrl_bfm;

  wire  [31:0]  axis_mst_tdata,  axis_slv_tdata , axis_req_tdata , axis_resp_tdata ;
  wire          axis_mst_tlast,  axis_slv_tlast , axis_req_tlast , axis_resp_tlast ;
  wire          axis_mst_tvalid, axis_slv_tvalid, axis_req_tvalid, axis_resp_tvalid;
  wire          axis_mst_tready, axis_slv_tready, axis_req_tready, axis_resp_tready;

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

  // Add a MUX and DEMUX on the ctrlport logic to loop responses
  // back into the endpoint and to allow external access from the
  // master and slave BFM.
  axi_mux #(
    .WIDTH(32), .SIZE(2), .PRIO(0), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
  ) mux_i (
    .clk(rfnoc_ctrl_clk), .reset(rfnoc_ctrl_rst), .clear(1'b0),
    .i_tdata ({m_ctrl.slave.tdata , axis_resp_tdata }),
    .i_tlast ({m_ctrl.slave.tlast , axis_resp_tlast }),
    .i_tvalid({m_ctrl.slave.tvalid, axis_resp_tvalid}),
    .i_tready({m_ctrl.slave.tready, axis_resp_tready}),
    .o_tdata (axis_mst_tdata ),
    .o_tlast (axis_mst_tlast ),
    .o_tvalid(axis_mst_tvalid),
    .o_tready(axis_mst_tready)
  );

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
    .o_tdata ({s_ctrl.master.tdata , axis_req_tdata }),
    .o_tlast ({s_ctrl.master.tlast , axis_req_tlast }),
    .o_tvalid({s_ctrl.master.tvalid, axis_req_tvalid}),
    .o_tready({s_ctrl.master.tready, axis_req_tready})
  );

  // --------------------------
  // [Dummy Control Port Slave]
  // Slave Model:
  // - Respond in 1 clock cycle
  // - Status = Upper 2 bits of the address
  // - Response Data = 0xFEED and Negated bottom 16 bits of addr
  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      cp_slv_resp_ack <= 1'b0;
    end else begin
      cp_slv_resp_ack <= cp_slv_req_wr | cp_slv_req_rd;
      cp_slv_resp_status <= cp_slv_req_addr[19:18];
      cp_slv_resp_data <= {16'hFEED, ~cp_slv_req_addr[15:0]};
    end
  end
  // --------------------------

  // ----------------------------
  // [Dummy AXIS-Ctrl Port Slave]
  // Slave Model:
  // - Response = Request but with the ACK bit set
  // - Status = Upper 2 bits of the address
  // - Response Data = Request Data
  reg [4:0] line_num = 5'd0;
  reg       pkt_has_time = 1'b0;
  wire pkt_hdr_line = (line_num == 5'd0);
  wire pkt_op_line = pkt_has_time ? (line_num == 5'd4) : (line_num == 5'd2);
  always @(posedge rfnoc_ctrl_clk) begin
    if (rfnoc_ctrl_rst) begin
      line_num <= 5'd0;
      pkt_has_time <= 1'b0;
    end else if (axis_req_tvalid & axis_resp_tready) begin
      if (pkt_hdr_line)
        pkt_has_time <= axis_req_tdata[30];
      line_num <= axis_req_tlast ? 5'd0 : (line_num + 1);
    end
  end
  assign axis_resp_tdata = 
    pkt_hdr_line ? {1'b1, axis_req_tdata[30:0]} : (
    pkt_op_line ? {axis_req_tdata[19:18], axis_req_tdata[29:0]} : 
    axis_req_tdata);
  assign axis_resp_tlast = axis_req_tlast;
  assign axis_resp_tvalid = axis_req_tvalid;
  assign axis_req_tready = axis_resp_tready;
  // ----------------------------

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
      `ASSERT_ERROR(cp_mst_resp_data == data,
        "Received Ctrlport response had the wrong data");
    end
  endtask

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
    output [1:0]  resp_status,
    output [31:0] resp_data
  );
    automatic AxisCtrlPacket tx_pkt, rx_pkt = null, exp_pkt = null;
    automatic axis_ctrl_header_t header;
    automatic ctrl_op_word_t op_word;
    automatic ctrl_status_t  exp_status;
    automatic ctrl_word_t    exp_data0;

    // Opcode specific logic
    case (ctrl_opcode_t'(opcode))
      CTRL_OP_SLEEP: begin
        // data[0] = cycles of sleep so limit its value
        if (data.size() > 0) data[0][31:5] = 0;
        exp_status = CTRL_STS_OKAY;
        exp_data0 = data[0];
      end
      CTRL_OP_WRITE_READ: begin
        exp_status = ctrl_status_t'(addr[19:18]);
        exp_data0 = {16'hFEED, ~addr[15:0]};
      end
      CTRL_OP_WRITE: begin
        exp_status = ctrl_status_t'(addr[19:18]);
        exp_data0 = data[0];
      end
      CTRL_OP_READ: begin
        exp_status = ctrl_status_t'(addr[19:18]);
        exp_data0 = {16'hFEED, ~addr[15:0]};
      end
      default: begin
        exp_status = CTRL_STS_CMDERR;
        exp_data0 = data[0];
      end
    endcase

    // Build TX packet
    tx_pkt = new();
    header = '{
      default      : '0,
      rem_dst_port : rem_portid,
      rem_dst_epid : rem_epid,
      is_ack       : 1'b0,
      has_time     : has_time,
      seq_num      : cached_seq_num,
      num_data     : data.size(),
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

    // Build expected packet (NULL if data vector is empty)
    if (data.size() > 0) begin
      exp_pkt = tx_pkt.copy();
      exp_pkt.header.is_ack = 1'b1;
      exp_pkt.op_word.status = exp_status;
      exp_pkt.data[0] = exp_data0;
    end

    if (VERBOSE) $display("*******************");
    fork
      // Send the packet
      begin
        axis_ctrl_bfm.put_ctrl(tx_pkt.copy());
        if (VERBOSE) begin $display("[TRANSMITTED]"); tx_pkt.print(); end
      end
      // Wait for response only if we are expecting one
      if (exp_pkt != null) begin
        axis_ctrl_bfm.get_ctrl(rx_pkt);
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

  // ----------------------------------------
  // Test Process
  // ----------------------------------------
  initial begin
    // Shared Variables
    // ----------------------------------------
    timeout_t    timeout;
    string       tc_label;
    logic [31:0] data_vtr[$];
    logic [1:0]  resp_status;
    logic [31:0] resp_data;

    // Initialize
    // ----------------------------------------
    test.start_tb("ctrlport_endpoint_tb");

    // Start the BFMs
    axis_ctrl_bfm = new(m_ctrl, s_ctrl);
    axis_ctrl_bfm.run();

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
        axis_ctrl_bfm.set_master_stall_prob(mst_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        axis_ctrl_bfm.set_slave_stall_prob(slv_cfg?SLOW_STALL_PROB:FAST_STALL_PROB);
        // Test multiple transactions
        for (int n = 0; n < NUM_XACT_PER_TEST; n++) begin
          // Generate random data for the payload
          // It is illegal in the protocol to have a zero
          // data length but we test it here to ensure no lockups
          data_vtr.delete();
          for (int i = 0; i < $urandom_range(15); i++)
            data_vtr[i] = $urandom();
          // Perform transaction
          test.start_timeout(timeout, 10us, "Waiting for AXIS-Ctrl transaction");
          axis_ctrl_transact(
            $urandom_range(5), // opcode
            $urandom(), // addr
            THIS_PORTID, // portid
            $urandom(), $urandom(), // rem_epid, rem_portid
            data_vtr,
            $urandom_range(15), // byte_en
            $urandom_range(1), // has_time
            {$urandom(), $urandom()}, // timestamp
            resp_status,
            resp_data
          );
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
    begin
      // Test multiple transactions
      for (int n = 0; n < NUM_XACT_PER_TEST * 4; n++) begin
        test.start_timeout(timeout, 10us, "Waiting for Ctrlport transaction");
        ctrlport_transact(
          $urandom_range(1), $urandom_range(1), // wr and rd
          $urandom(), // addr
          THIS_PORTID, // portid
          $urandom(), $urandom(), // rem_epid, rem_portid
          $urandom(), // data
          $urandom_range(15), // byte_en
          $urandom_range(1), // has_time
          {$urandom(), $urandom()}, // timestamp
          resp_status,
          resp_data
        );

        test.end_timeout(timeout);
      end
    end
    test.end_test();

    // AXIS-Ctrl Master+Slave Test
    // ----------------------------------------
    test.start_test("AXIS-Ctrl Master + Slave Simultaneously");
    begin
      axis_ctrl_bfm.set_master_stall_prob(FAST_STALL_PROB);
      axis_ctrl_bfm.set_slave_stall_prob(FAST_STALL_PROB);
      test.start_timeout(timeout, 10us * NUM_XACT_PER_TEST, "Waiting for test case");
      fork
        for (int n = 0; n < NUM_XACT_PER_TEST; n++) begin
          // Generate random data for the payload
          // It is illegal in the protocol to have a zero
          // data length but we test it here to ensure no lockups
          data_vtr.delete();
          for (int i = 0; i < $urandom_range(15); i++)
            data_vtr[i] = $urandom();
          // Perform transaction
          axis_ctrl_transact(
            $urandom_range(5), // opcode
            $urandom(), // addr
            THIS_PORTID, // portid
            $urandom(), $urandom(), // rem_epid, rem_portid
            data_vtr,
            $urandom_range(15), // byte_en
            $urandom_range(1), // has_time
            {$urandom(), $urandom()}, // timestamp
            resp_status,
            resp_data
          );
        end
        for (int n = 0; n < NUM_XACT_PER_TEST; n++) begin
          ctrlport_transact(
            $urandom_range(1), $urandom_range(1), // wr and rd
            $urandom(), // addr
            THIS_PORTID, // portid
            $urandom(), $urandom(), // rem_epid, rem_portid
            $urandom(), // data
            $urandom_range(15), // byte_en
            $urandom_range(1), // has_time
            {$urandom(), $urandom()}, // timestamp
            resp_status,
            resp_data
          );
        end
      join
      test.end_timeout(timeout);
    end
    test.end_test();

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb();
  end

endmodule
