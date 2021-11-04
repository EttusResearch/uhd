//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_radio_tb
//
// Description: This is the testbench for rfnoc_block_radio.
//


module rfnoc_block_radio_tb #(
  parameter int CHDR_W     = 128, // CHDR bus width
  parameter int ITEM_W     = 32,  // Sample width
  parameter int NIPC       = 2,   // Number of samples per radio clock cycle
  parameter int NUM_PORTS  = 2,   // Number of radio channels
  parameter int STALL_PROB = 25,  // Probability of AXI BFM stall
  parameter int STB_PROB   = 80,  // Probability of radio STB asserting
  parameter bit TEST_REGS  = 1    // Do register tests
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgRfnocBlockCtrlBfm::*;
  import PkgAxisCtrlBfm::*;
  import PkgChdrBfm::*;

  // Pull in radio register offsets and constants
  `include "rfnoc_block_radio_regs.vh"


  // Simulation Parameters
  localparam int          NOC_ID         = 32'h12AD1000;
  localparam logic [ 9:0] THIS_PORTID    = 10'h17;
  localparam logic [15:0] THIS_EPID      = 16'hDEAD;
  localparam int          MTU            = 8;
  localparam int          RADIO_W        = NIPC * ITEM_W;       // Radio word size
  localparam int          SPP            = 64;                  // Samples per packet
  localparam int          WPP            = SPP*ITEM_W/RADIO_W;  // Radio words per packet
  localparam int          CHDR_CLK_PER   = 5;                   // rfnoc_chdr_clk period in ns
  localparam int          CTRL_CLK_PER   = 25;                  // rfnoc_ctrl_clk period in ns
  localparam int          RADIO_CLK_PER  = 10;                  // radio_clk_per period in ns

  // Amount of time to wait for a packet to be fully acquired
  localparam realtime MAX_PKT_WAIT = 4*WPP*(RADIO_CLK_PER+CTRL_CLK_PER)*1ns;

  // Error reporting values to use
  localparam bit [ 9:0] TX_ERR_DST_PORT     = 10'h2B5;
  localparam bit [ 9:0] TX_ERR_REM_DST_PORT = 10'h14C;
  localparam bit [15:0] TX_ERR_REM_DST_EPID = 16'hA18E;
  localparam bit [19:0] TX_ERR_ADDRESS      = 20'hA31D3;



  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;
  bit rfnoc_ctrl_clk;
  bit radio_clk;

  // Don't start the clocks automatically (AUTOSTART=0), since we expect
  // multiple instances of this testbench to run in sequence. They will be
  // started before the first test.
  sim_clock_gen #(.PERIOD(CHDR_CLK_PER), .AUTOSTART(0))
    rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());
  sim_clock_gen #(.PERIOD(CTRL_CLK_PER), .AUTOSTART(0))
    rfnoc_ctrl_clk_gen (.clk(rfnoc_ctrl_clk), .rst());
  sim_clock_gen #(.PERIOD(RADIO_CLK_PER), .AUTOSTART(0)) 
    radio_clk_gen (.clk(radio_clk), .rst());



  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Connections to DUT as interfaces:
  RfnocBackendIf        backend            (rfnoc_chdr_clk, rfnoc_ctrl_clk);
  AxiStreamIf #(32)     m_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(32)     s_ctrl             (rfnoc_ctrl_clk, 1'b0);
  AxiStreamIf #(CHDR_W) m_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);
  AxiStreamIf #(CHDR_W) s_chdr [NUM_PORTS] (rfnoc_chdr_clk, 1'b0);

  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      sample_t;

  // Bus functional model for a software block controller
  RfnocBlockCtrlBfm #(CHDR_W, ITEM_W) blk_ctrl = new(backend, m_ctrl, s_ctrl);

  // Connect block controller to BFMs
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_connections
    initial begin
      blk_ctrl.connect_master_data_port(i, m_chdr[i]);
      blk_ctrl.connect_slave_data_port(i, s_chdr[i]);

      // Set the initial CHDR BFM stall probability
      blk_ctrl.set_master_stall_prob(i, STALL_PROB);
      blk_ctrl.set_slave_stall_prob(i, STALL_PROB);
    end
  end



  //---------------------------------------------------------------------------
  // Radio Data Model
  //---------------------------------------------------------------------------

  bit [NUM_PORTS*RADIO_W-1:0] radio_rx_data;
  bit [        NUM_PORTS-1:0] radio_rx_stb;

  bit [63:0] radio_time;
  bit        radio_pps;

  // Radio data generation
  sim_radio_gen #(
    .NSPC         (NIPC),
    .SAMP_W       (ITEM_W),
    .NUM_CHANNELS (NUM_PORTS),
    .STB_PROB     (STB_PROB),
    .INCREMENT    (NIPC),
    .PPS_PERIOD   (NIPC * 250)
  ) radio_gen (
    .radio_clk     (radio_clk),
    .radio_rst     (1'b0),
    .radio_rx_data (radio_rx_data),
    .radio_rx_stb  (radio_rx_stb),
    .radio_time    (radio_time),
    .radio_pps     (radio_pps)
  );



  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS-1:0] radio_rx_running;

  logic [NUM_PORTS*RADIO_W-1:0] radio_tx_data;
  logic [        NUM_PORTS-1:0] radio_tx_stb;
  logic [        NUM_PORTS-1:0] radio_tx_running;

  logic [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata_flat;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast_flat;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid_flat;
  logic [       NUM_PORTS-1:0] s_rfnoc_chdr_tready_flat;

  logic [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata_flat;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast_flat;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid_flat;
  logic [       NUM_PORTS-1:0] m_rfnoc_chdr_tready_flat;

  // Use the same strobe for both Rx and Tx
  assign radio_tx_stb = radio_rx_stb;

  // Flatten the data stream arrays into concatenated vectors
  genvar i;
  for (i = 0; i < NUM_PORTS; i++) begin : gen_radio_connections
    assign s_rfnoc_chdr_tdata_flat[CHDR_W*i+:CHDR_W] = m_chdr[i].tdata;
    assign s_rfnoc_chdr_tlast_flat[i]                = m_chdr[i].tlast;
    assign s_rfnoc_chdr_tvalid_flat[i]               = m_chdr[i].tvalid;
    assign m_chdr[i].tready                          = s_rfnoc_chdr_tready_flat[i];

    assign s_chdr[i].tdata             = m_rfnoc_chdr_tdata_flat[CHDR_W*i+:CHDR_W];
    assign s_chdr[i].tlast             = m_rfnoc_chdr_tlast_flat[i];
    assign s_chdr[i].tvalid            = m_rfnoc_chdr_tvalid_flat[i];
    assign m_rfnoc_chdr_tready_flat[i] = s_chdr[i].tready;
  end


  rfnoc_block_radio #(
    .THIS_PORTID (THIS_PORTID),
    .CHDR_W      (CHDR_W),
    .NIPC        (NIPC),
    .ITEM_W      (ITEM_W),
    .NUM_PORTS   (NUM_PORTS),
    .MTU         (MTU)
  ) rfnoc_block_radio_i (
    .rfnoc_chdr_clk          (backend.chdr_clk),
    .s_rfnoc_chdr_tdata      (s_rfnoc_chdr_tdata_flat),
    .s_rfnoc_chdr_tlast      (s_rfnoc_chdr_tlast_flat),
    .s_rfnoc_chdr_tvalid     (s_rfnoc_chdr_tvalid_flat),
    .s_rfnoc_chdr_tready     (s_rfnoc_chdr_tready_flat),
    .m_rfnoc_chdr_tdata      (m_rfnoc_chdr_tdata_flat),
    .m_rfnoc_chdr_tlast      (m_rfnoc_chdr_tlast_flat),
    .m_rfnoc_chdr_tvalid     (m_rfnoc_chdr_tvalid_flat),
    .m_rfnoc_chdr_tready     (m_rfnoc_chdr_tready_flat),
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
    .m_rfnoc_ctrl_tready     (s_ctrl.tready),
    .m_ctrlport_req_wr       (),
    .m_ctrlport_req_rd       (),
    .m_ctrlport_req_addr     (),
    .m_ctrlport_req_data     (),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     (1'b0),
    .m_ctrlport_resp_status  (2'b0),
    .m_ctrlport_resp_data    (32'b0),
    .radio_clk               (radio_clk),
    .radio_time              (radio_time),
    .radio_rx_data           (radio_rx_data),
    .radio_rx_stb            (radio_rx_stb),
    .radio_rx_running        (radio_rx_running),
    .radio_tx_data           (radio_tx_data),
    .radio_tx_stb            (radio_tx_stb),
    .radio_tx_running        (radio_tx_running)
  );



  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // Read a 32-bit register at offset "addr" from shared radio registers
  task automatic read_shared(logic [19:0] addr, output logic [31:0] data);
    addr = addr + SHARED_BASE_ADDR;
    blk_ctrl.reg_read(addr, data);
  endtask : read_shared

  // Write a 32-bit register at offset "addr" in shared radio registers
  task automatic write_shared(logic [19:0] addr, logic [31:0] data);
    addr = addr + SHARED_BASE_ADDR;
    blk_ctrl.reg_write(addr, data);
  endtask : write_shared

  // Read a 32-bit register at offset "addr" from radio "radio_num"
  task automatic read_radio(int radio_num, logic [19:0] addr, output logic [31:0] data);
    addr = addr + RADIO_BASE_ADDR + (radio_num * 2**RADIO_ADDR_W);
    blk_ctrl.reg_read(addr, data);
  endtask : read_radio

  // Read a 64-bit register at offset "addr" from radio "radio_num"
  task automatic read_radio_64(int radio_num, logic [19:0] addr, output logic [63:0] data);
    addr = addr + RADIO_BASE_ADDR + (radio_num * 2**RADIO_ADDR_W);
    blk_ctrl.reg_read(addr,   data[31:0]);
    blk_ctrl.reg_read(addr+4, data[63:32]);
  endtask : read_radio_64

  // Write a 32-bit register at offset "addr" in radio "radio_num"
  task automatic write_radio(int radio_num, logic [19:0] addr, logic [31:0] data);
    addr = addr + RADIO_BASE_ADDR + (radio_num * 2**RADIO_ADDR_W);
    blk_ctrl.reg_write(addr, data);
  endtask : write_radio

  // Write a 64-bit register at offset "addr" in radio "radio_num"
  task automatic write_radio_64(int radio_num, logic [19:0] addr, logic [63:0] data);
    addr = addr + RADIO_BASE_ADDR + (radio_num * 2**RADIO_ADDR_W);
    blk_ctrl.reg_write(addr,   data[31:0]);
    blk_ctrl.reg_write(addr+4, data[63:32]);
  endtask : write_radio_64


  // Start an Rx acquisition
  task automatic start_rx (
    int        radio_num,     // Radio channel to use
    bit [63:0] num_words = 0  // Number of radio words
  );
    logic [31:0] cmd;

    if (num_words == 0) begin
      // Do a continuous acquisition
      $display("Radio %0d: Start RX, continuous receive", radio_num);
      cmd = RX_CMD_CONTINUOUS;
    end else begin
      // Do a finite acquisition (num samps and done)
      $display("Radio %0d: Start RX, receive %0d words", radio_num, num_words);
      write_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, num_words);
      cmd = RX_CMD_FINITE;
    end

    // Write command to radio
    write_radio(radio_num, REG_RX_CMD, cmd);
  endtask : start_rx


  // Start an Rx acquisition at a specific time
  task automatic start_rx_timed (
    int        radio_num,      // Radio channel to use
    bit [63:0] num_words = 0,  // Number of radio words
    bit [63:0] start_time
  );
    logic [31:0] cmd;

    if (num_words == 0) begin
      // Do a continuous acquisition
      $display("Radio %0d: Start RX, continuous receive (timed)", radio_num);
      cmd = RX_CMD_CONTINUOUS;
    end else begin
      // Do a finite acquisition (num samps and done)
      $display("Radio %0d: Start RX, receive %0d words (timed)", radio_num, num_words);
      write_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, num_words);
      cmd = RX_CMD_FINITE;
    end

    // Mark that this is a timed command
    cmd[RX_CMD_TIMED_POS] = 1'b1;

    // Set start time for command
    write_radio_64(radio_num, REG_RX_CMD_TIME_LO, start_time);

    // Write command to radio
    write_radio(radio_num, REG_RX_CMD, cmd);
  endtask : start_rx_timed


  // Send the Rx stop command to the indicated radio channel
  task automatic stop_rx(int radio_num);
    $display("Radio %0d: Stop RX", radio_num);
    write_radio(radio_num, REG_RX_CMD, RX_CMD_STOP);
  endtask : stop_rx


  // Receive num_words from the indicated radio channel and verify that it's 
  // sequential and contiguous data aligned on packet boundaries.
  task automatic check_rx(
    int radio_num,    // Radio to receive from and check
    int num_words     // Number of radio words to expect
  );
    int              sample_count;    // Counter to track number of samples generated
    bit [ITEM_W-1:0] sample_val;      // Value of the next sample
    chdr_word_t      data[$];         // Array of data for the received packet
    int              num_samples;     // Number of samples to send
    int              byte_length;     // Number of data bytes in next packet
    int              expected_length; // Expected byte length of the next packet
    int              valid_words;     // Number of valid chdr_word_t in next packet

    num_samples = num_words * NIPC;

    sample_count = 0;
    while (sample_count < num_samples) begin
      // Fetch the next packet
      blk_ctrl.recv(radio_num, data, byte_length);

      // Take the first sample as a starting count for the remaining samples
      if (sample_count == 0) begin
        sample_val = data[0][ITEM_W-1:0];
      end

      // Calculate expected length in bytes
      if (num_samples - sample_count >= SPP) begin
        // Expecting a full packet
        expected_length = SPP*ITEM_W/8;
      end else begin
        // Expecting partial packet
        expected_length = (num_samples - sample_count) * ITEM_W/8;
      end

      // Check that the length matches our expectation
      `ASSERT_ERROR(
        byte_length == expected_length,
        "Received packet didn't have expected length."
      );

      // Loop over the packet, one chdr_word_t at a time
      valid_words = int'($ceil(real'(byte_length) / ($bits(chdr_word_t)/8)));
      for (int i = 0; i < valid_words; i++) begin
        // Check each sample of the next chdr_word_t value
        for (int sub_sample = 0; sub_sample < $bits(chdr_word_t)/ITEM_W; sub_sample++) begin
          sample_t actual;
          actual = data[i][ITEM_W*sub_sample +: ITEM_W];  // Work around Vivado 2018.3 issue
          `ASSERT_ERROR(
            actual == sample_val,
            $sformatf(
              "Sample %0d (0x%X) didn't match expected value (0x%X)",
              sample_count, actual, sample_val
            )
          );
          sample_val++;
          sample_count++;

          // Check if the word is only partially full
          if (sample_count >= num_samples) break;
        end
      end
    end
  endtask : check_rx


  // Send num_words to the indicated radio for transmission at the given time.
  task automatic start_tx_timed (
    int                radio_num,       // Radio channel to transmit on
    bit   [63:0]       num_words,       // Number of radio words to transmit
    logic [63:0]       start_time = 'X, // Time at which to begin transmit
    bit   [ITEM_W-1:0] start_val  = 1,  // Initial sample value
    bit                eob        = 1   // Set EOB flag at the end
  );
    int              sample_count;    // Counter to track number of samples generated
    bit [ITEM_W-1:0] sample_val;      // Value of the next sample
    chdr_word_t      data[$];         // Array of data for the packet
    int              num_samples;     // Number of samples to send
    int              byte_length;     // Number of bytes for next packet
    chdr_word_t      chdr_word;       // Next word to send to BFM
    packet_info_t    pkt_info = 0;    // Flags/timestamp for next packet

    $display("Radio %0d: Start TX, send %0d words", radio_num, num_words);

    num_samples = num_words * NIPC;

    if (!$isunknown(start_time)) pkt_info.has_time = 1;

    sample_val   = start_val;
    sample_count = 0;
    while (sample_count < num_samples) begin
      // Calculate timestamp for this packet
      if (pkt_info.has_time) begin
        pkt_info.timestamp = start_time + sample_count;
      end

      // Clear the payload
      data = {};

      // Loop until we've built up a packet
      forever begin
        // Generate the next word
        for (int sub_sample = 0; sub_sample < $bits(chdr_word_t)/ITEM_W; sub_sample++) begin
          chdr_word[ITEM_W*sub_sample +: ITEM_W] = sample_val;
          sample_val++;
          sample_count++;
        end

        // Add next word to the queue
        data.push_back(chdr_word);

        // Send the packet if we're at a packet boundary
        if (sample_count % SPP == 0) begin
          pkt_info.eob = (sample_count == num_samples && eob) ? 1 : 0;
          byte_length = SPP * ITEM_W/8;
          blk_ctrl.send(radio_num, data, byte_length, {}, pkt_info);
          break;
        end else if (sample_count >= num_samples) begin
          pkt_info.eob = eob;
          byte_length = (sample_count % SPP) * ITEM_W/8;
          blk_ctrl.send(radio_num, data, byte_length, {}, pkt_info);
          break;
        end
      end
    end
  endtask : start_tx_timed


  // Send num_words to the indicated radio for transmission.
  task automatic start_tx (
    int              radio_num,       // Radio channel to transmit on
    bit [63:0]       num_words,       // Number of radio words to transmit
    bit [ITEM_W-1:0] start_val = 1,   // Initial sample value
    bit              eob = 1          // Set EOB flag at the end
  );
    // Passing 'X tells the underlying BFM to not insert a timestamp
    start_tx_timed(radio_num, num_words, 'X, start_val, eob);
  endtask : start_tx


  // Verify the output of a packet, expecting it at a specific time
  task automatic check_tx_timed (
    int                radio_num,        // Radio channel to transmit on
    bit   [63:0]       num_words,        // Number of radio words to expect
    logic [63:0]       start_time = 'X,  // Expected start time
    bit   [ITEM_W-1:0] start_val  = 1    // Initial sample value
  );
    int sample_val;           // Expected value of next sample

    sample_val = start_val;

    // Wait for the packet to start
    wait(radio_tx_data[radio_num*RADIO_W +: ITEM_W] == start_val);

    // Check the time
    if (!$isunknown(start_time)) begin
      `ASSERT_ERROR(
        radio_time - start_time <= NIPC*2,
        $sformatf("Packet transmitted at radio time 0x%0X but expected 0x%0X", radio_time, start_time)
      );
    end

    // Verify output one word at a time
    for (int word_count = 0; word_count < num_words; word_count++) begin
      // Wait for the next radio word to be output
      do begin
        @(posedge radio_clk);
      end while (radio_tx_stb[radio_num] == 0);

      // Check each sample of the radio word
      for (int sub_sample = 0; sub_sample < NIPC; sub_sample++) begin
        `ASSERT_ERROR(
          radio_tx_data[radio_num*RADIO_W + ITEM_W*sub_sample +: ITEM_W] == sample_val,
          "Radio output doesn't match expected value"
        );
        sample_val++;
      end
    end
  endtask : check_tx_timed


  // Verify the output of a packet
  task automatic check_tx (
    int              radio_num,        // Radio to transmit on
    bit [63:0]       num_words,        // Number of radio words to expect
    bit [ITEM_W-1:0] start_val  = 1    // Initial sample value
  );
    check_tx_timed(radio_num, num_words, 'X, start_val);
  endtask : check_tx


  // When we expect and error, this task will check that control packets were
  // received and that they have the expected values.
  task check_error (int error);
    AxisCtrlPacket ctrl_packet;
    chdr_word_t word;

    // Get error code
    blk_ctrl.get_ctrl_bfm().get_ctrl(ctrl_packet);
    word = ctrl_packet.data[0];   // Work around Vivado 2018.3 issue
    `ASSERT_ERROR(
      word                            == error &&
      ctrl_packet.op_word.op_code     == CTRL_OP_WRITE &&
      ctrl_packet.op_word.address     == TX_ERR_ADDRESS &&
      ctrl_packet.header.is_ack       == 1'b0 &&
      ctrl_packet.header.has_time     == 1'b1 &&
      ctrl_packet.header.num_data     == 1 &&
      ctrl_packet.header.dst_port     == TX_ERR_DST_PORT &&
      ctrl_packet.header.rem_dst_port == TX_ERR_REM_DST_PORT &&
      ctrl_packet.header.rem_dst_epid == TX_ERR_REM_DST_EPID,
      "Unexpected error code response");

    // Send acknowledgment
    ctrl_packet.header = 0;
    ctrl_packet.header.is_ack = 1;
    blk_ctrl.get_ctrl_bfm().put_ctrl(ctrl_packet);
  endtask : check_error



  //---------------------------------------------------------------------------
  // Test Procedures
  //---------------------------------------------------------------------------

  task automatic test_block_info();
    test.start_test("Verify Block Info", 2us);

    // Get static block info and validate it
    `ASSERT_ERROR(blk_ctrl.get_noc_id() == NOC_ID, "Incorrect noc_id Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_i() == NUM_PORTS, "Incorrect num_data_i Value");
    `ASSERT_ERROR(blk_ctrl.get_num_data_o() == NUM_PORTS, "Incorrect num_data_o Value");
    `ASSERT_ERROR(blk_ctrl.get_mtu() == MTU, "Incorrect mtu Value");

    test.end_test();
  endtask : test_block_info



  task automatic test_shared_registers();
    logic [31:0] val;
    logic [63:0] time1;
    logic [63:0] time2;
    test.start_test("Shared Registers", 10us);

    // Compatibility number
    read_shared(REG_COMPAT_NUM, val);
    `ASSERT_ERROR(
      val == {
        rfnoc_block_radio_i.compat_major,
        rfnoc_block_radio_i.compat_minor
      },
      "REG_COMPAT_NUM didn't read correctly"
    );
    read_shared(REG_TIME_LO, time1[31:0]);
    read_shared(REG_TIME_HI, time1[63:32]);
    read_shared(REG_TIME_LO, time2[31:0]);
    read_shared(REG_TIME_HI, time2[63:32]);
    `ASSERT_ERROR(
      time2 > time1,
      "Time did not increment in REG_TIME_HI and REG_TIME_LO"
    );
    test.end_test();
  endtask : test_shared_registers



  task automatic test_general_registers(int radio_num);
    logic [31:0] val;
    test.start_test("General Registers", 10us);

    // Test loopback enable register (read/write)
    read_radio(radio_num, REG_LOOPBACK_EN, val);
    `ASSERT_ERROR(val == 0, "Initial value of REG_LOOPBACK_EN is incorrect");
    write_radio(radio_num, REG_LOOPBACK_EN, 32'hFFFFFFFF);
    read_radio(radio_num, REG_LOOPBACK_EN, val);
    `ASSERT_ERROR(val == 1, "REG_LOOPBACK_EN didn't update correctly");
    write_radio(radio_num, REG_LOOPBACK_EN, 0);

    // Read ITEM_W and NIPC (read only)
    read_radio(radio_num, REG_RADIO_WIDTH, val);
    `ASSERT_ERROR(val[15:0] == NIPC, "Value of NIPC register is incorrect");
    `ASSERT_ERROR(val[31:16] == ITEM_W, "Value of ITEM_W register is incorrect");

    test.end_test();
  endtask : test_general_registers



  task test_rx_registers(int radio_num);
    logic [63:0] val, temp, expected;
    localparam int num_words_len = RX_CMD_NUM_WORDS_LEN;

    test.start_test("Rx Registers", 50us);

    // REG_RX_CMD_STATUS (read only)
    expected = CMD_FIFO_SPACE_MAX;
    read_radio(radio_num, REG_RX_STATUS, val);
    `ASSERT_ERROR(val == expected, "REG_RX_STATUS not initially CMD_FIFO_SPACE_MAX");

    // REG_RX_CMD (read/write). Test a bogus timed stop command just to check 
    // read/write of the register.
    expected = 0;
    expected[RX_CMD_POS +: RX_CMD_LEN] = RX_CMD_STOP;
    expected[RX_CMD_TIMED_POS] = 1'b1;
    write_radio(radio_num, REG_RX_CMD, expected);
    read_radio(radio_num, REG_RX_CMD, val);
    `ASSERT_ERROR(val == expected, "REG_RX_CMD didn't update correctly");

    // REG_RX_CMD_NUM_WORDS (read/write)
    read_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, val);
    `ASSERT_ERROR(val == 0, "REG_RX_CMD_NUM_WORDS not initially 0");
    expected = 64'hFEDCBA9876543210;
    write_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, expected);
    read_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, val);
    `ASSERT_ERROR(
      val == expected[num_words_len-1:0],
     "REG_RX_CMD_NUM_WORDS didn't update correctly"
    );

    // REG_RX_CMD_TIME (read/write)
    read_radio_64(radio_num, REG_RX_CMD_TIME_LO, val);
    `ASSERT_ERROR(val == 0, "REG_RX_CMD_TIME not initially 0");
    expected = 64'hBEADFEED0123F1FE;
    write_radio_64(radio_num, REG_RX_CMD_TIME_LO, expected);
    read_radio_64(radio_num, REG_RX_CMD_TIME_LO, val);
    `ASSERT_ERROR(val == expected, "REG_RX_CMD_TIME didn't update correctly");

    // REG_RX_MAX_WORDS_PER_PKT (read/write)
    read_radio(radio_num, REG_RX_MAX_WORDS_PER_PKT, val);
    `ASSERT_ERROR(val == 64, "REG_RX_MAX_WORDS_PER_PKT not initially 64");
    expected = 32'hABBEC001;
    write_radio(radio_num, REG_RX_MAX_WORDS_PER_PKT, expected);
    read_radio(radio_num, REG_RX_MAX_WORDS_PER_PKT, val);
    `ASSERT_ERROR(val == expected, "REG_RX_MAX_WORDS_PER_PKT didn't update correctly");

    // REG_RX_ERR_PORT (read/write)
    read_radio(radio_num, REG_RX_ERR_PORT, val);
    `ASSERT_ERROR(val == 0, "REG_RX_ERR_PORT not initially 0");
    expected = $urandom() & 32'h000001FF;
    write_radio(radio_num, REG_RX_ERR_PORT, expected);
    read_radio(radio_num, REG_RX_ERR_PORT, val);
    `ASSERT_ERROR(val == expected, "REG_RX_ERR_PORT didn't update correctly");

    // REG_RX_ERR_REM_PORT (read/write)
    read_radio(radio_num, REG_RX_ERR_REM_PORT, val);
    `ASSERT_ERROR(val == 0, "REG_RX_ERR_REM_PORT not initially 0");
    expected = $urandom() & 32'h000001FF;
    write_radio(radio_num, REG_RX_ERR_REM_PORT, expected);
    read_radio(radio_num, REG_RX_ERR_REM_PORT, val);
    `ASSERT_ERROR(val == expected, "REG_RX_ERR_REM_PORT didn't update correctly");

    // REG_RX_ERR_REM_EPID (read/write)
    read_radio(radio_num, REG_RX_ERR_REM_EPID, val);
    `ASSERT_ERROR(val == 0, "REG_RX_ERR_REM_EPID not initially 0");
    expected = $urandom() & 32'h0000FFFF;
    write_radio(radio_num, REG_RX_ERR_REM_EPID, expected);
    read_radio(radio_num, REG_RX_ERR_REM_EPID, val);
    `ASSERT_ERROR(val == expected, "REG_RX_ERR_REM_EPID didn't update correctly");

    // REG_RX_ERR_ADDR (read/write)
    read_radio(radio_num, REG_RX_ERR_ADDR, val);
    `ASSERT_ERROR(val == 0, "REG_RX_ERR_ADDR not initially 0");
    expected = $urandom() & 32'h000FFFFF;
    write_radio(radio_num, REG_RX_ERR_ADDR, expected);
    read_radio(radio_num, REG_RX_ERR_ADDR, val);
    `ASSERT_ERROR(val == expected, "REG_RX_ERR_ADDR didn't update correctly");

    // REG_RX_DATA (read-only)
    temp = radio_tx_data[RADIO_W*radio_num +: RADIO_W];
    read_radio(radio_num, REG_RX_DATA, val);
    `ASSERT_ERROR(
      radio_rx_data[RADIO_W*radio_num +: RADIO_W] >= val && val >= temp, 
      "REG_RX_DATA wasn't in the expected range");
    read_radio(radio_num, REG_RX_DATA, temp);
    `ASSERT_ERROR(temp != val, "REG_RX_DATA didn't update");

    test.end_test();
  endtask : test_rx_registers



  task automatic test_tx_registers(int radio_num);
    logic [31:0] val, expected;

    test.start_test("Tx Registers", 50us);

    // REG_TX_IDLE_VALUE (read/write)
    read_radio(radio_num, REG_TX_IDLE_VALUE, val);
    `ASSERT_ERROR(val == 0, "REG_TX_IDLE_VALUE not initially 0");
    expected = $urandom() & {ITEM_W{1'b1}};
    write_radio(radio_num, REG_TX_IDLE_VALUE, expected);
    read_radio(radio_num, REG_TX_IDLE_VALUE, val);
    `ASSERT_ERROR(val == expected, "REG_TX_IDLE_VALUE didn't update correctly");

    // REG_TX_ERROR_POLICY (read/write)
    read_radio(radio_num, REG_TX_ERROR_POLICY, val);
    expected = TX_ERR_POLICY_PACKET;
    `ASSERT_ERROR(val == expected, "REG_TX_ERROR_POLICY not initially 'PACKET'");
    expected = TX_ERR_POLICY_BURST;
    write_radio(radio_num, REG_TX_ERROR_POLICY, expected);
    read_radio(radio_num, REG_TX_ERROR_POLICY, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERROR_POLICY didn't update to 'BURST'");
    expected = TX_ERR_POLICY_PACKET;
    write_radio(radio_num, REG_TX_ERROR_POLICY, 32'h03); // Try to set both bits!
    read_radio(radio_num, REG_TX_ERROR_POLICY, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERROR_POLICY didn't revert to 'PACKET'");

    // REG_TX_ERR_PORT (read/write)
    read_radio(radio_num, REG_TX_ERR_PORT, val);
    `ASSERT_ERROR(val == 0, "REG_TX_ERR_PORT not initially 0");
    expected = $urandom() & 32'h000001FF;
    write_radio(radio_num, REG_TX_ERR_PORT, expected);
    read_radio(radio_num, REG_TX_ERR_PORT, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERR_PORT didn't update correctly");

    // REG_TX_ERR_REM_PORT (read/write)
    read_radio(radio_num, REG_TX_ERR_REM_PORT, val);
    `ASSERT_ERROR(val == 0, "REG_TX_ERR_REM_PORT not initially 0");
    expected = $urandom() & 32'h000001FF;
    write_radio(radio_num, REG_TX_ERR_REM_PORT, expected);
    read_radio(radio_num, REG_TX_ERR_REM_PORT, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERR_REM_PORT didn't update correctly");

    // REG_TX_ERR_REM_EPID (read/write)
    read_radio(radio_num, REG_TX_ERR_REM_EPID, val);
    `ASSERT_ERROR(val == 0, "REG_TX_ERR_REM_EPID not initially 0");
    expected = $urandom() & 32'h0000FFFF;
    write_radio(radio_num, REG_TX_ERR_REM_EPID, expected);
    read_radio(radio_num, REG_TX_ERR_REM_EPID, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERR_REM_EPID didn't update correctly");

    // REG_TX_ERR_ADDR (read/write)
    read_radio(radio_num, REG_TX_ERR_ADDR, val);
    `ASSERT_ERROR(val == 0, "REG_TX_ERR_ADDR not initially 0");
    expected = $urandom() & 32'h000FFFFF;
    write_radio(radio_num, REG_TX_ERR_ADDR, expected);
    read_radio(radio_num, REG_TX_ERR_ADDR, val);
    `ASSERT_ERROR(val == expected, "REG_TX_ERR_ADDR didn't update correctly");

    test.end_test();
  endtask : test_tx_registers



  task automatic test_rx(int radio_num);

    //---------------------
    // Finite Acquisitions
    //---------------------

    test.start_test("Rx (finite)", 50us);

    // Set packet length
    write_radio(radio_num, REG_RX_MAX_WORDS_PER_PKT, WPP);

    // Grab and verify a partial packet
    start_rx(radio_num, WPP/2);
    check_rx(radio_num, WPP/2);

    // Grab a minimally-sized packet
    start_rx(radio_num, 1);
    check_rx(radio_num, 1);

    // Grab and verify several packets
    start_rx(radio_num, WPP*15/2);
    check_rx(radio_num, WPP*15/2);

    // Wait long enough to receive another packet and then make sure we didn't 
    // receive anything. That is, make sure Rx actually stopped.
    #MAX_PKT_WAIT;
    `ASSERT_ERROR(
      blk_ctrl.num_received(radio_num) == 0,
      "Received more packets than expected"
    );

    test.end_test();


    //-------------------------
    // Continuous Acquisitions
    //-------------------------

    test.start_test("Rx (continuous)", 100us);

    start_rx(radio_num);

    // Grab and verify several packets
    check_rx(radio_num, WPP*7);
    stop_rx(radio_num);

    // Grab and discard any remaining packets
    do begin
      while (blk_ctrl.num_received(radio_num) != 0) begin
        ChdrPacket #(CHDR_W) chdr_packet;
        blk_ctrl.get_chdr(radio_num, chdr_packet);
      end
      #MAX_PKT_WAIT;
    end while (blk_ctrl.num_received(radio_num) != 0);

    test.end_test();


    //--------------------------
    // Finite Timed Acquisition
    //--------------------------

    begin
      ChdrPacket #(CHDR_W) chdr_packet;
      chdr_word_t expected_time;

      test.start_test("Rx (finite, timed)", 100us);

      // Send Rx command with time in the future
      expected_time = radio_time + 2000;
      start_rx_timed(radio_num, WPP, expected_time);

      // Take a peak at the timestamp in the received packet to check it
      blk_ctrl.peek_chdr(radio_num, chdr_packet);
      `ASSERT_ERROR(
        chdr_packet.timestamp == expected_time,
        "Received packet didn't have expected timestamp"
      );

      // Verify the packet data
      check_rx(radio_num, WPP);
      test.end_test();
    end


    //------------------------------
    // Continuous Timed Acquisition
    //------------------------------

    begin
      ChdrPacket #(CHDR_W) chdr_packet;
      chdr_word_t expected_time;

      test.start_test("Rx (continuous, timed)", 100us);

      // Send Rx command with time in the future
      expected_time = radio_time + 2000;
      start_rx_timed(radio_num, 0, expected_time);

      // Take a peak at the timestamp in the received packet to check it
      blk_ctrl.peek_chdr(radio_num, chdr_packet);
      `ASSERT_ERROR(
        chdr_packet.timestamp == expected_time,
        "Received packet didn't have expected timestamp"
      );

      // Verify a few packets
      check_rx(radio_num, WPP*3);
      stop_rx(radio_num);

      // Grab and discard any remaining packets
      do begin
        while (blk_ctrl.num_received(radio_num) != 0) begin
          ChdrPacket #(CHDR_W) chdr_packet;
          blk_ctrl.get_chdr(radio_num, chdr_packet);
        end
        #(MAX_PKT_WAIT);
      end while (blk_ctrl.num_received(radio_num) != 0);

      test.end_test();
    end


    //-------------
    // Rx Overflow
    //-------------
    begin
      logic [31:0] val;

      test.start_test("Rx (now, overflow)", 200us);

      // Configure the error reporting registers
      write_radio(radio_num, REG_RX_ERR_PORT, TX_ERR_DST_PORT);
      write_radio(radio_num, REG_RX_ERR_REM_PORT, TX_ERR_REM_DST_PORT);
      write_radio(radio_num, REG_RX_ERR_REM_EPID, TX_ERR_REM_DST_EPID);
      write_radio(radio_num, REG_RX_ERR_ADDR, TX_ERR_ADDRESS);

      // Stall the BFM to force a backup of data
      blk_ctrl.set_slave_stall_prob(radio_num, 100);

      // Acquire continuously until we get an error
      start_rx(radio_num);

      // Check that we're acquiring
      read_radio(radio_num, REG_RX_STATUS, val);
      `ASSERT_ERROR(
        val[CMD_FIFO_SPACE_POS +: CMD_FIFO_SPACE_LEN] != CMD_FIFO_SPACE_MAX,
        "Rx radio reports that it is not busy"
      );

      // Verify that we receive an error
      check_error(ERR_RX_OVERRUN);

      // Restore the BFM stall probability
      blk_ctrl.set_slave_stall_prob(radio_num, STALL_PROB);

      // Verify that Rx stopped
      read_radio(radio_num, REG_RX_STATUS, val);
      `ASSERT_ERROR(
        val[CMD_FIFO_SPACE_POS +: CMD_FIFO_SPACE_LEN] == CMD_FIFO_SPACE_MAX,
        "Rx radio reports that it is still busy after overflow"
      );

      // Discard any packets we received. Rx should eventually stop 
      // automatically after an overflow.
      do begin
        while (blk_ctrl.num_received(radio_num) != 0) begin
          ChdrPacket #(CHDR_W) chdr_packet;
          blk_ctrl.get_chdr(radio_num, chdr_packet);
        end
        #(MAX_PKT_WAIT);
      end while (blk_ctrl.num_received(radio_num) != 0);

     test.end_test();
    end


    //--------------
    // Late Command
    //--------------

    test.start_test("Rx (timed, late)", 100us);

    start_rx_timed(radio_num, WPP, radio_time);
    check_error(ERR_RX_LATE_CMD);

    // Late command should be ignored. Make sure we didn't receive any packets.
    begin
      ChdrPacket #(CHDR_W) chdr_packet;
      #(MAX_PKT_WAIT);
      `ASSERT_ERROR(
        blk_ctrl.num_received(radio_num) == 0,
        "Packets received for late Rx command"
      );

      // Discard any remaining packets
      while (blk_ctrl.num_received(radio_num)) blk_ctrl.get_chdr(radio_num, chdr_packet);
    end

    test.end_test();


    //---------------
    // Command Queue
    //---------------

    test.start_test("Rx (queue multiple commands)");

    begin
      logic [31:0] expected, val;

      // Send one continuous command and verify the queue fullness
      start_rx(radio_num);
      expected = CMD_FIFO_SPACE_MAX-1;
      read_radio(radio_num, REG_RX_STATUS, val);
      `ASSERT_ERROR(
        val[CMD_FIFO_SPACE_POS+:CMD_FIFO_SPACE_LEN] == expected, 
        "CMD_FIFO_SPACE did not decrement"
      );

      // Fill the command FIFO, going one over
      for (int i = 0; i < CMD_FIFO_SPACE_MAX; i++) begin
        start_rx(radio_num, WPP);
      end
      expected = 0;
      read_radio(radio_num, REG_RX_STATUS, val);
      `ASSERT_ERROR(
        val[CMD_FIFO_SPACE_POS+:CMD_FIFO_SPACE_LEN] == expected, 
        "CMD_FIFO_SPACE did not reach 0"
      );

      // Issue stop command and verify that the FIFO empties
      stop_rx(radio_num);
      expected = CMD_FIFO_SPACE_MAX;
      read_radio(radio_num, REG_RX_STATUS, val);
      `ASSERT_ERROR(
        val[CMD_FIFO_SPACE_POS+:CMD_FIFO_SPACE_LEN] == expected, 
        "CMD_FIFO_SPACE did not return to max"
      );

      // Grab and discard any remaining packets
      do begin
        while (blk_ctrl.num_received(radio_num) != 0) begin
          ChdrPacket #(CHDR_W) chdr_packet;
          blk_ctrl.get_chdr(radio_num, chdr_packet);
        end
        #MAX_PKT_WAIT;
      end while (blk_ctrl.num_received(radio_num) != 0);

      // Queue several long commands back-to-back and make sure they all 
      // complete. The lengths are unique to ensure we execute the right 
      // commands in the expected order.
      for (int i = 0; i < 3; i++) start_rx(radio_num, WPP*20+i);
      for (int i = 0; i < 3; i++) check_rx(radio_num, WPP*20+i);

      // Make sure we don't get any more data
      do begin
        while (blk_ctrl.num_received(radio_num) != 0) begin
          `ASSERT_ERROR(0, "Received unexpected packets");
        end
        #MAX_PKT_WAIT;
      end while (blk_ctrl.num_received(radio_num) != 0);
    end

    test.end_test();

  endtask : test_rx



  task automatic test_tx(int radio_num);
    logic [RADIO_W-1:0] radio_data;
    enum { WAIT_FOR_EOP, WAIT_FOR_EOB } policy;

    //-------
    // Setup
    //-------

    test.start_test("Tx Init", 50us);

    // Configure the error reporting registers
    write_radio(radio_num, REG_TX_ERR_PORT, TX_ERR_DST_PORT);
    write_radio(radio_num, REG_TX_ERR_REM_PORT, TX_ERR_REM_DST_PORT);
    write_radio(radio_num, REG_TX_ERR_REM_EPID, TX_ERR_REM_DST_EPID);
    write_radio(radio_num, REG_TX_ERR_ADDR, TX_ERR_ADDRESS);

    test.end_test();


    //---------------
    // Test Tx (now)
    //---------------

    test.start_test("Tx (now)", 50us);

    // Grab and verify a partial packet
    start_tx(radio_num, WPP*3/4);
    check_tx(radio_num, WPP*3/4);
    check_error(ERR_TX_EOB_ACK);

    // Grab and verify multiple packets
    start_tx(radio_num, WPP*3/2);
    check_tx(radio_num, WPP*3/2);
    check_error(ERR_TX_EOB_ACK);

    // Test a minimally-sized packet
    start_tx(radio_num, 1);
    check_tx(radio_num, 1);
    check_error(ERR_TX_EOB_ACK);

    test.end_test();


    //---------------------
    // Test Tx (underflow)
    //---------------------

    test.start_test("Tx (now, underflow)", 50us);

    // Send some bursts without EOB
    start_tx(radio_num, WPP*3/4, 1, 0);  // Skip EOB
    check_tx(radio_num, WPP*3/4);
    check_error(ERR_TX_UNDERRUN);

    start_tx(radio_num, WPP*2, 1, 0);  // Skip EOB
    check_tx(radio_num, WPP*2);
    check_error(ERR_TX_UNDERRUN);

    test.end_test();


    //-----------------
    // Test Tx (timed)
    //-----------------

    test.start_test("Tx (timed)", 50us);

    // Grab and verify a partial packet
    start_tx_timed(radio_num, WPP*3/4, radio_time + 200);
    check_tx_timed(radio_num, WPP*3/4, radio_time + 200);
    check_error(ERR_TX_EOB_ACK);

    // Grab and verify whole packets
    start_tx_timed(radio_num, WPP*2, radio_time + 200);
    check_tx_timed(radio_num, WPP*2, radio_time + 200);
    check_error(ERR_TX_EOB_ACK);

    test.end_test();


    //-----------------
    // Test Tx (timed, underflow)
    //-----------------

    test.start_test("Tx (timed, underflow)", 50us);

    // Send some bursts without EOB
    start_tx_timed(radio_num, WPP*3/4, radio_time + 200, 1, 0);
    check_tx_timed(radio_num, WPP*3/4, radio_time + 200);
    check_error(ERR_TX_UNDERRUN);

    start_tx_timed(radio_num, WPP*2, radio_time + 200, 1, 0);
    check_tx_timed(radio_num, WPP*2, radio_time + 200);
    check_error(ERR_TX_UNDERRUN);

    test.end_test();


    //---------------------------
    // Test Tx (timed, late)
    //---------------------------

    test.start_test("Tx (timed, late)", 50us);

    // Test each error policy
    policy = policy.first();
    do begin
      // Set the policy
      if (policy == WAIT_FOR_EOP) begin
        write_radio(radio_num, REG_TX_ERROR_POLICY, TX_ERR_POLICY_PACKET);
      end else if (policy == WAIT_FOR_EOB) begin
        write_radio(radio_num, REG_TX_ERROR_POLICY, TX_ERR_POLICY_BURST);
      end

// Commenting out the fork code for now due to Vivado 2018.3 bug.
//      radio_data = radio_tx_data[radio_num];
//      fork : tx_fork
        // In this branch of the fork, we send the packets
        repeat (2) begin
          // Send late packets with random start value
          start_tx_timed(radio_num, WPP*3, 0, $urandom());

          if (policy == WAIT_FOR_EOP) begin
            // We should get three errors, one for each packet
            repeat (3) check_error(ERR_TX_LATE_DATA);
          end else if (policy == WAIT_FOR_EOB) begin
            // We should get one error for the entire burst
            check_error(ERR_TX_LATE_DATA);
          end
        end

//        // The packets sent in the above branch of the fork should be 
//        // dropped. In this branch of the fork we make sure that the Tx 
//        // output doesn't change.
//        begin
//          forever begin
//            @(posedge radio_clk)
//            `ASSERT_ERROR(
//              radio_data === radio_tx_data[radio_num], 
//              "Radio Tx output changed when late Tx packet should have been ignored"
//            );
//          end
//        end
//      join_any
//
//      // Stop checking the output
//      disable tx_fork;

      policy = policy.next();
    end while (policy != policy.first());

    // Make sure good transmissions can go through now.
    start_tx_timed(radio_num, WPP, radio_time + 200);
    check_tx_timed(radio_num, WPP, radio_time + 200);
    check_error(ERR_TX_EOB_ACK);

    test.end_test();

  endtask : test_tx



  // Test internal loopback and idle value
  task automatic test_loopback_and_idle(int radio_num);
    int              byte_length;
    chdr_word_t      data[$];
    bit [ITEM_W-1:0] idle;

    //----------------------------
    // Use IDLE value to loopback
    //----------------------------

    test.start_test("Idle Loopback", 50us);

    // Turn on loopback
    write_radio(radio_num, REG_LOOPBACK_EN, 1);

    // This test ensures we get the Tx output on Rx and not the TB's simulated 
    // radio data. It also tests updating the idle value. Run the test twice to 
    // make sure the IDLE value updates.
    repeat (2) begin
      // Set idle value
      idle = $urandom();
      write_radio(radio_num, REG_TX_IDLE_VALUE, idle);

      // Grab a radio word and check that it equals the IDLE value
      write_radio_64(radio_num, REG_RX_CMD_NUM_WORDS_LO, 1);
      write_radio(radio_num, REG_RX_CMD, RX_CMD_FINITE);
      blk_ctrl.recv(radio_num, data, byte_length);

      // Check the length
      `ASSERT_ERROR(byte_length == RADIO_W/8, "Didn't receive expected length");

      // Check the payload
      foreach (data[i]) begin
        chdr_word_t word;
        word = data[i];   // Work around Vivado 2018.3 issue
        `ASSERT_ERROR(
          word == {$bits(chdr_word_t)/ITEM_W{idle}},
          "Loopback data didn't match expected"
        );
      end
    end

    test.end_test();


    //---------------------
    // Loopback Tx packets
    //---------------------

    test.start_test("Tx Loopback", 50us);

    // This test ensures that loopback isn't reordering words or anything else
    // unexpected.

    // Configure the Tx error reporting registers
    write_radio(radio_num, REG_TX_ERR_PORT,     TX_ERR_DST_PORT);
    write_radio(radio_num, REG_TX_ERR_REM_PORT, TX_ERR_REM_DST_PORT);
    write_radio(radio_num, REG_TX_ERR_REM_EPID, TX_ERR_REM_DST_EPID);
    write_radio(radio_num, REG_TX_ERR_ADDR,     TX_ERR_ADDRESS);

    // Set packet length
    write_radio(radio_num, REG_RX_MAX_WORDS_PER_PKT, WPP);

    // Loopback a few packets, back-to-back. This code has a race condition 
    // since there's a delay between when we start Tx and when Rx starts, due 
    // to how long it takes to write the Rx registers. Therefore, we transmit a 
    // lot more packets than we receive to ensure we're still transmitting by 
    // the time we receive.
    start_tx(radio_num, WPP*16);
    start_rx(radio_num, WPP*2);

    // Check the results
    check_rx(radio_num, WPP*2);
    check_error(ERR_TX_EOB_ACK);

    // Turn off loopback
    write_radio(radio_num, REG_LOOPBACK_EN, 0);

    test.end_test();
  endtask : test_loopback_and_idle;



  //---------------------------------------------------------------------------
  // Test Process
  //---------------------------------------------------------------------------

  timeout_t timeout;

  initial begin : main
    string tb_name;

    //-------------------------------------------------------------------------
    // Initialization
    //-------------------------------------------------------------------------

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf(
      "rfnoc_block_radio_tb\nCHDR_W = %0D, ITEM_W = %0D, NIPC = %0D, NUM_PORTS = %0D, STALL_PROB = %0D, STB_PROB = %0D, TEST_REGS = %0D",
      CHDR_W, ITEM_W, NIPC, NUM_PORTS, STALL_PROB, STB_PROB, TEST_REGS
    );

    test.start_tb(tb_name);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    rfnoc_chdr_clk_gen.start();
    rfnoc_ctrl_clk_gen.start();
    radio_clk_gen.start();

    // Start the BFMs running
    blk_ctrl.run();


    //-------------------------------------------------------------------------
    // Reset
    //-------------------------------------------------------------------------

    test.start_test("Flush block then reset it", 10us);
    blk_ctrl.flush_and_reset();
    test.end_test();


    //-------------------------------------------------------------------------
    // Test Sequences
    //-------------------------------------------------------------------------

    // Run register tests first, since they check that initial values are 
    // correct.

    test_block_info();
    if (TEST_REGS) test_shared_registers();

    for (int radio_num = 0; radio_num < NUM_PORTS; radio_num++) begin
      $display("************************************************************");
      $display("Testing Radio Channel %0d", radio_num);
      $display("************************************************************");
      if (TEST_REGS) begin
        test_general_registers(radio_num);
        test_rx_registers(radio_num);
        test_tx_registers(radio_num);
      end
      test_rx(radio_num);
      test_tx(radio_num);
      test_loopback_and_idle(radio_num);
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
    radio_clk_gen.kill();

  end : main

endmodule : rfnoc_block_radio_tb
