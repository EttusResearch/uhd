//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fft_tb
//
// Description: Testbench for the FFT RFNoC block.
//

`default_nettype none


module rfnoc_block_fft_tb #(
  bit    FULL_TEST                = 1,
  int    NUM_PORTS                = 1,
  int    NUM_CORES                = 1,
  int    MAX_FFT_SIZE_LOG2        = 10,
  int    MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int    MAX_CP_LIST_LEN_REM_LOG2 = 5,
  bit    EN_MAGNITUDE_SQ          = 1,
  bit    EN_MAGNITUDE             = 1,
  bit    EN_FFT_ORDER             = 1,
  bit    EN_FFT_BYPASS            = 1,
  bit    USE_APPROX_MAG           = 0,
  bit    VERBOSE                  = 0
);

  `include "test_exec.svh"
  `include "usrp_utils.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  import PkgMath::*;

  // Import register descriptions
  import fft_core_regs_pkg::*;

  // Import Xilinx FFT helper functions
  import xfft_config_pkg::*;

  // FFT reorder constants
  import fft_reorder_pkg::*;


  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  // Block configuration
  localparam int NUM_CHAN_PER_CORE   = NUM_PORTS / NUM_CORES;
  localparam int MAX_CP_LIST_LEN_INS = 2**MAX_CP_LIST_LEN_INS_LOG2-1;
  localparam int MAX_CP_LIST_LEN_REM = 2**MAX_CP_LIST_LEN_REM_LOG2-1;
  localparam int CP_INSERTION_REPEAT = 1;
  localparam int CP_REMOVAL_REPEAT   = 1;
  localparam int MAX_FFT_SIZE        = 2**MAX_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN_LOG2     = MAX_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN          = 2**MAX_CP_LEN_LOG2-1;
  localparam int MIN_FFT_SIZE_LOG2   = 3;  // Minimum allowed by Xilinx FFT core
  localparam int MIN_FFT_SIZE        = 2**MIN_FFT_SIZE_LOG2;
  localparam int FFT_SCALING         = fft_scale_default(MAX_FFT_SIZE_LOG2);

  // RFNoC configuration
  localparam [9:0] THIS_PORTID    = 10'h123;
  localparam int   CHDR_W         = 64;     // CHDR size in bits
  localparam int   MTU            = 10;     // Log2 of max transmission unit in CHDR words
  localparam int   NUM_PORTS_I    = NUM_PORTS;
  localparam int   NUM_PORTS_O    = NUM_PORTS;
  localparam int   ITEM_W         = 32;     // Sample size in bits
  localparam int   SPP            = 64;     // Samples per packet. Must be a power of 2 for FFT.
  localparam int   PKT_SIZE_BYTES = SPP * (ITEM_W/8);
  localparam int   STALL_PROB     = 50;    // Default BFM stall probability
  localparam real  CHDR_CLK_PER   = 5.0;   // 200 MHz
  localparam real  CTRL_CLK_PER   = 8.0;   // 125 MHz
  localparam real  CE_CLK_PER     = 4.0;   // 250 MHz


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(.PERIOD(CHDR_CLK_PER), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CTRL_CLK_PER), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(.PERIOD(CE_CLK_PER),   .AUTOSTART(0))
    ce_clk_gen (.clk(ce_clk), .rst());


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

  typedef item_t item_queue_t[$];

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

  rfnoc_block_fft #(
    .THIS_PORTID             (THIS_PORTID),
    .CHDR_W                  (CHDR_W),
    .MTU                     (MTU),
    .NUM_PORTS               (NUM_PORTS),
    .NUM_CORES               (NUM_CORES),
    .MAX_FFT_SIZE_LOG2       (MAX_FFT_SIZE_LOG2),
    .MAX_CP_LIST_LEN_INS_LOG2(MAX_CP_LIST_LEN_INS_LOG2),
    .MAX_CP_LIST_LEN_REM_LOG2(MAX_CP_LIST_LEN_REM_LOG2),
    .CP_INSERTION_REPEAT     (CP_INSERTION_REPEAT),
    .CP_REMOVAL_REPEAT       (CP_REMOVAL_REPEAT),
    .EN_FFT_BYPASS           (EN_FFT_BYPASS),
    .EN_FFT_ORDER            (EN_FFT_ORDER),
    .EN_MAGNITUDE            (EN_MAGNITUDE),
    .EN_MAGNITUDE_SQ         (EN_MAGNITUDE_SQ),
    .USE_APPROX_MAG          (USE_APPROX_MAG)
  ) dut (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk),
    .ce_clk             (ce_clk),
    .rfnoc_core_config  (backend.cfg),
    .rfnoc_core_status  (backend.sts),
    .s_rfnoc_chdr_tdata (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid(s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready(s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid(m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready(m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata (m_ctrl.tdata),
    .s_rfnoc_ctrl_tlast (m_ctrl.tlast),
    .s_rfnoc_ctrl_tvalid(m_ctrl.tvalid),
    .s_rfnoc_ctrl_tready(m_ctrl.tready),
    .m_rfnoc_ctrl_tdata (s_ctrl.tdata),
    .m_rfnoc_ctrl_tlast (s_ctrl.tlast),
    .m_rfnoc_ctrl_tvalid(s_ctrl.tvalid),
    .m_rfnoc_ctrl_tready(s_ctrl.tready)
  );


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  task automatic write_reg (
    int unsigned addr,
    logic [31:0] write_val,
    int          core
  );
    int base_addr = core * (2**FFT_CORE_ADDR_W);
    `ASSERT_ERROR(addr < 2**REG_ADDR_W, "Register address is out of bounds");
    `ASSERT_ERROR(core < NUM_CORES, "Specified FFT core does not exist");
    blk_ctrl.reg_write(base_addr + addr, write_val);
  endtask


  task automatic read_reg (
    input  int unsigned addr,
    output logic [31:0] read_val,
    input  int          core
  );
    int base_addr = core * (2**FFT_CORE_ADDR_W);
    `ASSERT_ERROR(addr < 2**REG_ADDR_W, "Register address is out of bounds");
    `ASSERT_ERROR(core < NUM_CORES, "Specified FFT core does not exist");
    blk_ctrl.reg_read(base_addr + addr, read_val);
  endtask


  task automatic user_reset (int core);
    int dummy_val;
    write_reg(REG_RESET_ADDR, 1'b1, core);
    // Do a dummy read to ensure reset has time to complete
    read_reg(REG_RESET_ADDR, dummy_val, core);
  endtask


  // Configures the FFT core using the given parameters.
  //
  //   fft_size,       : Length of FFT (e.g., 4096 for 4k FFT)
  //   cp_insertions[] : List of cyclic-prefix insertions to load
  //   cp_removals[]   : List of cyclic-prefix removals to load
  //   fft_scaling     : Scaling value to be passed to FFT core
  //   fft_direction   : Direction of FFT (FFT_FORWARD, FFT_INVERSE)
  //   cp_list_clear   : When true, cyclic-prefix list will be reset
  //   fft_bypass      : Enable FFT bypass feature
  //   fft_order_sel   : FFT output order selection
  //   magnitude_sel   : Magnitude output selection
  //   core            : Which FFT core to configure
  //
  task automatic config_fft (
    int fft_size,
    int cp_insertions[] = {},
    int cp_removals[]   = {},
    int fft_scaling     = fft_scale_default($clog2(fft_size)),
    bit fft_direction   = FFT_FORWARD,
    bit cp_list_clear   = 1,
    bit fft_bypass      = 0,
    int fft_order_sel   = FFT_ORDER_NATURAL,
    int magnitude_sel   = 0,
    int core            = 0
  );
    logic [31:0] fft_size_log2 = $clog2(fft_size);

    if (cp_list_clear) begin
      if (VERBOSE) $display("config_fft(): Clearing Cyclic Prefix insertion FIFO");
      write_reg(REG_CP_INS_LIST_CLR_ADDR, 1'b1, core);
      if (VERBOSE) $display("config_fft(): Clearing Cyclic Prefix removal FIFO");
      write_reg(REG_CP_REM_LIST_CLR_ADDR, 1'b1, core);
    end
    if (VERBOSE) $display("config_fft(): Setting FFT Size (Log2) to %0d", fft_size_log2);
    write_reg(REG_LENGTH_LOG2_ADDR, fft_size_log2, core);
    if (VERBOSE) $display("config_fft(): Setting FFT Scaling to 0x%8h", fft_scaling);
    write_reg(REG_SCALING_ADDR, fft_scaling, core);
    if (VERBOSE) $display("config_fft(): Setting FFT Direction to %0d", fft_direction);
    write_reg(REG_DIRECTION_ADDR, fft_direction, core);
    foreach (cp_insertions[i]) begin
      `ASSERT_ERROR(
        cp_insertions[i] < fft_size,
        "Cyclic prefix insertion length must be less than FFT size"
      );
      if (VERBOSE) $display("config_fft(): Setting Cyclic Prefix (insertion) %0d",
        cp_insertions[i]);
      write_reg(REG_CP_INS_LEN_ADDR, cp_insertions[i], core);
      write_reg(REG_CP_INS_LIST_LOAD_ADDR, 1'b1, core);
    end
    foreach (cp_removals[i]) begin
      `ASSERT_ERROR(
        cp_removals[i] < fft_size,
        "Cyclic prefix removal length must be less than FFT size"
      );
      if (VERBOSE) $display("config_fft(): Setting Cyclic Prefix (removal) %0d",
        cp_removals[i]);
      write_reg(REG_CP_REM_LEN_ADDR, cp_removals[i], core);
      write_reg(REG_CP_REM_LIST_LOAD_ADDR, 1'b1, core);
    end
    if (EN_FFT_BYPASS) begin
      if (VERBOSE) $display("config_fft(): Setting FFT Bypass to %0d", fft_bypass);
      write_reg(REG_BYPASS_ADDR, fft_bypass, core);
    end
    if (EN_FFT_ORDER) begin
      if (VERBOSE) $display("config_fft(): Setting FFT Order to %0d", fft_order_sel);
      write_reg(REG_ORDER_ADDR, fft_order_sel, core);
    end
    if (EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin
      if (VERBOSE) $display("config_fft(): Setting Magnitude to %0d", magnitude_sel);
      write_reg(REG_MAGNITUDE_ADDR, magnitude_sel, core);
    end
  endtask


  // Test a readable/writable register
  task automatic test_rw_reg (
    input string       reg_name,
    input int unsigned addr,
    input int unsigned bit_width = 32,
    input int unsigned core      = 0
  );

    logic [31:0] init_val, write_val, read_val, check_val;
    string s;

    if (VERBOSE) $display("test_rw_reg(): Set and check %s register... ", reg_name);
    read_reg(addr, init_val, core);
    repeat (2) begin
      write_val = $random();
      write_reg(addr, write_val, core);
      read_reg(addr, read_val, core);
      check_val = write_val & 32'((1 << bit_width)-1); // Mask relevant bits
      `ASSERT_FATAL(
        read_val == check_val,
        $sformatf("%s register incorrect readback! Expected: %0d, Actual %0d",
          reg_name, check_val, read_val)
      );
    end
    write_reg(addr, init_val, core);
  endtask


  // Test read-only register
  task automatic test_ro_reg (
    input string       reg_name,
    input int unsigned addr,
    input int unsigned value,
    input int unsigned bit_width = 32,
    input int unsigned core      = 0
  );
    logic [31:0] read_val;
    string s;

    if (VERBOSE) $display("test_ro_reg(): Read and check %s register... ", reg_name);
    read_reg(addr, read_val, core);
    read_val = read_val & 32'((1 << bit_width)-1); // Mask relevant bits
    `ASSERT_FATAL(
      value == read_val,
      $sformatf("%s register incorrect readback! Expected: %0d, Actual %0d",
        reg_name, value, read_val)
    );
  endtask


  // Generates a complex sine wave with period of 4 samples.
  //
  //   num_samples : The number of samples to generate
  //   phase       : The phase offset of the first sample. -1 would start one
  //                 sample before the real part crosses y = 9.
  //   amplitude   : Amplitude of the signal
  //
  function automatic item_queue_t gen_sine_wave(
    int          num_samples,
    int          phase = 0,
    logic [15:0] amplitude = 16'd16383
  );
    item_t samples [] = new [num_samples];

    foreach (samples[samp_i]) begin
      case (unsigned'(samp_i + phase) % 4)
        0: samples[samp_i] = {  amplitude,      16'd0 };
        1: samples[samp_i] = {      16'd0,  amplitude };
        2: samples[samp_i] = { -amplitude,      16'd0 };
        3: samples[samp_i] = {      16'd0, -amplitude };
      endcase
    end
    return samples;
  endfunction


  // Generates a complex sine wave in the frequency domain.
  //
  //   num_samples : The number of samples to generate
  //   f_norm      : Normalized frequency
  //   amplitude   : Amplitude in the frequency domain
  //
  function automatic item_queue_t gen_tone(
    int          num_samples,
    real         f_norm = 0,
    logic [15:0] amplitude = 16'd16383
  );
    item_t samples [] = new [num_samples];
    samples = '{default: 0};
    samples[int'(f_norm*num_samples)] = amplitude;
    return samples;
  endfunction


  // Test the FFT block by putting a single frequency signal through it.
  //
  //   fft_size        : Size of the FFT to test (e.g., 4096 for 4k FFT)
  //   num_ffts        : Number of complete FFTs to test
  //   cp_insertions[] : Cyclic-prefix insertion list to use
  //   cp_removals[]   : Cyclic-prefix removal list to use
  //   pkt_size        : Packet size to use
  //   timed           : Test timed (1) or untimed (0) packets
  //   core            : Which FFT core to test
  //
  task automatic test_fft_sine(
    int fft_size,
    int num_ffts        = 1,
    int cp_insertions[] = {},
    int cp_removals[]   = {},
    int pkt_size        = fft_size < SPP ? fft_size : SPP,
    bit timed           = 1,
    int core            = 0
  );
    int first_port = core*NUM_CHAN_PER_CORE;
    int last_port  = (core+1)*NUM_CHAN_PER_CORE - 1;

    if (VERBOSE) begin
      $display("test_fft_sine():");
      $display("    fft_size:      %0d", fft_size);
      $display("    num_ffts:      %0d", num_ffts);
      $display("    cp_insertions: %p",  cp_insertions);
      $display("    cp_removals:   %p",  cp_removals);
      $display("    pkt_size:      %0d", pkt_size);
      $display("    timed:         %0d", timed);
      $display("    core:          %0d", core);
    end

    // Having both insertion and removal might work, but that's not a use case
    // we're supporting.
    assert(!(cp_insertions.size() && cp_removals.size())) else
      `ASSERT_ERROR(0, "Cannot specify both CP insertion and CP removal");

    config_fft(fft_size, cp_insertions, cp_removals, fft_scale_default(fft_size),
      FFT_FORWARD, 1, .core(core));

    // Create a thread for the sender and one (or more) for the receiver(s).
    fork
      begin : sender
        item_queue_t  send_data;
        packet_info_t send_pkt_info;
        int           cp_removal_len;
        int           num_pkts;
        int           total_fft_size;
        int           samp_count;

        send_pkt_info = '0;

        if (VERBOSE) $display("test_fft_sine(): send: Start send thread");

        // Generate the data we're going to send
        for (int fft_count = 0; fft_count < num_ffts; fft_count++) begin
          // This emulates the CP removal FIFO's behavior
          if (cp_removals.size() == 0) cp_removal_len = 0;
          else cp_removal_len = cp_removals[fft_count % cp_removals.size()];

          total_fft_size += fft_size + cp_removal_len;

          if (VERBOSE) begin
            $display("test_fft_sine(): send: Generating FFT %0d for %0d+%0d = %0d samples",
              fft_count, cp_removal_len, fft_size, fft_size + cp_removal_len);
          end

          // Generate the data we're going to send
          send_data = {send_data, gen_sine_wave(fft_size + cp_removal_len, -cp_removal_len)};
        end

        num_pkts = $ceil(real'(send_data.size()) / real'(pkt_size));

        // Send the data one packet at a time
        samp_count = 0;
        for (int pkt_count = 0; pkt_count < num_pkts; pkt_count++) begin
          int start, len;

          start = pkt_count * pkt_size;
          len = `MIN(total_fft_size - pkt_count*pkt_size, pkt_size);

          send_pkt_info.eob = (pkt_count == num_pkts-1);

          if (VERBOSE) begin
            $display("test_fft_sine(): send: PKT %0d: Sending %0d samples (EOB=%b)",
              pkt_count, len, send_pkt_info.eob);
          end

          if (timed) begin
            send_pkt_info.has_time = 1;
            send_pkt_info.timestamp = samp_count + 'hDEADBEEF;
          end

          for (int port = first_port; port <= last_port; port++) begin
            blk_ctrl.send_items(port, send_data[start : start+len-1], , send_pkt_info);
          end
          blk_ctrl.wait_complete(first_port);

          samp_count += len;
        end
        if (VERBOSE) $display("test_fft_sine(): send: End send thread");
      end : sender


      // Create one thread for each port to be verified
      begin : receiver
        for (int port_num = first_port; port_num <= last_port; port_num++) begin : recv_port_loop
          fork
            int port = port_num; // Pass the unique port value to each thread
            begin : receive_thread
              item_queue_t  recv_payload, recv_data;
              chdr_word_t   recv_metadata[$];
              packet_info_t recv_pkt_info;
              longint       timestamp = 'hDEADBEEF;
              int           samp_count, fft_samp_count;

              if (VERBOSE) $display("test_fft_sine(): recv: Start recv thread for port %0d", port);

              // Receive the burst
              for (int pkt_count = 0; ; pkt_count++) begin
                blk_ctrl.recv_items_adv(port, recv_payload, recv_metadata, recv_pkt_info);
                recv_data = { recv_data, recv_payload };

                if (VERBOSE) begin
                  $display("test_fft_sine(): recv: PKT %0d: Received %0d samples (EOV=%b, EOB=%b)",
                    pkt_count, recv_payload.size(), recv_pkt_info.eov, recv_pkt_info.eob);
                end

                // Check timestamp
                if (timed && pkt_count == 0) begin
                  `ASSERT_ERROR(
                    recv_pkt_info.has_time,
                    $sformatf({"test_fft_sine(): recv: PKT %0d: ",
                      "Expected timestamp on packet"},
                      pkt_count)
                  );
                  `ASSERT_ERROR(
                    recv_pkt_info.timestamp == timestamp,
                    $sformatf({"test_fft_sine(): recv: PKT %0d: ",
                      "Expected timestamp of 0x%X, received 0x%x"},
                      pkt_count, timestamp, recv_pkt_info.timestamp)
                  );
                  timestamp += recv_payload.size();
                end else begin
                  `ASSERT_ERROR(
                    recv_pkt_info.has_time == 0,
                    $sformatf({"test_fft_sine(): recv: PKT %0d: ",
                      "Unexpected timestamp"},
                      pkt_count)
                  );
                end

                if (cp_removals.size() == 0 && cp_insertions.size() == 0 &&
                  recv_data.size() % fft_size == 0) begin
                  `ASSERT_ERROR(recv_pkt_info.eov,
                    "test_fft_sine(): recv: EOV is not set on FFT multiple");
                end

                if (recv_pkt_info.eob) break;
              end


              // Verify the received data
              samp_count = 0;
              for (int fft_count = 0; fft_count < num_ffts; fft_count++) begin
                int peak_index;
                int cp_peak_index;
                int cp_insertion_len;
                int total_fft_size;

                // This emulates the CP insertion FIFO's behavior
                if (cp_insertions.size() == 0) cp_insertion_len = 0;
                else cp_insertion_len = cp_insertions[fft_count % cp_insertions.size()];

                total_fft_size = fft_size + cp_insertion_len;

                if (VERBOSE) begin
                  $display("test_fft_sine(): recv: Checking FFT %0d for %0d+%0d = %0d samples",
                    fft_count, cp_insertion_len, fft_size, total_fft_size);
                end

                // We should see a peak in the FFT region but may see one in
                // the cyclic prefix as well if it is long enough.
                peak_index = cp_insertion_len + fft_size/4;
                cp_peak_index = peak_index - fft_size;

                recv_payload = recv_data[samp_count : samp_count+total_fft_size-1];

                // Verify the sample values
                fft_samp_count = 0;
                foreach (recv_payload[samp_i]) begin
                  if (fft_samp_count == peak_index || fft_samp_count == cp_peak_index) begin
                    bit signed [15:0] real_val, imag_val;
                    int magnitude;
                    {real_val, imag_val} = recv_payload[samp_i];
                    magnitude = $sqrt(real_val**2 + imag_val**2);
                    `ASSERT_ERROR(
                      imag_val == 0,
                      $sformatf({"test_fft_sine(): recv: FFT %0d: ",
                        "Expected 0 for im at sample %0d, received 0x%x"},
                        fft_count, fft_samp_count, recv_payload[samp_i])
                    );
                    `ASSERT_ERROR(
                      real_val >= 16368,
                      $sformatf({"test_fft_sine(): recv: FFT %0d: ",
                        "Expected re >= 16368 at sample %0d, received 0x%x"},
                        fft_count, fft_samp_count, recv_payload[samp_i])
                    );
                  end else begin
                    `ASSERT_ERROR(
                      recv_payload[samp_i] == 0,
                      $sformatf({"test_fft_sine(): recv: FFT %0d: ",
                        "Expected 0 at sample %0d, received 0x%x"},
                        fft_count, fft_samp_count, recv_payload[samp_i])
                    );
                  end
                  fft_samp_count++;
                end

                samp_count += total_fft_size;
              end

              // Check the length
              `ASSERT_ERROR(
                samp_count == recv_data.size(),
                $sformatf({"test_fft_sine(): recv: ",
                  "Expected %0d samples for this burst but received %0d"},
                  samp_count, recv_data.size())
              );

              if (VERBOSE) $display("test_fft_sine(): recv: End recv thread for port %0d", port);

            end : receive_thread
          join_none
        end : recv_port_loop

        // Wait for all receive threads to finish
        wait fork;
      end : receiver
    join
  endtask


  // Calculates and returns the expected value for the magnitude calculation.
  function automatic item_queue_t calc_magnitude(item_queue_t data_in, int select);
    item_queue_t data_out;
    if (select == MAG_SEL_NONE) begin
      return data_in;
    end else begin
      int re, im, magnitude;
      bit signed [15:0] re16, im16;
      foreach (data_in[i]) begin
        // Resize with sign extension to int. Done in two steps to make Vivado
        // XSim happy. Otherwise, we'd do: re = signed'(data_in[i][31:16]);
        re16 = data_in[i][31:16];
        im16 = data_in[i][15:0];
        re = re16;
        im = im16;
        if (select == MAG_SEL_MAG && USE_APPROX_MAG) begin
          magnitude = `MAX(`ABS(re), `ABS(im)) + `MIN(`ABS(re), `ABS(im))/4;
        end else if (select == MAG_SEL_MAG && !USE_APPROX_MAG) begin
          magnitude = int'($sqrt(re**2 + im**2));
        end else if (select == MAG_SEL_MAG_SQ) begin
          magnitude = (re**2 + im**2) / 32'sh8000;
        end else begin
          `ASSERT_FATAL(0, "Invalid magnitude selection")
        end
        if (magnitude > 16'sh7FFF) magnitude = 16'h7FFF;
        data_out[i] = magnitude << 16;
      end
      return data_out;
    end
  endfunction


  // Check if two arrays of samples are approximately equal. Returns 1 if every
  // component of every sample is <= thresh. Otherwise returns 0.
  function automatic bit approx_equal(
    item_queue_t left,
    item_queue_t right,
    int thresh = 1
  );
    if (left.size() != right.size()) return 0;
    foreach (left[i]) begin
      bit signed [15:0] left_re, right_re, left_im, right_im;
      if (left[i] == right[i]) continue;
      left_re  = left [i][31:16];
      left_im  = left [i][15: 0];
      right_re = right[i][31:16];
      right_im = right[i][15: 0];
      if (`ABS(left_re - right_re) > thresh || `ABS(left_im - right_im) > thresh) begin
        $display("At index %0d, left = 0x%X, right = 0x%X", i,
          {left_re, left_im}, {right_re, right_im});
        return 0;
      end
    end
    return 1;
  endfunction


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Test a sequence of random configurations
  task automatic test_random(
    int num_iterations,
    int max_fft_size = MAX_FFT_SIZE,
    int min_fft_size = MIN_FFT_SIZE
  );
    test.start_test(
      $sformatf({
        "Test Random\n",
        "    num_iterations: %0d\n",
        "    max_fft_size:   %0d\n",
        "    min_fft_size:   %0d"},
        num_iterations, max_fft_size, min_fft_size
      ), num_iterations*max_fft_size*30ns
    );

    for (int test_iter = 0; test_iter < num_iterations; test_iter++) begin
      bit timed = $urandom_range(0, 1);
      int fft_size = 2**$urandom_range($clog2(min_fft_size), $clog2(max_fft_size));
      int num_ffts = $urandom_range(1, 3);
      int cp_mode = $urandom_range(0, 2);  // 0=None, 1=removal, 2=insertion
      bit has_cp = $urandom_range(0, 1);
      int cp_list_len = $urandom_range(0,
        `MIN(num_ffts-1, `MIN(MAX_CP_LIST_LEN_REM, MAX_CP_LIST_LEN_INS)));
      int cp_lengths[] = new [cp_list_len];
      int pkt_size;

      // We require the packet size to be a multiple of MIN_FFT_SIZE and up to
      // the current FFT size in length.
      pkt_size = $urandom_range(MIN_FFT_SIZE, `MIN(fft_size, SPP));
      pkt_size = `DIV_CEIL(pkt_size, MIN_FFT_SIZE)*MIN_FFT_SIZE;

      foreach (cp_lengths[i]) begin
        cp_lengths[i] = $urandom_range(0, fft_size-1);
      end

      if (cp_mode == 2) begin
        // Test with CP insertion
        test_fft_sine(fft_size, num_ffts, .timed(timed),
          .cp_insertions(cp_lengths), .pkt_size(pkt_size));
      end else if(cp_mode == 1) begin
        // Test with CP removal
        test_fft_sine(fft_size, num_ffts, .timed(timed),
          .cp_removals(cp_lengths), .pkt_size(pkt_size));
      end else begin
        // Test without cyclic prefix
        test_fft_sine(fft_size, num_ffts, .timed(timed), .pkt_size(pkt_size));
      end
    end

    test.end_test();
  endtask


  task automatic test_fft_config(int fft_size, int pkt_size = SPP);
    int cp_lengths[] = new [14];

    test.start_test(
      $sformatf("Test fft configuration (fft_size=%0d, pkt_size=%0d)", fft_size, pkt_size),
      2ms
    );

    if (fft_size == 4096) begin
      // Assume 122.88 MS/s, 30 kHz subcarrier spacing (mu=1), 28 fft symbols
      // per 1 ms subframe.
      cp_lengths = '{352, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288};
    end else if (fft_size == 8192) begin
      // Assume 245.76 MS/s, 30 kHz subcarrier spacing (mu=1), 28 fft symbols
      // per 1 ms subframe.
      cp_lengths = '{704, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576};
    end else begin
      $fatal(1, "Bad FFT size");
    end

    test_fft_sine(fft_size, cp_lengths.size(), .cp_insertions(cp_lengths));
    test_fft_sine(fft_size, cp_lengths.size(), .cp_removals(cp_lengths));

    test.end_test();
  endtask


  // Test loopback, i.e., sending a symbol and getting back the original symbol
  // that was sent, like you would do with OFDM.
  task automatic test_loopback(
    int fft_size,
    int num_ffts = 1,
    int cp[]     = {},
    int pkt_size = fft_size < SPP ? fft_size : SPP,
    int port     = 0
  );
    logic [31:0] data_in  [$];
    logic [31:0] signal   [$];
    logic [31:0] data_out [$];
    int fft_scaling  = fft_scale_default($clog2(fft_size)); // Get 1/N scaling
    int ifft_scaling = 0;
    packet_info_t send_pkt_info = '0;

    if (NUM_CHAN_PER_CORE > 1) begin
      `ASSERT_ERROR(0, "test_loopback() only works with one channel per core.");
      return;
    end

    test.start_test(
      $sformatf({
        "Test Loopback\n",
        "    fft_size: %0d\n",
        "    num_ffts: %0d\n",
        "    cp:       %p\n",
        "    pkt_size: %0d\n",
        "    port:     %0d"},
        fft_size, num_ffts, cp, pkt_size, port
      ), 2ms
    );

    // For this test, we send one FFT per packet, unless the FFT is larger than
    // the packet size, which is supported.
    assert(fft_size % pkt_size == 0) else
      `ASSERT_ERROR(0, "fft_size must be a multiple of pkt_size");

    // Set the SPP
    blk_ctrl.set_max_payload_length(port, pkt_size*4);

    // Do IFFT to convert the frequency domain signal to a time domain signal
    // with CP insertion.
    config_fft(fft_size, cp, {}, ifft_scaling, FFT_INVERSE, .core(port));
    repeat(num_ffts) data_in = {data_in, gen_tone(fft_size, 0.25)};
    send_pkt_info.eob = '1;
    blk_ctrl.send_packets_items(.port(port), .items(data_in), .pkt_info(send_pkt_info));
    blk_ctrl.recv_packets_items(.port(port), .items(signal), .eob(1));

    // Now do FFT with CP removal to get back the original symbol
    config_fft(fft_size, {}, cp, fft_scaling, FFT_FORWARD, .core(port));
    blk_ctrl.send_packets_items(.port(port), .items(signal), .pkt_info(send_pkt_info));
    blk_ctrl.recv_packets_items(.port(port), .items(data_out), .eob(1));

    `ASSERT_ERROR(data_in == data_out,
      "Samples sent does not match samples received");

    test.end_test();
  endtask : test_loopback


  // Test max and min FFT size without CP
  task automatic test_max_min_fft();
    test.start_test("Test min/max values", MAX_FFT_SIZE*30ns);
    test_fft_sine(MAX_FFT_SIZE, 2);
    test_fft_sine(MIN_FFT_SIZE, 2);
    test.end_test();
  endtask


  // Test max CP list length (2**MAX_CP_LIST_LEN_LOG2 - 1).
  task automatic test_max_cp_list_len(int fft_size = MIN_FFT_SIZE);
    test.start_test(
      $sformatf("Test maximum CP list size (fft_size = %0d)", fft_size),
      5ms
    );

    for (int insert = 0; insert < 2; insert++) begin
      int cp_list_len = insert ? MAX_CP_LIST_LEN_INS : MAX_CP_LIST_LEN_REM;
      int cp_lengths [] = new [cp_list_len];

      foreach (cp_lengths[i]) begin
        // Keep the insertion length small to limit test length. We'll test the
        // max length in another test.
        cp_lengths[i] = $urandom_range(0, 1);
      end

      // Test one FFT more than the CP list length to make sure it repeats
      if (insert) begin
        test_fft_sine(
          .fft_size     (fft_size),
          .num_ffts     (cp_list_len+1),
          .cp_insertions(cp_lengths)
        );
      end else begin
        test_fft_sine(
          .fft_size     (fft_size),
          .num_ffts     (cp_list_len+1),
          .cp_removals  (cp_lengths)
        );
      end
    end

    test.end_test();
  endtask


  // Test max FFT size with max and min CP insertion and removal lengths
  task automatic test_max_min_cp_len();
    int cp_lengths [] = {MAX_CP_LEN, 1, 0};
    test.start_test("Test min/max CP lengths", 5ms);
    test_fft_sine(
      .fft_size     (MAX_FFT_SIZE),
      .num_ffts     (cp_lengths.size()),
      .cp_insertions(cp_lengths)
    );
    test_fft_sine(
      .fft_size     (MAX_FFT_SIZE),
      .num_ffts     (cp_lengths.size()),
      .cp_removals  (cp_lengths)
    );
    test.end_test();
  endtask


  // Test all the registers to ensure that they read/write as expected. Except
  // the write-only registers which will be exercised during functional tests.
  task automatic test_registers();
    localparam bit [31:0] exp_compat = {16'd3, 16'd0};
    localparam bit [31:0] exp_capabilities =
      (8'(MAX_CP_LIST_LEN_INS_LOG2) << 24) |
      (8'(MAX_CP_LIST_LEN_REM_LOG2) << 16) |
      (8'(         MAX_CP_LEN_LOG2) <<  8) |
      (8'(       MAX_FFT_SIZE_LOG2) <<  0);
    localparam bit [31:0] exp_capabilities2 =
      (EN_MAGNITUDE_SQ << 3) |
      (EN_MAGNITUDE    << 2) |
      (EN_FFT_ORDER    << 1) |
      (EN_FFT_BYPASS   << 0);
    localparam bit [31:0] exp_port_config =
      (16'(NUM_CORES            ) << 16) |
      (16'(NUM_PORTS / NUM_CORES) <<  0);
    localparam bit [31:0] exp_overflow = '0;

    test.start_test("Test registers", 2ms);

    // Test registers in each core
    for (int core = 0; core < NUM_CORES; core++) begin
      if (VERBOSE) $display("test_registers(): Testing core %0d", core);
      // Test read/write registers
      test_rw_reg("FFT Size", REG_LENGTH_LOG2_ADDR,
        dut.gen_fft_cores[0].fft_core_i.REG_LENGTH_LOG2_WIDTH, core);
      test_rw_reg("FFT Scaling", REG_SCALING_ADDR,
        dut.gen_fft_cores[0].fft_core_i.REG_SCALING_WIDTH, core);
      test_rw_reg("FFT Direction", REG_DIRECTION_ADDR,
        REG_DIRECTION_WIDTH, core);
      test_rw_reg("CP Ins Length", REG_CP_INS_LEN_ADDR,
        dut.gen_fft_cores[0].fft_core_i.REG_CP_INS_LEN_WIDTH, core);
      test_rw_reg("CP Rem Length", REG_CP_REM_LEN_ADDR,
        dut.gen_fft_cores[0].fft_core_i.REG_CP_REM_LEN_WIDTH, core);

      // Test optional registers
      if (EN_FFT_BYPASS) begin
        test_rw_reg("FFT Bypass", REG_BYPASS_ADDR,
          REG_BYPASS_WIDTH, core);
      end else begin
        test_ro_reg("FFT Bypass", REG_BYPASS_ADDR, 0,
          REG_BYPASS_WIDTH, core);
      end
      if (EN_FFT_ORDER) begin
        test_rw_reg("FFT Order", REG_ORDER_ADDR,
          REG_ORDER_WIDTH, core);
      end else begin
        test_ro_reg("FFT Order", REG_ORDER_ADDR, 0,
          REG_ORDER_WIDTH, core);
      end
      if (EN_MAGNITUDE || EN_MAGNITUDE_SQ) begin
        test_rw_reg("FFT Magnitude", REG_MAGNITUDE_ADDR,
          REG_MAGNITUDE_WIDTH, core);
      end else begin
        test_ro_reg("FFT Magnitude", REG_MAGNITUDE_ADDR, 0,
          REG_MAGNITUDE_WIDTH, core);
      end

      // Test read-only registers
      test_ro_reg("Compatibility", REG_COMPAT_ADDR, exp_compat,
        REG_COMPAT_WIDTH, core);
      test_ro_reg("Capabilities", REG_CAPABILITIES_ADDR, exp_capabilities,
        REG_CAPABILITIES_WIDTH, core);
      test_ro_reg("Capabilities 2", REG_CAPABILITIES2_ADDR, exp_capabilities2,
        REG_CAPABILITIES2_WIDTH, core);
      test_ro_reg("Port Config", REG_PORT_CONFIG_ADDR, exp_port_config,
        REG_PORT_CONFIG_WIDTH, core);
      test_ro_reg("Overflow", REG_OVERFLOW_ADDR, exp_overflow, 32, core);
    end

    test.end_test();
  endtask : test_registers


  // Run a simple/quick test on all ports to make sure things are connected as
  // expected.
  task automatic test_basic();
    test.start_test("Test basic", 2ms);
    for (int core = 0; core < NUM_CORES; core++) begin
      test_fft_sine(
        .fft_size(32), .num_ffts(2),
        .cp_insertions({}),
        .cp_removals({}),
        .pkt_size(32),
        .timed(1),
        .core(core)
      );
    end
    test.end_test();
  endtask


  // Test different cyclic-prefix lists with insertion and removal
  task automatic test_cyclic_prefix();
    automatic int cp_lengths_single[] = '{ 12 };
    automatic int cp_lengths_multi[]  = '{ 320, 288, 111, 0, 320 };
    automatic int cp_lengths_golden[] = '{ 320, 288, 288, 288, 288, 288, 288,
                                           288, 288, 288, 288, 288, 288, 288 };

    test.start_test("Test CP insertion and removal", 2ms);
    test_fft_sine(128, cp_lengths_single.size(), .cp_insertions(cp_lengths_single));
    test_fft_sine(512, cp_lengths_multi.size(),  .cp_insertions(cp_lengths_multi));
    test_fft_sine(128, cp_lengths_single.size(), .cp_removals(cp_lengths_single));
    test_fft_sine(512, cp_lengths_multi.size(),  .cp_removals(cp_lengths_multi));
    test_fft_sine(512, cp_lengths_golden.size(), .cp_removals(cp_lengths_golden));
    test.end_test();
  endtask


  // Test underflow and back pressure
  task automatic test_throttle();
    test.start_test("Test throttle", 2ms);

    // Test back-pressure
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, 10);
      blk_ctrl.set_slave_stall_prob(port, 80);
    end
    test_fft_sine(512, 8, .pkt_size(256));

    // Test underflow
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, 80);
      blk_ctrl.set_slave_stall_prob(port, 10);
    end
    test_fft_sine(512, 8, .pkt_size(256));

    // Test full throttle
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, 0);
      blk_ctrl.set_slave_stall_prob(port, 0);
    end
    test_fft_sine(512, 8, .pkt_size(256));

    // Restore the default back-pressure
    for (int port = 0; port < NUM_PORTS; port++) begin
      blk_ctrl.set_master_stall_prob(port, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(port, STALL_PROB);
    end

    test.end_test();
  endtask


  // Measure the throughput to ensure it's adequate for USRP clock
  // configurations. For example, with 266.666 MHz CE clock and 250 MHz radio
  // clock, we need at least 93.75% efficiency.
  task automatic test_throughput(
    int fft_size = 512,
    int num_ffts = 8,
    int pkt_size = fft_size < SPP ? fft_size : SPP,
    int port     = 0
  );
    localparam real CE_CLK_FREQ_MHZ = 1000.0/CE_CLK_PER;
    localparam real MIN_RATE = 0.98 * CE_CLK_FREQ_MHZ;
    logic [31:0] data_in [$];
    logic [31:0] signal  [$];
    packet_info_t send_pkt_info = '0;

    if (NUM_CHAN_PER_CORE > 1) begin
      `ASSERT_ERROR(0, "test_throughput() only works with one channel per core. Test skipped.");
      return;
    end

    test.start_test(
      $sformatf({
        "Test Throughput\n",
        "    fft_size: %0d\n",
        "    num_ffts: %0d\n",
        "    pkt_size: %0d\n",
        "    port:     %0d"},
        fft_size, num_ffts, pkt_size, port
      ), 2ms
    );

    // For this test, we send one FFT per packet, unless the FFT is larger than
    // the packet size, which is supported.
    assert(fft_size % pkt_size == 0) else
      `ASSERT_ERROR(0, "fft_size must be a multiple of pkt_size");

    // Turn off back pressure in the BFM
    blk_ctrl.set_master_stall_prob(port, 0);
    blk_ctrl.set_slave_stall_prob(port, 0);

    blk_ctrl.set_max_payload_length(port, pkt_size*4);
    config_fft(.fft_size(fft_size), .core(port));
    repeat(num_ffts) data_in = {data_in, gen_tone(fft_size, 0.25)};
    send_pkt_info.eob = '1;
    fork
      begin : sender
        realtime start_time, elapsed;
        real rate;
        // Send the data twice, once to fill the pipes, the second to measure.
        repeat (2) begin
          blk_ctrl.send_packets_items(.port(port), .items(data_in), .pkt_info(send_pkt_info));
          start_time = $realtime;
          blk_ctrl.wait_complete(port);
        end
        elapsed = $realtime - start_time;
        rate = 1000.0 * num_ffts * fft_size / elapsed;  // In MS/s
        if (VERBOSE) $display("Throughput is %f MS/s", rate);
        `ASSERT_ERROR(
          rate >= MIN_RATE,
          $sformatf("Calculated throughput (%f MS/s) is below threshold (%f MS/s)",
            rate, MIN_RATE)
        );
      end
      begin : receiver
        repeat (2) blk_ctrl.recv_packets_items(.port(port), .items(signal), .eob(1));
      end
    join

    // Set back pressure back to default
    blk_ctrl.set_master_stall_prob(port, STALL_PROB);
    blk_ctrl.set_slave_stall_prob(port, STALL_PROB);

    test.end_test();
  endtask


  // Test the magnitude output option
  //
  //   mag_sel : Magnitude output selection
  //   port    : RFNoC port to test
  //
  task automatic test_magnitude(int mag_sel, int port = 0);
    packet_info_t send_pkt_info = '0;
    logic [31:0] data_in [$];
    logic [31:0] data_out [$];
    logic [31:0] data_exp [$];
    localparam FFT_SIZE = 128;
    localparam NUM_ITEMS = FFT_SIZE*10;
    localparam PKT_SIZE = FFT_SIZE;

    test.start_test($sformatf("Test Magnitude (mag_sel = %0d)", mag_sel), 2ms);

    if (NUM_CHAN_PER_CORE > 1) begin
      `ASSERT_ERROR(0, "test_magnitude() only works with one channel per core.");
      return;
    end

    `ASSERT_FATAL(EN_MAGNITUDE || EN_MAGNITUDE_SQ,
      "Magnitude/Magnitude-squared logic is not enabled in core.");
    `ASSERT_FATAL(EN_FFT_BYPASS,
      "Magnitude/Magnitude-squared test requires FFT bypass.");

    `ASSERT_FATAL(!(mag_sel == MAG_SEL_MAG && !EN_MAGNITUDE),
      "Magnitude logic is not enabled in core.");

    `ASSERT_FATAL(!(mag_sel == MAG_SEL_MAG_SQ && !EN_MAGNITUDE_SQ),
      "Magnitude-squared logic is not enabled in core.");

    // Set the packet size in bytes
    blk_ctrl.set_max_payload_length(port, PKT_SIZE*4);

    // Configure the FFT block to something sane, but bypass the FFT processing
    // stage so we can measure the effect of the post-processing directly. We
    // also set the FFT order to match the default input order so that samples
    // are not reordered during this test.
    config_fft(.fft_size(FFT_SIZE), .fft_bypass(1), .fft_order_sel(BIT_REVERSE),
      .magnitude_sel(mag_sel), .core(port));

    // Generate the data to send
    repeat(NUM_ITEMS) data_in.push_back($urandom());

    // Send and receive data
    send_pkt_info.eob = '1;
    blk_ctrl.send_packets_items(.port(port), .items(data_in), .pkt_info(send_pkt_info));
    blk_ctrl.recv_packets_items(.port(port), .items(data_out), .eob(1));

    data_exp = calc_magnitude(data_in, mag_sel);
    `ASSERT_ERROR(approx_equal(data_out, data_exp),
      "Magnitude output didn't match expected values");

    test.end_test();
  endtask


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf({
      "rfnoc_block_fft_tb\n",
      "\tFULL_TEST                = %0d\n",
      "\tNUM_PORTS                = %0d\n",
      "\tNUM_CORES                = %0d\n",
      "\tMAX_FFT_SIZE_LOG2        = %0d\n",
      "\tMAX_CP_LIST_LEN_INS_LOG2 = %0d\n",
      "\tMAX_CP_LIST_LEN_REM_LOG2 = %0d\n",
      "\tEN_MAGNITUDE_SQ          = %0d\n",
      "\tEN_MAGNITUDE             = %0d\n",
      "\tEN_FFT_ORDER             = %0d\n",
      "\tEN_FFT_BYPASS            = %0d\n",
      "\tUSE_APPROX_MAG           = %0d\n",
      "\tVERBOSE                  = %0d"},
      FULL_TEST, NUM_PORTS, NUM_CORES, MAX_FFT_SIZE_LOG2,
      MAX_CP_LIST_LEN_INS_LOG2, MAX_CP_LIST_LEN_REM_LOG2, EN_MAGNITUDE_SQ,
      EN_MAGNITUDE, EN_FFT_ORDER, EN_FFT_BYPASS, USE_APPROX_MAG, VERBOSE
    );

    // Initialize the test exec object for this testbench
    test.start_tb(tb_name);

    // Start the clocks
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();
    ce_clk_gen.start();

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
    `ASSERT_ERROR(blk_ctrl.get_noc_id()     == 32'hFF700002, "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I,  "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O,  "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu()        == MTU,          "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_registers();
    test_basic();

    if (FULL_TEST) begin
      if (NUM_CHAN_PER_CORE == 1) begin
        test_loopback(128, 2, '{80, 72, 111, 0, 80});
        test_loopback(256, 3, '{160, 144, 56, 0, 160});
      end
      if (MAX_FFT_SIZE >= 4096) test_fft_config(4096, 1024);
      if (MAX_FFT_SIZE >= 8192) test_fft_config(8192, 1024);
      test_cyclic_prefix();
      test_throttle();
      test_throughput();
      test_max_min_fft();
      test_max_cp_list_len();
      test_max_min_cp_len();
      test_random(.num_iterations(200), .max_fft_size(128), .min_fft_size(MIN_FFT_SIZE));
    end

    // Always test magnitude as part of the quick tests, so we can quickly test
    // different values of USE_APPROX_MAG.
    if (NUM_CHAN_PER_CORE == 1) begin
      if (EN_MAGNITUDE    && EN_FFT_BYPASS) test_magnitude(MAG_SEL_MAG);
      if (EN_MAGNITUDE_SQ && EN_FFT_BYPASS) test_magnitude(MAG_SEL_MAG_SQ);
    end

    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();
    rfnoc_ctrl_clk_gen.kill();
    ce_clk_gen.kill();
  end

endmodule


`default_nettype wire
