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
  bit FULL_TEST                = 1,
  int CHDR_W                   = 64,
  int NIPC                     = 1,
  int NUM_PORTS                = 1,
  int NUM_CORES                = 1,
  int MAX_FFT_SIZE_LOG2        = 10,
  int MAX_TEST_FFT_SIZE_LOG2   = MAX_FFT_SIZE_LOG2,
  bit EN_CP_REMOVAL            = 1,
  bit EN_CP_INSERTION          = 1,
  int MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int MAX_CP_LIST_LEN_REM_LOG2 = 5,
  bit EN_MAGNITUDE             = 1,
  bit EN_MAGNITUDE_SQ          = 1,
  bit EN_FFT_ORDER             = 1,
  bit EN_FFT_BYPASS            = 1,
  bit USE_APPROX_MAG           = 1,
  bit VERBOSE                  = 0
);

  `include "test_exec.svh"
  `include "usrp_utils.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrData::*;
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
  localparam int MAX_TEST_FFT_SIZE   = 2**MAX_TEST_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN_LOG2     = MAX_FFT_SIZE_LOG2;
  localparam int MAX_CP_LEN          = 2**MAX_CP_LEN_LOG2-1;
  localparam int MAX_TEST_CP_LEN     = MAX_TEST_FFT_SIZE-NIPC;
  localparam int MIN_FFT_SIZE_LOG2   = NIPC < 8 ? 3 :   // Minimum allowed by Xilinx FFT core
                                       $clog2(2*NIPC);  // The FFT must be at least 2 transfers
  localparam int MIN_FFT_SIZE        = 2**MIN_FFT_SIZE_LOG2;
  localparam bit EN_TIME_ALL_PKTS    = 1;

  // Native order used by FFT block when EN_FFT_ORDER is disabled
  localparam int NATIVE_FFT_ORDER = BIT_REVERSE;

  // RFNoC configuration
  localparam [9:0] THIS_PORTID    = 10'h123;
  localparam int   NUM_PORTS_I    = NUM_PORTS;
  localparam int   NUM_PORTS_O    = NUM_PORTS;
  localparam int   ITEM_W         = 32;          // Sample size in bits
  localparam int   BYTE_MTU       = $clog2(8*1024);  // 8 KiB packets
  localparam int   ITEM_MTU       = $clog2(2**BYTE_MTU / (ITEM_W/8));
  localparam int   CHDR_MTU       = $clog2(2**BYTE_MTU / (CHDR_W/8));
  localparam int   DEFAULT_SPP    = 128;         // Default samples per packet
  localparam int   MAX_SPP        = 2**ITEM_MTU; // Max samples per packet
  localparam int   MIN_SPP        = 8;           // Min samples per packet
  localparam int   STALL_PROB     = 50;          // Default BFM stall probability
  localparam real  CHDR_CLK_PER   = 5.0;         // 200 MHz
  localparam real  CTRL_CLK_PER   = 8.0;         // 125 MHz
  localparam real  CE_CLK_PER     = 4.0;         // 250 MHz


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
      blk_ctrl.connect_master_data_port(i, m_chdr[i], MAX_SPP*(ITEM_W/8));
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
    .THIS_PORTID             (THIS_PORTID             ),
    .CHDR_W                  (CHDR_W                  ),
    .MTU                     (CHDR_MTU                ),
    .NIPC                    (NIPC                    ),
    .NUM_PORTS               (NUM_PORTS               ),
    .NUM_CORES               (NUM_CORES               ),
    .MAX_FFT_SIZE_LOG2       (MAX_FFT_SIZE_LOG2       ),
    .EN_CP_REMOVAL           (EN_CP_REMOVAL           ),
    .EN_CP_INSERTION         (EN_CP_INSERTION         ),
    .MAX_CP_LIST_LEN_INS_LOG2(MAX_CP_LIST_LEN_INS_LOG2),
    .MAX_CP_LIST_LEN_REM_LOG2(MAX_CP_LIST_LEN_REM_LOG2),
    .CP_INSERTION_REPEAT     (CP_INSERTION_REPEAT     ),
    .CP_REMOVAL_REPEAT       (CP_REMOVAL_REPEAT       ),
    .EN_FFT_BYPASS           (EN_FFT_BYPASS           ),
    .EN_FFT_ORDER            (EN_FFT_ORDER            ),
    .EN_MAGNITUDE            (EN_MAGNITUDE            ),
    .EN_MAGNITUDE_SQ         (EN_MAGNITUDE_SQ         ),
    .USE_APPROX_MAG          (USE_APPROX_MAG          )
  ) dut (
    .rfnoc_chdr_clk     (rfnoc_chdr_clk     ),
    .rfnoc_ctrl_clk     (rfnoc_ctrl_clk     ),
    .ce_clk             (ce_clk             ),
    .rfnoc_core_config  (backend.cfg        ),
    .rfnoc_core_status  (backend.sts        ),
    .s_rfnoc_chdr_tdata (s_rfnoc_chdr_tdata ),
    .s_rfnoc_chdr_tlast (s_rfnoc_chdr_tlast ),
    .s_rfnoc_chdr_tvalid(s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready(s_rfnoc_chdr_tready),
    .m_rfnoc_chdr_tdata (m_rfnoc_chdr_tdata ),
    .m_rfnoc_chdr_tlast (m_rfnoc_chdr_tlast ),
    .m_rfnoc_chdr_tvalid(m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready(m_rfnoc_chdr_tready),
    .s_rfnoc_ctrl_tdata (m_ctrl.tdata       ),
    .s_rfnoc_ctrl_tlast (m_ctrl.tlast       ),
    .s_rfnoc_ctrl_tvalid(m_ctrl.tvalid      ),
    .s_rfnoc_ctrl_tready(m_ctrl.tready      ),
    .m_rfnoc_ctrl_tdata (s_ctrl.tdata       ),
    .m_rfnoc_ctrl_tlast (s_ctrl.tlast       ),
    .m_rfnoc_ctrl_tvalid(s_ctrl.tvalid      ),
    .m_rfnoc_ctrl_tready(s_ctrl.tready      )
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
  //   cp_removals[]   : List of cyclic-prefix removals to load
  //   cp_insertions[] : List of cyclic-prefix insertions to load
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
    int cp_removals[]   = {},
    int cp_insertions[] = {},
    int fft_scaling     = fft_scale_default($clog2(fft_size)),
    bit fft_direction   = FFT_FORWARD,
    bit cp_list_clear   = 1,
    bit fft_bypass      = 0,
    int fft_order_sel   = EN_FFT_ORDER ? NATURAL : NATIVE_FFT_ORDER,
    int magnitude_sel   = 0,
    int core            = 0
  );
    logic [31:0] fft_size_log2 = $clog2(fft_size);

    `ASSERT_ERROR(2**fft_size_log2 == fft_size,
      "config_fft(): fft_size must be a power of 2");
    `ASSERT_ERROR(EN_CP_REMOVAL || cp_removals.size() == 0,
      "config_fft(): Cyclic prefix removal not allowed when disabled");
    `ASSERT_ERROR(EN_CP_INSERTION || cp_insertions.size() == 0,
      "config_fft(): Cyclic prefix removal not allowed when disabled");
    `ASSERT_ERROR(EN_FFT_ORDER || fft_order_sel == NATIVE_FFT_ORDER,
      "The FFT output order can't be changed when EN_FFT_ORDER is false");

    // CP must be a multiple of SPC
    foreach (cp_removals[i]) begin
      `ASSERT_ERROR(cp_removals[i] % NIPC == 0,
        "config_fft(): CP removal length must be a multiple of NIPC");
      `ASSERT_ERROR(cp_insertions[i] < fft_size,
        "config_fft(): CP insertion length must be less than FFT size");
    end
    foreach (cp_insertions[i]) begin
      `ASSERT_ERROR(cp_insertions[i] % NIPC == 0,
        "config_fft(): CP insertion length must be a multiple of NIPC");
      `ASSERT_ERROR(cp_insertions[i] < fft_size,
        "config_fft(): CP insertion length must be less than FFT size");
    end

    // Restrict testing to MAX_TEST_FFT_SIZE
    `ASSERT_ERROR(MIN_FFT_SIZE <= fft_size && fft_size <= MAX_TEST_FFT_SIZE,
      "config_fft(): fft_size is outside allowed range");

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
        $sformatf("%s register incorrect readback! Expected: 0x%X, Actual 0x%X",
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
      $sformatf("%s register incorrect readback! Expected: 0x%X, Actual 0x%X",
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
  //   cp_removals[]   : Cyclic-prefix removal list to use
  //   cp_insertions[] : Cyclic-prefix insertion list to use
  //   pkt_size        : Packet size to use
  //   timed           : Test timed (1) or untimed (0) packets
  //   core            : Which FFT core to test
  //   fft_order       : Which FFT output order to test
  //
  task automatic test_fft_sine(
    int fft_size,
    int num_ffts        = 1,
    int cp_removals[]   = {},
    int cp_insertions[] = {},
    int pkt_size        = DEFAULT_SPP,
    bit timed           = 1,
    int core            = 0,
    int fft_order       = EN_FFT_ORDER ? NATURAL : NATIVE_FFT_ORDER
  );
    int first_port = core*NUM_CHAN_PER_CORE;
    int last_port  = (core+1)*NUM_CHAN_PER_CORE - 1;

    if (VERBOSE) begin
      $display("test_fft_sine():");
      $display("    fft_size:      %0d", fft_size);
      $display("    num_ffts:      %0d", num_ffts);
      $display("    cp_removals:   %p",  cp_removals);
      $display("    cp_insertions: %p",  cp_insertions);
      $display("    pkt_size:      %0d", pkt_size);
      $display("    timed:         %0d", timed);
      $display("    core:          %0d", core);
      $display("    fft_order:     %0d", fft_order);
    end

    if (fft_size > MAX_TEST_FFT_SIZE) begin
      `ASSERT_WARNING(0,
        "test_fft_sine(): Leaving test because fft_size > MAX_TEST_FFT_SIZE");
      return;
    end

    // Having both insertion and removal might work, but that's not a use case
    // we're supporting.
    `ASSERT_ERROR(!(cp_insertions.size() && cp_removals.size()),
      "Cannot specify both CP insertion and CP removal");

    config_fft(fft_size, cp_removals, cp_insertions, fft_scale_default(fft_size),
      FFT_FORWARD, 1, .fft_order_sel(fft_order), .core(core));

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
                if (timed && (pkt_count == 0 || EN_TIME_ALL_PKTS)) begin
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

                if (recv_data.size() % fft_size == 0) begin
                  `ASSERT_WARNING(recv_pkt_info.eov == 1,
                    "test_fft_sine(): recv: EOV is not set on FFT multiple");
                  if (VERBOSE && recv_pkt_info.eov) begin
                    $display("test_fft_sine(): recv: End of vector");
                end                end else begin
                  `ASSERT_WARNING(recv_pkt_info.eov == 0,
                    "test_fft_sine(): recv: Unexpected EOV was set");
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

                // We expect the tone to come through at the frequency Fs/4.
                // Here, we figure which bin that is depending on the FFT
                // output order.
                case (fft_order)
                  FFT_ORDER_NORMAL: begin
                    // -fs/2 to fs/2 (or fs/2 to fs then 0 to fs/2)
                    peak_index = 3*fft_size/4;
                  end
                  FFT_ORDER_REVERSE: begin
                    // fs/2 to -fs/2 (fs/2 to 0 then fs to fs/2)
                    peak_index = (fft_size-1) - 3*fft_size/4;
                  end
                  FFT_ORDER_NATURAL: begin
                    // 0 to fs
                    peak_index = fft_size/4;
                  end
                  FFT_ORDER_BIT_REVERSE: begin
                    // Same as natural, but the bits of the index are reversed
                    peak_index = bit_reverse(fft_size/4, $clog2(fft_size));
                  end
                endcase

                // Add the offset created by the cyclic prefix. If cyclic
                // prefix insertion is enabled, then we may also see the
                // frequency tone in the cyclic prefix as well. If
                // cp_peak_index is negative, that indicate the cyclic prefix
                // wasn't long enough to show it.
                peak_index += cp_insertion_len;
                cp_peak_index = peak_index - fft_size;

                // Grab the next FFT from the data for this burst
                recv_payload = recv_data[samp_count : samp_count+total_fft_size-1];

                // Verify the sample values
                fft_samp_count = 0;
                foreach (recv_payload[samp_i]) begin
                  if (fft_samp_count == peak_index || fft_samp_count == cp_peak_index) begin
                    bit signed [15:0] real_val, imag_val;
                    {real_val, imag_val} = recv_payload[samp_i];
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


  // Test a magnitude output selection of the FFT block
  //
  //   mag_sel  : Magnitude output selection
  //   num_ffts : Number of FFTs to send through
  //   fft_size : Size (in items) of FFT to use during the test
  //   pkt_size : Packet size (in items) to use during the test
  //   core     : Which FFT core to test
  //
  task automatic test_magnitude(
    int mag_sel,
    int num_ffts = 8,
    int fft_size = 128,
    int pkt_size = 2*fft_size,
    int core = 0
  );
    const int num_items = num_ffts*fft_size;
    packet_info_t send_pkt_info = '0;
    logic [31:0] data_in [$];
    logic [31:0] data_out [NUM_CHAN_PER_CORE][$];
    logic [31:0] data_ref [NUM_CHAN_PER_CORE][$];
    logic [31:0] data_exp [$];
    int ports [] = new [NUM_CHAN_PER_CORE];

    // Create a list of ports involved in this test
    foreach (ports[idx]) ports[idx] = core*NUM_CHAN_PER_CORE + idx;

    `ASSERT_FATAL(EN_MAGNITUDE || EN_MAGNITUDE_SQ,
      "Magnitude/Magnitude-squared logic is not enabled in core.");

    `ASSERT_FATAL(!(mag_sel == MAG_SEL_MAG && !EN_MAGNITUDE),
      "Magnitude logic is not enabled in core.");

    `ASSERT_FATAL(!(mag_sel == MAG_SEL_MAG_SQ && !EN_MAGNITUDE_SQ),
      "Magnitude-squared logic is not enabled in core.");

    if (VERBOSE) begin
      $display("test_magnitude():");
      $display("    mag_sel:  %0d", mag_sel);
      $display("    num_ffts: %0d", num_ffts);
      $display("    fft_size: %0d", fft_size);
      $display("    pkt_size: %0d", pkt_size);
      $display("    core:     %0d", core);
    end

    // Set the packet size to be sent, in bytes
    foreach (ports[idx]) begin
      blk_ctrl.set_max_payload_length(ports[idx], pkt_size*4);
    end

    // Configure for a normal inverse FFT
    config_fft(.fft_size(fft_size), .fft_scaling(0), .fft_direction(FFT_INVERSE),
       .magnitude_sel(MAG_SEL_NONE), .core(core));

    // Generate random data to send
    repeat (num_items) data_in.push_back($urandom());

    // Do an inverse FFT get reference data
    send_pkt_info.eob = '1;
    foreach (ports[idx]) begin
      blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in), .pkt_info(send_pkt_info));
    end
    foreach (ports[idx]) begin
      blk_ctrl.recv_packets_items(.port(ports[idx]), .items(data_ref[idx]), .eob(1));
    end

    // Make sure the reference data is the same for all ports
    foreach (ports[idx]) begin
      `ASSERT_ERROR(data_ref[idx] == data_ref[0],
        $sformatf("Reference data for port %0d doesn't match", idx));
    end

    // Configure for an inverse FFT with magnitude output
    config_fft(.fft_size(fft_size), .fft_scaling(0), .fft_direction(FFT_INVERSE),
      .magnitude_sel(mag_sel), .core(core));

    // Send and receive the data again, this time with magnitude output
    send_pkt_info.eob = '1;
    foreach (ports[idx]) begin
      blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in), .pkt_info(send_pkt_info));
    end
    foreach (ports[idx]) begin
      blk_ctrl.recv_packets_items(.port(ports[idx]), .items(data_out[idx]), .eob(1));
    end

    // Figure out what we should expect from the reference data.
    data_exp = calc_magnitude(data_ref[0], mag_sel);

    // Check that what we received matches what we expected, allowing for
    // rounding error.
    foreach (ports[idx]) begin
      `ASSERT_ERROR(approx_equal(data_out[idx], data_exp),
        "Magnitude output didn't match expected values");
    end
  endtask : test_magnitude


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Test a sequence of random configurations
  task automatic test_random(
    int num_iterations,
    int max_fft_size = MAX_TEST_FFT_SIZE,
    int min_fft_size = MIN_FFT_SIZE
  );
    test.start_test(
      $sformatf({
        "Test Random\n",
        "    num_iterations: %0d\n",
        "    max_fft_size:   %0d\n",
        "    min_fft_size:   %0d"},
        num_iterations, max_fft_size, min_fft_size
      ), num_iterations*max_fft_size*50ns
    );

    for (int test_iter = 0; test_iter < num_iterations; test_iter++) begin
      bit timed;
      int fft_size;
      int num_ffts;
      fft_order_t temp;
      int fft_order = NATIVE_FFT_ORDER;
      int cp_mode;
      bit has_cp;
      int cp_list_len;
      int cp_lengths[] = new [cp_list_len];
      int pkt_size;
      int core;

      // Randomize the parameters of this test iteration
      core        = $urandom_range(0, NUM_CORES-1);
      timed       = $urandom_range(0, 1);
      num_ffts    = $urandom_range(1, 3);
      has_cp      = $urandom_range(0, 1);
      cp_list_len = $urandom_range(0,
        `MIN(num_ffts-1, `MIN(MAX_CP_LIST_LEN_REM, MAX_CP_LIST_LEN_INS)));
      if (EN_FFT_ORDER) begin
        fft_order = $urandom_range(0, temp.num()-1);
      end

      // Randomly reset the core
      if ($urandom_range(0, 1)) user_reset(core);

      // Choose a random FFT size, but avoid some apparent bugs in the Xilinx
      // core that we know about.
      forever begin
        fft_size = 2**$urandom_range($clog2(min_fft_size), $clog2(max_fft_size));

        // FIXME: With 32k FFT core configured for size 8 FFT, the first FFT
        // output is garbled.
        if (MAX_FFT_SIZE == 32*1024 && fft_size == 8) begin
          `ASSERT_WARNING(0, "Skipping size 8 FFT with 32k FFT core");
          continue;
        end

        // FIXME: The 64k FFT doesn't reliably support 64k FFTs.
        if (MAX_FFT_SIZE == 64*1024 && fft_size == 64*1024) begin
          `ASSERT_WARNING(0, "Skipping size 64k FFT with 64k FFT core");
          continue;
        end

        break;
      end

      // Choose a random cyclic prefix mode, but make sure it's enabled.
      forever begin
        cp_mode = $urandom_range(0, 2);  // 0=None, 1=removal, 2=insertion
        if (cp_mode == 0) break;
        if (cp_mode == 1 && EN_CP_REMOVAL) break;
        if (cp_mode == 2 && EN_CP_INSERTION) break;
      end

      // Choose a random packet size that's a multiple of SPC
      if (fft_size >= MAX_SPP) begin
        pkt_size = $urandom_range(MIN_SPP, MAX_SPP) / NIPC * NIPC;
      end else begin
        if ($urandom_range(0,1)) begin
          // Choose one that's smaller than the FFT size
          pkt_size = $urandom_range(MIN_SPP, fft_size) / NIPC * NIPC;
        end else begin
          // Choose one that's larger than the FFT size
          pkt_size = $urandom_range(fft_size, `MIN(2*fft_size, MAX_SPP)) / NIPC * NIPC;
        end
      end

      if (cp_mode > 0) foreach (cp_lengths[i]) begin
        // CP must be a multiple of SPC
        cp_lengths[i] = $urandom_range(0, fft_size-1) / NIPC * NIPC;
      end

      if(cp_mode == 1) begin
        // Test with CP removal
        test_fft_sine(fft_size, num_ffts, .timed(timed),
          .cp_removals(cp_lengths), .pkt_size(pkt_size),
          .fft_order(fft_order));
      end else if (cp_mode == 2) begin
        // Test with CP insertion
        test_fft_sine(fft_size, num_ffts, .timed(timed),
          .cp_insertions(cp_lengths), .pkt_size(pkt_size),
          .fft_order(fft_order));
      end else begin
        // Test without cyclic prefix
        test_fft_sine(fft_size, num_ffts, .timed(timed), .pkt_size(pkt_size),
          .fft_order(fft_order));
      end
    end

    test.end_test();
  endtask


  // Test some OFDM configurations
  task automatic test_ofdm_config(int fft_size, int pkt_size = DEFAULT_SPP);
    int cp_lengths[] = new [14];

    test.start_test(
      $sformatf("Test FFT configuration (fft_size=%0d, pkt_size=%0d)", fft_size, pkt_size),
      2ms
    );

    if (fft_size > MAX_TEST_FFT_SIZE) begin
      `ASSERT_WARNING(0,
        "test_fft_sine(): Leaving test because fft_size > MAX_TEST_FFT_SIZE");
      test.end_test();
      return;
    end

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
    int pkt_size = DEFAULT_SPP,
    int core     = 0
  );
    logic [31:0] data_in  [$];
    logic [31:0] signal   [NUM_CHAN_PER_CORE][$];
    logic [31:0] data_out [NUM_CHAN_PER_CORE][$];
    int fft_scaling  = fft_scale_default($clog2(fft_size)); // Get 1/N scaling
    int ifft_scaling = 0;
    packet_info_t send_pkt_info = '0;
    int ports [] = new [NUM_CHAN_PER_CORE];

    // Create a list of ports involved in this test
    foreach (ports[idx]) ports[idx] = core*NUM_CHAN_PER_CORE + idx;

    test.start_test(
      $sformatf({
        "Test Loopback\n",
        "    fft_size: %0d\n",
        "    num_ffts: %0d\n",
        "    cp:       %p\n",
        "    pkt_size: %0d\n",
        "    core:     %0d"},
        fft_size, num_ffts, cp, pkt_size, core
      ), 2ms
    );

    if (fft_size > MAX_TEST_FFT_SIZE) begin
      `ASSERT_WARNING(0,
        "test_loopback(): Leaving test because fft_size > MAX_TEST_FFT_SIZE");
      test.end_test();
      return;
    end

    `ASSERT_ERROR(EN_CP_REMOVAL && EN_CP_INSERTION,
      "test_loopback(): Cyclic prefix insertion and removal must be enabled");

    // For this test, we send one FFT per packet, unless the FFT is larger than
    // the packet size, which is supported.
    assert(fft_size % pkt_size == 0) else
      `ASSERT_ERROR(0, "fft_size must be a multiple of pkt_size");

    // Set the packet size to be sent, in bytes
    foreach (ports[idx]) blk_ctrl.set_max_payload_length(ports[idx], pkt_size*4);

    // Do IFFT to convert the frequency domain signal to a time domain signal
    // with CP insertion.
    config_fft(fft_size, {}, cp, ifft_scaling, FFT_INVERSE, .core(core));
    repeat(num_ffts) data_in = {data_in, gen_tone(fft_size, 0.25)};
    send_pkt_info.eob = '1;
    foreach(ports[idx])
      blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in), .pkt_info(send_pkt_info));
    foreach(ports[idx])
      blk_ctrl.recv_packets_items(.port(ports[idx]), .items(signal[idx]), .eob(1));

    // Now do FFT with CP removal to get back the original symbol
    config_fft(fft_size, cp, {}, fft_scaling, FFT_FORWARD, .core(core));
    foreach(ports[idx])
      blk_ctrl.send_packets_items(.port(ports[idx]), .items(signal[idx]), .pkt_info(send_pkt_info));
    foreach(ports[idx])
      blk_ctrl.recv_packets_items(.port(ports[idx]), .items(data_out[idx]), .eob(1));

    foreach (ports[idx]) begin
      `ASSERT_ERROR(data_in == data_out[idx], $sformatf(
        "Samples sent does not match samples received on port %0d", idx));
    end

    test.end_test();
  endtask : test_loopback


  // Test max and min FFT size without cyclic prefix. Max/min CP insertion is
  // in another test.
  task automatic test_max_min_fft();
    test.start_test("Test min/max FFT length",
      1ms + (MIN_FFT_SIZE+MAX_TEST_FFT_SIZE)*50ns);
    test_fft_sine(MAX_TEST_FFT_SIZE, 2);
    test_fft_sine(MIN_FFT_SIZE, 2);
    test.end_test();
  endtask


  // Test max CP list length. Use a small FFT size by default to shorten the
  // test time.
  task automatic test_max_cp_list_len(int fft_size = 2**(MIN_FFT_SIZE_LOG2+1));
    int cp_rem_lengths [] = new [MAX_CP_LIST_LEN_INS];
    int cp_ins_lengths [] = new [MAX_CP_LIST_LEN_REM];
    int num_ffts;

    test.start_test(
      $sformatf("Test maximum CP list length (fft_size = %0d)", fft_size),
      5ms
    );

    if (EN_CP_REMOVAL) begin
      foreach (cp_rem_lengths[i]) begin
        // CP must be a multiple of SPC
        cp_rem_lengths[i] = $urandom_range(NIPC, fft_size-1) / NIPC * NIPC;
      end
      // Test more FFTs than the CP list length to make sure the list repeats
      // as expected.
      test_fft_sine(
        .fft_size     (fft_size),
        .num_ffts     (2*MAX_CP_LIST_LEN_INS),
        .cp_removals  (cp_rem_lengths)
      );
    end
    if (EN_CP_INSERTION) begin
      foreach (cp_ins_lengths[i]) begin
        // CP must be a multiple of SPC
        cp_ins_lengths[i] = $urandom_range(NIPC, fft_size-1) / NIPC * NIPC;
      end
      // Test more FFTs than the CP list length to make sure the list repeats
      // as expected.
      test_fft_sine(
        .fft_size     (fft_size),
        .num_ffts     (2*MAX_CP_LIST_LEN_REM),
        .cp_insertions(cp_ins_lengths)
      );
    end

    test.end_test();
  endtask


  // Test max FFT size with max and min CP insertion and removal lengths
  task automatic test_max_min_cp_len();
    int cp_lengths [];

    // CP lengths must be multiple of SPC
    cp_lengths = {
      MAX_TEST_CP_LEN-NIPC, // Max
      NIPC,                 // Min plus 1
      0                     // Min
    };

    if (EN_CP_REMOVAL) begin
      test.start_test("Test max/min CP removal lengths", 5ms);
      test_fft_sine(
        .fft_size   (MAX_TEST_FFT_SIZE),
        .num_ffts   (cp_lengths.size()),
        .cp_removals(cp_lengths)
      );
      test.end_test();
    end

    if (EN_CP_INSERTION) begin
      test.start_test("Test max/min CP insertion lengths", 5ms);
      test_fft_sine(
        .fft_size     (MAX_TEST_FFT_SIZE),
        .num_ffts     (cp_lengths.size()),
        .cp_insertions(cp_lengths)
      );
      test.end_test();
    end

  endtask


  // Test maximum packet size, minimum packet size, and worst-case internal
  // buffering requirements. In the worst case, we need to be able to buffer
  // all the symbols that make up a packet. This is maximized when the packet
  // size is at its maximum and the FFT size is at its minimum, with maximum CP
  // insertion/removal.
  task automatic test_max_min_packet(int core = 0);
    int fft_size;
    int pkt_size;
    int num_pkts;
    int num_ffts;
    int cp_size;

    test.start_test("Test buffering", 2ms);

    fft_size = MIN_FFT_SIZE; // Use worst-case FFT size (min)
    pkt_size = MAX_SPP;      // Use worst-case packet size (max)
    num_pkts = 2;            // Test multiple packets
    num_ffts = `DIV_CEIL(num_pkts * pkt_size, fft_size);
    // CP must be a multiple of SPC
    cp_size = (fft_size-1) / NIPC * NIPC;  // Use worst-case CP

    // Test with no CP
    test_fft_sine(
      .fft_size     (fft_size),
      .num_ffts     (num_ffts),
      .cp_removals  (),
      .cp_insertions(),
      .pkt_size     (pkt_size),
      .timed        (1),
      .core         (core)
    );

    // Test with CP insertion
    if (EN_CP_REMOVAL) begin
      test_fft_sine(
        .fft_size     (fft_size),
        .num_ffts     (num_ffts),
        .cp_removals  ({cp_size}),
        .cp_insertions(),
        .pkt_size     (pkt_size),
        .timed        (1),
        .core         (core)
      );
    end

    // Test with CP removal
    if (EN_CP_INSERTION) begin
      test_fft_sine(
        .fft_size     (fft_size),
        .num_ffts     (num_ffts),
        .cp_removals  (),
        .cp_insertions({cp_size}),
        .pkt_size     (pkt_size),
        .timed        (1),
        .core         (core)
      );
    end

    // Test with bypass enabled (and CP insertion, if available)
   if (EN_FFT_BYPASS) begin
     logic [31:0] data_in [$];
     logic [31:0] data_out [NUM_CHAN_PER_CORE][$];
     packet_info_t send_pkt_info = '0;
     int ports [] = new [NUM_CHAN_PER_CORE];

     // Create a list of ports involved in this test
     foreach (ports[idx]) ports[idx] = core*NUM_CHAN_PER_CORE + idx;

     config_fft(.fft_size(fft_size), .fft_bypass(1),
       .fft_order_sel(BIT_REVERSE), .core(core));

     // Generate the data to send
     repeat(num_ffts * fft_size) data_in.push_back($urandom());

     // Set the max packet size to be sent, in bytes
     foreach (ports[idx]) blk_ctrl.set_max_payload_length(ports[idx], pkt_size*4);

     // Send and receive data
     send_pkt_info.eob = '1;
     foreach (ports[idx]) begin
       blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in), .pkt_info(send_pkt_info));
     end
     foreach (ports[idx]) begin
       blk_ctrl.recv_packets_items(.port(ports[idx]), .items(data_out[idx]), .eob(1));
     end

     foreach (ports[idx]) begin
       `ASSERT_ERROR(data_out[idx] == data_in,
         "Data output didn't match expected values");
     end
   end

    // Now test minimum packet size and maximum FFT size
    pkt_size = MIN_SPP;
    fft_size = MAX_TEST_FFT_SIZE;
    num_ffts = 1;
    test_fft_sine(fft_size, num_ffts, {}, {}, pkt_size, 1, .core(core));

    test.end_test();
  endtask


  // Test all the registers to ensure that they read/write as expected. Except
  // the write-only registers which will be exercised during functional tests.
  task automatic test_registers();
    localparam bit [31:0] exp_compat = {16'd3, 16'd1};
    localparam bit [31:0] exp_capabilities =
      (8'(MAX_CP_LIST_LEN_INS_LOG2) << 24) |
      (8'(MAX_CP_LIST_LEN_REM_LOG2) << 16) |
      (8'(         MAX_CP_LEN_LOG2) <<  8) |
      (8'(       MAX_FFT_SIZE_LOG2) <<  0);
    localparam bit [31:0] exp_capabilities2 =
      ($clog2(NIPC)    << 8) |
      (EN_CP_INSERTION << 5) |
      (EN_CP_REMOVAL   << 4) |
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

      // Exercise the user reset, which is write only.
      user_reset(core);

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
    for (int core = 0; core < NUM_CORES; core++) begin : core_loop
      // Test small FFT
      test_fft_sine(
        .fft_size(32),
        .num_ffts(2*NIPC),
        .cp_removals({}),
        .cp_insertions({}),
        .pkt_size(32),
        .timed(1),
        .core(core)
      );
      // Test maximum FFT size to ensure the correct core is instantiated, but
      // skip it if we're going to do the full test later since this test can
      // take a long time.
      if (!FULL_TEST) begin
        test_fft_sine(
          .fft_size(MAX_FFT_SIZE),
          .num_ffts(1),
          .cp_removals({}),
          .cp_insertions({}),
          .pkt_size(DEFAULT_SPP),
          .timed(1),
          .core(core)
        );
      end
      // Test small FFT with CP removal
      if (EN_CP_REMOVAL) begin
        test_fft_sine(
          .fft_size(32),
          .num_ffts(2*NIPC),
          .cp_removals({NIPC}),
          .cp_insertions({}),
          .pkt_size(32),
          .timed(1),
          .core(core)
        );
      end
      // Test small FFT with CP insertion
      if (EN_CP_INSERTION) begin
        test_fft_sine(
          .fft_size(32),
          .num_ffts(2*NIPC),
          .cp_removals({}),
          .cp_insertions({NIPC}),
          .pkt_size(32),
          .timed(1),
          .core(core)
        );
      end
      // Test magnitude path
      if (EN_MAGNITUDE) begin
        test_magnitude(MAG_SEL_MAG, 1, 32, 32, core);
      end
      // Test magnituded squared path
      if (EN_MAGNITUDE_SQ) begin
        test_magnitude(MAG_SEL_MAG_SQ, 1, 32, 32, core);
      end
    end : core_loop
    test.end_test();
  endtask


  // Test different cyclic-prefix lists with insertion and removal
  task automatic test_cyclic_prefix();
    // CP must be a multiple of SPC
    automatic int cp_lengths_single[] = NIPC <= 4 ? '{ 12 } : '{ 16 };
    automatic int cp_lengths_multi[]  = NIPC == 1 ? '{ 320, 288, 111, 0, 320 } :
                                                    '{ 320, 288, 112, 0, 320 };
    automatic int cp_lengths_golden[] = '{ 320, 288, 288, 288, 288, 288, 288,
                                           288, 288, 288, 288, 288, 288, 288 };

    if (EN_CP_REMOVAL) begin
      test.start_test("Test CP removal", 2ms);
      test_fft_sine(128, cp_lengths_single.size(), .cp_removals(cp_lengths_single));
      test_fft_sine(512, cp_lengths_multi.size(),  .cp_removals(cp_lengths_multi));
      test_fft_sine(512, cp_lengths_golden.size(), .cp_removals(cp_lengths_golden));
      test.end_test();
    end
    if (EN_CP_INSERTION) begin
      test.start_test("Test CP insertion", 2ms);
      test_fft_sine(128, cp_lengths_single.size(), .cp_insertions(cp_lengths_single));
      test_fft_sine(512, cp_lengths_multi.size(),  .cp_insertions(cp_lengths_multi));
      test_fft_sine(512, cp_lengths_golden.size(), .cp_insertions(cp_lengths_golden));
      test.end_test();
    end
  endtask


  // Test the supported FFT data output orders to confirm they controls and
  // data path are connected correctly.
  task automatic test_fft_order();
    test.start_test("Test FFT output order", 2ms);
    test_fft_sine(32, 2, .pkt_size(32), .fft_order(FFT_ORDER_BIT_REVERSE));
    test_fft_sine(32, 2, .pkt_size(32), .fft_order(FFT_ORDER_NATURAL));
    test_fft_sine(32, 2, .pkt_size(32), .fft_order(FFT_ORDER_REVERSE));
    test_fft_sine(32, 2, .pkt_size(32), .fft_order(FFT_ORDER_NORMAL));
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
  //
  //   fft_size : Size of the FFT to test
  //   num_ffts : Number of FFTs to send during the test
  //   pkt_size : Packet size to use, in items
  //   core     : Which core to test
  //
  task automatic test_throughput(
    int fft_size = 256,
    int num_ffts = 4,
    int pkt_size = 512,
    int core     = 0
  );
    localparam real CE_CLK_FREQ_MHZ = 1000.0/CE_CLK_PER;
    localparam real CHDR_CLK_FREQ_MHZ = 1000.0/CHDR_CLK_PER;
    localparam real MIN_RATE_CE = 0.95 * CE_CLK_FREQ_MHZ * NIPC;
    localparam real MIN_RATE_CHDR = 0.95 * CHDR_CLK_FREQ_MHZ * CHDR_W/ITEM_W;
    logic [31:0] data_in [$];
    logic [31:0] signal  [$];
    packet_info_t send_pkt_info = '0;
    int ports [] = new [NUM_CHAN_PER_CORE];
    int master_stall_prob [] = new [NUM_CHAN_PER_CORE];
    int slave_stall_prob [] = new [NUM_CHAN_PER_CORE];

    // Create a list of ports involved in this test
    foreach (ports[idx]) ports[idx] = core*NUM_CHAN_PER_CORE + idx;

    test.start_test(
      $sformatf({
        "Test Throughput\n",
        "    fft_size: %0d\n",
        "    num_ffts: %0d\n",
        "    pkt_size: %0d\n",
        "    core:     %0d"},
        fft_size, num_ffts, pkt_size, core
      ), 2ms
    );

    `ASSERT_WARNING(MIN_RATE_CHDR >= MIN_RATE_CE,
      "test_throughput: The CHDR rate is limiting the DSP rate.");

    foreach (ports[idx]) begin
      // Turn off back pressure in the BFM
      master_stall_prob[idx] = blk_ctrl.get_master_stall_prob(ports[idx]);
      slave_stall_prob[idx] = blk_ctrl.get_slave_stall_prob(ports[idx]);
      blk_ctrl.set_master_stall_prob(ports[idx], 0);
      blk_ctrl.set_slave_stall_prob(ports[idx], 0);

      // Set the packet size to be sent, in bytes
      blk_ctrl.set_max_payload_length(ports[idx], pkt_size*ITEM_W/4);
    end

    config_fft(.fft_size(fft_size), .core(core));
    repeat(num_ffts) data_in = {data_in, gen_tone(fft_size, 0.25)};
    send_pkt_info.eob = '1;
    fork
      begin : sender
        realtime start_time, elapsed;
        real rate;
        // Send the data twice, once to fill the pipes, the second to measure.
        repeat (2) begin
          foreach (ports[idx]) begin
            blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in),
              .pkt_info(send_pkt_info));
          end
          start_time = $realtime;
          foreach (ports[idx]) begin
            blk_ctrl.wait_complete(ports[idx]);
          end
        end
        elapsed = $realtime - start_time;
        rate = 1000.0 * num_ffts * fft_size / elapsed;  // In MS/s
        $display("Measured throughput is %0.1f MS/s (%0.1f%% efficiency)",
          rate, 100*rate/(CE_CLK_FREQ_MHZ * NIPC));
        if (MIN_RATE_CE <= MIN_RATE_CHDR) begin
          // We're not limited by the CHDR rate, so we should hit the DSP rate
          `ASSERT_ERROR(
            rate >= MIN_RATE_CE,
            $sformatf("Calculated throughput (%f MS/s) is below threshold (%f MS/s)",
              rate, MIN_RATE_CE)
          );
        end else begin
          // We're limited by the CHDR rate, so check that instead
          `ASSERT_ERROR(
            rate >= MIN_RATE_CHDR,
            $sformatf("Calculated throughput (%f MS/s) is below threshold (%f MS/s)",
              rate, MIN_RATE_CHDR)
          );
        end
      end
      begin : receiver
        repeat (2) begin
          foreach (ports[idx]) begin
            blk_ctrl.recv_packets_items(.port(ports[idx]), .items(signal), .eob(1));
          end
        end
      end
    join

    // Set back pressure back to default
    foreach (ports[idx]) begin
      blk_ctrl.set_master_stall_prob(ports[idx], master_stall_prob[idx]);
      blk_ctrl.set_slave_stall_prob(ports[idx], slave_stall_prob[idx]);
    end

    test.end_test();
  endtask


  // Test all the supported magnitude output features, as well as the overflow
  // register.
  task automatic test_magnitude_options(
    int num_ffts = 8,
    int fft_size = 128,
    int pkt_size = 2*fft_size,
    int core     = 0
  );
    logic [31:0] val;

    // Clear the overflow register, in case it was set previously
    read_reg(REG_OVERFLOW_ADDR, val, core);
    // Make sure it's cleared
    if (val) begin
      read_reg(REG_OVERFLOW_ADDR, val, core);
      `ASSERT_ERROR(val == 0, "Overflow register did not clear.");
    end

    if (EN_MAGNITUDE) begin
      const int mag_sel = MAG_SEL_MAG;
      test.start_test($sformatf({
        "Test magnitude\n",
        "    mag_sel:  %0d\n",
        "    num_ffts: %0d\n",
        "    fft_size: %0d\n",
        "    pkt_size: %0d\n",
        "    core:     %0d"},
        MAG_SEL_MAG, num_ffts, fft_size, pkt_size, core), 2ms
      );
      test_magnitude(MAG_SEL_MAG, num_ffts, fft_size, pkt_size, core);
      // We expect some overflow, so make sure that bit was set
      read_reg(REG_OVERFLOW_ADDR, val, core);
      `ASSERT_ERROR(val != 0, "Overflow register did not set.");
      read_reg(REG_OVERFLOW_ADDR, val, core);
      `ASSERT_ERROR(val == 0, "Overflow register did not self-clear.");
      test.end_test();
    end
    if (EN_MAGNITUDE_SQ) begin
      const int mag_sel = MAG_SEL_MAG_SQ;
      test.start_test($sformatf({
        "Test magnitude\n",
        "    mag_sel:  %0d\n",
        "    num_ffts: %0d\n",
        "    fft_size: %0d\n",
        "    pkt_size: %0d\n",
        "    core:     %0d"},
        MAG_SEL_MAG_SQ, num_ffts, fft_size, pkt_size, core), 2ms
      );
      test_magnitude(MAG_SEL_MAG_SQ, num_ffts, fft_size, pkt_size, core);
      // We expect some overflow, so make sure that bit was set
      read_reg(REG_OVERFLOW_ADDR, val, core);
      `ASSERT_ERROR(val != 0, "Overflow register did not set.");
      read_reg(REG_OVERFLOW_ADDR, val, core);
      `ASSERT_ERROR(val == 0, "Overflow register did not self-clear.");
      test.end_test();
    end
  endtask : test_magnitude_options


  task automatic test_bypass(
    int num_ffts = 8,
    int fft_size = 128,
    int pkt_size = 2*fft_size,
    int core     = 0
  );
      const int num_items = num_ffts*fft_size;
      packet_info_t send_pkt_info = '0;
      logic [31:0] data_in [$];
      logic [31:0] data_out [NUM_CHAN_PER_CORE][$];
      int ports [] = new [NUM_CHAN_PER_CORE];

      // Create a list of ports involved in this test
      foreach (ports[idx]) ports[idx] = core*NUM_CHAN_PER_CORE + idx;

      `ASSERT_FATAL(EN_FFT_BYPASS, "Bypass logic is not enabled in core.");

      test.start_test($sformatf({
        "Test bypass\n",
        "    num_ffts: %0d\n",
        "    fft_size: %0d\n",
        "    pkt_size: %0d\n",
        "    core:     %0d"},
        num_ffts, fft_size, pkt_size, core), 2ms
      );

      // Set the packet size to be sent, in bytes
      foreach (ports[idx]) begin
        blk_ctrl.set_max_payload_length(ports[idx], pkt_size*4);
      end

      // Configure FFT with bypass. This FFT size shouldn't matter in bypass mode.
      config_fft(.fft_size(MAX_FFT_SIZE), .fft_bypass(1), .core(core));

      // Generate random data to send
      repeat (num_items) data_in.push_back($urandom());

      // Send and receive the data
      send_pkt_info.eob = '1;
      foreach (ports[idx]) begin
        blk_ctrl.send_packets_items(.port(ports[idx]), .items(data_in), .pkt_info(send_pkt_info));
      end
      foreach (ports[idx]) begin
        blk_ctrl.recv_packets_items(.port(ports[idx]), .items(data_out[idx]), .eob(1));
      end

      // Check that what we received exactly matches what we sent
      foreach (ports[idx]) begin
        `ASSERT_ERROR(data_out[idx] == data_in,
          "Bypass output didn't match expected values");
      end

      test.end_test();
  endtask : test_bypass


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf({
      "rfnoc_block_fft_tb\n",
      "\tFULL_TEST                = %0d\n",
      "\tCHDR_W                   = %0d\n",
      "\tNIPC                     = %0d\n",
      "\tNUM_PORTS                = %0d\n",
      "\tNUM_CORES                = %0d\n",
      "\tMAX_FFT_SIZE_LOG2        = %0d\n",
      "\tMAX_TEST_FFT_SIZE_LOG2   = %0d\n",
      "\tEN_CP_REMOVAL            = %0d\n",
      "\tEN_CP_INSERTION          = %0d\n",
      "\tMAX_CP_LIST_LEN_INS_LOG2 = %0d\n",
      "\tMAX_CP_LIST_LEN_REM_LOG2 = %0d\n",
      "\tEN_MAGNITUDE             = %0d\n",
      "\tEN_MAGNITUDE_SQ          = %0d\n",
      "\tEN_FFT_ORDER             = %0d\n",
      "\tEN_FFT_BYPASS            = %0d\n",
      "\tUSE_APPROX_MAG           = %0d\n",
      "\tVERBOSE                  = %0d"},

      FULL_TEST, CHDR_W, NIPC, NUM_PORTS, NUM_CORES, MAX_FFT_SIZE_LOG2,
      MAX_TEST_FFT_SIZE_LOG2, EN_CP_REMOVAL, EN_CP_INSERTION, MAX_CP_LIST_LEN_INS_LOG2,
      MAX_CP_LIST_LEN_REM_LOG2, EN_MAGNITUDE, EN_MAGNITUDE_SQ, EN_FFT_ORDER,
      EN_FFT_BYPASS, USE_APPROX_MAG, VERBOSE
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
    `ASSERT_ERROR(blk_ctrl.get_mtu()        == CHDR_MTU,     "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_registers();
    test_basic();

    if (FULL_TEST) begin
      if (EN_CP_INSERTION && EN_CP_REMOVAL) begin
        test_loopback(128, 2, '{80, 72, 112, 0, 80});
        test_loopback(256, 3, '{160, 144, 56, 0, 160});
        if (MAX_TEST_FFT_SIZE >= 4096) test_ofdm_config(4096, 1024);
        if (MAX_TEST_FFT_SIZE >= 8192) test_ofdm_config(8192, 1024);
      end
      if (EN_CP_INSERTION || EN_CP_REMOVAL) begin
        test_cyclic_prefix();
        test_max_min_cp_len();
        test_max_cp_list_len();
      end
      if (EN_FFT_ORDER) test_fft_order();
      test_throttle();
      test_throughput();
      test_max_min_packet();
      test_magnitude_options();
      test_max_min_fft();
      if (EN_FFT_BYPASS) test_bypass();

      // Do a lot of small tests to get more coverage in less time
      test_random(.num_iterations(200), .max_fft_size(`MIN(64, MAX_TEST_FFT_SIZE)));

      // Do some long tests
      test_random(.num_iterations(30),  .max_fft_size(MAX_TEST_FFT_SIZE));
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
