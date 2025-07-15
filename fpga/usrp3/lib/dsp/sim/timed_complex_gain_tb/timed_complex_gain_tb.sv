//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: timed_complex_gain_tb
//
// Description: This is the testbench for timed_complex_gain module.
//

`default_nettype none

module timed_complex_gain_tb #(
  parameter int ITEM_W = 32,
  parameter int NIPC = 2,
  parameter int COEFF_W = 20,
  parameter int COEFF_FRAC_BITS = 16,
  parameter int STB_STALL_PROB = 20
);

  // Include testbench utilities and time declarations
  `include "test_exec.svh"
  import PkgTestExec::*;

  import PkgAxiStreamBfm::*;
  import PkgRandom::*;
  import PkgComplex::*;
  import PkgMath::*;

  import ctrlport_bfm_pkg::*;
  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;

  import strobe_data_bfm_pkg::*;

  `include "../../timed_complex_gain_regs.svh"

  //---------------------------------------------------------------------------
  // Local parameters
  //---------------------------------------------------------------------------

  localparam int DUT_BASE_ADDR = '0;
  localparam int QUEUE_DEPTH = 5;
  localparam int TS_SHIFT_REG_DEPTH = 1;
  localparam int STROBE_CLK_PERIOD = 10.0;  // Clock period for strobe clock in ns
  localparam int NUM_CHANNELS = 1;  // Number of channels to test
  localparam int COEFF_UPDATE_LATENCY = 5;  // Latency from coeff update

  localparam int MAX_COMPONENT_VALUE = 2**(ITEM_W/2-1)-1;
  localparam int MIN_COMPONENT_VALUE = -2**(ITEM_W/2-1);

  localparam int NUM_TESTS = 100;  // Number of tests to run
  localparam int MAX_QUEUE_ITEMS = 2 ** QUEUE_DEPTH;

  localparam int VERBOSE = 0;  // Verbosity level for debug prints

  //---------------------------------------------------------------------------
  // Local Type Definitions
  //---------------------------------------------------------------------------
  typedef logic        [  ITEM_W-1:0] item_logic_t;
  typedef bit   signed [ITEM_W/2-1:0] complex_component_t;
  typedef logic signed [ITEM_W/2-1:0] component_t;
  typedef strobe_data_block#(
    .NSPC  (NIPC),
    .SAMP_W(ITEM_W)
  ) strobe_block_t;
  typedef strobe_block_t strobe_block_queue_t[$];
  typedef mailbox#(strobe_block_t) strobe_block_mailbox_t;
  typedef struct packed signed {
    logic [COEFF_W-1:0] re;
    logic [COEFF_W-1:0] im;
  } complex_gain_coeffs_t;
  typedef logic [CHDR_TIMESTAMP_W-1:0] timestamp_t;

  //---------------------------------------------------------------------------
  // Local Testbench Signals
  //---------------------------------------------------------------------------
  // Declare signals for DUT connections
  logic                                     strobe_clk;
  logic                                     strobe_rst;
  logic [NUM_CHANNELS-1:0][ITEM_W*NIPC-1:0] strobe_data;
  logic                                     strobe_data_stb;
  logic [ CHDR_TIMESTAMP_W-1:0]             timebase;
  logic [NUM_CHANNELS-1:0][ITEM_W*NIPC-1:0] dut_data_out;
  logic                                     dut_data_out_stb;

  logic                   [            1:0] temp_resp_status;

  ctrlport_if dut_ctrlport_if (
    .clk(strobe_clk),
    .rst(strobe_rst)
  );

  strobe_data_if #(
    .NSPC  (NIPC),
    .SAMP_W(ITEM_W)
  ) dut_in_strobe_if (
    .strobe_clk(strobe_clk),
    .strobe_rst(strobe_rst)
  );

  strobe_data_if #(
    .NSPC  (NIPC),
    .SAMP_W(ITEM_W)
  ) dut_out_strobe_if (
    .strobe_clk(strobe_clk),
    .strobe_rst(strobe_rst)
  );

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------
  ctrlport_bfm dut_ctrlport_bfm = new(dut_ctrlport_if);
  strobe_data_bfm #(
    .NSPC  (NIPC),
    .SAMP_W(ITEM_W)
  ) dut_strobe_bfm = new(
    .master(dut_in_strobe_if), .slave(dut_out_strobe_if)
  );

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------
  sim_clock_gen #(
    .PERIOD(STROBE_CLK_PERIOD),
    .AUTOSTART(0)
  ) strobe_clk_gen (
    .clk(strobe_clk),
    .rst(strobe_rst)
  );

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  // Instantiate the DUT
  timed_complex_gain #(
    .ITEM_W             (ITEM_W),
    .NIPC               (NIPC),
    .BASE_ADDR          (DUT_BASE_ADDR),
    .COEFFICIENT_W      (COEFF_W),
    .COEFF_FRAC_BITS    (COEFF_FRAC_BITS),
    .QUEUE_DEPTH        (QUEUE_DEPTH),
    .TS_SHIFT_REG_DEPTH (TS_SHIFT_REG_DEPTH)
  ) dut (
    .clk                    (strobe_clk),
    .rst                    (strobe_rst),
    .s_ctrlport_req_wr      (dut_ctrlport_if.req.wr),
    .s_ctrlport_req_rd      (dut_ctrlport_if.req.rd),
    .s_ctrlport_req_addr    (dut_ctrlport_if.req.addr),
    .s_ctrlport_req_data    (dut_ctrlport_if.req.data),
    .s_ctrlport_req_has_time(dut_ctrlport_if.req.has_time),
    .s_ctrlport_req_time    (dut_ctrlport_if.req.timestamp),
    .s_ctrlport_resp_ack    (dut_ctrlport_if.resp.ack),
    .s_ctrlport_resp_status ({dut_ctrlport_if.resp.status}),
    .s_ctrlport_resp_data   (dut_ctrlport_if.resp.data),
    .data_in                (dut_in_strobe_if.data),
    .data_out               (dut_out_strobe_if.data),
    .data_in_stb            (dut_in_strobe_if.strobe),
    .data_out_stb           (dut_out_strobe_if.strobe),
    .timestamp              (dut_in_strobe_if.timebase)
  );

  //---------------------------------------------------------------------------
  // Testbench Utilities
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Generate an strobe block with random payload.
  //---------------------------------------------------------------------------
  //  - block_size: The size of the block in data words of size ITEM_W * NIPC
  //---------------------------------------------------------------------------
  function automatic strobe_block_t generate_random_strobe_block(int block_size,
                                                                 timestamp_t timestamp = '0);
    strobe_block_t data_block = new();

    data_block.timestamp = timestamp;

    repeat (block_size) begin
      // Generate random data for each item in the data block
      logic [ITEM_W*NIPC-1:0] word = Rand#(ITEM_W * NIPC)::rand_logic();
      data_block.data.push_back(word);
    end
    return data_block;
  endfunction

  //---------------------------------------------------------------------------
  // Generate multiple AXI Stream data blocks with random payloads.
  //---------------------------------------------------------------------------
  //  - num_blocks: The number of blocks to generate
  //  - block_size: The size of each block in data words of size ITEM_W * NIPC
  //  - timestamps: Optional array of timestamps for each block, indicating when
  //                the block should be processed.
  //---------------------------------------------------------------------------
  function automatic strobe_block_queue_t generate_test_data(
      int block_size, timestamp_t timestamps[] = {});
    strobe_block_queue_t data_blocks;

    foreach (timestamps[i]) begin
      data_blocks.push_back(generate_random_strobe_block(block_size, timestamps[i]));
    end
    return data_blocks;
  endfunction

  //---------------------------------------------------------------------------
  // Convert an IQ sample item to complex type
  //---------------------------------------------------------------------------
  //  - item: The input item to convert
  //---------------------------------------------------------------------------
  function automatic complex_t convert_item_to_complex(item_logic_t item);
    complex_t c_item;

    c_item.re = $itor(component_t'(item[ITEM_W/2+:ITEM_W/2]));
    c_item.im = $itor(component_t'(item[0+:ITEM_W/2]));

    return c_item;
  endfunction

  //---------------------------------------------------------------------------
  // Convert an complex type to IQ sample item
  //---------------------------------------------------------------------------
  //  - c_item: The complex item to convert
  //---------------------------------------------------------------------------
  function automatic item_logic_t convert_complex_to_item(complex_t c_item);

    int out_rounded_real = $rtoi(round(c_item.re, ROUND_HALF_UP));
    int out_rounded_imag = $rtoi(round(c_item.im, ROUND_HALF_UP));
    int out_clipped_real = (out_rounded_real > MAX_COMPONENT_VALUE) ?
                              MAX_COMPONENT_VALUE :   // if too big, coerce to max
                              (out_rounded_real < MIN_COMPONENT_VALUE) ?
                                MIN_COMPONENT_VALUE : // if too small, coerce to min
                                out_rounded_real;     // otherwise, keep value as is
    int out_clipped_imag = (out_rounded_imag >  MAX_COMPONENT_VALUE) ?
                              MAX_COMPONENT_VALUE :   // if too big, coerce to max
                              (out_rounded_imag < MIN_COMPONENT_VALUE) ?
                                MIN_COMPONENT_VALUE : // if too small, coerce to min
                                out_rounded_imag;     // otherwise, keep value as is
    return { complex_component_t'(out_clipped_real) , complex_component_t'(out_clipped_imag) };
  endfunction

  //---------------------------------------------------------------------------
  // Convert fixed-point representation to real number
  //---------------------------------------------------------------------------
  //  - fixed_point: The fixed-point value to convert
  //  - fractional_bits: The number of fractional bits in the fixed-point value
  //---------------------------------------------------------------------------
  function automatic real convert_fixed_point_to_real(logic signed [COEFF_W-1:0] fixed_point,
                                                      int fractional_bits);
    // Convert fixed-point representation to real number
    return $itor(fixed_point) / (1 << fractional_bits);
  endfunction

  //---------------------------------------------------------------------------
  // Apply gain to sample payload of data block
  //---------------------------------------------------------------------------
  //  - data_block: The input data block to modify
  //  - coeffs: The complex gain coefficients to apply
  //---------------------------------------------------------------------------
  function automatic strobe_block_t apply_gain_to_block(strobe_block_t data_block,
                                                        complex_gain_coeffs_t coeffs);
    strobe_block_t modified_block = new();
    complex_t gain = '{ convert_fixed_point_to_real(coeffs.re, COEFF_FRAC_BITS),
                        convert_fixed_point_to_real(coeffs.im, COEFF_FRAC_BITS) };

    // Apply gain to each data item in the data block
    while (data_block.data.size() > 0) begin
      logic [ITEM_W*NIPC-1:0] word_in;
      logic [ITEM_W*NIPC-1:0] word_out;
      word_in = data_block.data.pop_front();
      for (int item = 0; item < NIPC; item++) begin
        item_logic_t iq_sample_out;
        item_logic_t item_in = word_in[item*ITEM_W +: ITEM_W];
        // Split the input data into real and imaginary parts
        real samp_real      = real'(signed'(item_in[ITEM_W/2 +: ITEM_W/2]));
        real samp_imag      = real'(signed'(item_in[0 +: ITEM_W/2]));
        complex_t samp_cplx = '{samp_real, samp_imag};
        complex_t out_cplx  = mul(samp_cplx, gain);
        // Convert the output complex sample back to IQ item
        iq_sample_out = convert_complex_to_item(out_cplx);
        word_out[item*ITEM_W+:ITEM_W] = iq_sample_out;
      end
      modified_block.data.push_back(word_out);
    end
    return modified_block;
  endfunction

  //---------------------------------------------------------------------------
  // Enqueue test data blocks into the DUT and populate expected mailbox
  // for later verification.
  //---------------------------------------------------------------------------
  //  - data_blocks: The input data blocks to enqueue
  //  - mailbox: The mailbox to populate with expected output data blocks
  //  - coeffs : The complex gain coefficients to apply
  //---------------------------------------------------------------------------
  task automatic enqueue_testdata(strobe_block_queue_t data_blocks,
                                  strobe_block_mailbox_t mailbox,
                                  complex_gain_coeffs_t coeffs,
                                  bit send_finish = 1);
    foreach (data_blocks[index]) begin
      // Enqueue each block into the DUT
      dut_strobe_bfm.put(data_blocks[index].copy());
      if (mailbox != null) begin
        // Add enqueued datablocks to mailbox for expected outputs.
        mailbox.put(apply_gain_to_block(data_blocks[index], coeffs));
      end
    end
    if (send_finish) begin
      mailbox.put(null);  // Indicate end of data blocks
    end
  endtask

  //---------------------------------------------------------------------------
  // Dequeue data blocks from DUT and check against expected values in mailbox.
  //---------------------------------------------------------------------------
  //  - expected_mailbox: The mailbox containing expected output data blocks
  //---------------------------------------------------------------------------
  task automatic dequeue_and_check(strobe_block_mailbox_t expected_mailbox);
    strobe_block_t expected_block;
    strobe_block_t received_block;
    int pkt_idx = 0;

    while (expected_mailbox.num > 0) begin
      expected_mailbox.peek(expected_block);
      if (expected_block != null) begin
        // Wait for a block from the DUT
        if (!(dut_strobe_bfm.try_get(received_block))) begin
          strobe_clk_gen.clk_wait_r(1);
        end else begin
          expected_mailbox.get(expected_block);
          // Check that the received block matches the expected block
          `ASSERT_ERROR(received_block.equal(expected_block, 1),
            $sformatf(
              {"Received block %0d does not match expected block:",
              "\n Received: %s",
              "\n Expected: %s"},
              pkt_idx++,
              received_block.to_string(),
              expected_block.to_string()
            )
          );
          pkt_idx++;
        end
      end else begin
        // No more expected data blocks
        break;
      end
    end
  endtask

  //---------------------------------------------------------------------------
  // Set gain coefficients and optional timestamp in the DUT via control port
  //---------------------------------------------------------------------------
  //  - coeffs: The complex gain coefficients to set
  //  - timestamp: Optional timestamp to set (default is 'X, meaning no timestamp
  //               is set, results in immediate application of gain)
  //---------------------------------------------------------------------------
  task automatic set_gain_coefficients_timeout(complex_gain_coeffs_t coeffs,
                                               timestamp_t timestamp = 'X,
                                               int timeout = 100,
                                               bit error_on_timeout = 1,
                                               output bit timedout);
    ctrlport_request_t request;
    ctrlport_response_t expected_response;

    request.wr = '1;
    request.addr = REG_CGAIN_COEFF;
    request.data = {coeffs.re, coeffs.im};

    expected_response.ack = '1;
    expected_response.status = STS_OKAY;
    expected_response.data = '0;

    if (!($isunknown(timestamp))) begin
      request.has_time = 1;
      request.timestamp = timestamp;
    end else begin
      request.has_time = 0;
      request.timestamp = '0;
    end
    dut_ctrlport_bfm.async_request(request, expected_response, 0);
    fork
      begin : wait_for_completion
        dut_ctrlport_bfm.wait_complete();
        disable timeout_block;
        timedout = 0;
      end
      begin : timeout_block
        strobe_clk_gen.clk_wait_r(timeout);
        if (error_on_timeout) begin
          `ASSERT_ERROR(0, $sformatf("Timeout waiting for control port transaction to complete"));
        end else begin
          `ASSERT_INFO(0,
            $sformatf("Expected timeout occurred waiting for control port transaction"));
        end
        disable wait_for_completion;
        timedout = 1;
      end
    join
  endtask

  task automatic set_gain_coefficients(complex_gain_coeffs_t coeffs,
                                       timestamp_t timestamp = 'X);
    ctrlport_request_t request;
    ctrlport_response_t expected_response;
    bit timeout = 0;
    int coeff_latency = COEFF_UPDATE_LATENCY;

    request.wr = '1;
    request.addr = REG_CGAIN_COEFF;
    request.data = {coeffs.re, coeffs.im};

    expected_response.ack = '1;
    expected_response.status = STS_OKAY;
    expected_response.data = '0;

    if (!($isunknown(timestamp))) begin
      request.has_time = 1;
      request.timestamp = timestamp;
    end else begin
      request.has_time = 0;
      request.timestamp = '0;
    end
    // Need to use async_request here to have access to the ctrlport
    // timed requests.
    dut_ctrlport_bfm.async_request(request, expected_response, 0);
    dut_ctrlport_bfm.wait_complete();

    // Wait a couple cycles for settings to take effect
    // Only needed for immediate (non-timestamped) settings during module tests.
    // When testing as part of a larger system, the ctrlport routing latency
    // will ensure sufficient delay.
    if(!($isunknown(timestamp))) begin
      coeff_latency = 0;
    end
    while (coeff_latency > 0) begin
      strobe_clk_gen.clk_wait_r();
      if (dut_in_strobe_if.strobe) begin
        coeff_latency--;
      end
    end
  endtask


  //---------------------------------------------------------------------------
  // Testcase Logic
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Test setting gain coefficients and ensure they are correctly applied.
  //---------------------------------------------------------------------------
  //  - apply_immediate: If set, apply coefficients immediately, else use
  //                     a random timestamp in the near future.
  //---------------------------------------------------------------------------
  task test_set_coefficients(bit apply_immediate = 1);
    complex_gain_coeffs_t coeffs;
    static complex_gain_coeffs_t  default_coeffs =
      '{re: 1'b1 << COEFF_FRAC_BITS, im: '0};
    logic signed [COEFF_W-1:0] read_real, read_imag;
    timestamp_t timestamp;

    // Generate random complex gain coefficients
    read_real = $urandom_range(0, (1 << (COEFF_W - 1)) - 1);
    read_imag = $urandom_range(0, (1 << (COEFF_W - 1)) - 1);

    coeffs.re = read_real;
    coeffs.im = read_imag;

    if (apply_immediate) begin
      timestamp = 'X;  // No timestamp, apply immediately
    end else begin
      // Set timestamp to a future time (current timebase + random offset)
      timestamp = dut_in_strobe_if.timebase + $urandom_range(50, 100);
    end

    // Set the gain coefficients in the DUT
    set_gain_coefficients(default_coeffs);
    set_gain_coefficients(coeffs, timestamp);

    // Read back and verify the gain coefficients

    dut_ctrlport_bfm.read(REG_CGAIN_COEFF, {read_real, read_imag});

    if (apply_immediate) begin
      `ASSERT_ERROR(
          read_real == coeffs.re, $sformatf(
          "Real coefficient readback mismatch: expected %0d, got %0d",
            coeffs.re, read_real));
      `ASSERT_ERROR(read_imag == coeffs.im, $sformatf(
                    "Imaginary coefficient readback mismatch: expected %0d, got %0d",
                    coeffs.im,
                    read_imag));
    end else begin
      // If timestamped, the readback should still show the previous coefficients (defaults)
      `ASSERT_ERROR(read_real == 1'b1 << COEFF_FRAC_BITS, $sformatf(
                    "Real coefficient changed before timestamp applied: got %0d", read_real));
      `ASSERT_ERROR(read_imag == 1'b0, $sformatf(
                    "Imaginary coefficient changed before timestamp applied: got %0d", read_imag));
      while (dut_in_strobe_if.timebase < timestamp) begin
        strobe_clk_gen.clk_wait_r(1);
      end

      // After timestamp is reached, the new coefficients should be applied
      dut_ctrlport_bfm.read(REG_CGAIN_COEFF, {read_real, read_imag});

      `ASSERT_ERROR(
          read_real == coeffs.re, $sformatf(
          "Real coefficient readback mismatch: expected %0d, got %0d",
            coeffs.re, read_real));
      `ASSERT_ERROR(read_imag == coeffs.im, $sformatf(
                    "Imaginary coefficient readback mismatch: expected %0d, got %0d",
                    coeffs.im,
                    read_imag));
    end
  endtask

  //---------------------------------------------------------------------------
  // Test default values of control registers
  //---------------------------------------------------------------------------
  task automatic test_coeff_defaults();
    complex_gain_coeffs_t coeffs;
    logic signed [COEFF_W-1:0] read_real, read_imag;

    dut_ctrlport_bfm.read(REG_CGAIN_COEFF, {read_real, read_imag});

    `ASSERT_ERROR(read_real == 1'b1 << COEFF_FRAC_BITS, $sformatf(
                  "Initial real coefficient not 1.0: got %0d", read_real));
    `ASSERT_ERROR(read_imag == 1'b0, $sformatf(
                  "Initial imaginary coefficient not zero: got %0d", read_imag));

  endtask

  //---------------------------------------------------------------------------
  // Test the coefficient queue, including filling, reading status,
  // filling beyond maximum capacity, and clearing.
  //---------------------------------------------------------------------------
  task automatic test_queue_status();
    complex_gain_coeffs_t coeffs;
    bit expected_timeout;

    // Fill up to the queue depth.
    repeat (MAX_QUEUE_ITEMS) begin
      coeffs.re = $urandom_range(0, (1 << (COEFF_W - 1)) - 1);
      coeffs.im = $urandom_range(0, (1 << (COEFF_W - 1)) - 1);

      set_gain_coefficients(coeffs, '1);
    end
    // Now the queue should be full, and the next addition should timeout
    // after the specified timeout period.
    set_gain_coefficients_timeout(coeffs, '1, 100, 0, expected_timeout);

    `ASSERT_ERROR(expected_timeout,
      $sformatf(
        "Expected timeout when adding to full queue, but received response"
      )
    );

    // To empty the queue, we need to reset the DUT, as there is no other way to
    // dequeue entries from the coefficient queue, other than waiting for timestamps
    // to be reached, which is not practical in a testbench.
    strobe_clk_gen.reset(4);
    // Wait for reset to deassert
    wait (strobe_rst == 0);
    strobe_clk_gen.clk_wait_r(40);

  endtask
  //---------------------------------------------------------------------------
  // Wrapper for all testcases that operate on registers and controlport interface
  //---------------------------------------------------------------------------
  task automatic test_registers();
    test.start_test("Register Defaults", 100us);
    test_coeff_defaults();
    test.end_test();
    test.start_test("Queue Status", 100us);
    test_queue_status();
    test.end_test();
    test.start_test("Set Coefficients", 100us);
    repeat (NUM_TESTS) begin
      test_set_coefficients();
    end
    test.end_test();
    test.start_test("Set Coefficients with Timestamp", 200us);
    repeat (NUM_TESTS) begin
      test_set_coefficients(0);
    end
    test.end_test();
  endtask

  //---------------------------------------------------------------------------
  // Test the complex gain functionality with random data blocks and coefficients.
  //---------------------------------------------------------------------------
  //  - passthrough: If set, use gain coefficients of 1.0 + 0.0j to verify
  //                 passthrough functionality.
  //---------------------------------------------------------------------------
  task automatic test_complex_gain(bit passthrough = 0);
    strobe_block_queue_t test_blocks;
    strobe_block_mailbox_t expected_mailbox = new();
    complex_gain_coeffs_t coeffs;
    timestamp_t timestamps[];

    int num_blocks = $urandom_range(1, 10);
    int block_size = $urandom_range(1, 20);

    if (VERBOSE) begin
      $display("Test with %0d data blocks of size %0d", num_blocks, block_size);
    end

    // Generate random test data blocks
    timestamps = new[num_blocks];
    foreach (timestamps[i]) begin
      timestamps[i] = '0;
    end
    test_blocks = generate_test_data(block_size, timestamps);

    if (passthrough) begin
      // Set gain coefficients to 1.0 + 0.0j for passthrough
      coeffs.re = 1'b1 << COEFF_FRAC_BITS;
      coeffs.im = 1'b0;
    end else begin
      // Generate random complex gain coefficients
      coeffs.re = Rand#(COEFF_W)::rand_logic();
      coeffs.im = Rand#(COEFF_W)::rand_logic();
    end

    if (VERBOSE) begin
      $display("Using gain coefficients: real = %0.4f, imag = %0.4f", convert_fixed_point_to_real(
               coeffs.re, COEFF_FRAC_BITS), convert_fixed_point_to_real(coeffs.im,
                                                                                COEFF_FRAC_BITS));
    end

    // Set the gain coefficients in the DUT
    set_gain_coefficients(coeffs);

    // Enqueue test data blocks into DUT and populate expected mailbox
    enqueue_testdata(test_blocks, expected_mailbox, coeffs);

    // Dequeue data blocks from DUT and check against expected mailbox
    dequeue_and_check(expected_mailbox);

  endtask

  //---------------------------------------------------------------------------
  // Test the complex gain functionality with random data blocks and coefficients,
  // using timed coefficient changes to verify correct application of different
  // coefficients at different times.
  //---------------------------------------------------------------------------
  task automatic test_complex_gain_timed();
    strobe_block_queue_t test_blocks[2];
    strobe_block_mailbox_t expected_mailbox = new();
    complex_gain_coeffs_t coeffs[2];
    timestamp_t timestamps[];
    timestamp_t first_timestamps[];
    timestamp_t second_timestamps[];
    timestamp_t current_ts, delayed_ts;

    int num_blocks = $urandom_range(4, 20);
    int block_size = $urandom_range(1, 20);

    int pkt_id_new_ts = $urandom_range(1, num_blocks-1);     // Index to start using new timestamp
    int num_first_blocks = pkt_id_new_ts;                   // Data blocks using first gain setting
    int num_second_blocks = num_blocks - num_first_blocks; // Data blocks using second gain setting

    if (VERBOSE) begin
      $display("Test with %0d data blocks of size %0d", num_blocks, block_size);
    end

    // Generate random test data blocks
    timestamps = new[num_blocks];
    first_timestamps = new[num_first_blocks];
    second_timestamps = new[num_second_blocks];

    foreach (coeffs[index]) begin
      // Generate random complex gain coefficients
      coeffs[index].re = Rand#(COEFF_W)::rand_logic();
      coeffs[index].im = Rand#(COEFF_W)::rand_logic();
    end

    // Generate timestamps for each block
    current_ts = dut_in_strobe_if.timebase + COEFF_UPDATE_LATENCY;

    // Delay the second half of data blocks by a random amount:
    // |         -- first half of data blocks --         |    -- start of second half --              |
    // |(num_blocks - pkt_id_new_ts) * (block_size + 1)  |  + STALL_PROB adjustment + random(25, 50)  |
    delayed_ts = current_ts + (num_first_blocks * (block_size + 1))*(1.0 + STB_STALL_PROB/100.0) + $urandom_range(25, 50);

    if (VERBOSE) begin
      foreach (coeffs[index]) begin
        $display("Time %0h: Coeff set %0d(Data Blocks %0d to %0d): real = %0.4f, imag = %0.4f",
                 index == 0 ? current_ts : delayed_ts,
                 index,
                 index == 0 ? 0 : num_first_blocks, index == 0 ? num_first_blocks - 1 : num_blocks - 1,
                 convert_fixed_point_to_real(coeffs[index].re, COEFF_FRAC_BITS),
                 convert_fixed_point_to_real(coeffs[index].im, COEFF_FRAC_BITS));
      end
    end

    // Set the gain coefficients in the DUT, with timestamp for second half
    // Assumption, setting gain does not take longer than 25(lower bound for random delay)
    set_gain_coefficients(coeffs[0]);
    set_gain_coefficients(coeffs[1], delayed_ts);

    foreach (timestamps[index]) begin
      // Default: send immediately
      timestamps[index] = '0;
      // Delay the second half until specified timestamp, only need to set the first block in that half
      // because afterwards we can just send immediately again.
      if (index == pkt_id_new_ts) begin
        timestamps[index] = delayed_ts;
      end
    end

    // Split timestamps into two arrays for the two sets of data blocks, first_timesamps contains the
    // timestamps which have the first gain coefficient applied, second_timestamps contains the timestamps
    // which have the second gain coefficient applied.
    first_timestamps  = new[pkt_id_new_ts];
    second_timestamps = new[num_blocks - pkt_id_new_ts];
    for (int i = 0; i < pkt_id_new_ts; i++) begin
      first_timestamps[i] = timestamps[i];
    end
    for (int i = pkt_id_new_ts; i < num_blocks; i++) begin
      second_timestamps[i - pkt_id_new_ts] = timestamps[i];
    end

    test_blocks[0]   = generate_test_data(block_size, first_timestamps);
    test_blocks[1]   = generate_test_data(block_size, second_timestamps);

    enqueue_testdata(test_blocks[0], expected_mailbox, coeffs[0], 0);
    enqueue_testdata(test_blocks[1], expected_mailbox, coeffs[1]);

    // Dequeue data blocks from DUT and check against expected mailbox
    dequeue_and_check(expected_mailbox);

  endtask


  //---------------------------------------------------------------------------
  // Test Execution
  //---------------------------------------------------------------------------
  initial begin : test_execution
    // Generate a string for the name of this instance of the testbench
    string tb_name;
    tb_name = $sformatf(
      {
        "timed_complex_gain_tb\nITEM_W = %0D, NIPC = %0D, COEFF_W = %0D,",
        "COEFF_FRAC_BITS = %0D, STB_STALLPROB = %0D."
      },
      ITEM_W,
      NIPC,
      COEFF_W,
      COEFF_FRAC_BITS,
      STB_STALL_PROB
    );

    test.start_tb(tb_name, NUM_TESTS * 1ms);

    strobe_clk_gen.start();
    strobe_clk_gen.reset();

    dut_ctrlport_bfm.run();

    // Start the AXI Stream BFM
    dut_strobe_bfm.set_master_stall_prob(STB_STALL_PROB);
    dut_strobe_bfm.run();

    // Wait for reset to deassert
    wait (strobe_rst == 0);

    test_registers();

    test.start_test("Complex Gain Test", NUM_TESTS * 10us);
    dut_strobe_bfm.set_inter_block_gap();
    test_complex_gain(1);  // Passthrough test
    for (int i = 0; i < NUM_TESTS; i++) begin
      test_complex_gain();
    end
    dut_strobe_bfm.set_inter_block_gap(0);
    test.end_test();
    test.start_test("Timed Complex Gain Test", NUM_TESTS * 20us);
    dut_strobe_bfm.set_inter_block_gap();
    for (int i = 0; i < NUM_TESTS; i++) begin
      test_complex_gain_timed();
    end
    dut_strobe_bfm.set_inter_block_gap(0);
    test.end_test();
    strobe_clk_gen.kill();
    test.end_tb(0);
  end

endmodule : timed_complex_gain_tb

`default_nettype wire
