//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ChdrIfaceBfm_tb
//
// Description: This is the testbench for the ChdrIfaceBfm class.
//

module ChdrIfaceBfm_tb #(
  parameter int CHDR_W   = 64,
  parameter int ITEM_W   = 32,
  parameter int SPP      = 64,
  parameter bit ADV      = 0
);

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgChdrIfaceBfm::*;


  //---------------------------------------------------------------------------
  // Simulation Constants
  //---------------------------------------------------------------------------

  localparam bit VERBOSE = 0;    // Set to 1 for more display output

  localparam realtime CLOCK_PER = 10.0ns;

  localparam int WPP            = SPP * ITEM_W / CHDR_W;  // CHDR words per packet
  localparam int BYTES_PER_ITEM = ITEM_W/8;
  localparam int MAX_PYLD_BYTES = SPP * BYTES_PER_ITEM;
  localparam int MAX_PACKETS    = 3;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit rfnoc_chdr_clk;

  sim_clock_gen #(CLOCK_PER) rfnoc_chdr_clk_gen (.clk(rfnoc_chdr_clk), .rst());


  //---------------------------------------------------------------------------
  // CHDR Types
  //---------------------------------------------------------------------------

  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  ChdrData #(CHDR_W, ITEM_W) chdr_data;


  //---------------------------------------------------------------------------
  // BFM
  //---------------------------------------------------------------------------

  AxiStreamIf #(CHDR_W) chdr_ifc (rfnoc_chdr_clk, 1'b0);

  // Loop the CHDR BFM back to itself
  ChdrIfaceBfm #(CHDR_W, ITEM_W) bfm = new(chdr_ifc, chdr_ifc, MAX_PYLD_BYTES);


  //---------------------------------------------------------------------------
  // Data Structures
  //---------------------------------------------------------------------------

  typedef struct {
    chdr_word_t   data[$];
    int           data_bytes;
    chdr_word_t   metadata[$];
    packet_info_t pkt_info;
  } packet_t;

  typedef enum int {
    TEST_RECV, TEST_RECV_ADV, TEST_NUM_ITEMS, TEST_EOB, TEST_EOV
  } test_variant_t;


  //---------------------------------------------------------------------------
  // Utilities
  //---------------------------------------------------------------------------

  // Rand#(WIDTH)::rand_logic() returns a WIDTH-bit random number. We avoid
  // std::randomize() due to license requirements and limited tool support.
  class Rand #(WIDTH = 32);

    static function logic [WIDTH-1:0] rand_logic();
      logic [WIDTH-1:0] result;
      int num_rand32 = (WIDTH + 31) / 32;
      for (int i = 0; i < num_rand32; i++) begin
        result = {result, $urandom()};
      end
      return result;
    endfunction : rand_logic

  endclass : Rand


  // Generate a random packet, in the form of a random packet_t structure.
  //
  //   packets: Generate multiple packets worth of data if 1, generate 1
  //            packet if 0.
  //   items:   Make the data a multiple of items if 1, a of bytes if 0.
  //
  function automatic packet_t rand_pkt(bit packets = 0, bit items = 0);
    packet_t packet;
    int      num_mdata;
    int      num_packets;

    // Decide how many packets we're going to generate
    if (packets) num_packets = $urandom_range(1, MAX_PACKETS);
    else num_packets = 1;

    // Decide how much metadata, assuming we can split it across multiple
    // packets.
    num_mdata = $urandom_range(0, num_packets*(31));

    // Randomize the rest of the packet info
    packet.pkt_info = Rand#($bits(packet.pkt_info))::rand_logic();

    // Decide how much data we're going to send. The last packet will be a
    // random length, all preceding packets will be full length.
    if (items) packet.data_bytes = $urandom_range(1, SPP) * BYTES_PER_ITEM;
    else packet.data_bytes = $urandom_range(1, SPP*BYTES_PER_ITEM);
    packet.data_bytes += (num_packets-1) * MAX_PYLD_BYTES;

    // Generate random data and metadata
    packet.data = {};
    for (int bytes = 0; bytes < packet.data_bytes; bytes += (CHDR_W/8))
      packet.data.push_back(Rand#(CHDR_W)::rand_logic());
    packet.metadata = {};
    for (int words = 0; words < num_mdata; words++)
       packet.metadata.push_back(Rand#(CHDR_W)::rand_logic());

    // Zero the timestamp if there's no time, since that's what the BFM will
    // return in that case for the time.
    if (packet.pkt_info.has_time == 0)  packet.pkt_info.timestamp = 0;

    return packet;
  endfunction : rand_pkt


  //---------------------------------------------------------------------------
  // Test Tasks
  //---------------------------------------------------------------------------

  task test_send(test_variant_t test_type);
    packet_t send_pkt, recv_pkt;

    // Generate a random packet
    send_pkt = rand_pkt(0, 0);
    if (VERBOSE) begin
      $display("test_send:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Send then receive it
    bfm.send(send_pkt.data, send_pkt.data_bytes, send_pkt.metadata, send_pkt.pkt_info);

    if (test_type == TEST_RECV_ADV) begin
      bfm.recv_adv(recv_pkt.data, recv_pkt.data_bytes, recv_pkt.metadata, recv_pkt.pkt_info);

      // Check if the metadata and packet info matches what we sent
      `ASSERT_ERROR(chdr_data.chdr_equal(send_pkt.metadata, recv_pkt.metadata), "Metadata did not match");
      `ASSERT_ERROR(send_pkt.pkt_info == recv_pkt.pkt_info, "Packet info did not match");
    end else begin
      bfm.recv(recv_pkt.data, recv_pkt.data_bytes);
    end

    // Check if we received the data what we sent
    `ASSERT_ERROR(chdr_data.chdr_equal(send_pkt.data, recv_pkt.data), "Data did not match");
    `ASSERT_ERROR(send_pkt.data_bytes == recv_pkt.data_bytes, "Data byte length did not match");
  endtask : test_send


  task test_send_items();
    packet_t send_pkt, recv_pkt;
    item_t send_items[$], recv_items[$];

    // Generate a random packet
    send_pkt = rand_pkt(0, 1);
    if (VERBOSE) begin
      $display("test_send_items:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Send then receive it, converting between CHDR and item words
    send_items = chdr_data.chdr_to_item(send_pkt.data, send_pkt.data_bytes);
    bfm.send_items(send_items, send_pkt.metadata, send_pkt.pkt_info);
    bfm.recv_items_adv(recv_items, recv_pkt.metadata, recv_pkt.pkt_info);

    // Check if we received what we sent
    `ASSERT_ERROR(chdr_data.item_equal(send_items, recv_items), "Data did not match");
    `ASSERT_ERROR(chdr_data.chdr_equal(send_pkt.metadata, recv_pkt.metadata), "Metadata did not match");
    `ASSERT_ERROR(send_pkt.pkt_info == recv_pkt.pkt_info, "Packet info did not match");
  endtask : test_send_items


  task test_send_packets();
    packet_t send_pkt, recv_pkt;
    int num_packets;
    int data_index, mdata_index;
    packet_info_t pkt_info;

    // Generate a random packet
    send_pkt = rand_pkt(1, 0);
    if (VERBOSE) begin
      $display("test_send_packets:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Send the packet data, all at once
    bfm.send_packets(send_pkt.data, send_pkt.data_bytes, send_pkt.metadata, send_pkt.pkt_info);

    // Receive and check all the generated packets
    num_packets = (send_pkt.data_bytes + MAX_PYLD_BYTES - 1) / MAX_PYLD_BYTES;
    data_index = 0;
    mdata_index = 0;
    pkt_info = send_pkt.pkt_info;
    pkt_info.eob = 0;   // EOB/EOV should only be set for last packet
    pkt_info.eov = 0;
    for (int pkt_count = 0; pkt_count < num_packets; pkt_count++) begin
      int exp_byte_length;
      int exp_word_length;
      int exp_num_mdata;
      chdr_word_t temp_queue[$];

      // Calculate the length of the next packet
      if (pkt_count < num_packets-1) begin
        exp_byte_length = MAX_PYLD_BYTES;
      end else begin
        // Last packet's length is whatever is left
        exp_byte_length = send_pkt.data_bytes - pkt_count * MAX_PYLD_BYTES;
      end
      exp_word_length = (exp_byte_length + (CHDR_W/8) - 1) / (CHDR_W/8);

      // Calculate how much metadata we have left for the next packet
      exp_num_mdata = send_pkt.metadata.size() - (pkt_count * 31);
      if (exp_num_mdata > 31) exp_num_mdata = 31;   // Up to 31 words per packet
      if (exp_num_mdata < 0) exp_num_mdata = 0;     // No less than 0

      // Receive the next packet
      bfm.recv_adv(recv_pkt.data, recv_pkt.data_bytes, recv_pkt.metadata, recv_pkt.pkt_info);

      // Check the data length of the received packet
      `ASSERT_ERROR(
        exp_byte_length == recv_pkt.data_bytes,
        $sformatf(
          "Length of packet %0d didn't match (received %0d, expected %0d)",
          pkt_count, recv_pkt.data_bytes, exp_byte_length
        )
      );

      // Check the data contents
      temp_queue = send_pkt.data[data_index:data_index+exp_word_length-1];
      `ASSERT_ERROR(
        chdr_data.chdr_equal(temp_queue, recv_pkt.data),
        "Data did not match"
      );

      // Check the metadata contents
      if (exp_num_mdata > 0) begin
        temp_queue = send_pkt.metadata[mdata_index:mdata_index+exp_num_mdata-1];
        `ASSERT_ERROR(
          chdr_data.chdr_equal(temp_queue, recv_pkt.metadata),
          "Metadata did not match"
        );
      end

      // Check the pkt_info
      if (pkt_count < num_packets-1) begin
        // Not the last packet
        `ASSERT_ERROR(
          pkt_info == recv_pkt.pkt_info,
          $sformatf("Packet info did not match on packet %0d", pkt_count)
        );
      end else begin
        // This is the last packet
        pkt_info.eob = send_pkt.pkt_info.eob;
        pkt_info.eov = send_pkt.pkt_info.eov;
        `ASSERT_ERROR(
          pkt_info == recv_pkt.pkt_info,
          $sformatf("Packet info did not match on packet %0d (last packet)", pkt_count)
        );
      end

      // Update counters for next iteration
      if (pkt_info.has_time) pkt_info.timestamp += exp_word_length * CHDR_W/ITEM_W;
      data_index  += exp_word_length;
      mdata_index += exp_num_mdata;
    end

  endtask : test_send_packets


  task test_send_packets_items();
    packet_t send_pkt, recv_pkt;
    int num_packets;
    int data_index, mdata_index;
    packet_info_t pkt_info;
    item_t send_items[$], recv_items[$];

    // Generate a random packet
    send_pkt = rand_pkt(1, 1);
    if (VERBOSE) begin
      $display("test_send_packets_items:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Send the packet data, all at once
    send_items = chdr_data.chdr_to_item(send_pkt.data, send_pkt.data_bytes);
    bfm.send_packets_items(send_items, send_pkt.metadata, send_pkt.pkt_info);

    // Receive and check all the generated packets
    num_packets = (send_pkt.data_bytes + MAX_PYLD_BYTES - 1) / MAX_PYLD_BYTES;
    data_index = 0;
    mdata_index = 0;
    pkt_info = send_pkt.pkt_info;
    pkt_info.eob = 0;   // EOB/EOV should only be set for last packet
    pkt_info.eov = 0;
    for (int pkt_count = 0; pkt_count < num_packets; pkt_count++) begin
      int exp_byte_length;
      int exp_word_length;
      int exp_num_mdata;
      chdr_word_t temp_queue[$];

      // Calculate the length of the next packet
      if (pkt_count < num_packets-1) begin
        exp_byte_length = MAX_PYLD_BYTES;
      end else begin
        // Last packet's length is whatever is left
        exp_byte_length = send_pkt.data_bytes - pkt_count * MAX_PYLD_BYTES;
      end
      exp_word_length = (exp_byte_length + (CHDR_W/8) - 1) / (CHDR_W/8);

      // Calculate how much metadata we have left for the next packet
      exp_num_mdata = send_pkt.metadata.size() - (pkt_count * 31);
      if (exp_num_mdata > 31) exp_num_mdata = 31;   // Up to 31 words per packet
      if (exp_num_mdata < 0) exp_num_mdata = 0;     // No less than 0

      // Receive the next packet
      bfm.recv_items_adv(recv_items, recv_pkt.metadata, recv_pkt.pkt_info);
      recv_pkt.data_bytes = recv_items.size() * (ITEM_W/8);
      recv_pkt.data       = chdr_data.item_to_chdr(recv_items);

      // Check the data length of the received packet
      `ASSERT_ERROR(
        exp_byte_length == recv_pkt.data_bytes,
        $sformatf(
          "Length of packet %0d didn't match (received %0d, expected %0d)",
          pkt_count, recv_pkt.data_bytes, exp_byte_length
        )
      );

      // Check the data contents
      temp_queue = send_pkt.data[data_index:data_index+exp_word_length-1];
      `ASSERT_ERROR(
        chdr_data.chdr_equal(temp_queue, recv_pkt.data),
        "Data did not match"
      );

      // Check the metadata contents
      if (exp_num_mdata > 0) begin
        temp_queue = send_pkt.metadata[mdata_index:mdata_index+exp_num_mdata-1];
        `ASSERT_ERROR(
          chdr_data.chdr_equal(temp_queue, recv_pkt.metadata),
          "Metadata did not match"
        );
      end

      // Check the pkt_info
      if (pkt_count < num_packets-1) begin
        // Not the last packet
        `ASSERT_ERROR(
          pkt_info == recv_pkt.pkt_info,
          $sformatf("Packet info did not match on packet %0d", pkt_count)
        );
      end else begin
        // This is the last packet
        pkt_info.eob = send_pkt.pkt_info.eob;
        pkt_info.eov = send_pkt.pkt_info.eov;
        `ASSERT_ERROR(
          pkt_info == recv_pkt.pkt_info,
          $sformatf("Packet info did not match on packet %0d (last packet)", pkt_count)
        );
      end

      // Update counters for next iteration
      if (pkt_info.has_time) pkt_info.timestamp += exp_word_length * CHDR_W/ITEM_W;
      data_index  += exp_word_length;
      mdata_index += exp_num_mdata;
    end

  endtask : test_send_packets_items


  task test_recv_packets_items(test_variant_t test_type);
    packet_t send_pkt, recv_pkt;
    item_t send_items[$], recv_items[$];
    chdr_word_t recv_metadata[$];

    // Generate a random packet
    send_pkt = rand_pkt(1, 1);
    if (VERBOSE) begin
      $display("test_recv_packets_items:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Set the flags, if needed
    case (test_type)
      TEST_EOB :
        send_pkt.pkt_info.eob = 1;
      TEST_EOV :
        send_pkt.pkt_info.eov = 1;
    endcase

    // Send the packet data, all at once
    send_items = chdr_data.chdr_to_item(send_pkt.data, send_pkt.data_bytes);
    bfm.send_packets_items(send_items, send_pkt.metadata, send_pkt.pkt_info);

    // Receive the data, all at once
    case (test_type)
      TEST_NUM_ITEMS :
        bfm.recv_packets_items(
          recv_items, send_items.size());
      TEST_EOB :
        bfm.recv_packets_items(
          recv_items, /* num_samps */, 1, 0);
      TEST_EOV :
        bfm.recv_packets_items
        (recv_items, /* num_samps */, 0, 1);
    endcase

    // Check if we received what we sent
    `ASSERT_ERROR(chdr_data.item_equal(send_items, recv_items), "Data did not match");

  endtask : test_recv_packets_items


  task test_recv_packets_items_adv(test_variant_t test_type);
    packet_t send_pkt, recv_pkt;
    item_t send_items[$], recv_items[$];
    chdr_word_t recv_metadata[$];

    // Generate a random packet
    send_pkt = rand_pkt(1, 1);
    if (VERBOSE) begin
      $display("test_recv_packets_items_adv:  data_bytes = %04d, num_mdata = %02d, pkt_info = %p",
        send_pkt.data_bytes, send_pkt.metadata.size(), send_pkt.pkt_info);
    end

    // Set the flags, if needed
    case (test_type)
      TEST_EOB :
        send_pkt.pkt_info.eob = 1;
      TEST_EOV :
        send_pkt.pkt_info.eov = 1;
    endcase

    // Send the packet data, all at once
    send_items = chdr_data.chdr_to_item(send_pkt.data, send_pkt.data_bytes);
    bfm.send_packets_items(send_items, send_pkt.metadata, send_pkt.pkt_info);

    // Receive the data, all at once
    case (test_type)
      TEST_NUM_ITEMS :
        bfm.recv_packets_items_adv(
          recv_items, recv_pkt.metadata, recv_pkt.pkt_info, send_items.size());
      TEST_EOB :
        bfm.recv_packets_items_adv(
          recv_items, recv_pkt.metadata, recv_pkt.pkt_info, /* num_samps */, 1, 0);
      TEST_EOV :
        bfm.recv_packets_items_adv
        (recv_items, recv_pkt.metadata, recv_pkt.pkt_info, /* num_samps */, 0, 1);
    endcase

    // Check if we received what we sent
    `ASSERT_ERROR(chdr_data.item_equal(send_items, recv_items), "Data did not match");
    `ASSERT_ERROR(chdr_data.chdr_equal(send_pkt.metadata, recv_pkt.metadata), "Metadata did not match");
    `ASSERT_ERROR(send_pkt.pkt_info == recv_pkt.pkt_info, "Packet info did not match");

  endtask : test_recv_packets_items_adv


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    //-------------------------------------------------------------------------
    // Initialization
    //-------------------------------------------------------------------------

    // Generate a string for the name of this instance of the testbench
    tb_name = $sformatf(
      "ChdrIfaceBfm_tb\nCHDR_W = %0d, ITEM_W = %0d",
      CHDR_W, ITEM_W
    );

    // We're not testing flow control, only correct data generate and
    // extraction, so there's not need to stall on the CHDR interface.
    bfm.set_slave_stall_prob(0);
    bfm.set_master_stall_prob(0);

    // Start the BFM runnings
    bfm.run();

    test.start_tb(tb_name, 100ms);


    //-------------------------------------------------------------------------
    // Test Sequences
    //-------------------------------------------------------------------------

    // There is an internal bug in Vivado 2021.1 that prevents all of these
    // tests to run at the same time. Therefore, we are splitting the *_adv
    // tests and running them separately in the *all_tb file.
    if (ADV) begin
      test.start_test("Test send_packets_items() / recv_packets_items_adv(num_items)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items_adv(TEST_NUM_ITEMS);
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items_adv(num_items)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items_adv(TEST_NUM_ITEMS);
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items_adv(eob)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items_adv(TEST_EOB);
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items_adv(eov)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items_adv(TEST_EOV);
      test.end_test();

    end else begin
      test.start_test("Test send() / recv()", 10ms);
      for (int i = 0; i < 1000; i++) test_send(TEST_RECV);
      test.end_test();

      test.start_test("Test send() / recv_adv()", 10ms);
      for (int i = 0; i < 1000; i++) test_send(TEST_RECV_ADV);
      test.end_test();

      test.start_test("Test send_items() / recv_items_adv()", 10ms);
      for (int i = 0; i < 1000; i++) test_send_items();
      test.end_test();

      test.start_test("Test send_packets() / recv_adv()", 10ms);
      for (int i = 0; i < 1000; i++) test_send_packets();
      test.end_test();

      test.start_test("Test send_packets_items() / recv_items_adv()", 10ms);
      for (int i = 0; i < 1000; i++) test_send_packets_items();
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items(num_items)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items(TEST_NUM_ITEMS);
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items(eob)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items(TEST_EOB);
      test.end_test();

      test.start_test("Test send_packets_items() / recv_packets_items(eov)", 10ms);
      for (int i = 0; i < 1000; i++) test_recv_packets_items(TEST_EOV);
      test.end_test();
    end

    // Make sure we don't get any more packets. Wait for more than a packet of
    // worth of time the check if we've received anything.
    #(CLOCK_PER * WPP * 8);
    `ASSERT_ERROR(bfm.num_received() == 0, "Received unexpected packets");

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    rfnoc_chdr_clk_gen.kill();

  end

endmodule : ChdrIfaceBfm_tb
