//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_resize_tb
//
// Description:
//
//   Testbench for chdr_resize.
//

`default_nettype none


module chdr_resize_tb #(
  parameter I_CHDR_W = 64,
  parameter O_CHDR_W = 128,
  parameter I_DATA_W = I_CHDR_W,
  parameter O_DATA_W = O_CHDR_W,
  parameter USER_W   = 4,
  parameter PIPELINE = "NONE"
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  `define MIN(A, B)      ((A)<(B)?(A):(B))
  `define DIV_CEIL(N, D) (((N)+(D)-1)/(D))    // ceiling(N/D)

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgChdrBfm::*;
  import PkgRandom::*;

  // Clock periods
  localparam real CLK_PERIOD = 10.0;

  // Widths for BFMs to use
  localparam ITEM_W = 8;

  // Test parameters
  localparam NUM_PACKETS     = 1000; // Number of packets to test
  localparam MAX_MDATA_WORDS = 31;   // Maximum number of metadata words (31
                                     // is the max supported by CHDR).
  localparam MAX_PYLD_WORDS  = 64;   // Maximum number of payload words.
  localparam USE_RANDOM      = 1;    // Use random vs. sequential data.
  localparam DEBUG           = 0;    // Display extra debug info


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk;
  bit rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Connections to DUT as interfaces:
  AxiStreamIf #(I_CHDR_W) m_chdr (clk, 1'b0);
  AxiStreamIf #(O_CHDR_W) s_chdr (clk, 1'b0);

  // CHDR BFMs. Because the input and output have different CHDR widths, we
  // need two BFMs, one for each CHDR width.
  ChdrBfm #(I_CHDR_W) m_bfm = new(m_chdr, null);
  ChdrBfm #(O_CHDR_W) s_bfm = new(null, s_chdr);

  // CHDR data types
  typedef ChdrPacket #(I_CHDR_W)::ChdrPacket_t      IChdrPacket_t;
  typedef ChdrPacket #(O_CHDR_W)::ChdrPacket_t      OChdrPacket_t;
  typedef ChdrData #(I_CHDR_W, ITEM_W)::chdr_word_t i_chdr_word_t;
  typedef ChdrData #(O_CHDR_W, ITEM_W)::chdr_word_t o_chdr_word_t;


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------
  //
  // To simplify the testbench we randomly generate CHDR packets, resize them,
  // then change their size back. This tests up-sizing and down-sizing
  // simultaneously and simplifies output checking. It's possible, however,
  // that we make a mistake up-sizing, then undo that mistake when downsizing,
  // so there is some risk to this simplification.
  //
  //---------------------------------------------------------------------------

  wire [I_DATA_W-1:0] i_data_tdata;
  wire [  USER_W-1:0] i_data_tuser;
  wire                i_data_tlast;
  wire                i_data_tvalid;
  wire                i_data_tready;

  wire [O_DATA_W-1:0] o_data_tdata;
  wire [  USER_W-1:0] o_data_tuser;
  wire                o_data_tlast;
  wire                o_data_tvalid;
  wire                o_data_tready;

  chdr_resize #(
    .I_CHDR_W (I_CHDR_W),
    .O_CHDR_W (O_CHDR_W),
    .I_DATA_W (I_DATA_W),
    .O_DATA_W (O_DATA_W),
    .USER_W   (USER_W),
    .PIPELINE (PIPELINE)
  ) chdr_resize_i (
    .clk           (clk),
    .rst           (rst),
    .i_chdr_tdata  (i_data_tdata),
    .i_chdr_tuser  (i_data_tuser),
    .i_chdr_tlast  (i_data_tlast),
    .i_chdr_tvalid (i_data_tvalid),
    .i_chdr_tready (i_data_tready),
    .o_chdr_tdata  (o_data_tdata),
    .o_chdr_tuser  (o_data_tuser),
    .o_chdr_tlast  (o_data_tlast),
    .o_chdr_tvalid (o_data_tvalid),
    .o_chdr_tready (o_data_tready)
  );


  //---------------------------------------------------------------------------
  // Bus Resizer for DUT
  //---------------------------------------------------------------------------
  //
  // Resize the DUT inputs/outputs to match match the widths used by our BFMs.
  //
  //---------------------------------------------------------------------------

  localparam IN_WORD_W  = `MIN(I_CHDR_W, I_DATA_W);
  localparam OUT_WORD_W = `MIN(O_CHDR_W, O_DATA_W);
  localparam OUT_KEEP_W = O_CHDR_W/OUT_WORD_W;

  wire [I_CHDR_W-1:0] i_chdr_tdata;
  wire                i_chdr_tlast;
  wire                i_chdr_tvalid;
  wire                i_chdr_tready;

  wire [  O_CHDR_W-1:0] o_chdr_tdata_unmasked;
  wire [  O_CHDR_W-1:0] o_chdr_tdata;
  wire [OUT_KEEP_W-1:0] o_chdr_tkeep;
  wire                  o_chdr_tlast;
  wire                  o_chdr_tvalid;
  wire                  o_chdr_tready;

  axis_width_conv #(
    .WORD_W    (IN_WORD_W),
    .IN_WORDS  (I_CHDR_W/IN_WORD_W),
    .OUT_WORDS (I_DATA_W/IN_WORD_W),
    .SYNC_CLKS (1),
    .PIPELINE  ("NONE")
  ) axis_width_conv_in (
    .s_axis_aclk   (clk),
    .s_axis_rst    (rst),
    .s_axis_tdata  (i_chdr_tdata),
    .s_axis_tkeep  ('1),
    .s_axis_tlast  (i_chdr_tlast),
    .s_axis_tvalid (i_chdr_tvalid),
    .s_axis_tready (i_chdr_tready),
    .m_axis_aclk   (clk),
    .m_axis_rst    (rst),
    .m_axis_tdata  (i_data_tdata),
    .m_axis_tkeep  (),
    .m_axis_tlast  (i_data_tlast),
    .m_axis_tvalid (i_data_tvalid),
    .m_axis_tready (i_data_tready)
  );

  axis_width_conv #(
    .WORD_W    (OUT_WORD_W),
    .IN_WORDS  (O_DATA_W/OUT_WORD_W),
    .OUT_WORDS (O_CHDR_W/OUT_WORD_W),
    .SYNC_CLKS (1),
    .PIPELINE  ("NONE")
  ) axis_width_conv_out (
    .s_axis_aclk   (clk),
    .s_axis_rst    (rst),
    .s_axis_tdata  (o_data_tdata),
    .s_axis_tkeep  ('1),
    .s_axis_tlast  (o_data_tlast),
    .s_axis_tvalid (o_data_tvalid),
    .s_axis_tready (o_data_tready),
    .m_axis_aclk   (clk),
    .m_axis_rst    (rst),
    .m_axis_tdata  (o_chdr_tdata_unmasked),
    .m_axis_tkeep  (o_chdr_tkeep),
    .m_axis_tlast  (o_chdr_tlast),
    .m_axis_tvalid (o_chdr_tvalid),
    .m_axis_tready (o_chdr_tready)
  );


  // Invalidate the bits we shouldn't be keeping by changing them to X. This
  // ensures we aren't checking bits that aren't there and thinking they're OK
  // because they happen to be 0.
  reg [O_CHDR_W-1:0] o_chdr_keep_mask;

  always_comb begin
    for (int w = 0; w < OUT_KEEP_W; w++) begin
      if (o_chdr_tkeep[w] == 1'b1) begin
        o_chdr_keep_mask[w*OUT_WORD_W+:OUT_WORD_W] = {OUT_WORD_W{1'b1}};
      end else begin
        o_chdr_keep_mask[w*OUT_WORD_W+:OUT_WORD_W] = {OUT_WORD_W{1'bX}};
      end
    end
  end

  // This mask operation leaves the bits we're keeping unmodified but changes
  // the ones we aren't keeping to X.
  assign o_chdr_tdata = o_chdr_tdata_unmasked & o_chdr_keep_mask;


  //---------------------------------------------------------------------------
  // BFM Connections
  //---------------------------------------------------------------------------

  // Input
  assign i_chdr_tdata  = m_chdr.tdata;
  assign i_chdr_tlast  = m_chdr.tlast;
  assign i_chdr_tvalid = m_chdr.tvalid;
  assign m_chdr.tready = i_chdr_tready;

  // Output
  assign s_chdr.tdata  = o_chdr_tdata;
  assign s_chdr.tlast  = o_chdr_tlast;
  assign s_chdr.tvalid = o_chdr_tvalid;
  assign o_chdr_tready = s_chdr.tready;


  //---------------------------------------------------------------------------
  // Debug Monitors
  //---------------------------------------------------------------------------
  //
  // Display packet info as packets go in/out of the DUT to make packets easier
  // to find in the simulator.
  //
  //---------------------------------------------------------------------------

  if (DEBUG) begin
    bit i_chdr_sop   = 1;
    bit mid_chdr_sop = 1;
    bit o_chdr_sop   = 1;

    always @(posedge clk) begin
      chdr_header_t header;

      if (i_chdr_tvalid && i_chdr_tready) begin
        if (i_chdr_sop) begin
          header = i_chdr_tdata;
          $display("In  packet, @%0t: 0x%16X %p", $realtime, header, header);
          i_chdr_sop <= 0;
        end
        i_chdr_sop <= i_chdr_tlast;
      end

      if (o_chdr_tvalid && o_chdr_tready) begin
        if (o_chdr_sop) begin
          header = o_chdr_tdata;
          $display("Out packet, @%0t: 0x%16X %p", $realtime, header, header);
          o_chdr_sop <= 0;
        end
        o_chdr_sop <= o_chdr_tlast;
      end
    end
  end


  //---------------------------------------------------------------------------
  // TUSER Generation
  //---------------------------------------------------------------------------
  //
  // Create a simple counter on TUSER than increments for each packet.
  //
  //---------------------------------------------------------------------------

  reg [USER_W-1:0] i_user_count = 0;

  always @(posedge clk) begin
    if (rst) begin
      i_user_count <= 0;
    end else if (i_data_tvalid && i_data_tready) begin
      if (i_data_tlast) begin
        i_user_count <= i_user_count + 1;
      end
    end
  end

  // Ensure that i_data_tuser is only valid during the packet to guarantee the
  // DUT doesn't sample it outside that.
  assign i_data_tuser =
    (i_data_tvalid && i_data_tready) ? i_user_count : 'X;


  //---------------------------------------------------------------------------
  // TUSER Checking
  //---------------------------------------------------------------------------
  //
  // Verify that each output packet has the expected count on TUSER.
  //
  //---------------------------------------------------------------------------

  reg [USER_W-1:0] o_user_count = 0;

  always @(posedge clk) begin
    if (rst) begin
      o_user_count <= 0;
    end else if (o_data_tvalid && o_data_tready) begin
      `ASSERT_ERROR(
        o_data_tuser == o_user_count,
        "TUSER output doesn't match expected count."
      );
      if (o_data_tlast) begin
        o_user_count <= o_user_count+1;
      end
    end
  end


  //---------------------------------------------------------------------------
  // Helper Logic
  //---------------------------------------------------------------------------

  // Convert the input packet to the output packet, based on the configured
  // I_CHDR_W and O_CHDR_W. Returns the expected output packet. This function
  // does to the input ChdrPacket what the DUT is supposed to do to the CHDR
  // AXI-Stream packet.
  function automatic OChdrPacket_t convert_packet(IChdrPacket_t i_packet);
    OChdrPacket_t o_packet = new;
    int num_mdata;
    int max_num_mdata = 2**$bits(o_packet.header.num_mdata)-1;

    // Check if we're doing any conversion or resizing
    if (I_CHDR_W == O_CHDR_W && I_CHDR_W == I_DATA_W && O_CHDR_W == O_DATA_W) begin
      // We don't modify the packet at all in this case. We should just pass it
      // through.
      o_packet = OChdrPacket_t'(i_packet.copy());
      return o_packet;
    end

    //---------------------------------
    // Update the header

    o_packet.header = i_packet.header;

    // NumMData
    if (I_CHDR_W > O_CHDR_W) begin
      // Make sure there isn't too much metadata for a smaller CHDR_W packet
      num_mdata = i_packet.header.num_mdata * I_CHDR_W / O_CHDR_W;
      if (num_mdata > max_num_mdata) num_mdata = max_num_mdata;
    end else begin
      // Round up to the nearest whole O_CHDR_W word
      num_mdata = `DIV_CEIL(i_packet.header.num_mdata * I_CHDR_W, O_CHDR_W);
    end
    o_packet.header.num_mdata = num_mdata;

    // Length
    o_packet.header.length =
      // Header
      (O_CHDR_W/8) +
      // Timestamp (goes in same word as header, unless O_CHDR_W is 64-bit)
      ((o_packet.header.pkt_type == CHDR_DATA_WITH_TS && O_CHDR_W == 64) ? (O_CHDR_W/8) : 0) +
      // Metadata
      (O_CHDR_W/8)*o_packet.header.num_mdata +
      // Payload data. We expect the length of the output packet to be a
      // multiple of CHDR_W for management packets.
      ((o_packet.header.pkt_type == CHDR_MANAGEMENT) ?
        `DIV_CEIL(i_packet.data_bytes(), I_CHDR_W/8) * (O_CHDR_W/8) : i_packet.data_bytes());

    // Timestamp
    o_packet.timestamp = i_packet.timestamp;

    //---------------------------------
    // Copy the data and metadata

    if (I_CHDR_W > O_CHDR_W) begin
      // Drop any metadata beyond what CHDR allows
      o_packet.metadata = ChdrData#(I_CHDR_W, O_CHDR_W)::chdr_to_item(i_packet.metadata,
        num_mdata * O_CHDR_W/8);
      // Drop any O_CHDR_W words that aren't part of the payload
      o_packet.data = ChdrData#(I_CHDR_W, O_CHDR_W)::chdr_to_item(i_packet.data,
        `DIV_CEIL(i_packet.data_bytes(), O_CHDR_W/8) * O_CHDR_W/8);
    end else begin
      o_chdr_word_t last_mdata;
      o_packet.metadata = ChdrData#(O_CHDR_W, I_CHDR_W)::item_to_chdr(i_packet.metadata);
      o_packet.data     = ChdrData#(O_CHDR_W, I_CHDR_W)::item_to_chdr(i_packet.data);

      // When I_CHDR_W < O_CHDR_W, the number of metadata bytes might not be a
      // multiple of O_CHDR_W. The DUT should zero these extra bytes.
      if (o_packet.metadata.size() > 0) begin
        last_mdata = o_packet.metadata[$];
        for (int i = 0; i < o_packet.mdata_bytes()-i_packet.mdata_bytes(); i++) begin
          last_mdata[$bits(o_chdr_word_t)-i*8-1 -: 8] = 8'd0;
        end
        o_packet.metadata[$] = last_mdata;
      end
    end

    //---------------------------------
    // Copy management packet data

    if (o_packet.header.pkt_type == CHDR_MANAGEMENT) begin
      // Management packets don't get serialized, so we just copy each word,
      // ignoring the upper bits.
      o_packet.data = {};
      for (int i = 0; i < i_packet.data.size(); i++) begin
        if (i == 0) begin
          // Update the CHDR width in the management header word
          o_chdr_word_t word;
          word = i_packet.data[i];
          word[47:45] = translate_chdr_w(O_CHDR_W);
          o_packet.data.push_back(word);
        end else begin
          o_packet.data.push_back(i_packet.data[i]);
        end
      end
    end

    return o_packet;
  endfunction : convert_packet


  // Compare the output packet to what we expect the output packet to be.
  function automatic string compare_packets(
    input OChdrPacket_t packet,
    input OChdrPacket_t exp_packet
  );
    string msg = "";

    // Check that the headers match (including num_mdata and length)
    if(packet.header != exp_packet.header) begin
      msg = { msg, "Headers do not match\n" };
    end

    // Check that the timestamps match
    if (exp_packet.header.pkt_type == CHDR_DATA_WITH_TS) begin
      if(packet.timestamp != exp_packet.timestamp) begin
        msg = { msg, "Timestamps do not match\n" };
      end
    end

    // Check that the metadata matches
    if(!exp_packet.chdr_word_queues_equal(
      packet.metadata, exp_packet.metadata, packet.mdata_bytes())
    ) begin
      msg = { msg, "Metadata does not match\n" };
    end

    // Check that the payloads match
    if(!exp_packet.chdr_word_queues_equal(
      packet.data, exp_packet.data, exp_packet.data_bytes())
    ) begin
      msg = { msg, "Payloads do not match\n" };
    end

    return msg;
  endfunction : compare_packets


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // Perform a randomized test with the given stall probabilities in the input
  // and output ports.
  task automatic test_random(int in_stall_prob, int out_stall_prob);
    longint word_count = 0;
    bit enable_input = 0;
    string msg;
    mailbox #(IChdrPacket_t) packets = new;

    msg = $sformatf("Test Random Packets (%0d%%, %0d%%)",
      in_stall_prob, out_stall_prob);
    test.start_test(msg, 10ms);

    m_bfm.set_master_stall_prob(in_stall_prob);
    s_bfm.set_slave_stall_prob(out_stall_prob);

    fork
      //-------------------------------
      // Input Process
      //-------------------------------

      begin : input_process
        IChdrPacket_t    chdr_packet = new;
        i_chdr_word_t    data[$];
        i_chdr_word_t    mdata[$];
        chdr_header_t    header;
        chdr_timestamp_t timestamp;
        int              data_byte_length;
        int              count;

        repeat (NUM_PACKETS) begin
          //-------------------------------
          // Generate a random packet
          //-------------------------------

          // Start with a random header. Make sure the packet type is legal.
          do begin
            header = Rand#($bits(chdr_header_t))::rand_logic();
          end while (header.pkt_type == CHDR_RESERVED_0 || header.pkt_type == CHDR_RESERVED_1);

          // Generate timestamp
          if (header.pkt_type == CHDR_DATA_WITH_TS) begin
            if (USE_RANDOM) timestamp = Rand#($bits(chdr_header_t))::rand_logic();
            else timestamp = 64'h0123456789ABCDEF;
          end else begin
            timestamp = 0;
          end

          // Generate random metadata (50% chance of no metadata)
          mdata = {};
          if ($urandom_range(0, 1)) begin
            count = 0;
            repeat ($urandom_range(1, MAX_MDATA_WORDS)) begin
              if (USE_RANDOM) mdata.push_back(Rand#(I_CHDR_W)::rand_logic());
              else mdata.push_back(64'hA1000000 + count++);
            end
          end

          // Generate random data (always at least one word)
          data = {};
          count = 0;
          repeat ($urandom_range(1, MAX_PYLD_WORDS)) begin
            if (USE_RANDOM) data.push_back(Rand#(I_CHDR_W)::rand_logic());
            else data.push_back(64'hB2000000 + count++);
          end

          // Calculate the size of data minus one word, in bytes
          data_byte_length = (data.size()-1) * (I_CHDR_W/8);
          // Add from 1 byte to a full word of bytes to test partially filling
          // the last word.
          if (header.pkt_type == CHDR_MANAGEMENT) begin
            // For management packets, the spec is not explicit about whether
            // the length must include the padding of the last word. We assume
            // the worst case, that either case is possible and we expect the
            // DUT to handle both correctly.
            data_byte_length += $urandom_range(1, I_CHDR_W/64) * 8;
          end else begin
            data_byte_length += $urandom_range(1, I_CHDR_W/8);
          end

          // Build packet
          chdr_packet.write_raw(
            header,
            data,
            mdata,
            timestamp,
            data_byte_length
          );

          // Queue the packet
          m_bfm.put_chdr(chdr_packet);

          // Queue up what we sent for the output process to check
          packets.put(chdr_packet.copy());
        end
        $display("Done inputting packets.");
      end : input_process

      //-------------------------------
      // Output Process
      //-------------------------------

      begin : output_process
        IChdrPacket_t i_packet;
        OChdrPacket_t o_packet;
        OChdrPacket_t exp_packet;
        int packet_count;
        string msg;

        repeat (NUM_PACKETS) begin
          s_bfm.get_chdr(o_packet);
          packets.get(i_packet);
          exp_packet = convert_packet(i_packet);
          msg = compare_packets(o_packet, exp_packet);
          if(msg != "") begin
            $display("Sent packet:");
            i_packet.print(0);
            $display("Received packet:");
            o_packet.print(0);
            $display("Expected packet:");
            exp_packet.print(0);
            `ASSERT_ERROR(0, $sformatf(
              "Output packet is incorrect for the following reasons:\n%s", msg)
            );
          end
        end
        $display("Done processing packets.");
      end : output_process
    join

    test.end_test();

  endtask : test_random


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string msg;
    string tb_name;

    tb_name = $sformatf( {
      "chdr_resize_tb\n",
      "I_CHDR_W = %03d\n",
      "O_CHDR_W = %03d\n",
      "I_DATA_W = %03d\n",
      "O_DATA_W = %03d\n",
      "PIPLINE  = %s" },
      I_CHDR_W, O_CHDR_W, I_DATA_W, O_DATA_W, PIPELINE
    );
    test.start_tb(tb_name, 100ms);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen.start();

    // Start the BFM
    m_bfm.run();
    s_bfm.run();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset", 10us);
    clk_gen.reset();
    if (rst) @rst;
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test_random(50, 50);  // Test 50% push-back
    test_random( 0,  0);  // Test no push-back
    test_random(50,  0);  // Test for underflow
    test_random( 0, 50);  // Test for overflow

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();

  end : tb_main

endmodule : chdr_resize_tb


`default_nettype wire
