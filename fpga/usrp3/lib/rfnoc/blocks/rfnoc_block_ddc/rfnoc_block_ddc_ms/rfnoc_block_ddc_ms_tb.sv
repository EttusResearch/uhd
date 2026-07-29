//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc_ms_tb
//
// Description:  Testbench for rfnoc_block_ddc (multisample DDC, SPC > 1)
//

module rfnoc_block_ddc_ms_tb #(
  parameter int CHDR_W        = 64,
  parameter int SPC           = 2,
  parameter int NUM_PORTS     = 1,
  parameter int EXTENDED_TEST = 0
) (
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrData::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;
  import PkgMath::*;

  import rfnoc_block_ddc_ms_regs_pkg::*;
  import axi_rate_change_ms_pkg::*;
  import ddc_ms_regs_pkg::*;


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  // Block configuration
  localparam int SAMP_W           = 32;
  localparam int COMP_W           = SAMP_W/2;
  localparam int THIS_PORTID      = 'h123;
  localparam int NUM_HB           = 3;
  localparam int CIC_ORDER        = 4;
  localparam int NOC_ID           = 32'hDDC00000;
  localparam int MTU              = 8;

  // Simulation parameters
  localparam int  STALL_PROB      = 25;    // BFM stall probability
  localparam int  VERBOSE         = 0;     // Verbose output
  // ... clock rate
  localparam real CHDR_CLK_PER    = 5.0;   // CHDR clock rate
  localparam real DDC_CLK_PER     = 4.0;   // DDC IP clock rate
  // ... packetization
  localparam int  CHDR_WORD_BYTES     = CHDR_W/8;
  localparam int  PKT_SIZE_WORDS      = 128;  // CHDR words per packet
  localparam int  ITEMS_PER_CHDR_WORD = CHDR_W/SAMP_W;
  localparam int  SPP                 = PKT_SIZE_WORDS*ITEMS_PER_CHDR_WORD; // Samples per packet
  localparam int  PKT_SIZE_BYTES      = PKT_SIZE_WORDS*CHDR_WORD_BYTES; // Bytes per packet
  // ... DUT parameters
  localparam int  CIC_MAX_DECIM  = 255;  // Max CIC decimation rate
  localparam int  DDS_PHASE_W     = 24;    // DDS phase command width
  localparam int  MAX_PHASE       = (1 << DDS_PHASE_W) - 1; // Max DDS phase value
  localparam int  SCALE_FRAC_W    = 15;    // CIC scale register fractional width
  // Per HBF ramp up period in samples
  localparam int HBF_RAMP_UP  = 23;

  localparam real PHASE_TOL    = 0.01;
  // From axi_tag_time_ms: command stream latency in ce_clk cycles.
  localparam int  CMD_LATENCY  = 4;
  // Minimum guard period to queue commands to make detection easy
  localparam int  MIN_GUARD    = 10;

  initial begin : chdr_param_checks
    `ASSERT_FATAL((CHDR_W % SAMP_W) == 0,
      "CHDR_W must be an integer multiple of SAMP_W");
    `ASSERT_FATAL(((CHDR_W & (CHDR_W - 1)) == 0),
      "CHDR_W must be a power of 2");
  end

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(DDC_CLK_PER)  ddc_clk_gen        (.clk(ce_clk), .rst());


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W, SAMP_W)::chdr_word_t  chdr_word_t;
  typedef ChdrData #(CHDR_W, SAMP_W)::item_t       item_t;
  typedef ChdrData #(CHDR_W, SAMP_W)::item_queue_t item_queue_t;


  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W, SAMP_W) blk_ctrl =
    new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i], PKT_SIZE_BYTES);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS-1:0][CHDR_W-1:0] s_rfnoc_chdr_tdata;
  logic [            NUM_PORTS-1:0] s_rfnoc_chdr_tlast;
  logic [            NUM_PORTS-1:0] s_rfnoc_chdr_tvalid;
  logic [            NUM_PORTS-1:0] s_rfnoc_chdr_tready;

  logic [NUM_PORTS-1:0][CHDR_W-1:0] m_rfnoc_chdr_tdata;
  logic [            NUM_PORTS-1:0] m_rfnoc_chdr_tlast;
  logic [            NUM_PORTS-1:0] m_rfnoc_chdr_tvalid;
  logic [            NUM_PORTS-1:0] m_rfnoc_chdr_tready;

  // Map the array of BFMs to a flat vector for the DUT
  for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_dut_connections
    // Connect BFM master to DUT slave port
    assign s_rfnoc_chdr_tdata[port]  = m_chdr[port].tdata;
    assign s_rfnoc_chdr_tlast[port]  = m_chdr[port].tlast;
    assign s_rfnoc_chdr_tvalid[port] = m_chdr[port].tvalid;
    assign m_chdr[port].tready       = s_rfnoc_chdr_tready[port];

    // Connect BFM slave to DUT master port
    assign s_chdr[port].tdata        = m_rfnoc_chdr_tdata[port];
    assign s_chdr[port].tlast        = m_rfnoc_chdr_tlast[port];
    assign s_chdr[port].tvalid       = m_rfnoc_chdr_tvalid[port];
    assign m_rfnoc_chdr_tready[port] = s_chdr[port].tready;
  end

  rfnoc_block_ddc_ms #(
    .THIS_PORTID    (THIS_PORTID),
    .CHDR_W         (CHDR_W),
    .NUM_PORTS      (NUM_PORTS),
    .MTU            (MTU),
    .NUM_HB         (NUM_HB),
    .CIC_MAX_DECIM  (CIC_MAX_DECIM),
    .NIPC           (SPC)
  ) rfnoc_block_ddc_ms_i (
    .rfnoc_chdr_clk          (backend.chdr_clk),
    .ce_clk                  (ce_clk),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready),
    .rfnoc_core_config       (backend.cfg),
    .rfnoc_core_status       (backend.sts),
    .rfnoc_ctrl_clk          (backend.ctrl_clk),
    .s_rfnoc_ctrl_tdata      (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast      (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid     (m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready     (m_ctrl.tready),
    .m_rfnoc_ctrl_tdata      (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast      (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid     (s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready     (s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Ctrlport register read/writes
  task automatic write_reg_ctrlport(int unsigned port,
                           int unsigned port_base_addr,
                           int unsigned module_base_addr,
                           int unsigned addr, bit [31:0] value,
                           chdr_timestamp_t timestamp = PkgCtrlIfaceBfm::RESERVED_TS);
    blk_ctrl.reg_write(port_base_addr + module_base_addr + port*(1<<DDC_PORT_ADDR_W) + addr,
      value, timestamp);
  endtask : write_reg_ctrlport
  task automatic read_reg_ctrlport(int unsigned port,
                          int unsigned port_base_addr,
                          int unsigned module_base_addr,
                          int unsigned addr, output logic [31:0] value);
    blk_ctrl.reg_read(port_base_addr + module_base_addr + port*(1<<DDC_PORT_ADDR_W) + addr, value);
  endtask : read_reg_ctrlport
  task automatic read_reg_ctrlport_shared(int unsigned addr, output logic [31:0] value);
    blk_ctrl.reg_read(DDC_SHARED_BASE_ADDR + addr, value);
  endtask : read_reg_ctrlport_shared

  // Read a shared (non-port-specific) read-only register and assert it equals expected.
  task automatic check_shared_reg_ro(
    int unsigned addr, logic [31:0] expected, string reg_name
  );
    logic [31:0] val32;
    read_reg_ctrlport_shared(addr, val32);
    `ASSERT_ERROR(val32 == expected,
      $sformatf("%s: expected=0x%08h got=0x%08h", reg_name, expected, val32));
  endtask : check_shared_reg_ro

  // Write wr_val to a port-specific register, read it back, and assert that the
  // readback matches wr_val ANDed with mask.
  task automatic check_reg_rw(
    int unsigned port_i, int unsigned port_base,
    int unsigned mod_base, int unsigned addr,
    logic [31:0] wr_val, logic [31:0] mask, string reg_name
  );
    logic [31:0] rd32;
    write_reg_ctrlport(port_i, port_base, mod_base, addr, wr_val);
    read_reg_ctrlport (port_i, port_base, mod_base, addr, rd32);
    `ASSERT_ERROR((rd32 & mask) == (wr_val & mask),
      $sformatf("%s readback mismatch {wr=0x%08h rd=0x%08h}",
        reg_name, wr_val & mask, rd32 & mask));
  endtask : check_reg_rw

  // Tracks the number of half-band filters enabled by the last set_decim_rate call.
  // Used by other tasks to compute the HBF ramp-up sample offset.
  int hbf_enables_global = 0;
  task automatic set_decim_rate(int unsigned port, input int decim_rate);
    logic [7:0] cic_rate;
    logic [1:0] hb_enables;
    int _decim_rate;
    int scale_iq;

    cic_rate = 8'd0;
    hb_enables = 2'b0;
    _decim_rate = decim_rate;

    // Enable one half-band filter for each trailing factor-of-2 in decim_rate
    // (up to NUM_HB). The remaining factor is handled by the CIC.
    while ((_decim_rate[0] == 0) && (hb_enables < NUM_HB)) begin
      hb_enables += 1'b1;
      _decim_rate = _decim_rate >> 1;
    end
    // CIC rate cannot be 0; clamp to 1 when the HBFs account for the full rate.
    cic_rate = (_decim_rate[7:0] == 8'd0) ? 8'd1 : _decim_rate[7:0];
    `ASSERT_ERROR(
      hb_enables <= NUM_HB,
      "Enabled halfbands may not exceed total number of half bands."
    );
    `ASSERT_ERROR(
      cic_rate > 0 && cic_rate <= CIC_MAX_DECIM,
      {"CIC Decimation rate must be positive, not exceed the max cic ",
      "decimation rate, and cannot equal 0!"}
    );

    // Setup DDC
    $display("Set decimation to %0d", decim_rate);
    $display("- Number of enabled HBs: %0d", hb_enables);
    $display("- CIC Rate:              %0d", cic_rate);
    $display("- SPC:                   %0d", SPC);
    scale_iq = cic_scale_word(cic_rate);
    write_reg_ctrlport(port, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                      REG_SR_DECIM_ADDR, {hb_enables,cic_rate});
    write_reg_ctrlport(port, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                      REG_SR_SCALE_IQ_ADDR, scale_iq);

    hbf_enables_global = hb_enables;
  endtask

  // Queue timed/untimed freq shift commands to the DDS
  task automatic set_dds_freq(
    input int              port,
    input int              freq_shift,
    input chdr_timestamp_t timestamp = PkgCtrlIfaceBfm::RESERVED_TS
  );
    write_reg_ctrlport(
      port, DDC_PORT_BASE_ADDR, DDC_PORT_DDS_OFFSET, REG_DDS_FREQ_ADDR,
      freq_shift, timestamp);

    if (VERBOSE) begin
      if (timestamp == PkgCtrlIfaceBfm::RESERVED_TS) begin
        $display ("Queued untimed DDS freq shift = 0x%0X", freq_shift);
      end else begin
        $display ("Queued timed DDS freq shift = 0x%0X, timestamp = 0x%0X",
          freq_shift, timestamp);
      end
    end
  endtask : set_dds_freq

  // Convert a frequency word to a normalized frequency in the range [-0.3, 0.3)
  // freq_norm = 0.6 * tone_freq / (2^data_width) - 0.3
  function automatic real gen_freq_norm(int tone_freq, int data_width);
    return 0.6 * real'(signed'(tone_freq)) / real'(longint'(1) << data_width) - 0.3;
  endfunction : gen_freq_norm


  // Compute sample's phase in radians, in the range [-pi, pi)
  function automatic real sample_phase(input logic [SAMP_W-1:0] sample);
    logic signed [COMP_W-1:0] i_comp;
    logic signed [COMP_W-1:0] q_comp;
    begin
      i_comp = sample[SAMP_W-1 -: COMP_W];
      q_comp = sample[COMP_W-1:0];
      return $atan2(real'(q_comp), real'(i_comp));
    end
  endfunction : sample_phase

  // Wrap phase to [-pi, pi)
  function automatic real wrap_phase_rad(input real phase);
    begin
      if (phase > PI) begin
        return phase - 2.0 * PI;
      end else if (phase < -PI) begin
        return phase + 2.0 * PI;
      end else begin
        return phase;
      end
    end
  endfunction : wrap_phase_rad

  // Compute scaling factor for CIC decimation rate:
  //  CIC order-4 DC gain = R^4.
  //  The scale multiplier computes: output = (cic_24bit * cic_scale) >> SCALE_FRAC_W,
  //  so cic gain to compensate: R^4 / 2^⌈log2(R^4)⌉.
  //  DDS gain is already handled inside dds_ms, so use 1<<SCALE_FRAC_W
  //  (Q3.14 unity) as base.
  function automatic int cic_scale_word(int cic_rate);
    real         cic_scale_float;
    logic [31:0] cic_scale;
    begin
      cic_scale_float = 1.0 /((real'(cic_rate)**CIC_ORDER)/(2.0**$clog2(cic_rate**CIC_ORDER)));
      cic_scale       = int'((1<<SCALE_FRAC_W) * cic_scale_float);
      $display("IQ scaling factor for CIC rate %0d: %5f (reg value = %0d)",
                cic_rate, cic_scale_float, cic_scale);
      return cic_scale;
    end
  endfunction : cic_scale_word

  // Verify steady-state passband gain using a constant complex input.
  // port: DDC port to test.
  // decim_rate: Decimation rate to configure.
  task automatic check_constant_gain(
    input int unsigned port,
    input int unsigned decim_rate
  );
    localparam int CONSTANT_AMPL           = 4096;
    localparam int GAIN_TOLERANCE_DIVISOR = 20;
    localparam int SETTLING_SAMPLES        = 64;
    localparam int OUTPUT_SAMPLES          = SPP;
    item_t        recv_payload[$], send_payload[$];
    chdr_word_t   recv_metadata[$];
    packet_info_t recv_pkt_info, send_pkt_info;
    longint signed sum_i, sum_q;
    int num_input_samples;
    int num_accum_samples;
    int avg_i, avg_q;

    set_dds_freq(port, 32'd0);
    set_decim_rate(port, decim_rate);
    @(posedge rfnoc_block_ddc_ms_i.ce_clk);

    num_input_samples = decim_rate * OUTPUT_SAMPLES;
    send_pkt_info = '{
      vc        : 0,
      eov       : 1'b0,
      eob       : 1'b0,
      has_time  : 1'b0,
      timestamp : 64'd0
    };

    fork
      begin
        item_t all_recv_payload[$];
        do begin
          blk_ctrl.recv_items_adv(port, recv_payload, recv_metadata, recv_pkt_info);
          all_recv_payload = {all_recv_payload, recv_payload};
        end while (recv_pkt_info.eob == 0);

        sum_i = 0;
        sum_q = 0;
        num_accum_samples = 0;
        for (int idx = SETTLING_SAMPLES; idx < all_recv_payload.size(); idx++) begin
          item_t recv_item = all_recv_payload[idx];
          sum_i += signed'(recv_item[SAMP_W-1 -: COMP_W]);
          sum_q += signed'(recv_item[COMP_W-1:0]);
          num_accum_samples++;
        end
        `ASSERT_ERROR(num_accum_samples > 0, "No steady-state samples received.");
        avg_i = sum_i / num_accum_samples;
        avg_q = sum_q / num_accum_samples;
        $display("Constant gain at decimation %0d: I=%0d, Q=%0d", decim_rate, avg_i, avg_q);
        `ASSERT_ERROR(
          avg_i >= CONSTANT_AMPL - CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR &&
          avg_i <= CONSTANT_AMPL + CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR,
          $sformatf("I gain mismatch at decimation %0d: expected %0d, got %0d",
                    decim_rate, CONSTANT_AMPL, avg_i));
        `ASSERT_ERROR(
          avg_q >= CONSTANT_AMPL - CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR &&
          avg_q <= CONSTANT_AMPL + CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR,
          $sformatf("Q gain mismatch at decimation %0d: expected %0d, got %0d",
                    decim_rate, CONSTANT_AMPL, avg_q));
      end
      begin
        while (num_input_samples > 0) begin
          send_payload = {};
          for (int idx = 0; idx < SPP && num_input_samples > 0; idx++) begin
            send_payload.push_back({COMP_W'(CONSTANT_AMPL), COMP_W'(CONSTANT_AMPL)});
            num_input_samples--;
          end
          send_pkt_info.eob = (num_input_samples == 0);
          blk_ctrl.send_items(port, send_payload, {}, send_pkt_info);
          blk_ctrl.wait_complete(port);
        end
      end
    join
  endtask : check_constant_gain

  // Generate sinusoidal wave samples
  function automatic item_queue_t generate_tone_items(
    input int  length,
    input real freq_norm,
    input real ampl = 0.8
  );
    item_queue_t pkt;
    real phase_incr = freq_norm * 2.0 * PI;
    for (int i = 0; i < length; i++) begin
      real sin_val = ampl * $sin(i * phase_incr);
      real cos_val = ampl * $cos(i * phase_incr);
      logic signed [COMP_W-1:0] i_comp = COMP_W'(int'(cos_val * (2**(COMP_W-1)-1)));
      logic signed [COMP_W-1:0] q_comp = COMP_W'(int'(sin_val * (2**(COMP_W-1)-1)));
      pkt.push_back({i_comp, q_comp});
    end
    return pkt;
  endfunction : generate_tone_items

  //----------------------------------------------------------------------------
  // Test decimation with ramp waveform
  //----------------------------------------------------------------------------
  // Configure the DDC to the given decimation rate, transmit a ramp waveform,
  // and verify that the output samples form a monotonically increasing ramp.
  //
  // Parameters:
  //   port:                 Port number
  //   decim_rate:           Decimation rate to configure
  //   drop_partial_packet:  When set, expect the block to drop any partial
  //                         output packet caused by extra_samples
  //   extra_samples:        Additional input samples beyond one full output
  //                         packet, used to test partial-packet handling
  //----------------------------------------------------------------------------

  task automatic send_ramp (
    input int unsigned port,
    input int unsigned decim_rate,
    // (Optional) For testing passing through partial packets
    input logic drop_partial_packet = 1'b0,
    input int unsigned extra_samples = 0
  );
    int pkt_length; // in samples
    int sample_idx_offset; // Offset in samples based on HBF ramp up
    // Setup DDC
    set_decim_rate(port, decim_rate);
    set_dds_freq  (port, 32'd0);

    // Set sample offset based on HBF ramp up to start phase check
    sample_idx_offset = hbf_enables_global ? HBF_RAMP_UP * hbf_enables_global : 0;

    // Reduce packet length for decimation rates > 64 to reduce simulation time
    if (decim_rate > 64) begin
      pkt_length = 80;
    end else begin
      pkt_length = SPP;
    end

    // Send a short ramp, should pass through unchanged
    fork
      begin
        item_t        send_items[$];
        chdr_word_t   send_payload[$];
        packet_info_t pkt_info;

        pkt_info = 0;
        // Generate ramp: at decimated output sample k, I(k) = 2k and Q(k) = 2k+1.
        // Sending decim_rate input samples per output sample produces a clean ramp.
        for (int i = 0; i < decim_rate*(pkt_length + extra_samples); i++) begin
          send_items.push_back({
            16'(2*i/decim_rate),    // I component (upper 16 bits)
            16'((2*i+1)/decim_rate) // Q component (lower 16 bits)
          });
        end
        send_payload = ChdrData#(CHDR_W, SAMP_W)::item_to_chdr(send_items);
        $display("Send ramp (%0d words)", send_payload.size());
        pkt_info.eob = 1;
        blk_ctrl.send_packets(port, send_payload, /*data_bytes*/, /*metadata*/, pkt_info);
        blk_ctrl.wait_complete(port);
        $display("Send ramp complete");
      end
      begin
        string s;
        item_t        samples, samples_old;
        chdr_word_t   recv_payload[$], temp_payload[$];
        item_t        recv_items[$];
        chdr_word_t   metadata[$];
        int           data_bytes, temp_bytes;
        packet_info_t pkt_info;
        logic [15:0]  ramp_start_val[2];

        $display("Check ramp");
        if (~drop_partial_packet && (extra_samples > 0)) begin
          blk_ctrl.recv_adv(port, temp_payload, temp_bytes, metadata, pkt_info);
          $sformat(s, "Invalid EOB state! Expected %b, Received: %b", 1'b0, pkt_info.eob);
          `ASSERT_ERROR(pkt_info.eob == 1'b0, s);
        end
        $display("Receiving packet");
        blk_ctrl.recv_adv(port, recv_payload, data_bytes, metadata, pkt_info);
        $display("Received!");
        $sformat(s, "Invalid EOB state! Expected %b, Received: %b", 1'b1, pkt_info.eob);
        `ASSERT_ERROR(pkt_info.eob == 1'b1, s);
        if (~drop_partial_packet && (extra_samples > 0)) begin
          recv_payload = {temp_payload, recv_payload};
          recv_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(
            recv_payload, temp_bytes + data_bytes);
        end else begin
          recv_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(recv_payload, data_bytes);
        end
        if (drop_partial_packet) begin
          $sformat(s, "Incorrect packet size! Expected: %0d, Actual: %0d",
            pkt_length, recv_items.size());
          `ASSERT_ERROR(recv_items.size() == pkt_length, s);
        end else begin
          $sformat(s, "Incorrect packet size! Expected: %0d, Actual: %0d",
            pkt_length + extra_samples, recv_items.size());
          `ASSERT_ERROR(recv_items.size() == pkt_length + extra_samples, s);
        end
        samples_old       = '0;
        ramp_start_val[0] = 16'd0;
        ramp_start_val[1] = 16'd0;

        for (int idx = sample_idx_offset; idx < pkt_length; idx++) begin
          samples = recv_items[idx];
          // j=0: Q component (bits [15:0]), j=1: I component (bits [31:16])
          for (int j = 0; j < 2; j++) begin
            if (ramp_start_val[j] == 16'd0) begin
              // Zero-prefix region: wait for first non-zero value.
              if (samples[16*j +: 16] != 16'd0) begin
                samples_old[16*j +: 16]     = samples[16*j +: 16];
                ramp_start_val[j]           = samples[16*j +: 16];
              end
            end else begin
              // Ramp has started: check pattern
              $sformat(s,
                "Ramp word %0d component %0d invalid! Expected: %0d-%0d, Received: %0d",
                idx, j,
                samples_old[16*j +: 16], samples_old[16*j +: 16]+16'd4,
                samples[16*j +: 16]);
              `ASSERT_ERROR(
                (samples_old[16*j +: 16]+16'd4 >= samples[16*j +: 16]) &&
                (samples[16*j +: 16] >= samples_old[16*j +: 16]), s);
              $sformat(s,
                "Ramp plateau at zero at sample %0d, component %0d!", idx, j);
              `ASSERT_ERROR(
                (samples[16*j +: 16] != 16'd0) ||
                (samples_old[16*j +: 16] != 16'd0), s);

              samples_old[16*j +: 16] = samples[16*j +: 16];
            end
          end
        end

        for (int j = 0; j < 2; j++) begin
          // Both components must have gone non-zero before the end of the packet.
          $sformat(s,
            "No non-zero sample received for component %0d in %0d samples!", j, pkt_length);
          `ASSERT_ERROR(ramp_start_val[j] != 16'd0, s);
          // Verify an actual ramp was received: the last sample must be strictly
          // greater than the first, ruling out an all-constant non-zero output.
          $sformat(s,
            "No ramp detected on component %0d: first=%0d, last=%0d",
            j, ramp_start_val[j], recv_items[pkt_length-1][16*j +: 16]);
          `ASSERT_ERROR(
            recv_items[pkt_length-1][16*j +: 16] > ramp_start_val[j], s);
        end
        $display("Check complete");
      end
    join
  endtask

  //----------------------------------------------------------------------------
  // Multiple frequency shift test
  //----------------------------------------------------------------------------
  // Send a complex tone through the DDC and verify that timed and untimed
  // DDS frequency-shift commands take effect at the correct output sample
  // positions.  DDS is initialized with zero freq shift and 2 DDS commands are
  // issued per run:
  //   1. A non-zero freq shift applied at a randomized output sample s1.
  //   2. Back to a zero (passthrough) freq shift applied at a randomized output
  //      sample s2.
  // Guard windows around each transition are left unchecked to tolerate filter
  // settling.  Phase continuity is verified across the checked regions using
  // PHASE_TOL as the per-step tolerance.
  //
  // Parameters:
  //   port:        Port number
  //   decim_rate:  Decimation rate to configure
  //   has_time:    When set, the input packet carries a timestamp; the output
  //                packet timestamp is checked for correctness
  //----------------------------------------------------------------------------

  task automatic multi_freq_shift_test(int port, int decim_rate, bit has_time);
    begin
      const chdr_timestamp_t start_time      = 64'd0;
      const int              input_num_samps = decim_rate * SPP;

      packet_info_t      send_pkt_info, recv_pkt_info;
      chdr_word_t        recv_metadata[$];
      int                data_bytes;
      item_queue_t       send_items, recv_items;
      chdr_word_t        send_payload[$], recv_payload[$];
      // Guard in output-sample space on each side of a DDS transition:
      // MIN_GUARD * ITEMS_PER_CHDR_WORD samples.
      int                guard_samps;
      // Offset based on HBF ramp up
      int                sample_idx_offset;
      // s1, s2: output-sample positions of the first and second DDS transitions.
      // ts1, ts2: corresponding input-sample timestamps sent to the DDS.
      int                s1, s2, ts1, ts2;
      int                phase_check_start;   // first sample to check DDS-active phase
      int                switch_check_stop;   // exclusive end of DDS-active check region
      int                switch_check_start;  // first sample to check post-reset phase
      int                fire_after_samps;    // input-sample count when second cmd is sent
      int                min_fire_after_samps, max_fire_after_samps;

      logic [DDS_PHASE_W-1:0] freq_shift_phase_word;
      real                    tone_freq_norm;
      real                    dds_freq_norm;
      real                    freq_shift_norm;

      set_decim_rate(port, decim_rate);

      // Keep post-decimation frequency comfortably away from Nyquist.
      tone_freq_norm        = gen_freq_norm($urandom_range(MAX_PHASE), DDS_PHASE_W) /
                              (2.0 * decim_rate);
      dds_freq_norm         = gen_freq_norm($urandom_range(MAX_PHASE), DDS_PHASE_W) /
                              (2.0 * decim_rate);
      freq_shift_phase_word = dds_freq_norm * (1 << DDS_PHASE_W);
      freq_shift_norm       = decim_rate * (tone_freq_norm + dds_freq_norm);
      $display("Tone freq norm = %f, DDS freq norm = %f", tone_freq_norm, dds_freq_norm);

      // Set sample offset based on HBF ramp up to start phase check
      sample_idx_offset = hbf_enables_global ? HBF_RAMP_UP * hbf_enables_global : 0;

      // Each DDS transition point is chosen in output-sample space within
      // [guard_samps, SPP - guard_samps - 1].  A guard_samps-wide unchecked
      // window is left on both sides of each transition, so checked regions are:
      //   DDS-active/non-zero freq shift : [s1+guard_samps,  s2-guard_samps)
      //   Passthrough/Zero freq shift    : [s2+guard_samps,  SPP)
      // s1 is picked freely within its valid range; s2 is assigned if possible.
      guard_samps = MIN_GUARD* ITEMS_PER_CHDR_WORD + sample_idx_offset;

      `ASSERT_FATAL(guard_samps < SPP - guard_samps,
        "Timed DDS test: SPP too small for the required guard regions");
      s1  = $urandom_range(SPP/4, guard_samps);
      ts1 = s1 * decim_rate;
      phase_check_start  = s1 + guard_samps;

      switch_check_start = SPP;  // default to no passthrough check if s2 is not assigned
      switch_check_stop  = SPP; // default to DDS-active check if s2 is not assigned
      if (phase_check_start + 2*guard_samps <= SPP - guard_samps - 1) begin
        s2 = $urandom_range(SPP - guard_samps - 1, phase_check_start + 2*guard_samps);
        switch_check_stop  = s2 - guard_samps;
        switch_check_start = s2 + guard_samps;
      end else s2 = SPP;
      ts2 = s2 * decim_rate;

      // Send the second timed command after ts1 but early enough to arrive
      // before ts2 (CMD_LATENCY SPC-cycles of pipeline latency each way).
      min_fire_after_samps = ts1 + CMD_LATENCY*SPC;
      max_fire_after_samps = ts2 - CMD_LATENCY*SPC;
      `ASSERT_FATAL(min_fire_after_samps < max_fire_after_samps,
        "Timed DDS test: not enough input samples between the two frequency commands");
      fire_after_samps = $urandom_range(max_fire_after_samps, min_fire_after_samps);

      if (VERBOSE) begin
        $display($sformatf({"DDS freq shift input sample = %0d, output sample = %0d, ",
                            "check from output sample = %0d"},
                            ts1, s1, phase_check_start));
        if (s2 < SPP)
          $display($sformatf({"DDS freq reset input sample = %0d, output sample = %0d, ",
                              "active check stops before = %0d, reset check from = %0d"},
                              ts2, s2, switch_check_stop, switch_check_start));
      end

      set_dds_freq(port, 32'd0);
      if (VERBOSE)
        $display ($sformatf({"Queue timed DDS freq shift command at sample offset %0d",
          " (timestamp = 0x%0X)"}, ts1, start_time + ts1));
      set_dds_freq(port, {freq_shift_phase_word, 8'b0}, start_time + ts1);

      fork
        begin : send_complex_tone
          send_items   = generate_tone_items(input_num_samps, tone_freq_norm);
          send_payload = ChdrData#(CHDR_W, SAMP_W)::item_to_chdr(send_items);

          send_pkt_info           = 0;
          send_pkt_info.has_time  = has_time;
          send_pkt_info.timestamp = start_time;
          send_pkt_info.eob       = 1'b1;
          blk_ctrl.send_packets(port, send_payload, /*data_bytes*/,
                                /*metadata*/, send_pkt_info);
          blk_ctrl.wait_complete(port);

          $display("Sent complex tone");
        end

        begin : recv_packet
          string s;
          real   recv_phase;
          real   prev_recv_phase;
          real   phase_step;
          real   phase_err;
          real   exp_phase_step;
          bit    dds_active;
          bit    have_prev_phase;

          $display("Check incoming samples");

          blk_ctrl.recv_adv(port, recv_payload, data_bytes, recv_metadata, recv_pkt_info);
          `ASSERT_ERROR(recv_pkt_info.eob == 1'b1, "Received packet is not EOB as expected");

          recv_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(recv_payload, data_bytes);

          $sformat(s, "Incorrect packet size! Expected: %0d, Actual: %0d",
            SPP, recv_items.size());
          `ASSERT_ERROR(recv_items.size() == SPP, s);

          if (has_time) begin
            $sformat(s,
              "Incorrect timestamp: has_time = %0d, timestamp = 0x%0X, expected 0x%0X",
              recv_pkt_info.has_time, recv_pkt_info.timestamp, start_time);
            `ASSERT_ERROR(
              recv_pkt_info.has_time == 1 && recv_pkt_info.timestamp == start_time,
              s);
          end else begin
            `ASSERT_ERROR(
              recv_pkt_info.has_time == 0,
              "Packet has timestamp when it shouldn't");
          end

          for (int sample_idx = sample_idx_offset;
              sample_idx < recv_items.size(); sample_idx++) begin
            recv_phase = sample_phase(recv_items[sample_idx]);
            if (have_prev_phase) begin
              dds_active = (sample_idx >= phase_check_start) &&
                           (sample_idx < switch_check_stop);
              exp_phase_step = dds_active ? 2.0 * PI * freq_shift_norm :
                              2.0 * PI * tone_freq_norm * decim_rate;
              exp_phase_step = wrap_phase_rad(exp_phase_step);
              phase_step     = wrap_phase_rad(recv_phase - prev_recv_phase);
              phase_err      = wrap_phase_rad(phase_step - exp_phase_step);

              if (VERBOSE)
                $display($sformatf({"sample %0d DDS active = %0d, ",
                  "exp_phase_step = %f, recv_phase_step = %f"},
                  sample_idx, dds_active, exp_phase_step, phase_step));

              if ((sample_idx >= phase_check_start && sample_idx < switch_check_stop) ||
                  (sample_idx >= switch_check_start)) begin
                s = $sformatf(
                  {"Phase step mismatch at sample %0d: ",
                    "DDS active = %0d, expected = %f, received = %f, error = %f"},
                  sample_idx, dds_active, exp_phase_step, phase_step, phase_err);
                `ASSERT_ERROR(phase_err >= -PHASE_TOL && phase_err <= PHASE_TOL, s);
              end
            end
            prev_recv_phase = recv_phase;
            have_prev_phase = 1'b1;
          end
        end

        begin : update_dds_freq
          if (switch_check_start < SPP) begin
            repeat (fire_after_samps / SPC) @(posedge ce_clk);
            if (VERBOSE)
              $display($sformatf({"Queue timed DDS freq reset at sample offset %0d ",
                      "(timestamp = 0x%0X, cmd sent after ~%0d samples)..."},
                      ts2, start_time + ts2, fire_after_samps));
            set_dds_freq(port, 0, start_time + ts2);
          end
        end
      join
    end
  endtask : multi_freq_shift_test

  //---------------------------------------------------------------------------
  // Send tone
  //---------------------------------------------------------------------------
  // Generates samples for a sine wave tone and sends them to the specified BFM
  // port.
  //
  // Parameters:
  //   port:              Port number
  //   decim_rate:        Decimation rate
  //   tone_ampl:         Amplitude of generated sine wave
  //   tone_freq_norm:    Normalized frequency of generated sine wave
  //   dsp_tune_norm:     Normalized digital frequency shift
  //   num_samps_to_send: Number of samples to send
  //   restrict_input:    Coerce input tone samples to 16-bit signed range
  //
  // Note: The tone_freq_norm and dsp_tune_norm parameters must be in the range
  // (-1/2, +1/2).
  //---------------------------------------------------------------------------
  task automatic send_tone (
    input int unsigned port              = 0,
    input int unsigned decim_rate        = 1,
    input real         tone_ampl         = 0.9,
    input real         tone_freq_norm    = 0.0,
    input real         dsp_tune_norm     = 0.0,
    input int          num_samps_to_send = 1000,
    input int          restrict_input    = 1
  );
    chdr_word_t     recv_metadata[$];
    item_t          recv_payload[$], send_payload[$];
    packet_info_t   recv_pkt_info, send_pkt_info;
    logic [31:0]    dsp_tune_word;


    // Tuning word = F_shift/Fs * 2^32 (Shift freq must be [-Fs/2,Fs/2))
    dsp_tune_word = int'(dsp_tune_norm * 2.0**32);
    $display("send_tone(): dsp_tune_word = %d", dsp_tune_word);
    set_dds_freq(port, dsp_tune_word);

    set_decim_rate(port, decim_rate);

    // Allow register writes to propagate before the data stream begins.
    @(posedge rfnoc_block_ddc_ms_i.ce_clk);
    if (num_samps_to_send > 0) begin
      fork
      // Receive samples
      begin
        automatic item_t prev_item, curr_item;
        real prev, curr;
        do begin
          blk_ctrl.recv_items_adv(port, recv_payload, recv_metadata, recv_pkt_info);
        end while (recv_pkt_info.eob == 0);
        // Check for jumps in I/Q data
        prev_item = recv_payload.pop_front();
        foreach (recv_payload[recv_idx]) begin
          curr_item = recv_payload.pop_front();
          // Compare previous sample data to current sample data. If delta is
          // more than half of the 16-bit signed value range(e.g. SHORT_MAX),
          // we assume an arithmetic overflow has occurred.
          prev = real'(signed'(prev_item[SAMP_W/2+:SAMP_W/2]));
          curr = real'(signed'(curr_item[SAMP_W/2+:SAMP_W/2]));
          `ASSERT_ERROR((Math#(real)::abs(curr-prev) < SHORT_MAX/2),
            "Detected jump in I data.");
          prev = real'(signed'(prev_item[0+:SAMP_W/2]));
          curr = real'(signed'(curr_item[0+:SAMP_W/2]));
          `ASSERT_ERROR((Math#(real)::abs(curr-prev) < SHORT_MAX/2),
            "Detected jump in Q data.");
          prev_item = curr_item;
        end
      end
      // Send tone
      begin
        typedef   logic [15:0] logic_t; //needed for typecasting to packed logic array
        automatic real         i_float, q_float;
        automatic logic [15:0] i, q;
        automatic longint      phase = 0;

        send_pkt_info = '{
          vc         : 0,
          eov        : 1'b0,
          eob        : 1'b0,
          has_time   : 1'b0,
          timestamp  : 64'd0
        };

        while (num_samps_to_send > 0) begin
          send_payload = {}; // Clear out previous iteration's samples
          for (int n = 0; n < SPP; n++) begin
            i_float = tone_ampl*(2.0**15)*$cos(2*PI*phase*tone_freq_norm);
            q_float = tone_ampl*(2.0**15)*$sin(2*PI*phase*tone_freq_norm);
            phase++;
            // if restrict_input is set, we need to coerce the float values to
            // 16-bit signed values instead of cutting off the additional MSBs,
            // ensuring a smooth transition in the tone.
            if (restrict_input) begin
              i = logic_t'(coerce_to_int16(i_float));
              q = logic_t'(coerce_to_int16(q_float));
            end else begin
              i = logic_t'(i_float);
              q = logic_t'(q_float);
            end
            send_payload.push_back({i,q});

            num_samps_to_send--;
            if (num_samps_to_send == 0) begin
              break;
            end
          end

          send_pkt_info.eob = (num_samps_to_send == 0);
          blk_ctrl.send_items(port, send_payload, {}, send_pkt_info);
          blk_ctrl.wait_complete(port);
        end
      end
      join
    end
  endtask


  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    static int unsigned port = 0;
    test.start_tb($sformatf("rfnoc_block_ddc_tb with SPC=%0d, NUM_PORTS=%0d",
                  SPC, NUM_PORTS));

    // Start the BFMs running
    blk_ctrl.run();


    //-------------------------------------------------------------------------
    // Reset
    //-------------------------------------------------------------------------

    test.start_test("Wait for Reset", 10us);
    fork
      blk_ctrl.reset_chdr();
      blk_ctrl.reset_ctrl();
    join;
    test.end_test();


    //-------------------------------------------------------------------------
    // Check NoC ID and Block Info
    //-------------------------------------------------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU Value");
    test.end_test();


    //-------------------------------------------------------------------------
    // Test read-back regs
    //-------------------------------------------------------------------------
    test.start_test("Test readback of readonly registers", 10us);
    // Check read-only registers for known default values.
    check_shared_reg_ro(REG_NUM_HB,        NUM_HB,       "REG_NUM_HB");
    check_shared_reg_ro(REG_CIC_MAX_DECIM, CIC_MAX_DECIM, "REG_CIC_MAX_DECIM");
    check_shared_reg_ro(REG_SPC,           SPC,           "REG_SPC");
    test.end_test();

    //-------------------------------------------------------------------------
    // Write + readback test for writable registers
    //-------------------------------------------------------------------------
    test.start_test("Write/readback all writable regs", 100us);
    for (int unsigned port_i = 0; port_i < NUM_PORTS; port_i++) begin
      logic [31:0] freq_val;
      logic [31:0] scale_val;
      logic [31:0] decim_val;
      logic [31:0] mux_val;
      logic [31:0] time_incr_val;
      logic [31:0] rd32;

      $display("... port %0d", port_i);

      // Test values are unique per port and pre-masked to each register's field width.
      freq_val      = (32'h0101_0202 * (port_i + 1)) & 32'hFFFF_FF00;
      scale_val     = (32'h0303_0404 * (port_i + 1)) & 32'h0003_FFFF;
      decim_val     = (32'h0505_0606 * (port_i + 1)) & 32'h0000_03FF;
      mux_val       = (32'h0707_0808 * (port_i + 1)) & 32'h0000_0003;
      time_incr_val = (32'h1111_1212 * (port_i + 1)) & 32'h0000_07FF;

      // SR registers
      check_reg_rw(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                   REG_SR_DECIM_ADDR,    decim_val, 32'h0000_03FF, "REG_SR_DECIM_ADDR");
      check_reg_rw(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                   REG_SR_MUX_ADDR,      mux_val,   32'h0000_0003, "REG_SR_MUX_ADDR");
      check_reg_rw(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                   REG_SR_SCALE_IQ_ADDR, scale_val, 32'h0003_FFFF, "REG_SR_SCALE_IQ_ADDR");

      // AXI rate-change register (full-width, no masking needed)
      check_reg_rw(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_AXI_RATE_OFFSET,
                   REG_AXI_RATE_SR_TIME_INCR_ADDR, time_incr_val, 32'hFFFF_FFFF,
                   "REG_AXI_RATE_SR_TIME_INCR_ADDR");

      // DDS frequency register is write-only; readback must return 0.
      write_reg_ctrlport(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_DDS_OFFSET,
                         REG_DDS_FREQ_ADDR, freq_val);
      read_reg_ctrlport (port_i, DDC_PORT_BASE_ADDR, DDC_PORT_DDS_OFFSET,
                         REG_DDS_FREQ_ADDR, rd32);
      `ASSERT_ERROR(rd32 == '0, "REG_DDS_FREQ_ADDR is write-only; expected readback 0");

      // Flush the pending freq_val phase through dds_ms: configure passthrough
      // (CIC=1, no HBFs, unity scale) and send a short zero-amplitude EOB
      // burst so axi_tag_time_ms dispatches the command before TC5/TC6 start.
      begin
        item_t        flush_items[$];
        item_t        discard_items[$];
        chdr_word_t   discard_meta[$];
        packet_info_t send_info, recv_info;
        send_info = 0;
        send_info.eob = 1'b1;
        write_reg_ctrlport(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                           REG_SR_DECIM_ADDR,    32'h0001);  // 0 HBFs, CIC=1
        write_reg_ctrlport(port_i, DDC_PORT_BASE_ADDR, DDC_PORT_SR_OFFSET,
                           REG_SR_SCALE_IQ_ADDR, 32'h8000);  // unity scale
        repeat (4 * SPC) flush_items.push_back('0);
        fork
          blk_ctrl.send_items(port_i, flush_items, {}, send_info);
          blk_ctrl.recv_items_adv(port_i, discard_items, discard_meta, recv_info);
        join
      end
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test no I/Q jumps at full amplitude
    //-------------------------------------------------------------------------
    test.start_test("Test no I/Q jumps at full amplitude", 0.5ms);
    send_tone(.port(0), .decim_rate(2), .tone_ampl(1.0),
              .tone_freq_norm(1.0/1024.0), .dsp_tune_norm(1.0/1024.0),
              .num_samps_to_send(10000), .restrict_input(1));
    test.end_test();

    //-------------------------------------------------------------------------
    // Test steady-state gain through CIC and halfband combinations
    //-------------------------------------------------------------------------
    begin
      automatic int unsigned gain_test_rates[$] = {1, 2, 3, 8, 13, 40};
      test.start_test("Test steady-state DDC gain", 2ms);
      foreach (gain_test_rates[i]) begin
        check_constant_gain(0, gain_test_rates[i]);
      end
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test various decimation rates
    //-------------------------------------------------------------------------
    begin
      // Rates and their HBF/CIC breakdown:
      //   1 -> HBs:0 CIC:1  |   2 -> HBs:1 CIC:1  |   3 -> HBs:0 CIC:3
      //   4 -> HBs:2 CIC:1  |   8 -> HBs:3 CIC:1  |  12 -> HBs:2 CIC:3
      //  13 -> HBs:0 CIC:13 |  40 -> HBs:3 CIC:5  | 255 -> HBs:0 CIC:255
      // Extended: 6 -> HBs:1 CIC:3 | 24 -> HBs:3 CIC:3 | 2040 -> HBs:3 CIC:255
      int unsigned decim_rates[$] = {1, 2, 3, 4, 8, 12, 13, 40, 255};
      if (EXTENDED_TEST) decim_rates = {decim_rates, 6, 24, 2040};

      port = NUM_PORTS - 1;
      test.start_test("Test decimation rates", 5ms);
      $display("Note: This test will take a long time!");
      foreach (decim_rates[i]) send_ramp(port, decim_rates[i]);
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test timed tune
    //-------------------------------------------------------------------------
    begin
      port = 0;
      test.start_test("Test multiple freq shifts (timed & untimed)", 1.0ms);
      $display("Note: This test will take a long time!");
      multi_freq_shift_test(port, 1,  1);   // HBs enabled: 0, CIC rate: 1
      multi_freq_shift_test(port, 40, 1);   // HBs enabled: 3, CIC rate: 5
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test passing through a partial packet
    //-------------------------------------------------------------------------
    begin
      // Rates cover: 1 HBF (2), no HBF (3), 2 HBFs (4), no HBF+high CIC (13).
      // Extended adds 3 HBFs with CIC=3 (24).
      int unsigned partial_rates[$] = {2, 3, 4, 13};
      if (EXTENDED_TEST) partial_rates = {partial_rates, 24};

      port = NUM_PORTS > 1 ? 1 : 0;
      test.start_test("Pass through partial packet", 5ms);
      // extra_samples causes a partial output packet that must not be dropped.
      foreach (partial_rates[i]) send_ramp(port, partial_rates[i], 0, 2*SPC);
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Finish
    //-------------------------------------------------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    ddc_clk_gen.kill();
  end
endmodule
