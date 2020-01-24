//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_pyld_ctxt_converter_tb
//

`default_nettype none


module axis_pyld_ctxt_converter_tb;

  // ----------------------------------------
  // Global settings
  // ----------------------------------------
  
  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgChdrUtils::*;
  import PkgChdrBfm::*;

  // Parameters
  localparam bit      VERBOSE           = 0;
  localparam int      CHDR_W            = 64;
  localparam int      MTU               = 7;
  localparam int      MTU_BITS          = (1 << MTU) * CHDR_W;
  localparam int      NINST             = 6;
  localparam int      START_INST        = 0;
  localparam int      STOP_INST         = NINST-1;
  localparam int      NUM_PKTS_PER_TEST = 100;
  localparam int      FAST_STALL_PROB   = 0;
  localparam int      SLOW_STALL_PROB   = 35;
  localparam realtime CHDR_CLK_PERIOD   = 3.0;
  localparam int      MAX_PYLD_W        = 256;

  typedef struct {
    realtime clk_period;
    int      item_w;
    int      nipc;
    int      ctxt_fifo;
    int      pyld_fifo;
    bit      prefetch;
  } inst_params_t;

  // Module instances to test
  localparam inst_params_t INST_PARAMS[0:NINST-1] = {
    '{clk_period: 6.0, item_w:64, nipc: 1, ctxt_fifo:5, pyld_fifo:7, prefetch:1},
    '{clk_period:20.0, item_w:32, nipc: 6, ctxt_fifo:5, pyld_fifo:1, prefetch:1},
    '{clk_period: 3.0, item_w:32, nipc: 4, ctxt_fifo:1, pyld_fifo:2, prefetch:0},
    '{clk_period:10.0, item_w:16, nipc: 4, ctxt_fifo:8, pyld_fifo:5, prefetch:1},
    '{clk_period: 3.0, item_w:32, nipc: 2, ctxt_fifo:1, pyld_fifo:7, prefetch:0},
    '{clk_period: 3.0, item_w:8,  nipc:13, ctxt_fifo:1, pyld_fifo:7, prefetch:0}
  };

  // ----------------------------------------
  // Interfaces and clocks
  // ----------------------------------------

  // Clocks and resets
  bit             rfnoc_chdr_clk, rfnoc_chdr_rst;
  bit [NINST-1:0] rfnoc_data_clk, rfnoc_data_rst;
  // Common CHDR Clock
  sim_clock_gen #(CHDR_CLK_PERIOD) chdr_clk_gen_i (rfnoc_chdr_clk, rfnoc_chdr_rst);

  // Flush interface
  logic [31:0] r2c_framer_errors[0:NINST-1];
  logic [31:0] r2c_flush_timeout[0:NINST-1], c2r_flush_timeout[0:NINST-1];
  logic [0:0]  r2c_flush_en     [0:NINST-1], c2r_flush_en     [0:NINST-1];
  wire  [0:0]  r2c_flush_active [0:NINST-1], c2r_flush_active [0:NINST-1];
  wire  [0:0]  r2c_flush_done   [0:NINST-1], c2r_flush_done   [0:NINST-1];

  // CHDR interface
  wire [CHDR_W-1:0]  chdr_tdata [0:NINST-1];
  wire               chdr_tlast [0:NINST-1];
  wire               chdr_tvalid[0:NINST-1];
  wire               chdr_tready[0:NINST-1];

  // AXIS interfaces and BFMs
  AxiStreamIf  #(CHDR_W, 4)  r2c_ctxt [0:NINST-1] ();
  AxiStreamIf  #(CHDR_W, 4)  c2r_ctxt [0:NINST-1] ();
  AxiStreamIf  #(MAX_PYLD_W) r2c_pyld [0:NINST-1] ();
  AxiStreamIf  #(MAX_PYLD_W) c2r_pyld [0:NINST-1] ();
  AxiStreamBfm #(CHDR_W, 4)  ctxt_bfm [0:NINST-1]   ;
  AxiStreamBfm #(MAX_PYLD_W) pyld_bfm [0:NINST-1]   ;

  // Instantiate DUTs
  genvar inst_i;
  generate for (inst_i = 0; inst_i < NINST; inst_i++) begin: inst

    // Assign clocks and resets to ctxt and pyld streams
    assign r2c_ctxt[inst_i].clk = rfnoc_data_clk[inst_i];
    assign r2c_ctxt[inst_i].rst = rfnoc_data_rst[inst_i];
    assign c2r_ctxt[inst_i].clk = rfnoc_data_clk[inst_i];
    assign c2r_ctxt[inst_i].rst = rfnoc_data_rst[inst_i];

    assign r2c_pyld[inst_i].clk = rfnoc_data_clk[inst_i];
    assign r2c_pyld[inst_i].rst = rfnoc_data_rst[inst_i];
    assign c2r_pyld[inst_i].clk = rfnoc_data_clk[inst_i];
    assign c2r_pyld[inst_i].rst = rfnoc_data_rst[inst_i];

    // Instantiate clock generator
    sim_clock_gen #(INST_PARAMS[inst_i].clk_period) dclk_gen (
      rfnoc_data_clk[inst_i], rfnoc_data_rst[inst_i]
    );

    // Instantiate PyldCtxt to Chdr DUT
    axis_pyld_ctxt_to_chdr #(
      .CHDR_W               (CHDR_W),
      .ITEM_W               (INST_PARAMS[inst_i].item_w),
      .NIPC                 (INST_PARAMS[inst_i].nipc),
      .SYNC_CLKS            (INST_PARAMS[inst_i].clk_period == CHDR_CLK_PERIOD),
      .CONTEXT_FIFO_SIZE    (INST_PARAMS[inst_i].ctxt_fifo),
      .PAYLOAD_FIFO_SIZE    (INST_PARAMS[inst_i].pyld_fifo),
      .MTU                  (MTU),
      .CONTEXT_PREFETCH_EN  (INST_PARAMS[inst_i].prefetch)
    ) r2c_dut (
      .axis_chdr_clk        (rfnoc_chdr_clk),
      .axis_chdr_rst        (rfnoc_chdr_rst),
      .axis_data_clk        (rfnoc_data_clk[inst_i]),
      .axis_data_rst        (rfnoc_data_rst[inst_i]),
      .m_axis_chdr_tdata    (chdr_tdata[inst_i]),
      .m_axis_chdr_tlast    (chdr_tlast[inst_i]),
      .m_axis_chdr_tvalid   (chdr_tvalid[inst_i]),
      .m_axis_chdr_tready   (chdr_tready[inst_i]),
      .s_axis_payload_tdata (r2c_pyld[inst_i].slave.tdata[(INST_PARAMS[inst_i].item_w*INST_PARAMS[inst_i].nipc)-1:0]),
      .s_axis_payload_tkeep (r2c_pyld[inst_i].slave.tkeep[INST_PARAMS[inst_i].nipc-1:0]),
      .s_axis_payload_tlast (r2c_pyld[inst_i].slave.tlast),
      .s_axis_payload_tvalid(r2c_pyld[inst_i].slave.tvalid),
      .s_axis_payload_tready(r2c_pyld[inst_i].slave.tready),
      .s_axis_context_tdata (r2c_ctxt[inst_i].slave.tdata),
      .s_axis_context_tuser (r2c_ctxt[inst_i].slave.tuser),
      .s_axis_context_tlast (r2c_ctxt[inst_i].slave.tlast),
      .s_axis_context_tvalid(r2c_ctxt[inst_i].slave.tvalid),
      .s_axis_context_tready(r2c_ctxt[inst_i].slave.tready),
      .framer_errors        (r2c_framer_errors[inst_i]),
      .flush_en             (r2c_flush_en[inst_i]),
      .flush_timeout        (r2c_flush_timeout[inst_i]),
      .flush_active         (r2c_flush_active[inst_i]),
      .flush_done           (r2c_flush_done[inst_i])
    );

    // Instantiate Chdr to PyldCtxt DUT
    chdr_to_axis_pyld_ctxt #(
      .CHDR_W               (CHDR_W),
      .ITEM_W               (INST_PARAMS[inst_i].item_w),
      .NIPC                 (INST_PARAMS[inst_i].nipc),
      .SYNC_CLKS            (INST_PARAMS[inst_i].clk_period == CHDR_CLK_PERIOD),
      .CONTEXT_FIFO_SIZE    (INST_PARAMS[inst_i].ctxt_fifo),
      .PAYLOAD_FIFO_SIZE    (INST_PARAMS[inst_i].pyld_fifo),
      .CONTEXT_PREFETCH_EN  (INST_PARAMS[inst_i].prefetch)
    ) c2r_dut (
      .axis_chdr_clk        (rfnoc_chdr_clk),
      .axis_chdr_rst        (rfnoc_chdr_rst),
      .axis_data_clk        (rfnoc_data_clk[inst_i]),
      .axis_data_rst        (rfnoc_data_rst[inst_i]),
      .s_axis_chdr_tdata    (chdr_tdata[inst_i]),
      .s_axis_chdr_tlast    (chdr_tlast[inst_i]),
      .s_axis_chdr_tvalid   (chdr_tvalid[inst_i]),
      .s_axis_chdr_tready   (chdr_tready[inst_i]),
      .m_axis_payload_tdata (c2r_pyld[inst_i].master.tdata[(INST_PARAMS[inst_i].item_w*INST_PARAMS[inst_i].nipc)-1:0]),
      .m_axis_payload_tkeep (c2r_pyld[inst_i].master.tkeep[INST_PARAMS[inst_i].nipc-1:0]),
      .m_axis_payload_tlast (c2r_pyld[inst_i].master.tlast),
      .m_axis_payload_tvalid(c2r_pyld[inst_i].master.tvalid),
      .m_axis_payload_tready(c2r_pyld[inst_i].master.tready),
      .m_axis_context_tdata (c2r_ctxt[inst_i].master.tdata),
      .m_axis_context_tuser (c2r_ctxt[inst_i].master.tuser),
      .m_axis_context_tlast (c2r_ctxt[inst_i].master.tlast),
      .m_axis_context_tvalid(c2r_ctxt[inst_i].master.tvalid),
      .m_axis_context_tready(c2r_ctxt[inst_i].master.tready),
      .flush_en             (c2r_flush_en[inst_i]),
      .flush_timeout        (c2r_flush_timeout[inst_i]),
      .flush_active         (c2r_flush_active[inst_i]),
      .flush_done           (c2r_flush_done[inst_i])
    );

    // Assert Reset and start BFMs
    initial begin
      dclk_gen.reset();
      r2c_flush_en[inst_i] = 0;
      c2r_flush_en[inst_i] = 0;
      pyld_bfm[inst_i] = new(r2c_pyld[inst_i], c2r_pyld[inst_i]);
      pyld_bfm[inst_i].run();
      ctxt_bfm[inst_i] = new(r2c_ctxt[inst_i], c2r_ctxt[inst_i]);
      ctxt_bfm[inst_i].run();
    end
  end endgenerate

  function automatic bit pyld_pkts_equal(
    ref AxiStreamPacket #(MAX_PYLD_W) exp,
    ref AxiStreamPacket #(MAX_PYLD_W) act,
    input int item_w,
    input int nipc
  );
    if (exp.data.size() != act.data.size()) return 0;
    if (exp.keep.size() != act.keep.size()) return 0;
    for (int i = 0; i < exp.data.size(); i++) begin
      // Convert to bit
      automatic bit [MAX_PYLD_W-1:0] mask     = '0;
      automatic bit [MAX_PYLD_W-1:0] data_exp = exp.data[i];
      automatic bit [MAX_PYLD_W-1:0] data_act = act.data[i];
      for (int r = 0; r < nipc; r++) begin
        if (exp.keep[i][r] === 1'b1) begin
          automatic bit [MAX_PYLD_W-1:0] samp_mask = ((1<<item_w)-1);
          mask |= (samp_mask << (r*item_w));
        end
      end
      if (exp.keep[i] !== act.keep[i]) return 0;
      if ((data_exp&mask) !== (data_act&mask)) return 0;
    end
    return 1;
  endfunction

  task automatic send_recv_data_packets(
    input int inst,
    input inst_params_t params, //We pass this separately to work around Vivado bug
    input int num_pkts,
    input int mst_stall_prob,
    input int slv_stall_prob,
    input bit flushing = 0
  );
    int nipc     = params.nipc;
    int item_w   = params.item_w;
    bit prefetch = params.prefetch;

    AxiStreamPacket #(MAX_PYLD_W) pyld_pkt_arr[$] = {};
    AxiStreamPacket #(CHDR_W, 4)  ctxt_pkt_arr[$] = {};

    // Set stall probabilities
    ctxt_bfm[inst].set_master_stall_prob(mst_stall_prob);
    ctxt_bfm[inst].set_slave_stall_prob(slv_stall_prob);
    pyld_bfm[inst].set_master_stall_prob(mst_stall_prob);
    pyld_bfm[inst].set_slave_stall_prob(slv_stall_prob);

    // Generate a stream of data packets
    for (int p = 0; p < num_pkts; p++) begin
      int len_lines = $urandom_range((MTU_BITS/(item_w*nipc))-10, 1);
      int keep_int = $urandom_range(nipc, 1);
      pyld_pkt_arr[p] = new();
      for (int i = 0; i < len_lines; i++) begin
        logic [MAX_PYLD_W-1:0] rand_samp;
        logic [(MAX_PYLD_W/8)-1:0] keep_val = 'x;
        for (int r = 0; r < (((nipc*item_w)+31)/32); r++)
          rand_samp[r*32 +: 32] = $urandom();
        pyld_pkt_arr[p].data.push_back(rand_samp);
        pyld_pkt_arr[p].user.push_back('x);
        for (int r = 0; r < nipc; r++) begin
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
      for (int r = 0; r < nipc; r++)
        if (pyld_pkt_arr[p].keep[num_pyld_lines-1][r] === 1'b0)
          invalid_samps++;
      length = 
        (CHDR_W/8) +                                        // header
        ((has_time && (CHDR_W == 64)) ? (CHDR_W/8) : 0) +   // timestamp
        (num_mdata * (CHDR_W/8)) +                          // metadata
        (num_pyld_lines * nipc * (item_w/8)) +              // payload
        (-invalid_samps * (item_w/8));                      // payload (back out empty slots)

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
          ctxt_bfm[inst].put(ctxt_pkt_arr[p].copy());
          test.end_timeout(timeout);
          if (VERBOSE) $display("[INST%0d:TxContext:%0d]\n%s", inst, p, ctxt_pkt_arr[p].sprint());
        end
      end
      begin: tx_payload
        timeout_t timeout;
        for (int p = 0; p < num_pkts; p++) begin
          test.start_timeout(timeout, 50us, "Waiting to send TX payload pkt");
          pyld_bfm[inst].put(pyld_pkt_arr[p].copy());
          test.end_timeout(timeout);
          if (VERBOSE) $display("[INST%0d:TxPayload:%0d]\n%s", inst, p, pyld_pkt_arr[p].sprint());
        end
      end
      begin: rx_context
        if (!flushing) begin
          timeout_t timeout;
          automatic AxiStreamPacket #(CHDR_W, 4) rx_ctxt_pkt;
          for (int p = 0; p < num_pkts; p++) begin
            test.start_timeout(timeout, 50us, "Waiting to recv RX context pkt");
            ctxt_bfm[inst].get(rx_ctxt_pkt);
            test.end_timeout(timeout);
            if (VERBOSE) $display("[INST%0d:RxContext:%0d]\n%s", inst, p, rx_ctxt_pkt.sprint());
            if (VERBOSE) $display("[INST%0d:ExpContext:%0d]\n%s", inst, p, ctxt_pkt_arr[p].sprint());
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
            pyld_bfm[inst].get(rx_pyld_pkt);
            test.end_timeout(timeout);
            if (VERBOSE) $display("[INST%0d:RxPayload:%0d]\n%s", inst, p, rx_pyld_pkt.sprint());
            if (VERBOSE) $display("[INST%0d:ExpPayload:%0d]\n%s", inst, p, pyld_pkt_arr[p].sprint());
            `ASSERT_ERROR(pyld_pkts_equal(pyld_pkt_arr[p], rx_pyld_pkt, item_w, nipc), "RX payload packet did not match TX");
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
    test.start_tb("axis_pyld_ctxt_converter_tb");

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

    for (int inst_num = START_INST; inst_num <= STOP_INST; inst_num++) begin
      $display("-----------------------------------------------------------------------------------------------");
      $display("Testing INST%0d:%p", inst_num, INST_PARAMS[inst_num]);
      $display("-----------------------------------------------------------------------------------------------");

      // Stream Random Data
      // ----------------------------------------
      for (int cfg = 0; cfg < 4; cfg++) begin
        automatic integer mst_cfg = cfg[0];
        automatic integer slv_cfg = cfg[1];
        $sformat(tc_label, "INST%0d: Stream Random Data (%s Mst, %s Slv)", 
          inst_num,(mst_cfg?"Slow":"Fast"), (slv_cfg?"Slow":"Fast"));
        test.start_test(tc_label);
        send_recv_data_packets(inst_num, INST_PARAMS[inst_num], NUM_PKTS_PER_TEST,
          mst_cfg ? SLOW_STALL_PROB : FAST_STALL_PROB,
          slv_cfg ? SLOW_STALL_PROB : FAST_STALL_PROB
        );
        `ASSERT_ERROR(r2c_framer_errors[inst_num] === '0, "Encountered framer errors");
        test.end_test();
      end

      // Flush
      // ----------------------------------------
      $sformat(tc_label, "INST%0d: Flush PyldCtxt => CHDR (Idle)", inst_num);
      test.start_test(tc_label);
      r2c_flush_timeout[inst_num] = $urandom_range(400, 200);
      r2c_flush_en[inst_num] = 1'b1;
      repeat (100) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_active[inst_num] === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(r2c_flush_done[inst_num] === 0, "Flushing ended prematurely");
      repeat (r2c_flush_timeout[inst_num] + 1) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_done[inst_num] === 1, "Flushing did not end on time");
      r2c_flush_en[inst_num] = 1'b0;
      @(posedge rfnoc_chdr_clk);
      test.end_test();

      $sformat(tc_label, "INST%0d: Flush CHDR => PyldCtxt (Idle)", inst_num);
      test.start_test(tc_label);
      c2r_flush_timeout[inst_num] = $urandom_range(400, 200);
      c2r_flush_en[inst_num] = 1'b1;
      repeat (100) @(posedge rfnoc_data_clk[inst_num]);
      `ASSERT_ERROR(c2r_flush_active[inst_num] === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(c2r_flush_done[inst_num] === 0, "Flushing ended prematurely");
      repeat (c2r_flush_timeout[inst_num] + 1) @(posedge rfnoc_data_clk[inst_num]);
      `ASSERT_ERROR(c2r_flush_done[inst_num] === 1, "Flushing did not end on time");
      c2r_flush_en[inst_num] = 1'b0;
      @(posedge rfnoc_data_clk[inst_num]);
      test.end_test();

      $sformat(tc_label, "INST%0d: Flush PyldCtxt => CHDR (Streaming)", inst_num);
      test.start_test(tc_label);
      r2c_flush_timeout[inst_num] = $urandom_range(400, 200);
      r2c_flush_en[inst_num] = 1'b1;
      repeat (100) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_active[inst_num] === 1, "Flushing did not begin on time");
      `ASSERT_ERROR(r2c_flush_done[inst_num] === 0, "Flushing ended prematurely");
      send_recv_data_packets(inst_num, INST_PARAMS[inst_num], NUM_PKTS_PER_TEST/10,
        FAST_STALL_PROB, FAST_STALL_PROB, 1 /*flushing*/
      );
      repeat (NUM_PKTS_PER_TEST/10 * (1<<MTU) * 4) @(posedge rfnoc_chdr_clk);
      repeat (r2c_flush_timeout[inst_num] + 1) @(posedge rfnoc_chdr_clk);
      `ASSERT_ERROR(r2c_flush_done[inst_num] === 1, "Flushing did not end on time");
      r2c_flush_en[inst_num] = 1'b0;
      @(posedge rfnoc_chdr_clk);
      test.end_test();

      $sformat(tc_label, "INST%0d: Stream Data After Flush", inst_num);
      test.start_test(tc_label);
      send_recv_data_packets(inst_num, INST_PARAMS[inst_num], NUM_PKTS_PER_TEST/10,
        FAST_STALL_PROB, FAST_STALL_PROB
      );
      `ASSERT_ERROR(r2c_framer_errors[inst_num] === '0, "Encountered framer errors");
      test.end_test();
    end

    // Finish Up
    // ----------------------------------------
    // Display final statistics and results
    test.end_tb();
  end

endmodule
