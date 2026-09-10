//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_pyld_ctxt_converter_tb
//

`default_nettype none


module axis_pyld_ctxt_converter_tb #(
  parameter realtime CLK_PERIOD = 6.0,
  parameter int      ITEM_W = 64,
  parameter int      NIPC = 1,
  parameter int      CONTEXT_FIFO_SIZE = 5,
  parameter int      PAYLOAD_FIFO_SIZE = 7,
  parameter bit      CONTEXT_PREFETCH_EN = 1
);

  // ----------------------------------------
  // Global settings
  // ----------------------------------------

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrBfm::*;

  // Parameters
  localparam bit      VERBOSE           = 0;
  localparam int      CHDR_W            = 64;
  localparam int      MTU               = 7;
  localparam int      MTU_BITS          = (1 << MTU) * CHDR_W;
  localparam int      NUM_PKTS_PER_TEST = 100;
  localparam int      FAST_STALL_PROB   = 0;
  localparam int      SLOW_STALL_PROB   = 35;
  localparam realtime CHDR_CLK_PERIOD   = 3.0;
  localparam int      MAX_PYLD_W        = 256;

  // ----------------------------------------
  // Interfaces and clocks
  // ----------------------------------------

  // Clocks and resets
  bit             rfnoc_chdr_clk, rfnoc_chdr_rst;
  bit             rfnoc_data_clk, rfnoc_data_rst;
  // Common CHDR Clock
  sim_clock_gen #(CHDR_CLK_PERIOD) chdr_clk_gen_i (rfnoc_chdr_clk, rfnoc_chdr_rst);

  // Flush interface
  logic [31:0] r2c_framer_errors;
  logic [31:0] r2c_flush_timeout, c2r_flush_timeout;
  logic [0:0]  r2c_flush_en, c2r_flush_en;
  wire  [0:0]  r2c_flush_active, c2r_flush_active;
  wire  [0:0]  r2c_flush_done, c2r_flush_done;

  // CHDR interface
  wire [CHDR_W-1:0]  chdr_tdata;
  wire               chdr_tlast;
  wire               chdr_tvalid;
  wire               chdr_tready;

  // AXIS interfaces and BFMs
  AxiStreamIf  #(CHDR_W, 4)  r2c_ctxt ();
  AxiStreamIf  #(CHDR_W, 4)  c2r_ctxt ();
  AxiStreamIf  #(MAX_PYLD_W) r2c_pyld ();
  AxiStreamIf  #(MAX_PYLD_W) c2r_pyld ();
  AxiStreamBfm #(CHDR_W, 4)  ctxt_bfm;
  AxiStreamBfm #(MAX_PYLD_W) pyld_bfm;

  // Instantiate DUTs

    // Assign clocks and resets to ctxt and pyld streams
    assign r2c_ctxt.clk = rfnoc_data_clk;
    assign r2c_ctxt.rst = rfnoc_data_rst;
    assign c2r_ctxt.clk = rfnoc_data_clk;
    assign c2r_ctxt.rst = rfnoc_data_rst;

    assign r2c_pyld.clk = rfnoc_data_clk;
    assign r2c_pyld.rst = rfnoc_data_rst;
    assign c2r_pyld.clk = rfnoc_data_clk;
    assign c2r_pyld.rst = rfnoc_data_rst;

    // Instantiate clock generator
    sim_clock_gen #(CLK_PERIOD) dclk_gen (
      rfnoc_data_clk, rfnoc_data_rst
    );

    // Instantiate PyldCtxt to Chdr DUT
    axis_pyld_ctxt_to_chdr #(
      .CHDR_W               (CHDR_W),
      .ITEM_W               (ITEM_W),
      .NIPC                 (NIPC),
      .SYNC_CLKS            (CLK_PERIOD == CHDR_CLK_PERIOD),
      .CONTEXT_FIFO_SIZE    (CONTEXT_FIFO_SIZE),
      .PAYLOAD_FIFO_SIZE    (PAYLOAD_FIFO_SIZE),
      .MTU                  (MTU),
      .CONTEXT_PREFETCH_EN  (CONTEXT_PREFETCH_EN)
    ) r2c_dut (
      .axis_chdr_clk        (rfnoc_chdr_clk),
      .axis_chdr_rst        (rfnoc_chdr_rst),
      .axis_data_clk        (rfnoc_data_clk),
      .axis_data_rst        (rfnoc_data_rst),
      .m_axis_chdr_tdata    (chdr_tdata),
      .m_axis_chdr_tlast    (chdr_tlast),
      .m_axis_chdr_tvalid   (chdr_tvalid),
      .m_axis_chdr_tready   (chdr_tready),
      .s_axis_payload_tdata (r2c_pyld.slave.tdata[ITEM_W*NIPC-1:0]),
      .s_axis_payload_tkeep (r2c_pyld.slave.tkeep[NIPC-1:0]),
      .s_axis_payload_tlast (r2c_pyld.slave.tlast),
      .s_axis_payload_tvalid(r2c_pyld.slave.tvalid),
      .s_axis_payload_tready(r2c_pyld.slave.tready),
      .s_axis_context_tdata (r2c_ctxt.slave.tdata),
      .s_axis_context_tuser (r2c_ctxt.slave.tuser),
      .s_axis_context_tlast (r2c_ctxt.slave.tlast),
      .s_axis_context_tvalid(r2c_ctxt.slave.tvalid),
      .s_axis_context_tready(r2c_ctxt.slave.tready),
      .framer_errors        (r2c_framer_errors),
      .flush_en             (r2c_flush_en),
      .flush_timeout        (r2c_flush_timeout),
      .flush_active         (r2c_flush_active),
      .flush_done           (r2c_flush_done)
    );

    // Instantiate Chdr to PyldCtxt DUT
    chdr_to_axis_pyld_ctxt #(
      .CHDR_W               (CHDR_W),
      .ITEM_W               (ITEM_W),
      .NIPC                 (NIPC),
      .SYNC_CLKS            (CLK_PERIOD == CHDR_CLK_PERIOD),
      .CONTEXT_FIFO_SIZE    (CONTEXT_FIFO_SIZE),
      .PAYLOAD_FIFO_SIZE    (PAYLOAD_FIFO_SIZE),
      .CONTEXT_PREFETCH_EN  (CONTEXT_PREFETCH_EN)
    ) c2r_dut (
      .axis_chdr_clk        (rfnoc_chdr_clk),
      .axis_chdr_rst        (rfnoc_chdr_rst),
      .axis_data_clk        (rfnoc_data_clk),
      .axis_data_rst        (rfnoc_data_rst),
      .s_axis_chdr_tdata    (chdr_tdata),
      .s_axis_chdr_tlast    (chdr_tlast),
      .s_axis_chdr_tvalid   (chdr_tvalid),
      .s_axis_chdr_tready   (chdr_tready),
      .m_axis_payload_tdata (c2r_pyld.master.tdata[ITEM_W*NIPC-1:0]),
      .m_axis_payload_tkeep (c2r_pyld.master.tkeep[NIPC-1:0]),
      .m_axis_payload_tlast (c2r_pyld.master.tlast),
      .m_axis_payload_tvalid(c2r_pyld.master.tvalid),
      .m_axis_payload_tready(c2r_pyld.master.tready),
      .m_axis_context_tdata (c2r_ctxt.master.tdata),
      .m_axis_context_tuser (c2r_ctxt.master.tuser),
      .m_axis_context_tlast (c2r_ctxt.master.tlast),
      .m_axis_context_tvalid(c2r_ctxt.master.tvalid),
      .m_axis_context_tready(c2r_ctxt.master.tready),
      .flush_en             (c2r_flush_en),
      .flush_timeout        (c2r_flush_timeout),
      .flush_active         (c2r_flush_active),
      .flush_done           (c2r_flush_done)
    );

    // Assert Reset and start BFMs
    initial begin
      dclk_gen.reset();
      r2c_flush_en = 0;
      c2r_flush_en = 0;
      pyld_bfm = new(r2c_pyld, c2r_pyld);
      pyld_bfm.run();
      ctxt_bfm = new(r2c_ctxt, c2r_ctxt);
      ctxt_bfm.run();
    end

  function automatic bit pyld_pkts_equal(
    ref AxiStreamPacket #(MAX_PYLD_W) exp,
    ref AxiStreamPacket #(MAX_PYLD_W) act
  );
    if (exp.data.size() != act.data.size()) return 0;
    if (exp.keep.size() != act.keep.size()) return 0;
    for (int i = 0; i < exp.data.size(); i++) begin
      // Convert to bit
      automatic bit [MAX_PYLD_W-1:0] mask     = '0;
      automatic bit [MAX_PYLD_W-1:0] data_exp = exp.data[i];
      automatic bit [MAX_PYLD_W-1:0] data_act = act.data[i];
      for (int r = 0; r < NIPC; r++) begin
        if (exp.keep[i][r] === 1'b1) begin
          automatic bit [MAX_PYLD_W-1:0] samp_mask = '1;
          samp_mask >>= (MAX_PYLD_W-ITEM_W);
          mask |= (samp_mask << (r*ITEM_W));
        end
      end
      if (exp.keep[i] !== act.keep[i]) return 0;
      if ((data_exp&mask) !== (data_act&mask)) return 0;
    end
    return 1;
  endfunction

  task automatic send_recv_data_packets(
    input int num_pkts,
    input int mst_stall_prob,
    input int slv_stall_prob,
    input bit flushing = 0
  );
    AxiStreamPacket #(MAX_PYLD_W) pyld_pkt_arr[$] = {};
    AxiStreamPacket #(CHDR_W, 4)  ctxt_pkt_arr[$] = {};

    // Set stall probabilities
    ctxt_bfm.set_master_stall_prob(mst_stall_prob);
    ctxt_bfm.set_slave_stall_prob(slv_stall_prob);
    pyld_bfm.set_master_stall_prob(mst_stall_prob);
    pyld_bfm.set_slave_stall_prob(slv_stall_prob);

    // Generate a stream of data packets
    for (int p = 0; p < num_pkts; p++) begin
      int len_lines = $urandom_range((MTU_BITS/(ITEM_W*NIPC))-10, 1);
      int keep_int = $urandom_range(NIPC, 1);
      pyld_pkt_arr[p] = new();
      for (int i = 0; i < len_lines; i++) begin
        logic [MAX_PYLD_W-1:0] rand_samp = '0;
        logic [(MAX_PYLD_W/8)-1:0] keep_val = 'x;
        for (int r = 0; r < (((NIPC*ITEM_W)+31)/32); r++)
          rand_samp[r*32 +: 32] = $urandom();
        pyld_pkt_arr[p].data.push_back(rand_samp);
        pyld_pkt_arr[p].user.push_back('x);
        for (int r = 0; r < NIPC; r++) begin
          if (i == len_lines-1)
            keep_val[r] = (r < keep_int) ? 1'b1 : 1'b0;
          else
            keep_val[r] = 1'b1;
        end
        pyld_pkt_arr[p].keep.push_back(keep_val);
      end
    end

    // Generate context packet for each data packet
    foreach (pyld_pkt_arr[p]) begin
      automatic chdr_header_t chdr_hdr;
      automatic bit has_time = $urandom_range(1);
      automatic int num_mdata = $urandom_range(5);
      automatic int num_pyld_lines = pyld_pkt_arr[p].data.size();
      automatic int invalid_samps = 0;
      automatic int length;
      for (int r = 0; r < NIPC; r++)
        if (pyld_pkt_arr[p].keep[num_pyld_lines-1][r] === 1'b0)
          invalid_samps++;
      length =
        (CHDR_W/8) +                                        // header
        ((has_time && (CHDR_W == 64)) ? (CHDR_W/8) : 0) +   // timestamp
        (num_mdata * (CHDR_W/8)) +                          // metadata
        (num_pyld_lines * NIPC * (ITEM_W/8)) +              // payload
        (-invalid_samps * (ITEM_W/8));                      // payload (back out empty slots)

      chdr_hdr = '{
        vc        : $urandom_range(63),
        eob       : $urandom_range(1),
        eov       : $urandom_range(1),
        pkt_type  : has_time ? CHDR_DATA_WITH_TS : CHDR_DATA_NO_TS,
        num_mdata : num_mdata,
        seq_num   : p,
        length    : length,
        dst_epid  : $urandom()
      };

      ctxt_pkt_arr[p] = new();
      ctxt_pkt_arr[p].data.push_back(chdr_hdr);
      ctxt_pkt_arr[p].user.push_back((has_time && (CHDR_W > 64)) ? CONTEXT_FIELD_HDR_TS : CONTEXT_FIELD_HDR);
      ctxt_pkt_arr[p].keep.push_back('x);
      if (has_time && (CHDR_W == 64)) begin
        ctxt_pkt_arr[p].data.push_back(~p);
        ctxt_pkt_arr[p].user.push_back(CONTEXT_FIELD_TS);
        ctxt_pkt_arr[p].keep.push_back('x);
      end
      for (int i = 0; i < num_mdata; i++) begin
        ctxt_pkt_arr[p].data.push_back(i);
        ctxt_pkt_arr[p].user.push_back(CONTEXT_FIELD_MDATA);
        ctxt_pkt_arr[p].keep.push_back('x);
      end
    end

    // Spin up 4 threads: {RX, TX} x {Context, Payload}
    fork
      begin: tx_context
        timeout_t timeout;
        for (int p = 0; p < num_pkts; p++) begin
          test.start_timeout(timeout, 50us, "Waiting to send TX context pkt");
          ctxt_bfm.put(ctxt_pkt_arr[p].copy());
          test.end_timeout(timeout);
          if (VERBOSE) $display("[TxContext:%0d]\n%s", p, ctxt_pkt_arr[p].sprint());
        end
      end
      begin: tx_payload
        timeout_t timeout;
        for (int p = 0; p < num_pkts; p++) begin
          test.start_timeout(timeout, 50us, "Waiting to send TX payload pkt");
          pyld_bfm.put(pyld_pkt_arr[p].copy());
          test.end_timeout(timeout);
          if (VERBOSE) $display("[TxPayload:%0d]\n%s", p, pyld_pkt_arr[p].sprint());
        end
      end
      begin: rx_context
        if (!flushing) begin
          timeout_t timeout;
          automatic AxiStreamPacket #(CHDR_W, 4) rx_ctxt_pkt;
          for (int p = 0; p < num_pkts; p++) begin
            test.start_timeout(timeout, 50us, "Waiting to recv RX context pkt");
            ctxt_bfm.get(rx_ctxt_pkt);
            test.end_timeout(timeout);
            if (VERBOSE) $display("[RxContext:%0d]\n%s", p, rx_ctxt_pkt.sprint());
            if (VERBOSE) $display("[ExpContext:%0d]\n%s", p, ctxt_pkt_arr[p].sprint());
            `ASSERT_ERROR(ctxt_pkt_arr[p].equal(rx_ctxt_pkt), "RX context packet did not match TX");
          end
        end
      end
      begin: rx_payload
        if (!flushing) begin
          timeout_t timeout;
          automatic AxiStreamPacket #(MAX_PYLD_W) rx_pyld_pkt;
          for (int p = 0; p < num_pkts; p++) begin
            test.start_timeout(timeout, 50us, "Waiting to recv RX payload pkt");
            pyld_bfm.get(rx_pyld_pkt);
            test.end_timeout(timeout);
            if (VERBOSE) $display("[RxPayload:%0d]\n%s", p, rx_pyld_pkt.sprint());
            if (VERBOSE) $display("[ExpPayload:%0d]\n%s", p, pyld_pkt_arr[p].sprint());
            `ASSERT_ERROR(pyld_pkts_equal(pyld_pkt_arr[p], rx_pyld_pkt), "RX payload packet did not match TX");
          end
        end
      end
    join
  endtask


  // ----------------------------------------
  // Test Process
  // ----------------------------------------
  initial begin

    // Shared Variables
    // ----------------------------------------
    timeout_t    timeout;
    string       tc_label;

    // Initialize
    // ----------------------------------------
    test.start_tb($sformatf(
      "axis_pyld_ctxt_converter_tb: ITEM_W:%0d NIPC:%0d", ITEM_W, NIPC));

    // Reset
    // ----------------------------------------
    chdr_clk_gen_i.reset();

    test.start_test("Wait for reset");
    test.start_timeout(timeout, 1us, "Waiting for reset");
    while (rfnoc_chdr_rst)  @(posedge rfnoc_chdr_clk);
    while (|rfnoc_data_rst) @(posedge rfnoc_chdr_clk);
    repeat (100) @(posedge rfnoc_chdr_clk);
    test.end_timeout(timeout);
    `ASSERT_ERROR(!rfnoc_chdr_rst && !(|rfnoc_data_rst), "Reset did not deassert");
    test.end_test();

    $display("-----------------------------------------------------------------------------------------------");
    $display("Testing ITEM_W:%0d NIPC:%0d", ITEM_W, NIPC);
    $display("-----------------------------------------------------------------------------------------------");

      // Stream Random Data
      // ----------------------------------------
      for (int cfg = 0; cfg < 4; cfg++) begin
        automatic integer mst_cfg = cfg[0];
        automatic integer slv_cfg = cfg[1];
        $sformat(tc_label, "Stream Random Data (%s Mst, %s Slv)",
          (mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
        test.start_test(tc_label);
        send_recv_data_packets(NUM_PKTS_PER_TEST,
          mst_cfg ? SLOW_STALL_PROB : FAST_STALL_PROB,
          slv_cfg ? SLOW_STALL_PROB : FAST_STALL_PROB
        );
        `ASSERT_ERROR(r2c_framer_errors === '0, "Encountered framer errors");
        test.end_test();
      end

      // Flush
      // ----------------------------------------
      $sformat(tc_label, "Flush PyldCtxt => CHDR (Idle)");
      test.start_test(tc_label);
      r2c_flush_timeout = $urandom_range(400, 200);
      r2c_flush_en = 1'b1;
      repeat (100) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_active === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(r2c_flush_done === 0, "Flushing ended prematurely");
      repeat (r2c_flush_timeout + 1) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_done === 1, "Flushing did not end on time");
      r2c_flush_en = 1'b0;
      @(posedge rfnoc_chdr_clk);
      test.end_test();

      $sformat(tc_label, "Flush CHDR => PyldCtxt (Idle)");
      test.start_test(tc_label);
      c2r_flush_timeout = $urandom_range(400, 200);
      c2r_flush_en = 1'b1;
      repeat (100) @(posedge rfnoc_data_clk);
      `ASSERT_ERROR(c2r_flush_active === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(c2r_flush_done === 0, "Flushing ended prematurely");
      repeat (c2r_flush_timeout + 1) @(posedge rfnoc_data_clk);
      `ASSERT_ERROR(c2r_flush_done === 1, "Flushing did not end on time");
      c2r_flush_en = 1'b0;
      @(posedge rfnoc_data_clk);
      test.end_test();

      $sformat(tc_label, "Flush PyldCtxt => CHDR (Streaming)");
      test.start_test(tc_label);
      r2c_flush_timeout = $urandom_range(400, 200);
      r2c_flush_en = 1'b1;
      repeat (100) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_active === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(r2c_flush_done === 0, "Flushing ended prematurely");
      send_recv_data_packets(NUM_PKTS_PER_TEST/10,
        FAST_STALL_PROB, FAST_STALL_PROB, 1 /*flushing*/
      );
      repeat (NUM_PKTS_PER_TEST/10 * (1<<MTU) * 4) @(posedge rfnoc_chdr_clk);
      repeat (r2c_flush_timeout + 1) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_done === 1, "Flushing did not end on time");
      r2c_flush_en = 1'b0;
      @(posedge rfnoc_chdr_clk);
      test.end_test();

      $sformat(tc_label, "Stream Data After Flush");
      test.start_test(tc_label);
      send_recv_data_packets(NUM_PKTS_PER_TEST/10,
        FAST_STALL_PROB, FAST_STALL_PROB
      );
      `ASSERT_ERROR(r2c_framer_errors === '0, "Encountered framer errors");
      test.end_test();

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb(0);
    dclk_gen.kill();
    chdr_clk_gen_i.kill();
  end

endmodule
