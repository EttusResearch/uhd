//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_mux_tb.sv
//
// Description:
//
// A simple testbench for the chdr_channel_mux module.
//

`default_nettype none

module chdr_channel_mux_tb#(
  parameter int NUM_PORTS      = 2,
  parameter int CHDR_W         = 64,
  parameter int CHANNEL_OFFSET = 0
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrBfm::*;
  import PkgRandom::*;

  //---------------------------------------------------------------------------
  // Parameters
  //---------------------------------------------------------------------------
  localparam int PRE_FIFO_SIZE = 1;
  localparam int POST_FIFO_SIZE = 1;
  localparam int PRIORITY = 0;
  localparam real CLK_PERIOD = 10.0;

  localparam int PKTS_PER_BURST = 10;
  localparam int WORDS_PER_PKT = 10;

  // Default stall probability for the BFMs as defined in AxiStreamBfm
  localparam int DEF_STALL_PROB = 38;

  //---------------------------------------------------------------------------
  // Local Typedefs
  //---------------------------------------------------------------------------
  typedef ChdrPacket#(.CHDR_W(CHDR_W)) chdr_pkt_t;
  typedef chdr_pkt_t chdr_pkt_queue_t[$];

  //---------------------------------------------------------------------------
  // Local Signals
  //---------------------------------------------------------------------------
  logic                             clk;
  logic                             rst;
  logic [NUM_PORTS-1:0][CHDR_W-1:0] in_tdata;
  logic [NUM_PORTS-1:0]             in_tvalid;
  logic [NUM_PORTS-1:0]             in_tlast;
  logic [NUM_PORTS-1:0]             in_tready;


  //---------------------------------------------------------------------------
  // Clocking and Reset
  //---------------------------------------------------------------------------
  sim_clock_gen #(
    .PERIOD(CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  //---------------------------------------------------------------------------
  // Chdr BFM
  //---------------------------------------------------------------------------
  // AXI Stream interface for the DUT inputs
  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W)
  ) to_dut[NUM_PORTS] (
    .clk(clk),
    .rst(rst)
  );

  // AXI Stream interface for the DUT output
  AxiStreamIf #(
    .DATA_WIDTH(CHDR_W)
  ) from_dut (
    .clk(clk),
    .rst(rst)
  );

  // Chdr BFM
  ChdrBfm #(.CHDR_W(CHDR_W)) chdr_bfm[NUM_PORTS];

  // Connect the BFMs to the interfaces
  generate
    // Connect the mandatory ports to the first BFM
    initial begin
      chdr_bfm[0] = new(to_dut[0], from_dut);
    end
    // Connect additional input ports to separate BFMs
    for (genvar port = 1; port < NUM_PORTS; port++) begin : gen_bfm_connections
      initial begin
        chdr_bfm[port] = new(to_dut[port], null);
      end
    end
    for (genvar port = 0; port < NUM_PORTS; port++) begin : gen_input_connections
      assign in_tdata[port] = to_dut[port].tdata;
      assign in_tvalid[port] = to_dut[port].tvalid;
      assign in_tlast[port] = to_dut[port].tlast;
      assign to_dut[port].tready = in_tready[port];
    end
  endgenerate

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------
  chdr_channel_mux #(
    .NUM_PORTS     (NUM_PORTS),
    .CHDR_W        (CHDR_W),
    .CHANNEL_OFFSET(CHANNEL_OFFSET),
    .PRE_FIFO_SIZE (PRE_FIFO_SIZE),
    .POST_FIFO_SIZE(POST_FIFO_SIZE),
    .PRIORITY      (PRIORITY)
  ) dut (
    .clk       (clk),
    .rst       (rst),
    .in_tdata  (in_tdata),
    .in_tvalid (in_tvalid),
    .in_tlast  (in_tlast),
    .in_tready (in_tready),
    .out_tdata (from_dut.tdata),
    .out_tvalid(from_dut.tvalid),
    .out_tlast (from_dut.tlast),
    .out_tready(from_dut.tready)
  );

  //---------------------------------------------------------------------------
  // Testbench logic
  //---------------------------------------------------------------------------
  // Reset the DUT
  task automatic reset_dut();
    // Reset the DUT asynchronously and reset it synchronously after 4 clock
    // cycles to ensure that the DUT is in a known state before starting the test.
    // The DUT is assumed to be fully reset and ready for input when the reset
    // signal is deasserted.
    clk_gen.reset(4);
    @(negedge rst);
  endtask

  // Randomize the header fields of the packet.
  function automatic void randomize_header(ref chdr_pkt_t packet);
    packet.header = chdr_build_header(Rand#(CHDR_VC_W)::rand_logic(),
                                      Rand#(CHDR_EOB_W)::rand_logic(),
                                      Rand#(CHDR_EOV_W)::rand_logic(),
                                      Rand#(CHDR_PKT_TYPE_W)::rand_bit_range(
                                          CHDR_PKT_TYPE_DATA_TS
                                      ),
                                      Rand#(CHDR_NUM_MDATA_W)::rand_logic(),
                                      Rand#(CHDR_SEQ_NUM_W)::rand_logic(),
                                      0,
                                      Rand#(CHDR_DST_EPID_W)::rand_logic());
  endfunction

  // Generate a packet of length num_words for the given channel
  function automatic chdr_pkt_t gen_channel_packet(int chan, int num_words, bit has_mdata);
    chdr_pkt_t packet;
    packet = new();
    randomize_header(packet);
    if (chdr_get_pkt_type(packet.header) == CHDR_PKT_TYPE_DATA_TS) begin
      packet.timestamp = Rand#(CHDR_TIMESTAMP_W)::rand_logic();
    end
    if (!has_mdata) begin
      packet.header = chdr_set_num_mdata(packet.header, 0);
    end
    if (chdr_get_num_mdata(packet.header) > 0) begin
      for (int meta_word = 0; meta_word < chdr_get_num_mdata(packet.header); meta_word++) begin
        packet.metadata.push_back(Rand#(CHDR_W)::rand_logic());
      end
    end
    if (num_words < 0) begin
      // Randomize the number of words in the packet, leave some space for the metadata
      num_words = $urandom_range(2**(CHDR_LENGTH_W-2)*8/CHDR_W, 10);
    end
    for (int word = 0; word < num_words; word++) begin
      packet.data.push_back(Rand#(CHDR_W)::rand_logic());
    end
    packet.update_lengths();
    return packet;
  endfunction

  // Generate a burst of packets of length num_packets for the given channel
  function automatic chdr_pkt_queue_t gen_channel_burst(int chan, int num_packets, int num_words,
                                                        bit has_mdata);
    chdr_pkt_queue_t packets;
    repeat (num_packets) begin
      packets.push_back(gen_channel_packet(chan, num_words, has_mdata));
    end
    return packets;
  endfunction

  // Send a burst of packets to the given channel of the dut
  task automatic send_channel_burst(input int chan, input int num_packets, input int num_words,
                                    output chdr_pkt_queue_t packets, input bit has_mdata = 1,
                                    input bit mult_chdr_size = 1);
    packets = gen_channel_burst(chan, num_packets, num_words, has_mdata);
    if (!mult_chdr_size) begin
      foreach (packets[pkts]) begin
        // Reduce the size of the  packet to be less than CHDR_W by removing half the data
        // of the last word.
        chdr_pkt_t pkt;
        pkt = packets[pkts];
        pkt.data.pop_back();
        pkt.data.push_back({'X, Rand#(CHDR_W/2)::rand_logic()});
        pkt.header = chdr_set_length(pkt.header, chdr_get_length(pkt.header) - (CHDR_W/2)/8);
      end
    end
    foreach (packets[pkt]) begin
      chdr_bfm[chan].put_chdr(packets[pkt].copy());
    end
  endtask

  // Validate the dut output against the expected packets
  function automatic void validate_dut_output(int num_packets[NUM_PORTS],
                                              chdr_pkt_queue_t expected_packets[NUM_PORTS],
                                              chdr_pkt_queue_t dut_output);
    chdr_pkt_queue_t received_packets[NUM_PORTS];
    // Separate the output packets by channel
    foreach (dut_output[ch]) begin
      int vc = chdr_get_vc(dut_output[ch].header);
      received_packets[vc].push_back(dut_output[ch]);
    end
    // Validate the received packets for each channel
    foreach (num_packets[ch]) begin
      if (num_packets[ch] != received_packets[ch].size()) begin
        $error("Number of packets received on channel %0d does not match expected", ch);
      end
      foreach (expected_packets[ch, pkt]) begin
        if (!(expected_packets[ch][pkt].equal(received_packets[ch][pkt]))) begin
          $error("Packet %0d on channel %0d does not match expected", pkt, ch);
        end
      end
    end
  endfunction

  // Get all the packets from the dut output
  task automatic get_dut_output(input int num_packets[NUM_PORTS], output chdr_pkt_queue_t packets);
    int total_packets = 0;
    chdr_pkt_t pkt;
    foreach (num_packets[ch]) begin
      total_packets += num_packets[ch];
    end
    wait(chdr_bfm[0].rx_packets.num() >= total_packets);
    repeat (total_packets) begin
      chdr_bfm[0].get_chdr(pkt);
      packets.push_back(pkt);
    end
  endtask

  //---------------------------------------------------------------------------
  // Testcases
  //---------------------------------------------------------------------------
  // Send a burst of packets to all channels and validate the output
  task automatic testcase_simple_burst_all_channels();
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    foreach (num_packets[ch]) begin
      num_packets[ch] = PKTS_PER_BURST;
    end

    // Send the packets to the dut
    foreach (num_packets[ch]) begin
      send_channel_burst(ch, num_packets[ch], WORDS_PER_PKT, expected_packets[ch]);
    end

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);

  endtask

  // Send a burst of packets to only on one channel channels and validate the output
  task automatic testcase_simple_burst_one_channel(int channel);
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    for (int ch = 0; ch < NUM_PORTS; ch++) begin
      num_packets[ch] = 0;
      if (ch == channel) begin
        num_packets[ch] = PKTS_PER_BURST;
      end
    end

    // Send the packets to the dut
    send_channel_burst(channel, num_packets[channel], WORDS_PER_PKT, expected_packets[channel]);

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);
  endtask

  // Send a burst of packets to all channels with an asymetric number of packets
  // and validate the output. The channel specified will have twice the number
  // of packets as the other channels.
  task automatic testcase_asymetric_bursts(int channel, int multiple = 2);
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    foreach (num_packets[ch]) begin
      num_packets[ch] = PKTS_PER_BURST;
      if (ch == channel) begin
        num_packets[ch] = PKTS_PER_BURST * multiple;
      end
    end

    // Send the packets to the dut
    foreach (num_packets[ch]) begin
      send_channel_burst(ch, num_packets[ch], WORDS_PER_PKT, expected_packets[ch]);
    end

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);
  endtask

  // Send a burst of packets of random sizes to all channels and validate the output.
  task automatic testcase_simple_bursts_random_pkt_size();
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    foreach (num_packets[ch]) begin
      num_packets[ch] = PKTS_PER_BURST;
    end

    // Send the packets to the dut
    foreach (num_packets[ch]) begin
      send_channel_burst(ch, num_packets[ch], -1, expected_packets[ch]);
    end

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);
  endtask

    // Send a burst of packets without payload or metadata to all channels and validate the output.
  task automatic testcase_simple_bursts_min_pkt_size();
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    foreach (num_packets[ch]) begin
      num_packets[ch] = PKTS_PER_BURST;
    end

    // Send the packets to the dut
    foreach (num_packets[ch]) begin
      send_channel_burst(ch, num_packets[ch], 0, expected_packets[ch], 0);
    end

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);
  endtask

  task automatic testcase_burst_non_mult_of_CHDR_size();
    chdr_pkt_queue_t expected_packets[NUM_PORTS];
    chdr_pkt_queue_t dut_output;
    int num_packets[NUM_PORTS];

    // Reset the DUT
    reset_dut();

    foreach (num_packets[ch]) begin
      num_packets[ch] = PKTS_PER_BURST;
    end

    // Send the packets to the dut
    foreach (num_packets[ch]) begin
      send_channel_burst(ch, num_packets[ch], WORDS_PER_PKT-1, expected_packets[ch], 1, 0);
    end

    // Process expected packets. Set the VC field in the header to the channel number.
    // This is required because the DUT does modify the VC field and sets it to the input channel.
    foreach (expected_packets[channel, pkt]) begin
      expected_packets[channel][pkt].header = chdr_set_vc(expected_packets[channel][pkt].header,
                                                        channel + CHANNEL_OFFSET);
    end

    // Wait for the output to be ready
    get_dut_output(num_packets, dut_output);

    // Validate the output
    validate_dut_output(num_packets, expected_packets, dut_output);

  endtask

  //---------------------------------------------------------------------------
  // Test Execution
  //---------------------------------------------------------------------------
  initial begin
    test.start_tb("chdr_channel_mux_tb");
    // Print TB parameters.
    $display("NUM_PORTS: %0d", NUM_PORTS);
    $display("CHDR_W: %0d", CHDR_W);

    // Start the clock.
    clk_gen.start();

    // Start the BFMs.
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].run();
    end
    // Run the testcases.
    test.start_test("testcase_simple_burst_one_channel");
    for(int ch = 0; ch < NUM_PORTS; ch++) begin
      testcase_simple_burst_one_channel(ch);
    end
    test.end_test();

    test.start_test("testcase_simple_burst_all_channels");
    testcase_simple_burst_all_channels();
    test.end_test();

    test.start_test("testcase_asymetric_bursts");
    for (int ch = 0; ch < NUM_PORTS; ch++) begin
      testcase_asymetric_bursts(ch);
    end
    test.end_test();

    test.start_test("testcase_simple_bursts_random_pkt_size");
    testcase_simple_bursts_random_pkt_size();
    test.end_test();

    test.start_test("testcase_simple_bursts_min_pkt_size");
    testcase_simple_bursts_min_pkt_size();
    test.end_test();

    test.start_test("testcase_burst_non_mult_of_CHDR_size");
    testcase_burst_non_mult_of_CHDR_size();
    test.end_test();

    test.start_test("testcase_simple_burst_no_input_stalls");
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].set_master_stall_prob(0);
    end
    testcase_simple_burst_all_channels();
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].set_master_stall_prob(DEF_STALL_PROB);
    end
    test.end_test();

    test.start_test("testcase_simple_burst_high_input_stalls");
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].set_master_stall_prob(80);
    end
    testcase_simple_burst_all_channels();
    foreach (chdr_bfm[ch]) begin
      chdr_bfm[ch].set_master_stall_prob(DEF_STALL_PROB);
    end
    test.end_test();

    test.start_test("testcase_simple_burst_no_output_stalls");
    chdr_bfm[0].set_slave_stall_prob(0);
    testcase_simple_burst_all_channels();
    chdr_bfm[0].set_slave_stall_prob(DEF_STALL_PROB);
    test.end_test();

    test.start_test("testcase_simple_burst_high_output_stalls");
    chdr_bfm[0].set_slave_stall_prob(80);
    testcase_simple_burst_all_channels();
    chdr_bfm[0].set_slave_stall_prob(DEF_STALL_PROB);
    test.end_test();

    // Cleanup after all testcases have run.
    test.end_tb(0);

    clk_gen.kill();

  end



endmodule

`default_nettype wire
