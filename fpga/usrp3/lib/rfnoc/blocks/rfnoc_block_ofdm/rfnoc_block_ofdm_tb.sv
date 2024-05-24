//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ofdm_tb
//
// Description: Testbench for the OFDM RFNoC block.
//

`default_nettype none

`define ABS(X)    ((X) < 0 ? -(X) : (X))
`define MIN(X, Y) ((X) < (Y) ? (X) : (Y))


module rfnoc_block_ofdm_tb;

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgRfnocItemUtils::*;

  import PkgMath::*;

  function automatic string prepend_base_path(string path);
    string file_path = `__FILE__;
    int len;
    for (len = file_path.len-len; len > 0; len--) begin
      if (file_path[len] == "/") break;
    end
    return {file_path.substr(0, len), path};
  endfunction

  //---------------------------------------------------------------------------
  // User Configuration
  //---------------------------------------------------------------------------
  localparam int    NUM_PORTS                = 1;
  localparam int    CP_INSERTION_REPEAT      = 1;  // Enable CP list FIFO loopback
  localparam int    MAX_CP_LIST_LEN_INS_LOG2 = 5;  // Up to 32 CP lengths in FIFO
  localparam int    CP_REMOVAL_REPEAT        = 1;  // Enable CP list FIFO loopback
  localparam int    MAX_CP_LIST_LEN_REM_LOG2 = 5;  // Up to 32 CP lengths in FIFO
  localparam int    MAX_CP_LEN_LOG2          = 12;
  localparam int    MAX_FFT_SIZE_LOG2        = 12;
  localparam int    MAX_FFT_SIZE             = 2**MAX_FFT_SIZE_LOG2;
  localparam int    FFT_SCALING              = dut.DEFAULT_FFT_SCALING;

  localparam bit    CHECK_OUTPUT             = 1;
  localparam bit    WRITE_OUTPUT             = 0; // Useful for creating a new expected output file

  string GOLDEN_INPUT_FILENAMES[]            = '{prepend_base_path("test_data/ifft_in_qpsk_ant_0_int16.bin"       )};
  string EXPECTED_OUTPUT_FILENAMES[]         = '{prepend_base_path("test_data/rfnoc_block_ofdm_dl_proc_output.bin")};
  string OUTPUT_FILENAMES[]                  = '{prepend_base_path("test_data/rfnoc_block_ofdm_dl_proc_output.bin")};

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------
  localparam        DUT_NAME        = "rfnoc_block_ofdm";
  localparam [31:0] NOC_ID          = 32'h0FD30000;
  localparam [ 9:0] THIS_PORTID     = 10'h123;
  localparam int    CHDR_W          = 64;     // CHDR size in bits
  localparam int    MTU             = 10;     // Log2 of max transmission unit in CHDR words
  localparam int    NUM_PORTS_I     = NUM_PORTS;
  localparam int    NUM_PORTS_O     = NUM_PORTS;
  localparam int    ITEM_W          = 32;     // Sample size in bits
  localparam int    SPP             = 64;     // Samples per packet
  localparam int    PKT_SIZE_BYTES  = SPP * (ITEM_W/8);
  localparam int    STALL_PROB      = 0;     // Default BFM stall probability
  localparam real   CHDR_CLK_PER    = 5.0;   // 200 MHz
  localparam real   CTRL_CLK_PER    = 8.0;   // 125 MHz
  localparam real   CE_CLK_PER      = 4.0;   // 250 MHz

  string input_filenames[]           = GOLDEN_INPUT_FILENAMES;
  string expected_output_filenames[] = CHECK_OUTPUT ? EXPECTED_OUTPUT_FILENAMES : {};
  string write_output_filenames[]    = WRITE_OUTPUT ? OUTPUT_FILENAMES          : {};

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit ce_clk;

  sim_clock_gen #(CHDR_CLK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(CTRL_CLK_PER) rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(CE_CLK_PER) ce_clk_gen (.clk(ce_clk), .rst());

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

  rfnoc_block_ofdm #(
    .THIS_PORTID             (THIS_PORTID             ),
    .CHDR_W                  (CHDR_W                  ),
    .MTU                     (MTU                     ),
    .NUM_PORTS               (NUM_PORTS               ),
    .MAX_FFT_SIZE_LOG2       (MAX_FFT_SIZE_LOG2       ),
    .MAX_CP_LEN_LOG2         (MAX_CP_LEN_LOG2         ),
    .MAX_CP_LIST_LEN_INS_LOG2(MAX_CP_LIST_LEN_INS_LOG2),
    .MAX_CP_LIST_LEN_REM_LOG2(MAX_CP_LIST_LEN_REM_LOG2),
    .CP_INSERTION_REPEAT     (CP_INSERTION_REPEAT     ),
    .CP_REMOVAL_REPEAT       (CP_REMOVAL_REPEAT       )
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
    input logic [31:0] addr,
    input logic [31:0] write_val
  );
    blk_ctrl.reg_write(4*addr, write_val);
  endtask

  task automatic read_reg (
    input  logic [31:0] addr,
    output logic [31:0] read_val
  );
    blk_ctrl.reg_read(4*addr, read_val);
  endtask

  task automatic write_reg_and_check (
    input logic [31:0] addr,
    input logic [31:0] write_val,
    input string reg_name = "'unspecified register'",
    input string func     = ""
  );
    logic [31:0] read_val;
    string func_int, s;

    write_reg(addr, write_val);
    read_reg(addr, read_val);
    if (func == "") begin
      $sformat(func_int, "write_reg_and_check()");
    end else begin
      $sformat(func_int, "%s() -> write_reg_and_check()", func);
    end
    $sformat(s, "ERROR: %s: Incorrect readback value for %s (addr: 0x%08x), Expected: 0x%08x, Received: 0x%08x",
      func_int, reg_name, addr, write_val, read_val);
    `ASSERT_ERROR(write_val == read_val, s);
  endtask

  task automatic user_reset ();
    int dummy_val;
    write_reg(dut.REG_USER_RESET_ADDR, 1'b1);
    read_reg(dut.REG_USER_RESET_ADDR, dummy_val); // Dummy read to ensure reset is complete
  endtask

  task automatic config_fft (
    input int fft_size,
    input int cyclic_prefix_insertion_lengths[] = {}, // Empty if not specified
    input int cyclic_prefix_removal_lengths[]   = {},
    input int fft_scaling                       = dut.DEFAULT_FFT_SCALING,
    input bit fft_direction                     = dut.FFT_FORWARD,
    input bit clear_config_fifo                 = 1
  );
    logic [31:0] fft_size_log2 = $clog2(fft_size);

    if (clear_config_fifo) begin
      $display("config_fft(): Clearing Cyclic Prefix insertion FIFO");
      write_reg(dut.REG_CP_INSERTION_CP_LEN_FIFO_CLR_ADDR, 1'b1);
      $display("config_fft(): Clearing Cyclic Prefix removal FIFO");
      write_reg(dut.REG_CP_REMOVAL_CP_LEN_FIFO_CLR_ADDR, 1'b1);
    end
    $display("config_fft(): Setting FFT Size (Log2) to %0d", fft_size_log2);
    write_reg(dut.REG_FFT_SIZE_LOG2_ADDR,                    fft_size_log2);
    $display("config_fft(): Setting FFT Scaling to 0x%8h",   fft_scaling);
    write_reg(dut.REG_FFT_SCALING_ADDR,                      fft_scaling);
    $display("config_fft(): Setting FFT Direction to %0d",   fft_direction);
    write_reg(dut.REG_FFT_DIRECTION_ADDR,                    fft_direction);
    foreach (cyclic_prefix_insertion_lengths[i]) begin
      $display("config_fft(): Setting Cyclic Prefix (insertion) %0d", cyclic_prefix_insertion_lengths[i]);
      write_reg(dut.REG_CP_INSERTION_CP_LEN_ADDR,                     cyclic_prefix_insertion_lengths[i]);
      write_reg(dut.REG_CP_INSERTION_CP_LEN_FIFO_LOAD_ADDR,           1'b1);
    end
    foreach (cyclic_prefix_removal_lengths[i]) begin
      $display("config_fft(): Setting Cyclic Prefix (removal) %0d", cyclic_prefix_removal_lengths[i]);
      write_reg(dut.REG_CP_REMOVAL_CP_LEN_ADDR,                     cyclic_prefix_removal_lengths[i]);
      write_reg(dut.REG_CP_REMOVAL_CP_LEN_FIFO_LOAD_ADDR,           1'b1);
    end

    // Commit the changes
    write_reg(dut.REG_FFT_COMMIT_ADDR, 1'b0);
  endtask

  // Sends 1/8th rate sine wave and checks output
  task automatic test_fft_sine_wave (
    input int fft_size,
    input int cyclic_prefix_insertion_lengths[] = {},
    input int cyclic_prefix_removal_lengths[]   = {},
    input int num_ports                         = 1
  );

    int unsigned num_ffts = cyclic_prefix_insertion_lengths.size() > cyclic_prefix_removal_lengths.size() ?
      cyclic_prefix_insertion_lengths.size() : cyclic_prefix_removal_lengths.size();

    config_fft(fft_size, cyclic_prefix_insertion_lengths, cyclic_prefix_removal_lengths, dut.DEFAULT_FFT_SCALING, dut.FFT_FORWARD, 1);

    $display("test_fft_sine_wave():");
    $display("\tfft_size: %d", fft_size);
    $display("\tcyclic_prefix_insertion_lengths: %p", cyclic_prefix_insertion_lengths);
    $display("\tcyclic_prefix_removal_lengths: %p", cyclic_prefix_removal_lengths);
    $display("\tnum_ports: %d", num_ports);

    // Send the 1/4th rate sine wave
    fork
      begin
        item_t        send_payload[$];
        packet_info_t send_pkt_info;
        int           phase   = 0;
        int           spp_cnt = 0;
        int           cp_removal_len;

        send_pkt_info = '{
          vc         : 0,
          eov        : 1'b0,
          eob        : 1'b0,
          has_time   : 1'b0,
          timestamp  : 64'd0
        };

        $display("test_fft_sine_wave(): Start send thread");

        for (int n = 0; n < num_ffts; n++) begin
          $display("test_fft_sine_wave(): FFT %2d: Start sending samples", n);

          // This emulates the CP removal FIFO's behavior
          if (cyclic_prefix_removal_lengths.size() == 0) begin
            cp_removal_len = 0;
          end else begin
            cp_removal_len = cyclic_prefix_removal_lengths[n % cyclic_prefix_removal_lengths.size()];
          end

          for (int i = 0; i < (fft_size + cp_removal_len); i++) begin
            case (phase)
              // Half scale to prevent overflow in Xilinx FFT
              0: send_payload.push_back({  16'd16383,  16'd0     });
              1: send_payload.push_back({  16'd0,      16'd16383 });
              2: send_payload.push_back({ -16'd16383,  16'd0     });
              3: send_payload.push_back({  16'd0,     -16'd16383 });
            endcase
            if (phase >= 3) begin
              phase = 0;
            end else begin
              phase++;
            end
            spp_cnt++;

            send_pkt_info.eob = ((n == num_ffts-1 && i == fft_size+cp_removal_len-1));

            // Send packet if we have accumulated SPP number of samples or we
            // are on the very last sample (which may not end on a SPP boundry).
            if (spp_cnt >= SPP || send_pkt_info.eob) begin
              send_pkt_info.has_time = (i == SPP-1);
              send_pkt_info.timestamp = (i == SPP-1) ? 64'h0123456789ABCDEF : 0;
              for (int port = 0; port < num_ports; port++) begin
                blk_ctrl.send_items(port, send_payload, {}, send_pkt_info);
                blk_ctrl.wait_complete(port);
              end
              send_payload = {};
              spp_cnt = 0;
            end
          end
          phase = 0;

          $display("test_fft_sine_wave(): FFT %2d: Done sending samples", n);
        end

        $display("test_fft_sine_wave(): Send thread complete");
      end

      begin
        string                s;
        item_t                recv_payload[$][$], temp_payload[$];
        chdr_word_t           recv_metadata[$];
        packet_info_t         recv_pkt_info[$][$], temp_pkt_info;
        int                   cp_insertion_len;
        logic signed   [15:0] real_val;
        logic signed   [15:0] imag_val;
        logic unsigned [31:0] mag_squared;

        $display("test_fft_sine_wave(): Start receive thread");

        for (int n = 0; n < num_ffts; n++) begin

          // This emulates the CP insertion FIFO's behavior
          if (cyclic_prefix_insertion_lengths.size() == 0) begin
            cp_insertion_len = 0;
          end else begin
            cp_insertion_len = cyclic_prefix_insertion_lengths[n % cyclic_prefix_insertion_lengths.size()];
          end

          $display("test_fft_sine_wave(): FFT %2d: Start receiving samples", n);

          for (int port = 0; port < num_ports; port++) begin
            recv_payload[port] = {};
            recv_pkt_info[port] = {};
          end

          // Receive packets in a loop on each port until we get
          // fft_size number of samples.
          // We will also get rx metadata (i.e. recv_pkt_info) for each received
          // packet.
          while (recv_payload[0].size() < (fft_size + cp_insertion_len)) begin
            for (int port = 0; port < num_ports; port++) begin
              temp_payload = {};
              blk_ctrl.recv_items_adv(port, temp_payload, recv_metadata, temp_pkt_info);
              foreach (temp_payload[i]) begin
                recv_payload[port].push_back(temp_payload[i]);
              end
              recv_pkt_info[port].push_back(temp_pkt_info);
            end
          end

          foreach (recv_payload[port]) begin
            $display("test_fft_sine_wave(): FFT %2d, port %0d received %0d samples",
              n, port, $size(recv_payload[port]));
          end
          $display("test_fft_sine_wave(): FFT %2d: Start checking samples", n);

          // EOB should be set only on the final packet
          if (n == num_ffts-1) begin
            for (int port = 0; port < num_ports; port++) begin
              for (int k = 0; k < recv_pkt_info[port].size(); k++) begin
                if (k == recv_pkt_info[port].size()-1) begin
                  $sformat(s,
                    "test_sine_wave(): Port %1d, EOB not set on final packet!", port);
                    `ASSERT_ERROR(recv_pkt_info[port][k].eob == 1'b1, s);
                end else begin
                  $sformat(s,
                    "test_sine_wave(): Port %1d, EOB set before final packet!", port);
                    `ASSERT_ERROR(recv_pkt_info[port][k].eob == 1'b0, s);
                end
              end
            end
          end

          // Check received samples on port
          for (int port = 0; port < num_ports; port++) begin
            for (int k = 0; k < recv_payload[port].size(); k++) begin

              {real_val, imag_val} = recv_payload[port][k];
              mag_squared          = real_val**2 + imag_val**2;

              if (k == (fft_size/4 + cp_insertion_len)) begin
                // Assert that for the special case of a 1/4th sample rate sine wave input,
                // the real part of the corresponding 1/4th sample rate FFT bin should always be greater than 0 and
                // the complex part equal to 0.
                $sformat(s,
                  "test_sine_wave(): Port %1d, FFT output %2d, Sample index %4d -- Expected magnitude: >= %0d, Received: %0d",
                  port, n, k, 16368**2, mag_squared);
                  `ASSERT_ERROR(mag_squared >= 16368**2, s);
              end else begin
                // Assert all other FFT bins should be 0 for both complex and real parts
                $sformat(s,
                  "test_sine_wave(): Port %1d, FFT output %2d, Sample index %4d -- Expected (real value): 0, Received: %0d",
                  port, n, k, real_val);
                  `ASSERT_ERROR(real_val == 0, s);
                $sformat(s,
                  "test_sine_wave(): Port %1d, FFT output %2d, Sample index %4d -- Expected (imag value): 0, Received: %0d",
                  port, n, k, imag_val);
                  `ASSERT_ERROR(imag_val == 0, s);
              end
            end
          end

          $display("test_fft_sine_wave(): FFT %2d: Done checking samples", n);
        end
      $display("test_fft_sine_wave(): Receive thread complete");
      end
    join
  endtask

  task automatic test_user_reg (
    input string       reg_name,
    input int unsigned addr,
    input int unsigned value,
    input int unsigned bit_width = 32,
    input int unsigned port      = 1
  );

    logic [31:0] write_val, read_val, check_val;
    string s;

    $write("%s: Set and check %s register... ", DUT_NAME, reg_name);
    write_val = $random();
    write_reg(addr, write_val);
    read_reg(addr, read_val);
    check_val = write_val & 32'((1 << bit_width)-1); // Mask relevant bits
    $sformat(s, "%s: %s register incorrect readback! Expected: %0d, Actual %0d", DUT_NAME, reg_name, check_val, read_val);
    `ASSERT_FATAL(read_val == check_val, s);
    $write("Done\n");

  endtask

  // Send input vectors read for files
  task automatic check_input_vectors (
    input string       input_filename[],                 // Array of input filenames, one entry per port to send
    input string       expected_output_filename[]  = {}, // Array of expected output filenames, skips output checking if empty
    input string       output_filename[]           = {}, // Array of output filenames, skips writing output to file if empty
    input int unsigned num_samples_to_send         = 0,  // Sends entire input file if set to 0
    input int unsigned num_samples_to_check        = 0   // Checks entire output file if set to 0
  );
    int unsigned num_ports         = input_filename.size();
    bit          skip_check_output = (expected_output_filename.size() == 0);
    bit          skip_write_output = (output_filename.size() == 0);
    string s;

    int sent_samp_cnt[];
    int recv_samp_cnt[];
    sent_samp_cnt = new [num_ports];
    recv_samp_cnt = new [num_ports];
    foreach (sent_samp_cnt[i]) sent_samp_cnt[i] = 0;
    foreach (recv_samp_cnt[i]) recv_samp_cnt[i] = 0;

    $sformat(s,
      "check_input_vectors(): Not enough input filenames provided! Expected: > 0, Found: %2d!",
      num_ports);
    `ASSERT_FATAL(num_ports > 0, s);

    $sformat(s,
      "check_input_vectors(): Not enough expected output filenames provided! Expected: %2d, Found: %2d!",
      num_ports, expected_output_filename.size());
    `ASSERT_FATAL(skip_check_output || (expected_output_filename.size() == num_ports), s);

    $sformat(s,
      "check_input_vectors(): Not enough output filenames provided! Expected: %2d, Found: %2d!",
      num_ports, output_filename.size());
    `ASSERT_FATAL(skip_write_output || (output_filename.size() == num_ports), s);

    fork
    // Send input file
    begin
      automatic string                s;
      automatic bit                   done[];
      automatic int                   done_cnt = 0;
      automatic int                   fi[];
      automatic int                   eof[];
      automatic logic [63:0]          send_timestamp[];
      automatic item_t                send_samples[$];
      automatic packet_info_t         send_pkt_info;
      automatic logic [31:0]          samp_in;
      automatic logic [15:0]          samp_in_re, samp_in_im;

      done           = new [num_ports];
      fi             = new [num_ports];
      eof            = new [num_ports];
      send_timestamp = new [num_ports];
      foreach (done[i]) done[i] = 0;
      foreach (send_timestamp[i]) send_timestamp[i] = 0;

      $display("check_input_vectors(): Start Send Thread");

      for (int n = 0; n < num_ports; n++) begin
        $display("check_input_vectors(): Send Thread: Opening %s", input_filename[n]);
        fi[n] = $fopen(input_filename[n], "rb");
        $sformat(s, "check_input_vectors(): Send Thread: Unable to open file for reading!");
        `ASSERT_FATAL(fi[n] != 0, s);
        $sformat(s, "check_input_vectors(): Send Thread: File is empty!");
        `ASSERT_FATAL($feof(fi[n]) == 0, s);
      end

      // Send samples on each port
      while (done_cnt < num_ports) begin
        for (int n = 0; n < num_ports; n++) begin
          if (!done[n]) begin
            send_samples = {};
            for (int i = 1; i <= SPP; i++) begin
              int code;
              // Read sample from file and load into send queue
              code = $fread(samp_in, fi[n]);
              // Reverse endianess
              {samp_in_re,samp_in_im} = {samp_in[23:16], samp_in[31:24], samp_in[7:0], samp_in[15:8]};
              send_samples.push_back({samp_in_re, samp_in_im});
              sent_samp_cnt[n] = sent_samp_cnt[n] + 1;
              // Break loop if we have sent the desired number of samples
              if ((num_samples_to_send != 0) && (sent_samp_cnt[n] >= num_samples_to_send)) break;
              // Break loop on EOF
              code = $fread(samp_in, fi[n]); // Dummy read
              eof[n] = $feof(fi[n]);  // Check for EOF
              code = $fseek(fi[n], -4, 1);   // Return to original spot
              if (eof[n]) break;
            end

            send_pkt_info = '{
              vc         : 0,
              eov        : 1'b0,
              eob        : 1'b0,
              has_time   : 1'b1,
              timestamp  : send_timestamp[n]
            };

            // If end of file, set EOB and end loop
            if (eof[n]) begin
              $display("check_input_vectors(): Send Thread (Port %2d): Encountered EOF after %7d samples, sending EOB", n, sent_samp_cnt[n]);
              send_pkt_info.eob = 1'b1;
              done[n] = 1;
            end
            // If we have sent the expected number of samples, set EOB and end loop
            if ((num_samples_to_send != 0) && (sent_samp_cnt[n] >= num_samples_to_send)) begin
              $display("check_input_vectors(): Send Thread (Port %2d): Sending EOB", n);
              send_pkt_info.eob = 1'b1;
              done[n] = 1;
            end

            // Send packet (but only if there are samples to send)
            if (send_samples.size() > 0) begin
              // Update timestamp for next loop's packet info, assuming a tick rate of 1
              send_timestamp[n] += send_samples.size();

              blk_ctrl.send_items(n, send_samples, {}, send_pkt_info);

              // Synchronize sending data on all ports together by only waiting
              // for send to complete on the last port
              if (n == num_ports-1) begin
                blk_ctrl.wait_complete(n);
              end
            end

            $display("check_input_vectors(): Send Thread (Port %2d): Number of samples sent: %7d", n, sent_samp_cnt[n]);
            if (done[n]) begin
              $display("check_input_vectors(): Send Thread (Port %2d): Total number of samples sent: %7d", n, sent_samp_cnt[n]);
              $display("check_input_vectors(): Send Thread (Port %2d): Send complete!", n);
            end
          end
        end
        done_cnt = 0;
        foreach (done[n]) begin
          done_cnt += done[n];
        end
      end

      for (int n = 0; n < num_ports; n++) begin
        $fclose(fi[n]);
      end
      $display("check_input_vectors(): Send Thread Complete!");
    end
    // Receive and check output and/or write output to file
    begin
      automatic string                s;
      automatic bit                   done[];
      automatic int                   done_cnt = 0;
      automatic int                   fo[];
      automatic int                   fchk[];
      automatic int                   eof[];
      automatic item_t                recv_samples[$];
      automatic chdr_word_t           recv_metadata[$];
      automatic packet_info_t         recv_pkt_info;
      automatic logic [31:0]          samp_chk;
      automatic logic signed [15:0]   samp_chk_re, samp_chk_im;
      automatic logic [31:0]          samp_out;
      automatic logic signed [15:0]   samp_out_re, samp_out_im;
      const     logic signed [15:0]   threshold = 16'd2;

      int num_samps_recv;

      done = new [num_ports];
      fo   = new [output_filename.size];
      fchk = new [expected_output_filename.size];
      eof  = new [expected_output_filename.size];
      foreach (done[i]) done[i] = 0;

      $display("check_input_vectors(): Start Receive Thread");

      for (int n = 0; n < fchk.size(); n++) begin
        fchk[n] = $fopen(expected_output_filename[n], "rb");
        $sformat(s, "check_input_vectors(): Receive Thread: Unable to read %s!", expected_output_filename[n]);
        `ASSERT_FATAL(fchk[n] != 0, s);
      end

      for (int n = 0; n < fo.size(); n++) begin
        fo[n] = $fopen(output_filename[n], "wb");
        $sformat(s, "check_input_vectors(): Receive Thread: Unable to open %s!\n", output_filename[n]);
        `ASSERT_FATAL(fo[n] != 0, s);
      end

      // Receive samples on each port
      while (done_cnt < num_ports) begin
        for (int n = 0; n < num_ports; n++) begin
          if (!done[n]) begin
            // Receive the output packet
            recv_samples = {};
            blk_ctrl.recv_items_adv(n, recv_samples, recv_metadata, recv_pkt_info);
            num_samps_recv = recv_samples.size();
            recv_samp_cnt[n] = recv_samp_cnt[n] + num_samps_recv;
            $display("check_input_vectors(): Receive Thread (Port %2d): Number samples received: %7d", n, recv_samp_cnt[n]);

            // Check received samples
            if (!skip_check_output) begin
              for (int i = 0; i < num_samps_recv; i++) begin
                logic signed [15:0] diff_re, diff_im;
                int code;
                code = $fread(samp_chk, fchk[n]);
                eof[n] = $feof(fchk[n]);
                if (!eof[n]) begin
                  // Reverse endianess
                  {samp_chk_re, samp_chk_im} = {samp_chk[23:16], samp_chk[31:24], samp_chk[7:0], samp_chk[15:8]};
                end else begin
                  $sformat(s,
                    "check_input_vectors(): Receive Thread (Port %2d): Not enough samples to check! Expected output samples file %s reached EOF after %d samples!",
                    n, expected_output_filename[n], recv_samp_cnt[n] - num_samps_recv + i);
                  `ASSERT_FATAL(0, s);
                end
                {samp_out_re, samp_out_im} = recv_samples[i];
                // TODO: Need to figure out why the scaling is off here
                samp_chk_re = samp_chk_re >>> 5;
                samp_chk_im = samp_chk_im >>> 5;
                diff_re = `ABS(samp_chk_re - samp_out_re);
                diff_im = `ABS(samp_chk_im - samp_out_im);
                $sformat(s,
                  "check_input_vectors(): Receive Thread (Port %2d, Sample %0d): Sample invalid! Expected: %d + %di, Received: %d + %di",
                  n, i, samp_chk_re, samp_chk_im, samp_out_re, samp_out_im);
                `ASSERT_ERROR((diff_re <= threshold) && (diff_im <= threshold), s);
              end
            end

            // Write received samples to file
            if (!skip_write_output) begin
              for (int i = 0; i < num_samps_recv; i++) begin
                samp_out = recv_samples[i];
                $fwrite(fo[n], "%c%c%c%c", samp_out[23:16], samp_out[31:24], samp_out[7:0], samp_out[15:8]);
              end
            end

            // End due to receiving EOB
            if (recv_pkt_info.eob) begin
              $display("check_input_vectors(): Receive Thread (Port %2d): Receive complete due to EOB detected!", n);
              done[n] = 1;
            end
            // End receive due to receiving the expected number of samples to check
            if ((num_samples_to_check != 0) && (recv_samp_cnt[n] >= num_samples_to_check)) begin
              done[n] = 1;
            end

            if (done[n]) begin
              $display("check_input_vectors(): Receive Thread (Port %2d): Total number samples received: %7d", n, recv_samp_cnt[n]);
              $display("check_input_vectors(): Receive Thread (Port %2d): Receive complete!", n);
            end
          end
        end
        done_cnt = 0;
        foreach (done[n]) begin
          done_cnt += done[n];
        end
      end

      for (int n = 0; n < fchk.size; n++) begin
        $fclose(fchk[n]);
      end
      for (int n = 0; n < fo.size; n++) begin
        $fclose(fo[n]);
      end
      $display("check_input_vectors(): Receive Thread Complete!");
    end
    join
  endtask


  // Generates a complex sine wave with period of 4 samples.
  //
  //   num_samples : The number of samples to generate
  //   phase       : The phase offset of the first sample. -1 would start one
  //                 sample before the real part crosses y = 9.
  //   amplitude   : Amplitude of the signal
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

  // pkt_size : Packet size in samples
  task automatic test_fft_sine(
    int fft_size,
    int num_ffts        = 1,
    int cp_insertions[] = {},
    int cp_removals[]   = {},
    int pkt_size        = fft_size < SPP ? fft_size : SPP,
    int port            = 0
  );

    // For now, we send one FFT per packet, unless the FFT is larger than the
    // packet size, which is supported.
    assert(fft_size % pkt_size == 0) else
      $error("pkt_size must be a multiple of fft_size");

    // This might work, but it's not an expected use case
    assert(!(cp_insertions.size() && cp_removals.size())) else
      $error("Cannot specify both CP insertion and CP removal");

    $display("test_fft_sine():");
    $display("    fft_size:      %0d", fft_size);
    $display("    num_ffts:      %0d", num_ffts);
    $display("    cp_insertions: %p",  cp_insertions);
    $display("    cp_removals:   %p",  cp_removals);

    config_fft(fft_size, cp_insertions, cp_removals, dut.DEFAULT_FFT_SCALING, dut.FFT_FORWARD);

    fork
      begin : sender
        item_queue_t  send_payload;
        packet_info_t send_pkt_info;
        int           cp_removal_len;
        int           num_pkts_per_fft;
        int           total_fft_size;

        send_pkt_info = '0;

        $display("test_fft_sine(): send: Start send thread");

        for (int fft_count = 0; fft_count < num_ffts; fft_count++) begin
          // This emulates the CP removal FIFO's behavior
          if (cp_removals.size() == 0) cp_removal_len = 0;
          else cp_removal_len = cp_removals[fft_count % cp_removals.size()];

          total_fft_size = fft_size + cp_removal_len;
          num_pkts_per_fft = $ceil(real'(total_fft_size) / pkt_size);
          send_payload = gen_sine_wave(fft_size + cp_removal_len, -cp_removal_len);

          $display("test_fft_sine(): send: Starting FFT %0d for %0d+%0d = %0d samples", fft_count, cp_removal_len, fft_size, total_fft_size);

          for (int pkt_count = 0; pkt_count < num_pkts_per_fft; pkt_count++) begin
            int start, len;

            start = pkt_count * pkt_size;
            len = `MIN(total_fft_size - pkt_count*pkt_size, pkt_size);

            //$display("test_fft_sine(): send: FFT %0d, PKT %0d: Sending %0d samples", fft_count, pkt_count, len);

            send_pkt_info.eob = (fft_count == num_ffts-1 && pkt_count == num_pkts_per_fft-1);
            // TODO: Set EOV? send_pkt_info.eov = (pkt_count == num_pkts_per_fft-1);

            blk_ctrl.send_items(port, send_payload[start : start+len-1], , send_pkt_info);
            blk_ctrl.wait_complete(port);
          end
        end
        $display("test_fft_sine(): send: End send thread");
      end : sender

      begin : receiver
        item_queue_t  recv_payload;
        chdr_word_t   recv_metadata[$];
        packet_info_t recv_pkt_info;
        int           cp_insertion_len;
        bit           eob, eov;
        int           num_pkts_per_fft;
        int           samp_count;
        int           total_fft_size;


        $display("test_fft_sine(): recv: Start recv thread");

        for (int fft_count = 0; fft_count < num_ffts; fft_count++) begin
          // This emulates the CP insertion FIFO's behavior
          if (cp_insertions.size() == 0) cp_insertion_len = 0;
          else cp_insertion_len = cp_insertions[fft_count % cp_insertions.size()];

          total_fft_size = fft_size + cp_insertion_len;
          num_pkts_per_fft = $ceil(real'(total_fft_size) / pkt_size);

          $display("test_fft_sine(): recv: Starting FFT %0d for %0d+%0d = %0d samples", fft_count, cp_insertion_len, fft_size, total_fft_size);

          samp_count = 0;
          for (int pkt_count = 0; pkt_count < num_pkts_per_fft; pkt_count++) begin
            //$display("test_fft_sine(): recv: FFT %0d, PKT %0d: Receiving samples", fft_count, pkt_count);

            eob = (fft_count == num_ffts-1);
            eov = (pkt_count == num_pkts_per_fft-1);

            blk_ctrl.recv_items_adv(port, recv_payload, recv_metadata, recv_pkt_info);

            // Check flags
            if (pkt_count == num_pkts_per_fft-1) begin
              `ASSERT_ERROR(
                eov, $sformatf("test_fft_sine(): recv: EOV not set on last packet for FFT.")
              );
              if (fft_count == num_ffts-1) begin
                `ASSERT_ERROR(
                  eob, $sformatf("test_fft_sine(): recv: EOB not set on last packet of last FFT.")
                );
              end
            end

            // Verify the sample values
            foreach (recv_payload[samp_i]) begin
              if (samp_count == (cp_insertion_len + fft_size/4)) begin
                bit signed [15:0] real_val, imag_val;
                int magnitude;
                {real_val, imag_val} = recv_payload[samp_i];
                magnitude = $sqrt(real_val**2 + imag_val**2);
                `ASSERT_ERROR(
                  imag_val == 0,
                  $sformatf("test_fft_sine(): recv: FFT %0d, PKT %0d: Expected 0 for im at sample %0d, received 0x%x",
                    fft_count, pkt_count, samp_count, recv_payload[samp_i])
                );
                `ASSERT_ERROR(
                  real_val >= 16368,
                  $sformatf("test_fft_sine(): recv: FFT %0d, PKT %0d: Expected re >= 16368 at sample %0d, received 0x%x",
                    fft_count, pkt_count, samp_count, recv_payload[samp_i])
                );
              end else begin
                `ASSERT_ERROR(
                  recv_payload[samp_i] == 0,
                  $sformatf("test_fft_sine(): recv: FFT %0d, PKT %0d: Expected 0 at sample %0d, received 0x%x",
                    fft_count, pkt_count, samp_count, recv_payload[samp_i])
                );
              end
              samp_count++;
            end
          end

          // Check the length
          `ASSERT_ERROR(
            samp_count == total_fft_size,
            $sformatf("test_fft_sine(): recv: FFT %0d: Expected %0d samples but received %0d",
              fft_count, total_fft_size, samp_count)
          );
        end
        $display("test_fft_sine(): recv: End recv thread");
      end : receiver
    join
  endtask


  task automatic test_customer_config(int fft_size, int pkt_size = SPP);
    int cp_lengths[] = new [14];

    if (fft_size == 4096) begin
      // Assume 122.88 MS/s, 30 kHz subcarrier spacing (mu=1), 28 OFDM symbols
      // per 1 ms subframe.
      cp_lengths = '{352, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288};
    end else if (fft_size == 8192) begin
      // Assume 245.76 MS/s, 30 kHz subcarrier spacing (mu=1), 28 OFDM symbols
      // per 1 ms subframe.
      cp_lengths = '{704, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576, 576};
    end else begin
      $fatal(1, "Bad FFT size");
    end

    // But this does work for 8192?
    test_fft_sine(fft_size, cp_lengths.size(), .cp_insertions(cp_lengths));
    test_fft_sine(fft_size, cp_lengths.size(), .cp_removals(cp_lengths));

    // Why doesn't this work for 8192?
    test_fft_sine_wave(fft_size, .cyclic_prefix_insertion_lengths(cp_lengths));
    test_fft_sine_wave(fft_size, .cyclic_prefix_removal_lengths(cp_lengths));
  endtask


  task automatic test_loopback();
    localparam int port = 0;
    localparam int spp = 1024;
    localparam int fft_size = 4096;
    localparam int fft_scaling  = 12'b_10_10_10_10_10_10; // Default: 12'b_01_10_10_10_10_10;
    localparam int ifft_scaling = 12'b_00_00_00_00_00_00;
    packet_info_t send_pkt_info;


    logic [31:0] data_in  [] = new [fft_size];
    logic [31:0] signal   [$];
    logic [31:0] data_out [$];


    // Set the SPP
    blk_ctrl.set_max_payload_length(port, spp);

    // Configure for IFFT
    config_fft(fft_size, , , ifft_scaling, dut.FFT_REVERSE);

    // Generate a test input signal
    data_in = '{default: 0};
    data_in[1024] = {16'd32766, 16'd0};

    // Send symbols
    send_pkt_info = '0;
    send_pkt_info.eov = 1;
    send_pkt_info.eob = 1;
    blk_ctrl.send_packets_items(port, data_in, , send_pkt_info);

    // Get the the resulting signal
    blk_ctrl.recv_packets_items(port, signal);
    $display("Received %0d samples:", signal.size());
    for (int i; i < 16; i++) begin
      $display("(%d,%d)", signed'(signal[i][31:16]), signed'(signal[i][15:0]));
    end

    // Configure for IFFT
    config_fft(fft_size, , , fft_scaling, dut.FFT_FORWARD);

    // Send signal
    send_pkt_info = '0;
    send_pkt_info.eov = 1;
    send_pkt_info.eob = 1;
    blk_ctrl.send_packets_items(port, signal, , send_pkt_info);

    // Get the the resulting FFT
    blk_ctrl.recv_packets_items(port, data_out);
    $display("Received %0d samples:", data_out.size());
    foreach (data_out[i]) begin
      if (data_out[i] != 0) $display("Output peak at %0d with value (%d,%d)",
        i, signed'(data_out[i][31:16]), signed'(data_out[i][15:0]));
    end
  endtask


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------
  initial begin : tb_main

    automatic int cp_lengths_single[] = '{320};
    automatic int cp_lengths_multi[]  = '{320, 288, 111, 0, 320};
    automatic int cp_lengths_golden[] = '{320, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288, 288};

    // Initialize the test exec object for this testbench
    test.start_tb(DUT_NAME);

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

    test.stop_on_error = 0;

    test.start_test("Verify Block Info", 2us);
    `ASSERT_ERROR(blk_ctrl.get_noc_id()     == NOC_ID,      "Incorrect NOC_ID Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS_I, "Incorrect NUM_DATA_I Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS_O, "Incorrect NUM_DATA_O Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu()        == MTU,         "Incorrect MTU Value");
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test.start_test("Basic tests");

    // TODO:
    //   - Test with multiple ports with different delays
    //   - Test with timestamps on every packet and just the first of burst
    //   - Only one port in use in a multi-port system
    //   - Test EOV being added at appropriate intervals when input packets are larger than FFT
    //   - Add register to read back capabilities (FFT size, CP size, etc.)
    //   - Add compat register

    if (NUM_PORTS > 1) begin
      test_fft_sine_wave(4096, .num_ports(NUM_PORTS), .cyclic_prefix_insertion_lengths(cp_lengths_single));
      test_fft_sine_wave(4096, .num_ports(NUM_PORTS), .cyclic_prefix_insertion_lengths(cp_lengths_multi));
      test_fft_sine_wave(4096, .num_ports(NUM_PORTS), .cyclic_prefix_removal_lengths(cp_lengths_single));
      test_fft_sine_wave(4096, .num_ports(NUM_PORTS), .cyclic_prefix_removal_lengths(cp_lengths_multi));
    end else begin
      // Test the customer use cases
      test_customer_config(4096, 1024);

      test_loopback();

      // Run all the old tests
      test_fft_sine_wave(4096, .num_ports(1), .cyclic_prefix_insertion_lengths(cp_lengths_single));
      test_fft_sine_wave(4096, .num_ports(1), .cyclic_prefix_insertion_lengths(cp_lengths_multi));
      test_fft_sine_wave(4096, .num_ports(1), .cyclic_prefix_removal_lengths(cp_lengths_single));
      test_fft_sine_wave(4096, .num_ports(1), .cyclic_prefix_removal_lengths(cp_lengths_multi));

      test_fft_sine(4096, cp_lengths_single.size(), .cp_removals  (cp_lengths_single));
      test_fft_sine(4096, cp_lengths_multi.size(),  .cp_removals  (cp_lengths_multi));
      test_fft_sine(4096, cp_lengths_single.size(), .cp_insertions(cp_lengths_single));
      test_fft_sine(4096, cp_lengths_multi.size(),  .cp_insertions(cp_lengths_multi));
    end

    test.end_test();

//    // Set and check configuration registers
//    test.start_test("Test Configuration Registers", 100us);
//
//    test_user_reg("FFT Size",      dut.REG_FFT_SIZE_LOG2_ADDR, FFT_SIZE_LOG2, dut.REG_FFT_SIZE_LOG2_WIDTH);
//    test_user_reg("FFT Scaling",   dut.REG_FFT_SCALING_ADDR,   FFT_SCALING,   dut.REG_FFT_SCALING_WIDTH);
//    test_user_reg("FFT Direction", dut.REG_FFT_DIRECTION_ADDR, FFT_DIRECTION, dut.REG_FFT_DIRECTION_WIDTH);
//
//    test.end_test();
//
//    // Test IFFT with cyclic prefix insertion with a sine wave
//    test.start_test("Test Cyclic Prefix Insertion", 1ms);
//
//    user_reset();
//
//    test_fft_sine_wave(FFT_SIZE, cp_lengths_single, /* No CP removal */);
//    test_fft_sine_wave(FFT_SIZE, cp_lengths_multi, /* No CP removal */);
//
//    test.end_test();
//
//    test.start_test("Test Cyclic Prefix Removal", 1ms);
//
//    user_reset();
//
//    test_fft_sine_wave(FFT_SIZE, /* No CP insertion */, cp_lengths_single);
//    test_fft_sine_wave(FFT_SIZE, /* No CP insertion */, cp_lengths_multi);
//
//    test.end_test();
//
//  // This test mosly passes, but fails after a bunch of packets.
//  test.start_test("Test Golden Input Data", 1ms);
//
//  user_reset();
//  config_fft(FFT_SIZE, cp_lengths_golden, /* No CP removal */, dut.DEFAULT_FFT_SCALING, dut.FFT_FORWARD, 1);
//  check_input_vectors(input_filenames,           // Golden input file
//                      expected_output_filenames, // Golden expected output file
//                      write_output_filenames,    // Output file
//                      0,  // Number of input samples to send, 0 for entire file
//                      0); // Number of output samples to check, 0 for entire file
//
//  test.end_test();
//
    //--------------------------------
    // Finish Up
    //--------------------------------

    // Display final statistics and results
    test.end_tb();
  end

endmodule


`default_nettype wire
