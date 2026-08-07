//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_ms_tb
//
// Description:
//
//   Testbench for rfnoc_block_duc_ms.
//


module rfnoc_block_duc_ms_tb #(
  parameter int CHDR_W         = 64,
  parameter int SPC            = 2,
  parameter int NUM_PORTS      = 1,
  parameter int MTU            = 8,
  parameter bit EXTENDED_TEST  = 1
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrData::*;
  import PkgRfnocBlockCtrlBfm::*;
  import rfnoc_block_duc_ms_pkg::*;
  import duc_ms_regs_pkg::*;
  import axi_rate_change_ms_pkg::*;
  import PkgMath::*;


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------
  // Block configuration
  localparam int SAMP_W           = 32;
  localparam int COMP_W           = SAMP_W/2;
  localparam int THIS_PORTID      = 'h123;
  localparam int NUM_HB           = 3;
  localparam int CIC_ORDER        = 4;
  localparam int NOC_ID           = 32'hD0C00000;

  // Simulation parameters
  localparam int  STALL_PROB      = 25;    // BFM stall probability
  localparam int  VERBOSE         = 0;     // Verbose output
  // ... clock rate
  localparam real CHDR_CLK_PER    = 5.0;   // CHDR clock rate
  localparam real DUC_CLK_PER     = 4.0;   // DUC IP clock rate
  // ... packetization
  localparam int  CHDR_WORD_BYTES     = CHDR_W/8;
  localparam int  PKT_SIZE_WORDS      = 128;  // CHDR words per packet
  localparam int  ITEMS_PER_CHDR_WORD = CHDR_W/SAMP_W;
  localparam int  SPP                 = PKT_SIZE_WORDS*ITEMS_PER_CHDR_WORD; // Samples per packet
  localparam int  PKT_SIZE_BYTES      = PKT_SIZE_WORDS*CHDR_WORD_BYTES; // Bytes per packet
  // ... DUT parameters
  localparam int  CIC_MAX_INTERP  = 255;  // Max CIC interpolation rate
  localparam int  DDS_PHASE_W     = 24;    // DDS phase command width
  localparam int  MAX_PHASE       = (1 << DDS_PHASE_W) - 1; // Max DDS phase value
  localparam int  SCALE_FRAC_W    = 15;    // CIC scale register fractional width
  localparam int  HBF_NUM_COEFFS  = axis_hb_utils_pkg::HB47_NUM_COEFFS;

  // ... offset, latency and tolerances
  localparam int  JITTER_TOL                = 4;
  localparam real PHASE_TOL                 = 0.05;
  // From axi_tag_time_ms + dds_ms:
  // Latency in cycles to apply timed command freq shift value to output data
  localparam int  MIN_LATENCY = 6;

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CHDR_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(DUC_CLK_PER ) duc_clk_gen        (.clk(ce_clk),         .rst());


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W, SAMP_W)::chdr_word_t       chdr_word_t;
  typedef ChdrData #(CHDR_W, SAMP_W)::chdr_word_queue_t chdr_word_queue_t;
  typedef ChdrData #(CHDR_W, SAMP_W)::item_t            item_t;
  typedef ChdrData #(CHDR_W, SAMP_W)::item_queue_t      item_queue_t;

  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W, SAMP_W) blk_ctrl =
    new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(port, m_chdr[port], PKT_SIZE_BYTES);
      blk_ctrl.connect_slave_data_port(port, s_chdr[port]);
      blk_ctrl.set_master_stall_prob(port, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    end
  end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS-1:0][CHDR_W-1:0] s_rfnoc_chdr_tdata;
  logic [NUM_PORTS-1:0]             s_rfnoc_chdr_tlast;
  logic [NUM_PORTS-1:0]             s_rfnoc_chdr_tvalid;
  logic [NUM_PORTS-1:0]             s_rfnoc_chdr_tready;

  logic [NUM_PORTS-1:0][CHDR_W-1:0] m_rfnoc_chdr_tdata;
  logic [NUM_PORTS-1:0]             m_rfnoc_chdr_tlast;
  logic [NUM_PORTS-1:0]             m_rfnoc_chdr_tvalid;
  logic [NUM_PORTS-1:0]             m_rfnoc_chdr_tready;

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

  rfnoc_block_duc #(
    .THIS_PORTID    (THIS_PORTID   ),
    .CHDR_W         (CHDR_W        ),
    .NUM_PORTS      (NUM_PORTS     ),
    .MTU            (MTU           ),
    .NUM_HB         (NUM_HB        ),
    .CIC_MAX_INTERP (CIC_MAX_INTERP),
    .NIPC           (SPC           ),
    .USE_MS         (1             )
  ) rfnoc_block_duc_i (
    .rfnoc_chdr_clk      (backend.chdr_clk   ),
    .ce_clk              (ce_clk             ),
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata ),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast ),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata ),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast ),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    .rfnoc_core_config   (backend.cfg        ),
    .rfnoc_core_status   (backend.sts        ),
    .rfnoc_ctrl_clk      (backend.ctrl_clk   ),
    .s_rfnoc_ctrl_tdata  (m_ctrl.tdata       ),
    .s_rfnoc_ctrl_tlast  (m_ctrl.tlast       ),
    .s_rfnoc_ctrl_tvalid (m_ctrl.tvalid      ),
    .s_rfnoc_ctrl_tready (m_ctrl.tready      ),
    .m_rfnoc_ctrl_tdata  (s_ctrl.tdata       ),
    .m_rfnoc_ctrl_tlast  (s_ctrl.tlast       ),
    .m_rfnoc_ctrl_tvalid (s_ctrl.tvalid      ),
    .m_rfnoc_ctrl_tready (s_ctrl.tready      )
  );

  // This fixup is needed for modelsim to not error out on the DDS LUT with
  // "ERROR:add_1 must be in range [-1,DEPTH-1]"
  // Basically it forces the reset to be asserted until the real reset is asserted.
  for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_startup_reset
    initial begin
      force rfnoc_block_duc_i.gen_ms.rfnoc_block_duc_ms_i
        .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
        .duc_dds_ms_i.rst_stretch = 1'b1;
      wait (rfnoc_block_duc_i.gen_ms.rfnoc_block_duc_ms_i
        .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
        .duc_dds_ms_i.rst);
      release rfnoc_block_duc_i.gen_ms.rfnoc_block_duc_ms_i
        .gen_multisample_chains[port].rfnoc_block_duc_ms_channel_i
        .duc_dds_ms_i.rst_stretch;
    end
  end


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Ctrlport read/write
  // Translate the desired port register access to a ctrlport write request.
  task automatic write_reg(
    input int              port,
    input int unsigned     module_base_addr,
    input int unsigned     addr,
    input ctrl_word_t      value,
    input chdr_timestamp_t timestamp = PkgCtrlIfaceBfm::RESERVED_TS
  );
    blk_ctrl.reg_write(
      DUC_PORT_BASE_ADDR + module_base_addr + port*(1<<DUC_PORT_ADDR_W) + addr,
      value, timestamp);
  endtask : write_reg


  // Translate the desired shared register access to a ctrlport read request.
  task automatic read_reg_ctrlport_shared(
    input  int unsigned  addr,
    output ctrl_word_t   value
  );
    blk_ctrl.reg_read(DUC_SHARED_BASE_ADDR + addr, value);
  endtask : read_reg_ctrlport_shared

  // Compute sample's phase in radians, in the range [-PI, PI]
  function automatic real sample_phase(input logic [SAMP_W-1:0] sample);
    logic signed [COMP_W-1:0] i_comp;
    logic signed [COMP_W-1:0] q_comp;
    begin
      i_comp = sample[SAMP_W-1 -: COMP_W];
      q_comp = sample[COMP_W-1:0];
      return $atan2(real'(q_comp), real'(i_comp));
    end
  endfunction : sample_phase

  // Returns the phase angle in [-PI, PI] of delta = curr * conj(prev),
  // where conj() is the complex conjugate. Also returns the magnitude of
  // delta via the output argument.
  function automatic real sample_delta_phase(
    input  logic [SAMP_W-1:0] curr_sample,
    input  logic [SAMP_W-1:0] prev_sample,
    output real                magnitude
  );
    logic signed [COMP_W-1:0] i_curr, q_curr, i_prev, q_prev;
    real delta_i, delta_q;
    i_curr    = curr_sample[SAMP_W-1 -: COMP_W];
    q_curr    = curr_sample[COMP_W-1:0];
    i_prev    = prev_sample[SAMP_W-1 -: COMP_W];
    q_prev    = prev_sample[COMP_W-1:0];
    // delta = curr * conj(prev)
    delta_i   = real'(i_curr)*real'(i_prev) + real'(q_curr)*real'(q_prev);
    delta_q   = real'(q_curr)*real'(i_prev) - real'(i_curr)*real'(q_prev);
    magnitude = $sqrt(delta_i*delta_i + delta_q*delta_q);
    return $atan2(delta_q, delta_i);
  endfunction : sample_delta_phase

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

  // Compute scaling factor for CIC interpolation rate
  //  The scale multiplier computes: output = (cic_24bit * cic_scale) >> SCALE_FRAC_W,
  //  so the CIC gain to compensate is R^(N-1) / 2^ceil(log2(R^(N-1))).
  function automatic int cic_scale_word(int cic_rate);
    real         cic_scale_float;
    logic [31:0] cic_scale;
    begin
      cic_scale_float =
        1.0 /((real'(cic_rate)**(CIC_ORDER-1))/(2.0**$clog2(cic_rate**(CIC_ORDER-1))));
      cic_scale       = int'((1<<SCALE_FRAC_W) * cic_scale_float);
      if (VERBOSE) begin
        $display("CIC scale factor for rate %0d = %f", cic_rate, cic_scale_float);
      end
      return cic_scale;
    end
  endfunction : cic_scale_word


  // Set the interpolation rate and IQ scaling factor
  logic [7:0] hb_enables_global = 8'd0;
  logic [7:0] cic_rate_global   = 8'd1;
  task automatic set_interp_rate(int port, int interp_rate);
    begin
      logic [7:0] cic_rate   = 8'd0;
      logic [7:0] hb_enables = 2'b0;
      int _interp_rate = interp_rate;
      int scale_iq;

      // Calculate which halfband filters to enable an what CIC settings to use
      while ((_interp_rate[0] == 0) && (hb_enables < NUM_HB)) begin
        hb_enables += 1'b1;
        _interp_rate = _interp_rate >> 1;
      end

      // CIC rate cannot be set to 0
      cic_rate = (_interp_rate[7:0] == 8'd0) ? 8'd1 : _interp_rate[7:0];
      scale_iq = cic_scale_word(cic_rate);
      `ASSERT_ERROR(hb_enables <= NUM_HB,
        "Enabled halfbands may not exceed total number of half bands.");
      `ASSERT_ERROR(cic_rate > 0 && cic_rate <= CIC_MAX_INTERP,
       {"CIC Interpolation rate must be positive, not exceed the max cic ",
        "interpolation rate, and cannot equal 0!"});

      // Setup DUC
      $display("Set interpolation to %0d", interp_rate);
      $display("- Number of enabled HBs: %0d", hb_enables);
      $display("- CIC Rate:              %0d", cic_rate);
      $display("- CIC Scale:             0x%0X", scale_iq);
      // Each output word contains SPC samples, so packet timestamps advance by SPC.
      write_reg(
        port, DUC_PORT_AXI_RATE_OFFSET, REG_AXI_RATE_SR_TIME_INCR_ADDR,
        SPC);
      // Enable HBs, set CIC rate
      write_reg(
        port, DUC_PORT_SR_OFFSET, REG_SR_INTERP_ADDR,
        {hb_enables, cic_rate});
      // Set CIC scaling factor
      write_reg(
        port, DUC_PORT_SR_OFFSET, REG_SR_SCALE_IQ_ADDR,
        scale_iq);

      // Store global values for use in other tasks
      hb_enables_global = hb_enables;
      cic_rate_global   = cic_rate;
    end
  endtask

  // Queue timed/untimed freq shift commands to the DDS
  task automatic set_dds_freq(
    input int              port,
    input int              freq_shift,
    input chdr_timestamp_t timestamp = PkgCtrlIfaceBfm::RESERVED_TS
  );
    write_reg(
      port, DUC_PORT_DDS_OFFSET, REG_DDS_FREQ_ADDR,
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

  // Verify steady-state passband gain using a constant complex input.
  // port: DUC port to test.
  // interp_rate: Interpolation rate to configure.
  task automatic check_constant_gain(
    input int port,
    input int interp_rate
  );
    localparam int CONSTANT_AMPL           = 4096;
    localparam int GAIN_TOLERANCE_DIVISOR = 20;
    packet_info_t     send_pkt_info, recv_pkt_info;
    item_queue_t      recv_items, all_recv_items;
    chdr_word_queue_t send_payload, recv_payload;
    chdr_word_t       constant_word;
    longint signed    sum_i, sum_q;
    int               first_sample, last_sample;
    int               num_accum_samples;
    int               avg_i, avg_q;

    set_interp_rate(port, interp_rate);
    set_dds_freq(port, 32'd0, PkgCtrlIfaceBfm::RESERVED_TS);

    constant_word = '0;
    for (int item_idx = 0; item_idx < ITEMS_PER_CHDR_WORD; item_idx++) begin
      constant_word[item_idx*SAMP_W +: SAMP_W] = {
        COMP_W'(CONSTANT_AMPL), COMP_W'(CONSTANT_AMPL)};
    end
    for (int word_idx = 0; word_idx < PKT_SIZE_WORDS; word_idx++) begin
      send_payload.push_back(constant_word);
    end

    send_pkt_info     = '0;
    send_pkt_info.eob = 1'b1;
    fork
      begin
        blk_ctrl.send_packets(
          port, send_payload, /*data_bytes*/, /*metadata*/, send_pkt_info);
        blk_ctrl.wait_complete(port);
      end
      begin
        int         data_bytes;
        chdr_word_t metadata[$];
        for (int pkt_idx = 0; pkt_idx < interp_rate; pkt_idx++) begin
          blk_ctrl.recv_adv(port, recv_payload, data_bytes, metadata, recv_pkt_info);
          recv_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(recv_payload, data_bytes);
          all_recv_items = {all_recv_items, recv_items};
        end
        `ASSERT_ERROR(recv_pkt_info.eob, "EOB not set on final constant-gain packet");
      end
    join

    first_sample = all_recv_items.size() / 4;
    last_sample  = 3 * all_recv_items.size() / 4;
    sum_i = 0;
    sum_q = 0;
    num_accum_samples = 0;
    for (int sample_idx = first_sample; sample_idx < last_sample; sample_idx++) begin
      item_t recv_item = all_recv_items[sample_idx];
      sum_i += signed'(recv_item[SAMP_W-1 -: COMP_W]);
      sum_q += signed'(recv_item[COMP_W-1:0]);
      num_accum_samples++;
    end
    `ASSERT_ERROR(num_accum_samples > 0, "No steady-state samples received");
    avg_i = sum_i / num_accum_samples;
    avg_q = sum_q / num_accum_samples;
    $display("Constant gain at interpolation %0d: I=%0d, Q=%0d", interp_rate, avg_i, avg_q);
    `ASSERT_ERROR(
      avg_i >= CONSTANT_AMPL - CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR &&
      avg_i <= CONSTANT_AMPL + CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR,
      $sformatf("I gain mismatch at interpolation %0d: expected %0d, got %0d",
                interp_rate, CONSTANT_AMPL, avg_i));
    `ASSERT_ERROR(
      avg_q >= CONSTANT_AMPL - CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR &&
      avg_q <= CONSTANT_AMPL + CONSTANT_AMPL/GAIN_TOLERANCE_DIVISOR,
      $sformatf("Q gain mismatch at interpolation %0d: expected %0d, got %0d",
                interp_rate, CONSTANT_AMPL, avg_q));
  endtask : check_constant_gain

  // Convert a frequency word to a normalized frequency in the range [-0.3, 0.3)
  // freq_norm = 0.6 * tone_freq / (2^data_width) - 0.3
  function automatic real gen_freq_norm(int tone_freq, int data_width);
    return 0.6 * real'(signed'(tone_freq)) / real'(longint'(1) << data_width) - 0.3;
  endfunction : gen_freq_norm

  // Generate sinusoidal wave samples
  function automatic chdr_word_queue_t generate_tone_pkt(
    input int  length,
    input real freq_norm,
    input real ampl = 1.0
  );
    chdr_word_queue_t pkt;
    real phase_incr = freq_norm * 2.0 * PI; // in range [-pi, pi)
    phase_incr = wrap_phase_rad(phase_incr);
    for (int word_idx = 0; word_idx < length; word_idx++) begin
      chdr_word_t sample;
      for (int item_idx = 0; item_idx < ITEMS_PER_CHDR_WORD; item_idx++) begin
        real sin_val = ampl * $sin((word_idx*ITEMS_PER_CHDR_WORD + item_idx) * phase_incr);
        real cos_val = ampl * $cos((word_idx*ITEMS_PER_CHDR_WORD + item_idx) * phase_incr);
        logic signed [COMP_W-1:0] i_comp = COMP_W'(int'(cos_val * (2**(COMP_W-1)-1)));
        logic signed [COMP_W-1:0] q_comp = COMP_W'(int'(sin_val * (2**(COMP_W-1)-1)));
        sample[item_idx*SAMP_W +: SAMP_W] = {i_comp, q_comp};
      end
      pkt.push_back(sample);
    end
    return pkt;
  endfunction : generate_tone_pkt

  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // During the initial ramp-up period, the HBFs will output zeroes/transient
  // samples, not allowing for reliable phase step measurement.
  // So, we calculate the duration of the ramp-up period in order to start
  // the phase step check after that.
  // Example 3 halfband filters are enabled:
  //   - First HBF outputs 46 transient samples, following HBFs apply
  //     upsampling of 2x2 to that such that overall contribution is 46*4.
  //   - Second HBF has one following stage, so its overall contribution is 46*2.
  //   - Third HBF has no following stages, so its overall contribution is 46.
  //   The overall ramp-up period is 46*(4+2+1) = 46*7.
  function automatic int hbf_ramp_output_samples(int hb_enables);
    int sum = 0;
    for (int k = 0; k < hb_enables; k++) begin
      sum += (1 << k);
    end
    if (VERBOSE)
      $display("HBF ramp-up samples for %0d enabled HBFs = %0d", hb_enables, (HBF_NUM_COEFFS - 1) * sum);
    return (HBF_NUM_COEFFS - 1) * sum;
  endfunction : hbf_ramp_output_samples

  // Test frequency shift with a single tone with timed and untimed commands.
  //  - 1 untimed + 1 timed command - queued before start of burst
  //  - 1 timed command - queued mid-burst at a random packet boundary
  //  - Each CHDR word is populated with ITEMS_PER_WORD items.
  task automatic multi_freq_shift_test(int port, int interp_rate, bit has_time);
    begin
      const chdr_timestamp_t start_time           = 64'd0;
      const int              expected_num_packets = interp_rate;

      packet_info_t      send_pkt_info, recv_pkt_info;
      chdr_word_queue_t  send_payload, recv_payload;
      int                sample_idx;
      int                ramp_up_samples;
      int                word_idx_offset = 0;
      int                dds_timed_word_offset, switch_phase_pkt, fire_after_pkts;
      int                min_switch_phase_pkt;

      logic [DDS_PHASE_W-1:0] freq_shift_phase_word;
      real                    tone_freq_norm;
      real                    dds_freq_norm;
      real                    freq_shift_norm;

      // Configure DUC
      set_interp_rate(port, interp_rate);

      // During the initial ramp-up period, the HBFs will output zeroes/transient
      // samples, not allowing for reliable phase step measurement.
      // So, we calculate the duration of the ramp-up period in order to start
      // the phase step check after that.
      // Example 3 halfband filters are enabled:
      //   - HBF transients:
      //     - First HBF outputs 46 transient samples, following HBFs apply
      //       upsampling of 2x2 to that such that overall contribution is 46*4.
      //     - Second HBF has one following stage, so its overall contribution is 46*2.
      //     - Third HBF has no following stages, so its overall contribution is 46.
      //   - CIC will then upsample the output of the last HBF by cic_rate, so the
      //     overall ramp-up period is 46*(4+2+1)*cic_rate = 46*7*cic_rate.
      // Note: 3+2+1 is a geometric sum and can be calculated as (2^3-1),
      //       where 3 is the number of enabled HBFs.
      ramp_up_samples = hbf_ramp_output_samples(hb_enables_global) * cic_rate_global;

      // Convert samples to CHDR words using ceiling division so a partial word
      // is skipped in full: ceil(samples/items) = (samples+items-1)/items.
      word_idx_offset = (ramp_up_samples + ITEMS_PER_CHDR_WORD - 1) /
            ITEMS_PER_CHDR_WORD;

      // Issue first timed command after the HBF ramp-up period. $urandom_range
      // arguments are ordered as (maximum, minimum).
      dds_timed_word_offset = $urandom_range(
        ((expected_num_packets > 1) ? 4 : 1) * PKT_SIZE_WORDS-3,
        word_idx_offset+MIN_LATENCY);
      switch_phase_pkt      = -1;
      fire_after_pkts       = -1;

      // Set input tone frequency normalized to input sampling rate
      tone_freq_norm           = gen_freq_norm($urandom_range(MAX_PHASE), DDS_PHASE_W);
      // Set DDS frequency shift normalized to output sampling rate
      dds_freq_norm            = gen_freq_norm($urandom_range(MAX_PHASE), DDS_PHASE_W);
      freq_shift_phase_word    = dds_freq_norm * (1 << DDS_PHASE_W);
      $display("Tone freq norm = %f, DDS freq norm = %f", tone_freq_norm, dds_freq_norm);
      // Calculate the frequency-shifted tone normalized to output sampling rate
      freq_shift_norm         = tone_freq_norm/interp_rate + dds_freq_norm;

      // Queue untimed and timed command for first packet
      set_dds_freq(port, 32'd0, PkgCtrlIfaceBfm::RESERVED_TS);
      if (VERBOSE)
        $display ("Queue timed DDS freq shift command at word offset %0d", dds_timed_word_offset);
      set_dds_freq(port, {freq_shift_phase_word, 8'b0},
                  start_time + dds_timed_word_offset*ITEMS_PER_CHDR_WORD);

      // Queue a timed DDS freq shift update at a random packet boundary,
      // firing the command some time during the burst.
      if (expected_num_packets > 2) begin
        min_switch_phase_pkt = dds_timed_word_offset / PKT_SIZE_WORDS + 1;
        if (min_switch_phase_pkt < 2) min_switch_phase_pkt = 2;
        if (min_switch_phase_pkt < expected_num_packets) begin
          switch_phase_pkt = $urandom_range(
            expected_num_packets - 1, min_switch_phase_pkt);
          fire_after_pkts = $urandom_range(switch_phase_pkt - 1, 0);
        end
      end

      send_payload = {};
      fork
        begin : send_complex_tone
          send_payload = generate_tone_pkt(PKT_SIZE_WORDS, tone_freq_norm);

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
          string      s;
          chdr_word_t recv_samples;
          int         data_bytes;

          chdr_word_queue_t  metadata;
          logic [SAMP_W-1:0] recv_sample;
          int                dds_start_sample_idx;
          int                phase_check_start;
          int                switch_sample_idx;
          real               recv_phase;
          real               prev_recv_phase;
          real               phase_step;
          real               phase_err;
          real               exp_phase_step;
          bit                dds_active;
          bit                have_prev_phase;
          int                start_word_idx;
          int                pkt_start_word_idx;

          $display("Check incoming samples");
          // Sample at which dds applies non-zero freq shift
          dds_start_sample_idx = dds_timed_word_offset * ITEMS_PER_CHDR_WORD;
          // Sample at which to start checking phase step against expected value
          phase_check_start    = (dds_timed_word_offset + 1)*ITEMS_PER_CHDR_WORD;
          // Sample at which freq shift is switched back to zero (if applicable)
          switch_sample_idx    = (switch_phase_pkt > 0) ?
                                 switch_phase_pkt * PKT_SIZE_WORDS * ITEMS_PER_CHDR_WORD : -1;

          for (int pkt = 0; pkt < expected_num_packets; pkt++) begin
            blk_ctrl.recv_adv(port, recv_payload, data_bytes, metadata, recv_pkt_info);

            // Check the packet size
            $sformat(s,
              {"Incorrect packet size! expected words: %0d, actual words: %0d, ",
              "expected bytes: %0d, actual bytes: %0d"},
              PKT_SIZE_WORDS, recv_payload.size(), PKT_SIZE_BYTES, data_bytes);
            `ASSERT_ERROR(recv_payload.size() == PKT_SIZE_WORDS, s);

            // Check the timestamp
            if (has_time && pkt == 0) begin
              chdr_timestamp_t expected_time;
              // Calculate what the timestamp should be
              expected_time = start_time + pkt * SPP;
              $sformat(s,
                "Incorrect timestamp: has_time = %0d, timestamp = 0x%0X, expected 0x%0X",
                recv_pkt_info.has_time, recv_pkt_info.timestamp, expected_time);
              `ASSERT_ERROR(
                recv_pkt_info.has_time == 1 && recv_pkt_info.timestamp == expected_time,
                s);
            end else begin
              `ASSERT_ERROR(
                recv_pkt_info.has_time == 0,
                "Packet has timestamp when it shouldn't");
            end

            // Check EOB
            `ASSERT_ERROR(recv_pkt_info.eob == (pkt == expected_num_packets - 1),
                          "EOB not set on last packet");

            // Check the sample values, starting after the filter ramp-up period.
            pkt_start_word_idx = pkt * PKT_SIZE_WORDS;
            if (pkt_start_word_idx + recv_payload.size() <= word_idx_offset) begin
              continue;
            end
            start_word_idx = (word_idx_offset > pkt_start_word_idx) ?
                             word_idx_offset - pkt_start_word_idx : 0;
            for (int word_idx = start_word_idx; word_idx < recv_payload.size(); word_idx++) begin
              recv_samples = recv_payload[word_idx];
              for (int item_idx = 0; item_idx < ITEMS_PER_CHDR_WORD; item_idx++) begin
                sample_idx  = pkt*PKT_SIZE_WORDS*ITEMS_PER_CHDR_WORD +
                              word_idx*ITEMS_PER_CHDR_WORD + item_idx;
                recv_sample = recv_samples[item_idx*SAMP_W +: SAMP_W];
                recv_phase  = sample_phase(recv_sample);

                // Skip first word after ramp up to initialize prev_recv_phase
                if (have_prev_phase) begin
                  dds_active = (sample_idx > dds_start_sample_idx) &&
                               (switch_sample_idx < 0 || sample_idx < switch_sample_idx);
                  exp_phase_step = dds_active ? 2.0 * PI * freq_shift_norm :
                                  (2.0 * PI * tone_freq_norm)/interp_rate;
                  exp_phase_step = wrap_phase_rad(exp_phase_step);
                  phase_step     = wrap_phase_rad(recv_phase - prev_recv_phase);
                  if (VERBOSE)
                    $display($sformatf({"pkt %0d word %0d item %0d",
                    " exp_phase_step = %f, recv_phase_step = %f"},
                    pkt, word_idx, item_idx, exp_phase_step, phase_step));

                  phase_err = wrap_phase_rad(phase_step - exp_phase_step);

                  // Check phase step error only after the DDS starts applying non-zero freq shift
                  // and after the switch back to zero freq shift (if applicable)
                  if(sample_idx >= phase_check_start &&
                    (dds_active ||
                    sample_idx > switch_sample_idx + MIN_LATENCY*ITEMS_PER_CHDR_WORD)) begin
                    s = $sformatf(
                      {"Phase step mismatch at pkt %0d word %0d item %0d: ",
                        "DDS active = %0d, expected = %f, received = %f, error = %f"},
                      pkt, word_idx, item_idx, dds_active, exp_phase_step,
                      phase_step, phase_err);
                    `ASSERT_ERROR(phase_err >= -PHASE_TOL && phase_err <= PHASE_TOL, s);
                  end
                end
                prev_recv_phase = recv_phase;
                have_prev_phase = 1'b1;
              end
            end
          end
        end
        begin : update_dds_freq
          if (switch_phase_pkt > 0) begin
            // Wait a random number of packet-widths (0 to switch_phase_pkt-1)
            // so the ctrl write occurs mid-burst before the target timestamp.
            repeat (fire_after_pkts * PKT_SIZE_WORDS) @(posedge rfnoc_chdr_clk);
            if (VERBOSE)
              $display($sformatf({"Queue timed DDS freq shift update for packet %0d",
                      "(cmd sent after ~%0d pkts)..."},
                      switch_phase_pkt, fire_after_pkts));
            set_dds_freq(port, 0,
                        start_time + switch_phase_pkt * SPP);
          end
        end
      join
    end
  endtask : multi_freq_shift_test

  // Test interpolation rates with ramp pattern
  //  - Send a single ramp packet with interpolation rate step per 32-bit sample item.
  //  - Each CHDR word is populated with ITEMS_PER_WORD items.
  task automatic send_ramp(int port, int interp_rate, bit has_time);
    begin
      const chdr_timestamp_t start_time           = 64'h0;
      const int              expected_num_packets = interp_rate;

      int                    pkt_length = PKT_SIZE_WORDS; // in CHDR words
      int                    local_spp  = SPP;

      packet_info_t          send_pkt_info, recv_pkt_info;
      item_queue_t           sent_items, recv_items;
      chdr_word_queue_t      send_payload, recv_payload;
      chdr_word_t            ramp_word;
      int                    sample_idx;
      int                    min_pkt_length;
      logic [SAMP_W-1:0]     ramp_item;

      set_interp_rate(port, interp_rate);

      // Setup DUC
      // Queue untimed zero freq shift
      set_dds_freq(port, 32'd0, PkgCtrlIfaceBfm::RESERVED_TS);

      // Reduce packet length for interpolation rates > 64 to
      // reduce simulation time.
      // However, the packet must be long enough for the HBF cascade to exit its
      // startup transient before the CIC expands the stream — so take the larger of the
      // two constraints.
      if (interp_rate > 64) begin
        // Runtime-reduction target
        pkt_length     = PKT_SIZE_WORDS * 64 / interp_rate;
        // HBF-startup minimum is ramp output samples / HBF upsampling factor
        min_pkt_length = hbf_ramp_output_samples(hb_enables_global) /
                         (1 << hb_enables_global) / ITEMS_PER_CHDR_WORD;
        // ... ensure at least two words are sent after startup transient
        min_pkt_length += 2;
        // Selected packet length is at least the minimum packet length
        pkt_length     = (pkt_length > min_pkt_length) ? pkt_length : min_pkt_length;
        local_spp      = pkt_length * ITEMS_PER_CHDR_WORD;
        if (VERBOSE)
          $display("Send ramp: packet length %0d (min length %0d), SPP = %0d, interp_rate = %0d",
            pkt_length, min_pkt_length, local_spp, interp_rate);
      end

      fork
        begin: send_ramp_packet
          int comp = 0;
          for (int word_idx = 0; word_idx < pkt_length; word_idx++) begin
            ramp_word = '0;
            for (int item_idx = 0; item_idx < ITEMS_PER_CHDR_WORD; item_idx++) begin
              sample_idx = word_idx*ITEMS_PER_CHDR_WORD + item_idx;
              ramp_item = {COMP_W'(comp), COMP_W'(comp)};
              ramp_word[item_idx*SAMP_W +: SAMP_W] = ramp_item;
              if (comp + interp_rate <= (1 << (COMP_W-1)) - 1)
                comp += interp_rate;
            end
            send_payload.push_back(ramp_word);
          end
          sent_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(send_payload,
                                                              pkt_length * (CHDR_W/8));

          send_pkt_info = 0;
          send_pkt_info.has_time  = has_time;
          send_pkt_info.timestamp = start_time;
          send_pkt_info.eob       = 1'b1;
          blk_ctrl.send_packets(port, send_payload, /*data_bytes*/,
            /*metadata*/, send_pkt_info);
          blk_ctrl.wait_complete(port);

          $display("Send ramp complete");
        end
        begin : recv_packet
          string      s;
          chdr_word_t samples;
          int         data_bytes;
          chdr_word_t metadata[$];
          int         interp_count = 0;
          logic        ramp_started[2]   = '{default: 1'b0};
          logic [15:0] ramp_start_val[2] = '{default: 16'b0};
          logic signed [COMP_W-1:0] prev_recv_comp[2] = '{default: '0};

          $display("Check incoming samples");
          for (int pkt = 0; pkt < expected_num_packets; pkt++) begin
            blk_ctrl.recv_adv(port, recv_payload, data_bytes, metadata, recv_pkt_info);
            recv_items = ChdrData#(CHDR_W, SAMP_W)::chdr_to_item(recv_payload, data_bytes);

            // Check the packet size
            $sformat(s,
              {"Incorrect packet size! expected words: %0d, actual words: %0d, ",
              "expected bytes: %0d, actual bytes: %0d"},
              pkt_length, recv_payload.size(),
              pkt_length * (CHDR_W/8), data_bytes);
            `ASSERT_ERROR(recv_payload.size() == pkt_length, s);

            // Check the timestamp
            if (has_time && pkt == 0) begin
              chdr_timestamp_t expected_time;
              // Calculate what the timestamp should be
              expected_time = start_time + pkt * local_spp;
              $sformat(s,
                "Incorrect timestamp: has_time = %0d, timestamp = 0x%0X, expected 0x%0X",
                recv_pkt_info.has_time, recv_pkt_info.timestamp, expected_time);
              `ASSERT_ERROR(
                recv_pkt_info.has_time == 1 && recv_pkt_info.timestamp == expected_time,
                s);
            end else begin
              `ASSERT_ERROR(
                recv_pkt_info.has_time == 0,
                "Packet has timestamp when it shouldn't");
            end

            // Check EOB
            `ASSERT_ERROR(recv_pkt_info.eob == (pkt == expected_num_packets - 1),
                          "EOB not set on last packet");

            // Check the sample values for the ramp pattern
            begin : ramp_check
              for (int word_idx = 0; word_idx < recv_payload.size(); word_idx++) begin
                samples = recv_payload[word_idx];
                for (int item_idx = 0; item_idx < ITEMS_PER_CHDR_WORD; item_idx++) begin
                  sample_idx  = pkt*pkt_length*ITEMS_PER_CHDR_WORD +
                                word_idx*ITEMS_PER_CHDR_WORD + item_idx;
                  if (interp_count == 0) begin
                    if (sample_idx/interp_rate >= local_spp) begin
                      ramp_item = sent_items[local_spp-1];
                    end else begin
                      ramp_item = sent_items[sample_idx/interp_rate];
                    end
                  end
                  for (int comp_idx = 0; comp_idx < 2; comp_idx++) begin
                    logic signed [COMP_W-1:0] sample_comp;
                    logic signed [COMP_W-1:0] prev_comp;
                    logic signed [COMP_W-1:0] ramp_comp;
                    int expected_comp;
                    int min_expected;
                    int max_expected;

                    sample_comp     = samples[item_idx*SAMP_W + comp_idx*COMP_W +: COMP_W];
                    prev_comp       = prev_recv_comp[comp_idx];
                    ramp_comp       = ramp_item[comp_idx*COMP_W +: COMP_W];
                    if (!ramp_started[comp_idx]) begin
                      if (sample_comp > 0) begin
                        ramp_started[comp_idx]   = 1'b1;
                        ramp_start_val[comp_idx] = sample_comp;
                      end
                    end
                    if (ramp_started[comp_idx]) begin
                      if (prev_comp < ramp_comp)
                        expected_comp = prev_comp + 1;
                      else if (prev_comp > ramp_comp)
                        expected_comp = prev_comp - 1;
                      else
                        expected_comp = prev_comp;
                      min_expected = expected_comp - JITTER_TOL;
                      max_expected = expected_comp + JITTER_TOL;
                      $sformat(s,
                        {"Pkt %0d Ramp word %0d item %0d component %0d invalid! ",
                        "Expected: %0x-%0x, Received: %0x"},
                        pkt, word_idx, item_idx, comp_idx,
                        min_expected, max_expected, sample_comp);
                      `ASSERT_ERROR(
                        sample_comp >= min_expected && sample_comp <= max_expected, s);
                    end
                  end
                  if (interp_count == interp_rate - 1)
                    interp_count = 0;
                  else
                    interp_count++;
                  for (int comp_idx = 0; comp_idx < 2; comp_idx++) begin
                    prev_recv_comp[comp_idx] = samples[item_idx*SAMP_W + comp_idx*COMP_W +: COMP_W];
                  end
                end
              end

            end
          end

          // After all packets: verify ramp was present in the burst output.
          for (int j = 0; j < 2; j++) begin
            // Both components must have gone non-zero before the end of the burst.
            $sformat(s,
              "No non-zero sample received for component %0d in %0d samples!",
              j, expected_num_packets*local_spp);
            `ASSERT_ERROR(ramp_started[j], s);
            // Verify an actual ramp was received: the last sample of the last packet
            // must be strictly greater than the first non-zero sample, ruling out
            // an all-constant non-zero output.
            $sformat(s,
              "No ramp detected on component %0d: first=%0d, last=%0d",
              j, ramp_start_val[j],
              recv_items[local_spp-1][16*j +: 16]);
            `ASSERT_ERROR(
              recv_items[local_spp-1][16*j +: 16] >
              ramp_start_val[j], s);
          end
        end
      join
    end
  endtask : send_ramp


  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    static int port = 0;
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf({
      "rfnoc_block_duc_ms_tb\n",
      "\tCHDR_W         = %0d\n",
      "\tSPC            = %0d\n",
      "\tNUM_PORTS      = %0d\n",
      "\tCIC_MAX_INTERP = %0d\n",
      "\tMTU            = %0d"},
      CHDR_W, SPC, NUM_PORTS, CIC_MAX_INTERP, MTU
    );

    test.start_tb(tb_name);

    // Start the BFMs running
    blk_ctrl.run();


    //-------------------------------------------------------------------------
    // Reset
    //-------------------------------------------------------------------------

    test.start_test("Reset", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();


    //-------------------------------------------------------------------------
    // Check NoC ID and Block Info
    //-------------------------------------------------------------------------

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect NOC_ID value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect NUM_DATA_I value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect NUM_DATA_O value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect MTU value");
    test.end_test();


    //-------------------------------------------------------------------------
    // Test read-back regs
    //-------------------------------------------------------------------------

    begin
      logic [31:0] val32;
      test.start_test("Test registers", 10us);
      read_reg_ctrlport_shared(REG_NUM_HB, val32);
      `ASSERT_ERROR(val32 == NUM_HB,
        "Register REG_NUM_HB didn't read back expected value");
      read_reg_ctrlport_shared(REG_CIC_MAX_INTERP, val32);
      `ASSERT_ERROR(val32 == CIC_MAX_INTERP,
        "Register REG_CIC_MAX_INTERP didn't read back expected value");
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test multiple timed and untimed freq shifts with a complex tone
    //-------------------------------------------------------------------------
    begin
      test.start_test("Test multiple freq shifts (timed & untimed)", 1.0ms);
      $display("Note: This test will take a long time!");
      multi_freq_shift_test(port, 1,  1);   // HBs enabled: 0, CIC rate: 1
      multi_freq_shift_test(port, 16, 1);   // HBs enabled: 3, CIC rate: 2
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test steady-state gain through CIC and halfband combinations
    //-------------------------------------------------------------------------
    begin
      // Test CIC only rate 3 back to back to detect EoB reset with unchanged
      // gain bug.
      automatic int gain_test_rates[$] = {1, 2, 3, 3, 8, 13, 40};
      test.start_test("Test steady-state DUC gain", 2.0ms);
      foreach (gain_test_rates[i]) begin
        check_constant_gain(0, gain_test_rates[i]);
      end
      test.end_test();
    end

    //-------------------------------------------------------------------------
    // Test various interpolation rates (with timestamp)
    //-------------------------------------------------------------------------
    begin
      port = NUM_PORTS - 1;
      test.start_test("Test interpolation rates (with timestamp)", 1.0ms);
      $display("Note: This test will take a long time!");
      send_ramp(port, 1,  1);   // HBs enabled: 0, CIC rate: 1
      send_ramp(port, 2,  1);   // HBs enabled: 1, CIC rate: 1
      send_ramp(port, 3,  1);   // HBs enabled: 0, CIC rate: 3
      send_ramp(port, 4,  1);   // HBs enabled: 2, CIC rate: 1
      send_ramp(port, 6,  1);   // HBs enabled: 1, CIC rate: 3
      send_ramp(port, 8,  1);   // HBs enabled: 3, CIC rate: 1
      send_ramp(port, 12, 1);   // HBs enabled: 2, CIC rate: 3
      send_ramp(port, 13, 1);   // HBs enabled: 0, CIC rate: 13
      send_ramp(port, 40, 1);   // HBs enabled: 3, CIC rate: 5
      test.end_test();
      if (EXTENDED_TEST) begin
        test.start_test("Test interpolation rates (with timestamp) - extended", 5.0ms);
        send_ramp(port, 255, 1);  // HBs enabled: 0, CIC rate: 255
        send_ramp(port, 2040, 1); // HBs enabled: 3, CIC rate: 255
        test.end_test();
      end
    end

    //-------------------------------------------------------------------------
    // Test various interpolation rates (without timestamp)
    //-------------------------------------------------------------------------
    begin
      test.start_test("Test interpolation rates (no timestamp)", 0.5ms);
      send_ramp(port, 1,  0);   // HBs enabled: 0, CIC rate: 1
      send_ramp(port, 2,  0);   // HBs enabled: 1, CIC rate: 1
      send_ramp(port, 3,  0);   // HBs enabled: 0, CIC rate: 3
      send_ramp(port, 4,  0);   // HBs enabled: 2, CIC rate: 1
      send_ramp(port, 8,  0);   // HBs enabled: 3, CIC rate: 1
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
    duc_clk_gen.kill();
  end
endmodule
