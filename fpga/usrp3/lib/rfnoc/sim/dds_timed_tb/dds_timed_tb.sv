//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_timed_tb
//

`default_nettype none


module dds_timed_tb;

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgComplex::*;
  import PkgMath::*;
  import PkgRandom::*;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD = 10.0;

  // Values needed by the DUT (use the same values as the DUC)
  localparam int SR_FREQ_ADDR      = 132;
  localparam int SR_SCALE_IQ_ADDR  = 133;
  localparam int SR_AWIDTH         = 8;
  localparam int SR_DWIDTH         = 32;
  localparam int SR_TWIDTH         = 64;
  localparam int PHASE_ACCUM_WIDTH = 32;
  localparam int SCALING_WIDTH     = 18;

  // Bit widths for our sample size
  localparam int FRAC_W   = 15;       // Number of fixed point fractional bits
  localparam int COMP_W   = 16;       // Width of just the imag/real part
  localparam int SAMPLE_W = 2*COMP_W; // Width of a complex sample

  // Max min possible values for the components of a sample
  localparam bit signed [COMP_W-1:0] MAX_COMP =  2**(COMP_W-1) - 1;
  localparam bit signed [COMP_W-1:0] MIN_COMP = -2**(COMP_W-1);

  // Max/min possible values for the scale register
  localparam real MAX_SCALE = +(2**(SCALING_WIDTH-1) - 1) / (2.0**FRAC_W);
  localparam real MIN_SCALE = -(2**(SCALING_WIDTH-1)    ) / (2.0**FRAC_W);

  // TUSER bit positions
  localparam int HAS_TIME_POS  = 125;
  localparam int EOB_POS       = 124;
  localparam int TIMESTAMP_POS = 0;

  // AXI-Stream data bus parameters
  localparam int DATA_W = SAMPLE_W;
  localparam int USER_W = 128;

  // Amount of rounding error to allow (in ULPs). We generally expect the error
  // in computation to be +/- 1 ULP, but since the DUT performs several
  // computations, error can accumulate. The testbench computations also
  // introduce some error. In particular, using the scale register in the DUT
  // also scales the error. The MAX_ERROR can be reduced to 2 if you keep the
  // scale register < 1.0.
  localparam MAX_ERROR = 8;


  //---------------------------------------------------------------------------
  // Type Definitions
  //---------------------------------------------------------------------------

  // Burst test configuration
  typedef struct {
    int     spp;             // Samples per packet to generate.
    int     spp_last;        // Length of last packet, if different from SPP.
                             // Set to -1 to use spp value.
    int     num_packets;     // Number of packets in each burst.
    int     num_bursts;      // Number of bursts to send.
    real    amp;             // Amplitude of the test signal to generate.
    real    freq;            // Normalized frequency of test signal to generate.
    bit     timed;           // Set to 1 for timed packet, 0 for non-timed. If
                             // doing a timed tune, this must be 1.
    real    scale;           // Scale value to use, in the range [-4,4).
    real    freq_shift;      // Initial frequency shift to use.
    real    tune_freq_shift; // New frequency shift to tune to.
    longint tune_time;       // Time after which to tune the frequency (set a
                             // new frequency shift). Set to -1 to disable.
  } burst_cfg_t;

  typedef AxiStreamPacket #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W)) axis_pkt_t;
  typedef axis_pkt_t axis_pkt_queue_t[$];

  // Default settings to use for burst_cfg_t. This creates a nice complex
  // sinusoid and the output should match the input, unchanged.
  localparam burst_cfg_t DEFAULT_BURST_CFG = '{
    spp             : 256,
    spp_last        : -1,
    num_packets     : 1,
    num_bursts      : 1,
    freq            : 1.0/16.0,
    amp             : 0.75,
    timed           : 1,
    scale           : 1.0,
    freq_shift      : 0.0,
    tune_freq_shift : 0.0,
    tune_time       : -1.0
  };


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;

  sim_clock_gen #(CLK_PERIOD) clk_gen (clk, rst);


  //---------------------------------------------------------------------------
  // AXI-Stream BFM
  //---------------------------------------------------------------------------

  // AXI-Stream interfaces to/from DUT
  AxiStreamIf #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    to_dut (clk, rst);
  AxiStreamIf #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    from_dut (clk, rst);

  // BFM for the AXI-Stream interface to DUT
  AxiStreamBfm #(.DATA_WIDTH(DATA_W), .USER_WIDTH(USER_W), .TKEEP(0))
    axis_bfm = new(to_dut, from_dut);


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic                 clear = 1'b0;
  logic                 timed_cmd_fifo_full;
  logic                 set_stb = 1'b0;
  logic [SR_AWIDTH-1:0] set_addr;
  logic [SR_DWIDTH-1:0] set_data;
  logic [SR_TWIDTH-1:0] set_time;
  logic                 set_has_time;
  logic [ SAMPLE_W-1:0] i_tdata;
  logic                 i_tlast;
  logic                 i_tvalid;
  logic                 i_tready;
  logic [   USER_W-1:0] i_tuser;
  logic [ SAMPLE_W-1:0] o_tdata;
  logic                 o_tlast;
  logic                 o_tvalid;
  logic                 o_tready;
  logic [   USER_W-1:0] o_tuser;

  dds_timed #(
    .SR_FREQ_ADDR      (SR_FREQ_ADDR),
    .SR_SCALE_IQ_ADDR  (SR_SCALE_IQ_ADDR),
    .PHASE_ACCUM_WIDTH (PHASE_ACCUM_WIDTH),
    .SCALING_WIDTH     (SCALING_WIDTH),
    .SR_AWIDTH         (SR_AWIDTH),
    .SR_DWIDTH         (SR_DWIDTH),
    .SR_TWIDTH         (SR_TWIDTH)
  ) dds_timed_i (
    .clk                 (clk),
    .reset               (rst),
    .clear               (clear),
    .timed_cmd_fifo_full (timed_cmd_fifo_full),
    .set_stb             (set_stb),
    .set_addr            (set_addr),
    .set_data            (set_data),
    .set_time            (set_time),
    .set_has_time        (set_has_time),
    .i_tdata             (to_dut.tdata),
    .i_tlast             (to_dut.tlast),
    .i_tvalid            (to_dut.tvalid),
    .i_tready            (to_dut.tready),
    .i_tuser             (to_dut.tuser),
    .o_tdata             (from_dut.tdata),
    .o_tlast             (from_dut.tlast),
    .o_tvalid            (from_dut.tvalid),
    .o_tready            (from_dut.tready),
    .o_tuser             (from_dut.tuser)
  );


  //---------------------------------------------------------------------------
  // Timer
  //---------------------------------------------------------------------------
  //
  // Count the samples going into the DUT so we have something that tracks
  // packet timestamp in the testbench.
  //
  //---------------------------------------------------------------------------

  longint current_time = 0;

  always @(posedge clk) begin
    if (to_dut.tvalid && to_dut.tready) begin
      current_time <= current_time + 1;
    end
  end


  //---------------------------------------------------------------------------
  // Expected Output
  //---------------------------------------------------------------------------
  //
  // This assigns the expected output to a signal so we can visualize what the
  // testbench is expecting. Error checking isn't done here. This is only to
  // aid in debug.
  //
  //---------------------------------------------------------------------------

  mailbox #(axis_pkt_t) exp_pkts_mb = new();
  bit exp_data_mismatch = 0;
  bit exp_user_mismatch = 0;

  logic [SAMPLE_W-1:0] exp_tdata;
  logic [  USER_W-1:0] exp_tuser;

  always @(posedge clk) begin
    if (rst) begin
      exp_tdata  = 'X;
      exp_tuser  = 'X;
    end else begin
      static axis_pkt_t exp_pkt = null;
      static bit        out_valid = 0;

      // Give time for the DUT to update its status, so we know what to do.
      #(0.01ns);

      // Output the next expected sample if we haven't done so already
      if (from_dut.tvalid && !out_valid) begin
        int rval;

        // Get the next packet from the mailbox if needed
        if (exp_pkt == null) begin
          rval = exp_pkts_mb.try_get(exp_pkt);
          `ASSERT_ERROR(rval, "Couldn't get first packet from exp_pkts_mb.");
        end else if (exp_pkt.data.size() == 0) begin
          rval = exp_pkts_mb.try_get(exp_pkt);
          `ASSERT_ERROR(rval, "Couldn't get next packet from exp_pkts_mb.");
        end

        // Output the next sample
        `ASSERT_ERROR(exp_pkt.data.size(), "exp_pkt.data is empty");
        exp_tdata = exp_pkt.data.pop_front();
        `ASSERT_ERROR(exp_pkt.user.size(), "exp_pkt.user is empty");
        exp_tuser = exp_pkt.user.pop_front();
        out_valid = 1;
      end

      exp_data_mismatch = compare_samples(exp_tdata, from_dut.tdata);
      exp_user_mismatch = compare_samples(exp_tuser, from_dut.tuser);

      // Check if the output has been accepted and needs to update
      if (from_dut.tvalid && from_dut.tready) begin
        out_valid = 0;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  // Round a floating point number to num_bits bits of precision.
  function automatic real round_bits(real num, int num_bits);
    return real'(longint'(num * (2.0**num_bits))) / (2.0**num_bits);
  endfunction : round_bits


  // Compare the samples a and b to see if either component differs by more
  // than MAX_ERROR.
  function automatic bit compare_samples(sc16_t a, sc16_t b);
    Math #(s16_t) m;
    sc16_t diff;
    diff = sub_sc16(a, b);
    if (m.abs(diff.re) > MAX_ERROR || m.abs(diff.im) > MAX_ERROR) return 1;
    return 0;
  endfunction : compare_samples


  // Compare the packets, sample by sample. Returns a string error message
  // explaining the nature of the mismatch. If packets match, an empty string
  // is returned.
  function automatic string compare_packets(axis_pkt_t actual, axis_pkt_t expected);
    if (actual.data.size() != expected.data.size()) begin
      return $sformatf("Packet lengths do not match. Actual is %0d, expected is %0d.",
        actual.data.size(), expected.data.size());
    end

    foreach(actual.data[i]) begin
      sc16_t a, b;

      // Check the samples in TDATA.
      // Calculate the difference between the actual end expected values.
      a = actual.data[i];
      b = expected.data[i];
      if (compare_samples(a, b)) begin
        `ASSERT_WARNING(0, "compare_packets: Skipping rest of packet due to mismatch.")
        return $sformatf("Word %0d in packet TDATA does not match. Actual is 0x%X, expected is 0x%X.",
          i, actual.data[i], expected.data[i]);
      end

      // Check TUSER. This is only guaranteed to be valid on the last sample of
      // each packet due to the way it's currently implemented.
      if (i == actual.data.size()-1 && actual.user[i] != expected.user[i]) begin
        string fields;
        if (actual.user[i][EOB_POS] != expected.user[i][EOB_POS]) begin
          fields = {fields, "(EOB)"};
        end
        if (actual.user[i][HAS_TIME_POS] != expected.user[i][HAS_TIME_POS]) begin
          fields = {fields, "(HAS_TIME)"};
        end
        if (actual.user[i][TIMESTAMP_POS+:64] != expected.user[i][TIMESTAMP_POS+:64]) begin
          fields = {fields, "(TIMESTAMP)"};
        end
        if (fields == "") fields = "<None>";
        `ASSERT_WARNING(0, "compare_packets: Skipping rest of packet due to mismatch.")
        return $sformatf({
          "Word %0d in packet TUSER does not match. ",
          "Fields not matching: %s. ",
          "Actual is %X, expected is %X."},
          i, fields, actual.user[i], expected.user[i]);
      end
    end
    // Return empty string if all is well
    return "";
  endfunction : compare_packets


  // Generate a test packet containing a complex sinusoid signal e^(j∙2π∙f∙t)
  // and return it.
  //
  //   length:     The length of the packet to generate in samples.
  //   freq:       Normalized frequency of the signal to generate.
  //   eob:        EOB flag for the packet.
  //   timed:      Set to 1 for a timed packet, 0 for non-timed. Timed is the
  //               default.
  //   timestamp:  Timestamp for the first packet. Leave at the default value
  //               to continue from the time of the previous packet.
  //   init:       Initial phase value to use (t in e^jt). Leave at the default
  //               value to use the last value of the previous packet. Must be
  //               in the range [0,1), where 1.0 corresponds to 2*pi radians.
  //
  function automatic axis_pkt_t gen_test_packet(
    int      length,
    real     freq,
    real     amp       = 0.75,
    bit      eob       = 0,
    longint  timed     = 1,
    longint  timestamp = -1,
    real     init      = -1.0
  );
    static real phase;
    static longint next_time = 0;
    bit signed [COMP_W-1:0] re, im;
    int re_int, im_int;
    logic [USER_W-1:0] user;
    axis_pkt_t packet;

    if (init != -1.0) begin
      phase = init;
    end

    if (timestamp >= 0) begin
      next_time = timestamp;
    end

    packet = new();
    for (int sample_num = 0; sample_num < length; sample_num++) begin
      // Calculate I/Q
      re_int = $cos(phase*TAU) * amp * 2**FRAC_W;
      im_int = $sin(phase*TAU) * amp * 2**FRAC_W;

      // Saturate
      if(re_int > MAX_COMP) re = MAX_COMP;
      else if(re_int < MIN_COMP) re = MIN_COMP;
      else re = re_int;
      if(im_int > MAX_COMP) im = MAX_COMP;
      else if(im_int < MIN_COMP) im = MIN_COMP;
      else im = im_int;

      // Calculate TUSER (header)
      user                      = '0;
      user[EOB_POS]             = eob;
      user[HAS_TIME_POS]        = timed;
      user[TIMESTAMP_POS +: 64] = timed ? next_time : 'X;

      // Enqueue the sample
      packet.data.push_back({re, im});
      packet.user.push_back(user);
      phase += freq;
    end

    // Calculate the timestamp for the next packet
    next_time += length;

    return packet;
  endfunction : gen_test_packet


  // Apply a frequency shift to the packet data, by multiplying each sample by
  // the output of a complex NCO. The implementation here models the HDL so
  // that we don't accumulate error over time.
  //
  //   packet       : Input packet with the samples to frequency shift.
  //   freq         : Normalized frequency shift to apply.
  //   reset_nco    : If 1, reset the NCO to 0 before beginning. Otherwise
  //                  continue from previous value.
  //   first_sample : First sample to frequency shift
  //   last_sample  : Last sample to frequency shift (inclusive)
  //
  //   Returns: A new packet with the frequency-shifted data.
  //
  function automatic axis_pkt_t freq_shift_pkt(
    axis_pkt_t packet,
    real       freq,
    bit        reset_nco = 0,
    int        first_sample = 0,
    int        last_sample = -1
  );
    // Normalized phase angle in the range [0,1), corresponding to [0,2π)
    // radians.
    static bit [PHASE_ACCUM_WIDTH-1:0] phase = 0;
    bit [PHASE_ACCUM_WIDTH-1:0] phase_inc;
    axis_pkt_t new_packet;

    new_packet = packet.copy();

    phase_inc = freq * (2.0**PHASE_ACCUM_WIDTH);
    if (reset_nco) begin
      phase = 0;
    end

    if (packet == null) return null;

    last_sample = last_sample < 0 ? packet.data.size()-1 : last_sample;
    for (int i = first_sample; i <= last_sample; i++) begin
      // There are a lot of redundant variables in this loop. This was done to
      // aid in debugging so we can correlate what's calculated here to what
      // the DUT computes, and to have both fixed-point and floating point
      // values.
      sc16_t in_sc16, out_sc16;
      complex_t nco;
      complex_t in_c, out_c;
      real phase_real;

      // Get the next input sample and convert it
      in_sc16 = packet.data[i];
      in_c = sc16_to_complex(in_sc16);

      // Convert the phase
      phase_real = real'(phase) / (2.0**PHASE_ACCUM_WIDTH);

      // Compute the new NCO value: nco = exp(j∙2π∙phase)
      nco = polar_to_complex(1.0, TAU * phase_real);

      // Compute the new data output: sample_out = nco * sample_in
      out_c = mul(nco, in_c);
      out_sc16 = complex_to_sc16(out_c);
      new_packet.data[i] = out_sc16;

      // Update the phase for the next iteration
      phase = phase + phase_inc;
    end
    return new_packet;
  endfunction : freq_shift_pkt

  // Return a scaled version of the input data packet. That is, where each
  // sample is multiplied by scale. This models the precision provided by the
  // scaler in the DUT.
  function automatic axis_pkt_t scale_packet(axis_pkt_t packet, real scale);
    bit        [SAMPLE_W-1:0] sample;
    bit signed [COMP_W-1:0]   re, im, a, b;
    int                       re_tmp, im_tmp;
    axis_pkt_t                new_packet;

    // Make sure scale is in the range supported by hardware
    if (scale > MAX_SCALE) scale = MAX_SCALE;
    else if (scale < MIN_SCALE) scale = MIN_SCALE;

    new_packet = packet.copy();
    foreach (packet.data[i]) begin
      sample = packet.data[i];
      re = sample[1*COMP_W +: COMP_W];
      im = sample[0*COMP_W +: COMP_W];
      // Scale with full precision
      re_tmp = re * scale;
      im_tmp = im * scale;
      // Saturate the values
      if (re_tmp > MAX_COMP) re = MAX_COMP;
      else if (re_tmp < MIN_COMP) re = MIN_COMP;
      else re = re_tmp;
      if (im_tmp > MAX_COMP) im = MAX_COMP;
      else if (im_tmp < MIN_COMP) im = MIN_COMP;
      else im = im_tmp;
      new_packet.data[i] = { re, im };
    end
    return new_packet;
  endfunction : scale_packet

  // Generate the output packets we expect from the DUT given the provided
  // burst of packets and configuration.
  //
  //   cfg     : Burst test configuration used
  //   packets : Queue of packets that were input to the DUT
  //
  //   returns : Expected packets from DUT
  //
  function automatic axis_pkt_queue_t generate_expected(
    burst_cfg_t      cfg,
    axis_pkt_queue_t packets
  );
    static longint timestamp = 0;
    axis_pkt_t expected[$];
    axis_pkt_t packet;
    bit reset_nco;
    int first_sample;
    int last_sample;
    real freq_shift;

    freq_shift = cfg.freq_shift;

    foreach(packets[i]) begin
      // Make a copy of the input
      packet = packets[i].copy();

      // Check if we're supposed to tune the frequency in this packet
      first_sample = 0;
      if (cfg.timed && timestamp <= cfg.tune_time &&
          timestamp + packet.data.size() > cfg.tune_time) begin
        last_sample = cfg.tune_time - timestamp;
      end else begin
        last_sample = -1;
      end

      // Apply a frequency shift (reset the NCO before each burst)
      reset_nco = i % cfg.num_packets == 0;
      packet = freq_shift_pkt(packet, freq_shift, reset_nco, first_sample, last_sample);

      // If there was a tune, shift the rest of the packet differently
      if (last_sample >= 0 && last_sample < packet.data.size()) begin
        freq_shift = cfg.tune_freq_shift;
        reset_nco = 1;
        first_sample = last_sample + 1;
        last_sample = -1;
        packet = freq_shift_pkt(packet, freq_shift, reset_nco, first_sample, last_sample);
      end

      // Multiply packet samples by a scaler
      packet = scale_packet(packet, cfg.scale);

      // Add this packet to the queue
      expected.push_back(packet);

      // Send this packet to the expected packets mailbox, for debug
      `ASSERT_ERROR(exp_pkts_mb.try_put(packet.copy()), "Unable to put expected packet");

      // Calculate new timestamp
      timestamp += packet.data.size();
    end

    return expected;
  endfunction : generate_expected


  // Generate a queue of packets modeled after the burst test configuration
  // defined by cfg.
  function automatic axis_pkt_queue_t generate_bursts(burst_cfg_t cfg);
    axis_pkt_t packets[$];

    // Reset initial phase and time to 0 in generated packets by calling the
    // generator with init and timestamp set to 0.
    void'(gen_test_packet(.length(0), .freq(0), .init(0)));

    // Build the packets to send
    for (int burst_num = 0; burst_num < cfg.num_bursts; burst_num++) begin
      for (int packet_num = 0; packet_num < cfg.num_packets; packet_num++) begin
        axis_pkt_t packet;
        bit eob;
        int length;

        // Set EOB and use spp_last for the last packet
        if (packet_num == cfg.num_packets-1) begin
          eob = 1;
          length = (cfg.spp_last > 0) ? cfg.spp_last : cfg.spp;
        end else begin
          eob = 0;
          length = cfg.spp;
        end

        packet = gen_test_packet(
          .length    (length),
          .freq      (cfg.freq),
          .amp       (cfg.amp),
          .eob       (eob),
          .timed     (cfg.timed));
        packets.push_back(packet);
      end
    end

    return packets;
  endfunction : generate_bursts


  // Write a value to a settings register.
  //
  //  addr      : Address of the register to write to.
  //  value     : Value to write to the register.
  //  timestamp : Timestamp to provide with the write. Set to -1 if the write
  //              should not be timed.
  //
  task automatic write_reg(
    bit [SR_AWIDTH-1:0] addr,
    bit [SR_DWIDTH-1:0] value,
    longint             timestamp = -1
  );
    @(posedge clk);
    set_stb      <= 1;
    set_addr     <= addr;
    set_data     <= value;
    set_time     <= (timestamp > 0) ? timestamp : 'X;
    set_has_time <= (timestamp > 0);
    @(posedge clk);
    set_stb      <= 0;
    set_addr     <= 'X;
    set_data     <= 'X;
    set_time     <= 'X;
    set_has_time <= 'X;
    @(posedge clk);
  endtask : write_reg


  // Write a value to the frequency register.
  //
  //  freq      : Normalized frequency to write to the register. E.g., in the
  //              range [-0.5,0.5) or [0,1). Numerically, either works.
  //  timestamp : Timestamp to provide with the write. Set to -1 if the write
  //              should not be timed.
  //
  task automatic write_reg_freq(real freq, longint timestamp = -1);
    write_reg(SR_FREQ_ADDR, freq * (2.0**PHASE_ACCUM_WIDTH), timestamp);
  endtask : write_reg_freq


  // Write a value to the scale register.
  //
  //  scale     : Scaler to write to the register, in the range [-4,4).
  //  timestamp : Timestamp to provide with the write. Set to -1 if the write
  //              should not be timed.
  //
  task automatic write_reg_scale(real scale);
    // Saturate to the range allowed by the register
    scale = scale > MAX_SCALE ? MAX_SCALE : scale;
    scale = scale < MIN_SCALE ? MIN_SCALE : scale;
    write_reg(SR_SCALE_IQ_ADDR, scale * (2.0**FRAC_W));
  endtask : write_reg_scale


  // Check that the output matches what we would expect.
  //
  //   cfg:     Test configuration
  //   packets: The packets that were input to the DUT
  //
  task automatic verify_output(burst_cfg_t cfg, axis_pkt_queue_t packets);
    axis_pkt_t expected[$];

    expected = generate_expected(cfg, packets);

    foreach(packets[i]) begin
      axis_pkt_t recvd;
      string msg;
      axis_bfm.get(recvd);
      msg = compare_packets(recvd, expected[i]);
      `ASSERT_ERROR(msg == "",
        $sformatf("Error in packet %0d: %s", i, msg));
    end
  endtask : verify_output


  // Test a burst (i.e., multiple packets ending with EOB) through the DUT
  // using the provided configuration.
  task automatic test_bursts(burst_cfg_t cfg);
    axis_pkt_t packets[$];

    // Are we doing timed packets?
    cfg.timed = (cfg.timed || cfg.tune_time >= 0);

    // Set the registers
    write_reg_scale(cfg.scale);
    write_reg_freq(cfg.freq_shift);

    // Schedule a timed tune, if requested
    if (cfg.tune_time >= 0) begin
      write_reg_freq(cfg.tune_freq_shift, cfg.tune_time);
    end

    // Wait a bit for the register changes to take effect
    clk_gen.clk_wait_r(10);


    // Generate test packets to send
    packets = generate_bursts(cfg);

    // Send the packets
    foreach (packets[i]) axis_bfm.put(packets[i]);

    // Check the results
    verify_output(cfg, packets);
  endtask : test_bursts


  //---------------------------------------------------------------------------
  // Test Procedures
  //---------------------------------------------------------------------------

  // This performs a few directed test as a sanity check and to test a few
  // corner cases.
  task automatic directed_tests();
    burst_cfg_t cfg;

    // Iterate over different flow control settings to exercise different
    // scenarios.
    for (int bfm_config = 0; bfm_config < 4; bfm_config++) begin
      case (bfm_config)
        0 : begin
          // No stalls: on input or output to DUT
          axis_bfm.set_master_stall_prob(0);
          axis_bfm.set_slave_stall_prob(0);
        end
        1 : begin
          // Overflow: Input to DUT faster than output
          axis_bfm.set_master_stall_prob(10);
          axis_bfm.set_slave_stall_prob(30);
        end
        2 : begin
          // Underflow: Input to DUT slower than output
          axis_bfm.set_master_stall_prob(30);
          axis_bfm.set_slave_stall_prob(10);
        end
        3 : begin
          // Lots of stalls: Input and output stall frequently
          axis_bfm.set_master_stall_prob(40);
          axis_bfm.set_slave_stall_prob(40);
        end
      endcase

      //-------------------------------
      // Test Basic Configurations
      //-------------------------------

      // Test the default configuration
      cfg = DEFAULT_BURST_CFG;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();

      // Test a somewhat arbitrary but different configuration
      cfg = DEFAULT_BURST_CFG;
      cfg.spp         = 97;
      cfg.spp_last    = 33;
      cfg.num_bursts  = 2;
      cfg.num_packets = 3;
      cfg.amp         = 0.5;
      cfg.scale       = 1.25;
      cfg.freq        = 0.23;
      cfg.freq_shift  = 0.17;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();

      // Repeat with a single-sample packet
      cfg.spp      = 1;
      cfg.spp_last = 1;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();

      //-------------------------------
      // Test timed tunes
      //-------------------------------

      cfg = DEFAULT_BURST_CFG;
      cfg.spp             = 135;
      cfg.freq            = 1.0/32.0;
      cfg.freq_shift      = 0.0;
      cfg.num_bursts      = 1;
      cfg.num_packets     = 3;
      cfg.scale           = 0.75;
      cfg.freq_shift      = 0.0;    // Initial frequency shift
      cfg.tune_freq_shift = 0.13;   // New frequency shift

      // Test tuning in the middle of a packet
      cfg.tune_time = current_time + cfg.num_packets*cfg.spp/2;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();

      // Test tuning at the end of the first packet
      cfg.tune_time = current_time + cfg.spp-1;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();

      // Test tuning at the beginning of a packet
      cfg.tune_time = current_time + cfg.spp;
      test.start_test($sformatf("Directed Test: bfm_config: %0d, %p", bfm_config, cfg));
      test_bursts(cfg);
      test.end_test();
    end
  endtask : directed_tests


  // This generates a randomized configuration exercises the DUT with that
  // configuration. This is repeated num_tests times, with a unique
  // configuration each time.
  task automatic random_tests(int num_tests);
    burst_cfg_t cfg;
    int master_stall_prob, slave_stall_prob;

    repeat (num_tests) begin
      // Choose random values for this run. Round the floating point numbers to
      // a smaller number of bits to reduce rounding differences between the
      // testbench and the DUT.
      cfg = DEFAULT_BURST_CFG;
      cfg.spp             = $urandom_range(1, 64);
      cfg.spp_last        = $urandom_range(1, 64);
      cfg.num_packets     = $urandom_range(1, 3);
      cfg.num_bursts      = $urandom_range(1, 2);
      cfg.amp             = round_bits(frand_range(1.0/16.0, 15.0/16.0), 15);
      cfg.freq            = frand(0.5);
      cfg.timed           = $urandom_range(0, 1);
      cfg.scale           = round_bits(frand_range(-4.0, 4.0), 15);
      cfg.freq_shift      = round_bits(frand(0.5), 32);
      cfg.tune_freq_shift = round_bits(frand(0.5), 32);
      if (cfg.timed) begin
        cfg.tune_time = current_time +
          $urandom_range(0, (cfg.num_packets-1)*cfg.spp + cfg.spp_last - 1);
      end
      master_stall_prob = $urandom_range(0, 50);
      slave_stall_prob  = $urandom_range(0, 50);

      // Run the test
      test.start_test($sformatf("Random Test: InStall: %0d, OutStall: %0d, %p",
        master_stall_prob, slave_stall_prob, cfg));
      axis_bfm.set_master_stall_prob(master_stall_prob);
      axis_bfm.set_slave_stall_prob(slave_stall_prob);
      test_bursts(cfg);
      test.end_test();
    end
  endtask : random_tests


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : main
    test.start_tb("dds_timed_tb");

    // Start the BFMs running
    axis_bfm.run();

    // Reset
    clk_gen.reset();
    @(negedge rst);

    //-------------------------------
    // Run Tests
    //-------------------------------

    directed_tests();
    random_tests(200);

    test.end_tb();
  end

endmodule


`default_nettype wire
