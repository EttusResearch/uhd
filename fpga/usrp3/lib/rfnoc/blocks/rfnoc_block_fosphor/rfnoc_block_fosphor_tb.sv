//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fosphor_tb
//
// Description: Testbench for the fosphor RFNoC block.
//

`default_nettype none


module rfnoc_block_fosphor_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  `include "rfnoc_block_fosphor_regs.vh"

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam [31:0] NOC_ID          = 32'h666F0000;
  localparam int    CHDR_W          = 64;
  localparam int    ITEM_W          = 32;
  localparam int    NUM_PORTS_I     = 1;
  localparam int    NUM_PORTS_O     = 2;
  localparam int    MTU             = 10;
  localparam int    SPP             = 128;
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 60;      // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;     // 200 MHz
  localparam real   CTRL_CLK_PER    = 25.0;    // 40 MHz
  localparam real   CE_CLK_PER      = 5.0;     // 200 MHz

  localparam int HIST_PKT_PER_BURST = 66;   // Always 64 hist + 1 max + 1 avg
  localparam int HIST_PORT          = 0;
  localparam int WF_PORT            = 1;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER)   ce_clk_gen         (.clk(ce_clk), .rst());

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Backend Interface
  RfnocBackendIf backend (rfnoc_chdr_clk, rfnoc_ctrl_clk);

  // AXIS-Ctrl Interface
  AxiStreamIf #(32) m_ctrl (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32) s_ctrl (rfnoc_ctrl_clk, 1'b0);

  // AXIS-CHDR Interfaces
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS_I] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS_O] (rfnoc_chdr_clk, 1'b0);

  // Block Controller BFM
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // CHDR word and item/sample data types
  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_bfm_input_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
    end
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_bfm_output_connections
    initial begin
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  // DUT Slave (Input) Port Signals
  logic [CHDR_W*NUM_PORTS_I-1:0] s_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_I-1:0] s_rfnoc_chdr_tready;

  // DUT Master (Output) Port Signals
  logic [CHDR_W*NUM_PORTS_O-1:0] m_rfnoc_chdr_tdata;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tlast;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tvalid;
  logic [       NUM_PORTS_O-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT connections
  for (genvar i = 0; i < NUM_PORTS_I; i++) begin : gen_dut_input_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                     = s_rfnoc_chdr_tready[i];
  end
  for (genvar i = 0; i < NUM_PORTS_O; i++) begin : gen_dut_output_connections
    // Connect BFM slave to DUT master port
    assign s_chdr[i].tdata        = m_rfnoc_chdr_tdata[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast        = m_rfnoc_chdr_tlast[i];
    assign s_chdr[i].tvalid       = m_rfnoc_chdr_tvalid[i];
    assign m_rfnoc_chdr_tready[i] = s_chdr[i].tready;
  end

  rfnoc_block_fosphor #(
    .THIS_PORTID         (THIS_PORTID),
    .CHDR_W              (CHDR_W),
    .MTU                 (MTU)
  ) dut (
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    .ce_clk              (ce_clk),
    .rfnoc_core_config   (backend.cfg),
    .rfnoc_core_status   (backend.sts),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready (s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  typedef enum bit       { WF_MAX_HOLD, WF_AVERAGE }           wf_mode_t;
  typedef enum bit [1:0] { WF_1_1, WF_1_8, WF_1_64, WF_1_256 } wf_div_t;

  // Data structure to hold the Fosphor configuration state
  typedef struct packed {
    bit              en_wf;
    bit              en_hist;
    bit              en_noise;
    bit              en_dither;
    bit       [11:0] hist_decim;
    bit       [15:0] offset;
    bit       [15:0] scale;
    bit       [15:0] trise;
    bit       [15:0] tdecay;
    bit       [15:0] alpha;
    bit       [15:0] epsilon;
    wf_mode_t        wf_mode;
    wf_div_t         wf_div;
    bit       [ 7:0] wf_decim;
  } fosphor_config_t;

  // Default configuration copied from GNURadio
  const fosphor_config_t DEFAULT_CONFG = '{
    en_wf      : 1,
    en_hist    : 1,
    en_noise   : 0,
    en_dither  : 0,
    hist_decim : 2,
    offset     : 0,
    scale      : 256,
    trise      : 4096,
    tdecay     : 16384,
    alpha      : 65280,
    epsilon    : 2,
    wf_mode    : WF_MAX_HOLD,
    wf_div     : WF_1_8,
    wf_decim   : 2
  };


  // Rand#(WIDTH)::rand_logic() returns a WIDTH-bit random number. We avoid
  // std::randomize() due to license requirements and limited tool support.
  class Rand #(WIDTH = 32);
    
    static function logic [WIDTH-1:0] rand_bits();
      bit [WIDTH-1:0] result;
      int num_rand32 = (WIDTH + 31) / 32;
      for (int i = 0; i < num_rand32; i++) begin
        result = {result, $urandom()};
      end
      return result;
    endfunction : rand_bits

  endclass : Rand


  // Set all Fosphor registers based off the cfg data structure
  task automatic set_registers(fosphor_config_t cfg);
    blk_ctrl.reg_write(REG_ENABLE,     (int'(cfg.en_wf)     << 1) |
                                       (int'(cfg.en_hist)   << 0));
    blk_ctrl.reg_write(REG_RANDOM,     (int'(cfg.en_noise)  << 1) |
                                       (int'(cfg.en_dither) << 0));
    blk_ctrl.reg_write(REG_HIST_DECIM, cfg.hist_decim);
    blk_ctrl.reg_write(REG_OFFSET,     cfg.offset);
    blk_ctrl.reg_write(REG_SCALE,      cfg.scale);
    blk_ctrl.reg_write(REG_TRISE,      cfg.trise);
    blk_ctrl.reg_write(REG_TDECAY,     cfg.tdecay);
    blk_ctrl.reg_write(REG_ALPHA,      cfg.alpha);
    blk_ctrl.reg_write(REG_EPSILON,    cfg.epsilon);
    blk_ctrl.reg_write(REG_WF_CTRL,    (int'(cfg.wf_mode) << 7) | int'(cfg.wf_div));
    blk_ctrl.reg_write(REG_WF_DECIM,   cfg.wf_decim);
  endtask : set_registers;


  // Verify that all the Fosphor registers match the cfg data structure
  task automatic verify_registers(fosphor_config_t cfg);
    bit [31:0] value;

    blk_ctrl.reg_read(REG_ENABLE, value);
    `ASSERT_ERROR(value[1] == cfg.en_wf, "REG_ENABLE[1] didn't have expected value");
    `ASSERT_ERROR(value[0] == cfg.en_hist, "REG_ENABLE[0] didn't have expected value");
    
    blk_ctrl.reg_read(REG_CLEAR, value);
    `ASSERT_ERROR(value == 0, "REG_CLEAR didn't have expected value");
    
    blk_ctrl.reg_read(REG_RANDOM, value);
    `ASSERT_ERROR(value[1] == cfg.en_noise, "REG_RANDOM[1] didn't have expected value");
    `ASSERT_ERROR(value[0] == cfg.en_dither, "REG_RANDOM[0] didn't have expected value");
    
    blk_ctrl.reg_read(REG_HIST_DECIM, value);
    `ASSERT_ERROR(value == cfg.hist_decim, "REG_HIST_DECIM didn't have expected value");
    
    blk_ctrl.reg_read(REG_OFFSET, value);
    `ASSERT_ERROR(value == cfg.offset, "REG_OFFSET didn't have expected value");
    
    blk_ctrl.reg_read(REG_SCALE, value);
    `ASSERT_ERROR(value == cfg.scale, "REG_SCALE didn't have expected value");
    
    blk_ctrl.reg_read(REG_TRISE, value);
    `ASSERT_ERROR(value == cfg.trise, "REG_TRISE didn't have expected value");
    
    blk_ctrl.reg_read(REG_TDECAY, value);
    `ASSERT_ERROR(value == cfg.tdecay, "REG_TDECAY didn't have expected value");
    
    blk_ctrl.reg_read(REG_ALPHA, value);
    `ASSERT_ERROR(value == cfg.alpha, "REG_ALPHA didn't have expected value");
    
    blk_ctrl.reg_read(REG_EPSILON, value);
    `ASSERT_ERROR(value == cfg.epsilon, "REG_EPSILON didn't have expected value");
    
    blk_ctrl.reg_read(REG_WF_CTRL, value);
    `ASSERT_ERROR(value[7] == cfg.wf_mode, "REG_WF_CTRL didn't have expected value");
    `ASSERT_ERROR(value[1:0] == cfg.wf_div, "REG_WF_CTRL didn't have expected value");
    
    blk_ctrl.reg_read(REG_WF_DECIM, value);
    `ASSERT_ERROR(value == cfg.wf_decim, "REG_WF_DECIM didn't have expected value");
  endtask : verify_registers;


  // Generate a random Fosphor configuration to test
  task automatic randomize_cfg(output fosphor_config_t cfg, output int spp);
    // Chase a random SPP size, but make it a power of 2 (like the FFT) up to
    // the define SPP value.
    spp = 2**$urandom_range(4, $clog2(SPP));

    // Start by randomizing the entire fosphor configuration, but then
    cfg = Rand #($bits(cfg))::rand_bits();

    // Keep decimation relatively small to decrease simulation time
    cfg.hist_decim = $urandom_range(0, 8);

    // Make sure wf_mode and wf_div are valid values
    cfg.wf_mode = wf_mode_t'($urandom_range(cfg.wf_mode.num()-1));
    cfg.wf_div  = wf_div_t'($urandom_range(cfg.wf_div.num()-1));
  endtask : randomize_cfg


  // Test the passed Fosphor configuration. This updates the registers, inputs
  // num_packets of data (spp-samples each) and verifies the output.
  task automatic test_config(fosphor_config_t cfg, int num_packets, int spp);
    item_t fft_items[$];

    $display("Testing . . .");
    $display("  packets:    %0d", num_packets);
    $display("  spp:        %0d", spp);
    $display("  en_wf       %0d", cfg.en_wf);
    $display("  en_hist     %0d", cfg.en_hist);
    $display("  hist_decim: %0d", cfg.hist_decim);
    $display("  wf_decim:   %0d", cfg.wf_decim);

    // Clear any existing data
    blk_ctrl.reg_write(REG_CLEAR, 1);

    // Configure all the core's registers
    set_registers(cfg);

    // Generate packets to send
    fft_items = {};
    for (int i = 0; i < spp; i++) begin
      fft_items.push_back({
        shortint'(i),
        shortint'(0)
      });
    end

    // Send the packets
    for (int i = 0; i < num_packets; i++) begin
      blk_ctrl.send_items(0, fft_items);
    end

    fork
      begin : fork_waterfall
        item_t recv_items[$];
        int exp_num_packets;

        if (cfg.en_wf) begin
          // Calculate expected number of packets
          exp_num_packets = num_packets / (cfg.wf_decim + 2);
        end else begin
          exp_num_packets = 0;
        end

        $display("Expecting %0d waterfall packets of length %0d bytes",
          exp_num_packets, spp);

        if (exp_num_packets > 0) begin 
          for (int i = 0; i < exp_num_packets; i++) begin
            string err_string;
            blk_ctrl.recv_items(WF_PORT, recv_items);

            // We expect one byte output per sample input
            $sformat(
              err_string,
              "Waterfall packet of %0d bytes didn't match expected length of %0d bytes",
              recv_items.size()*4, spp
            );
            `ASSERT_ERROR(recv_items.size()*4 == spp, err_string);
          end
          $display("All waterfall packets received!");
        end
      end

      begin : fork_histogram
        item_t        recv_items[$];
        chdr_word_t   mdata[$];
        int           exp_num_packets;
        packet_info_t pkt_info;

        if(cfg.en_hist) begin
          // Calculate expected number of packets
          exp_num_packets = num_packets / (cfg.hist_decim + 2);
          // Round it down to a multiple of HIST_PKT_PER_BURST, since it always
          // outputs HIST_PKT_PER_BURST packets at a time.
          exp_num_packets = (exp_num_packets / HIST_PKT_PER_BURST) * HIST_PKT_PER_BURST;
        end else begin
          exp_num_packets = 0;
        end

        $display("Expecting %0d histogram packets of length %0d bytes",
          exp_num_packets, spp);

        if (exp_num_packets > 0) begin
          for (int i = 0; i < exp_num_packets; i++) begin
            string err_string;
            blk_ctrl.recv_items_adv(HIST_PORT, recv_items, mdata, pkt_info);
            //$display("Recvd hist pkt %0d", i);

            // We expect one byte output per sample input
            $sformat(
              err_string,
              "Histogram packet of %0d bytes didn't match expected length of %0d bytes",
              recv_items.size()*4, spp
            );
            `ASSERT_ERROR(recv_items.size()*4 == spp, err_string);

            // Check that the last packet of each burst has EOB set 
            if ((i+1) % HIST_PKT_PER_BURST == 0) begin
              `ASSERT_ERROR(pkt_info.eob == 1, "EOB was not set on last packet of histogram");
            end else begin
              `ASSERT_ERROR(pkt_info.eob == 0, "EOB was set on middle packet histogram");
            end
          end
          $display("All histogram packets received!");
        end
      end
    join

    // Wait until all input packets were accepted before moving on, since we
    // don't want any output from these packets to be confused with the next
    // test.
    blk_ctrl.wait_complete(0);
    #(CE_CLK_PER * SPP * 2);

    // The current Fosphor core doesn't cleanly handle transitions between
    // settings, so we reset the core before each test.
    blk_ctrl.reg_write(REG_CLEAR, 2);

  endtask : test_config


  //---------------------------------------------------------------------------
  // Test Sequences
  //---------------------------------------------------------------------------
  
  // Test that all the registers read/write as expected
  task automatic test_registers();
    fosphor_config_t cfg;

    // All registers reset to 0
    test.start_test("Test Registers (reset values)", 50us);
    cfg = '0; 
    verify_registers(cfg);
    test.end_test();

    test.start_test("Test Registers (max values)", 50us);
    cfg = '{
      en_wf      : 'h1,
      en_hist    : 'h1,
      en_noise   : 'h1,
      en_dither  : 'h1,
      hist_decim : 'hFFF,
      offset     : 'hFFFF,
      scale      : 'hFFFF,
      trise      : 'hFFFF,
      tdecay     : 'hFFFF,
      alpha      : 'hFFFF,
      epsilon    : 'hFFFF,
      wf_mode    : wf_mode_t'('h1),
      wf_div     : wf_div_t'('h3),
      wf_decim   : 'hFF
    };
    set_registers(cfg);
    verify_registers(cfg);
    test.end_test();

    test.start_test("Test Registers (default values)", 50us);
    cfg = DEFAULT_CONFG;
    set_registers(cfg);
    verify_registers(cfg);
    test.end_test();
  endtask : test_registers;


  // Test waterfall decimation settings
  task automatic test_wf_decimation();
    const int spp    = 16;
    const int num_wf = 4;
    fosphor_config_t cfg;
    int num_packets;

    test.start_test("Test waterfall decimation", 1ms);
    cfg = DEFAULT_CONFG;
    cfg.en_hist = 0;
    for (int wf_decim = 0; wf_decim < 5; wf_decim++) begin
      cfg.wf_decim = wf_decim;
      // Input enough packets to get num_wf packets out
      num_packets = (wf_decim+2) * (num_wf+1) - 1;
      test_config(cfg, num_packets, spp);
    end
    test.end_test();
  endtask : test_wf_decimation


  // Test histogram decimation settings
  task automatic test_hist_decimation();
    const int spp      = 16;
    const int num_hist = HIST_PKT_PER_BURST * 4;
    fosphor_config_t cfg;
    int num_packets;

    test.start_test("Test histogram decimation", 1ms);
    cfg = DEFAULT_CONFG;
    cfg.en_wf = 0;
    for (int hist_decim = 0; hist_decim < 5; hist_decim++) begin
      cfg.hist_decim = hist_decim;
      // Input enough packets to get num_hist packets out
      num_packets = (hist_decim+2) * (num_hist+HIST_PKT_PER_BURST/2);
      test_config(cfg, num_packets, spp);
    end
    test.end_test();
  endtask : test_hist_decimation


  // Choose num_iter random configurations and test each one
  task automatic test_rand_config(int num_iter);
    int num_packets, num_packets_wf, num_packets_hist;
    int spp;
    fosphor_config_t cfg;
    const int num_wf   = 2;                       // Get 2 waterfall packets
    const int num_hist = HIST_PKT_PER_BURST * 2;  // Get 2 histogram bursts

    test.start_test("Test random configurations", num_iter * 10ms);
    for(int i = 0; i < num_iter; i++) begin
      string str;
      $display("<<<<<<<< RANDOM ITERATION %0d >>>>>>>>", i);
      //test.current_test = $sformatf("%0d", i);

      // Choose a random configuration
      randomize_cfg(cfg, spp);

      // Only allow the output of waterfall or histogram at one time. Because
      // they operate independently and their outputs overlap, we only check
      // one at a time. This way we can end testing cleanly between output
      // packets without cutting off either the waterfall or histogram output.
      if (($urandom() & 1) == 0) begin
        cfg.en_wf   = 1;
        cfg.en_hist = 0;
        num_packets = (cfg.wf_decim+2) * (num_wf+1) - 1;
      end else begin
        cfg.en_wf   = 0;
        cfg.en_hist = 1;
        num_packets = (cfg.hist_decim+2) * (num_hist+HIST_PKT_PER_BURST/2);
      end
      test_config(cfg, num_packets, spp);
    end
    test.end_test();
  endtask : test_rand_config


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main

    // Initialize the test exec object for this testbench
    test.start_tb("rfnoc_block_fosphor_tb");

    // Start the BFMs running
    blk_ctrl.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();

    //--------------------------------
    // Verify Block Info
    //--------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------
    
    test_registers();
    test_wf_decimation();
    test_hist_decimation();
    test_rand_config(16);

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end : tb_main

endmodule : rfnoc_block_fosphor_tb


`default_nettype wire
