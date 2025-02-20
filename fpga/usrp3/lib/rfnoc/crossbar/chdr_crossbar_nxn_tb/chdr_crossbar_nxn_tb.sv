//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description: Testbench for chdr_crossbar_nxn.
//
// Parameters:
//
//   NUM_PORTS       : Size of crossbar to test will be NUM_PORTS x NUM_PORTS.
//   CHDR_WIDTHS     : Descending array of NUM_PORT integers representing the
//                     width of each crossbar port. The width of port n is
//                     given by CHDR_WIDTHS[n].
//   ROUTES          : Descending 2D array representing which crossbar routes
//                     to enable. This is an NPORTS x NPORTS array where bit
//                     ROUTES[A][B] corresponds to the path from input port A
//                     to output port B. A '1' indicates the logic for that
//                     route is included.
//   TEST_BAD_ROUTES : When 1, test all routes, whether enabled or not. When 0,
//                     test only the enabled routes specified in the ROUTES
//                     parameter.
//   NUM_PKTS        : Number of packets to test on each port. This determines
//                     the length of the simulation.

//   USE_MGMT_PORTS  : Indicates whether or not to use the management port for
//                     crossbar configuration. Set to 1 to use the CHDR
//                     management port. Set to 0 to use the CtrlPort instead.
//
//   Note: The array parameters above are SystemVerilog arrays, but they are
//         ordered to match the DUT, which uses flat Verilog arrays.
//
//   CHDR_WIDTHS Bit Mapping Example (4x4):
//
//     Port:   3   2   1   0
//             ↓   ↓   ↓   ↓
//          '{64, 64, 64, 64}
//
//   ROUTES Bit Mapping Example (4x4):
//
//            Output Port 3210
//                        ↓↓↓↓
//     Input Port 3 → '{'b1111,
//     Input Port 2 →   'b1111,
//     Input Port 1 →   'b1111,
//     Input Port 0 →   'b1111}
//

`default_nettype none


module chdr_crossbar_nxn_tb #(
  parameter int                                NUM_PORTS       = 2,
  parameter bit [NUM_PORTS-1:0][31:0]          CHDR_WIDTHS     = {NUM_PORTS{32'd64}},
  parameter bit [NUM_PORTS-1:0][NUM_PORTS-1:0] ROUTES          = {NUM_PORTS**2{1'b1}},
  parameter bit                                TEST_BAD_ROUTES = 1,
  parameter int                                NUM_PKTS        = 64,
  parameter bit                                USE_MGMT_PORTS  = 0
);
  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgChdrBfm::*;
  import rfnoc_chdr_utils_pkg::*;
  import PkgChdrData::*;


  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  // Determine the largest port size, which will be used as the signal width
  // for the crossbar ports.
  function static int max_port_width();
    static int max = 0;
    for (int i = 0; i < NUM_PORTS; i++) begin
      if (CHDR_WIDTHS[i] > max) max = CHDR_WIDTHS[i];
    end
    return max;
  endfunction : max_port_width


  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  localparam real CLK_PERIOD    = 10.0;
  localparam bit  DEBUG         = 0;    // Set to 1 to enable more log prints
  localparam int  MAX_PKT_BYTES = 512;  // Max packet length in bytes to test

  localparam int  BYTE_MTU       = $clog2(MAX_PKT_BYTES);
  localparam int  PORT_W         = max_port_width();
  // MAX_PYLD_BYTES is MTU minus two CHDR words for the header
  localparam int  MAX_PYLD_BYTES = MAX_PKT_BYTES - 2*(PORT_W/8);

  // DUT default parameters
  localparam [15:0] PROTOVER       = {8'd1, 8'd0};
  localparam [7:0]  DEFAULT_PORT   = 0;
  localparam        ROUTE_TBL_SIZE = NUM_PORTS**2;  // One route for every port combination
  localparam        MUX_ALLOC      = "ROUND-ROBIN";
  localparam        OPTIMIZE       = "AREA";
  localparam [7:0]  NPORTS_MGMT    = USE_MGMT_PORTS ? NUM_PORTS : 0;
  localparam        EXT_RTCFG_PORT = 1;
  localparam        DEVICE_ID      = 16'hBEEF;


  //---------------------------------------------------------------------------
  // Inter-process Communication
  //---------------------------------------------------------------------------

  // Event to indicate if the mailboxes have been initialized.
  event start_consumer;

  // Mailbox to communicate how many packets to expect. These mailboxes are
  // created prior to the start_consumer event.
  mailbox #(int) mb_num_pkts [NUM_PORTS];

  // Semaphore to track the number of output ports that have received their
  // expected number of packets.
  semaphore ports_done = new();


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;

  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0)) clk_gen (.clk(clk), .rst(rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  // Interfaces for AXI-Stream
  AxiStreamIf #(PORT_W) chdr_to_dut   [NUM_PORTS] (clk, rst);
  AxiStreamIf #(PORT_W) chdr_from_dut [NUM_PORTS] (clk, rst);

  // Bus functional model for each of the CHDR ports
  ChdrBfm #(PORT_W) chdr_bfm [NUM_PORTS];

  // Create the BFM instances
  for (genvar i = 0; i < NUM_PORTS; i++) begin : gen_bfm_creation
    initial chdr_bfm[i] = new(chdr_to_dut[i], chdr_from_dut[i]);
  end


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  logic [NUM_PORTS-1:0][PORT_W-1:0] dut_in_tdata;
  logic [NUM_PORTS-1:0][       0:0] dut_in_tlast;
  logic [NUM_PORTS-1:0][       0:0] dut_in_tvalid;
  logic [NUM_PORTS-1:0][       0:0] dut_in_tready;
  logic [NUM_PORTS-1:0][PORT_W-1:0] dut_out_tdata;
  logic [NUM_PORTS-1:0][       0:0] dut_out_tlast;
  logic [NUM_PORTS-1:0][       0:0] dut_out_tvalid;
  logic [NUM_PORTS-1:0][       0:0] dut_out_tready;

  logic        ext_rtcfg_stb  = 1'b0;
  logic [15:0] ext_rtcfg_addr = 'X;
  logic [31:0] ext_rtcfg_data = 'X;
  logic        ext_rtcfg_ack;

  chdr_crossbar_nxn #(
    .PROTOVER      (PROTOVER      ),
    .PORT_W        (PORT_W        ),
    .NPORTS        (NUM_PORTS     ),
    .CHDR_WIDTHS   (CHDR_WIDTHS   ),
    .DEFAULT_PORT  (DEFAULT_PORT  ),
    .ROUTES        (ROUTES        ),
    .BYTE_MTU      (BYTE_MTU      ),
    .ROUTE_TBL_SIZE(ROUTE_TBL_SIZE),
    .MUX_ALLOC     (MUX_ALLOC     ),
    .OPTIMIZE      (OPTIMIZE      ),
    .NPORTS_MGMT   (NPORTS_MGMT   ),
    .EXT_RTCFG_PORT(EXT_RTCFG_PORT)
  ) chdr_crossbar_nxn_i (
    .clk           (clk           ),
    .reset         (rst           ),
    .device_id     (DEVICE_ID     ),
    .s_axis_tdata  (dut_in_tdata  ),
    .s_axis_tlast  (dut_in_tlast  ),
    .s_axis_tvalid (dut_in_tvalid ),
    .s_axis_tready (dut_in_tready ),
    .m_axis_tdata  (dut_out_tdata ),
    .m_axis_tlast  (dut_out_tlast ),
    .m_axis_tvalid (dut_out_tvalid),
    .m_axis_tready (dut_out_tready),
    .ext_rtcfg_stb (ext_rtcfg_stb ),
    .ext_rtcfg_addr(ext_rtcfg_addr),
    .ext_rtcfg_data(ext_rtcfg_data),
    .ext_rtcfg_ack (ext_rtcfg_ack )
  );


  //---------------------------------------------------------------------------
  // Resize
  //---------------------------------------------------------------------------
  //
  // Resize the input and output to each port on the crossbar to the same size.
  // This allows us to use BFMs of the same type to interface to each port
  // without having to worry about the port-width.
  //
  //---------------------------------------------------------------------------

  for (genvar port_index = 0; port_index < NUM_PORTS; port_index++) begin : gen_resize
    chdr_resize #(
      .I_CHDR_W(PORT_W                 ),
      .O_CHDR_W(CHDR_WIDTHS[port_index]),
      .PIPELINE("NONE"                 )
    ) chdr_resize_input (
      .clk          (clk                           ),
      .rst          (rst                           ),
      .i_chdr_tdata (chdr_to_dut[port_index].tdata ),
      .i_chdr_tuser ('0                            ),
      .i_chdr_tlast (chdr_to_dut[port_index].tlast ),
      .i_chdr_tvalid(chdr_to_dut[port_index].tvalid),
      .i_chdr_tready(chdr_to_dut[port_index].tready),
      .o_chdr_tdata (dut_in_tdata[port_index]      ),
      .o_chdr_tuser (                              ),
      .o_chdr_tlast (dut_in_tlast[port_index]      ),
      .o_chdr_tvalid(dut_in_tvalid[port_index]     ),
      .o_chdr_tready(dut_in_tready[port_index]     )
    );

    chdr_resize #(
      .I_CHDR_W(CHDR_WIDTHS[port_index]),
      .O_CHDR_W(PORT_W                 ),
      .PIPELINE("NONE"                 )
    ) chdr_resize_output (
      .clk          (clk                             ),
      .rst          (rst                             ),
      .i_chdr_tdata (dut_out_tdata[port_index]       ),
      .i_chdr_tuser ('0                              ),
      .i_chdr_tlast (dut_out_tlast[port_index]       ),
      .i_chdr_tvalid(dut_out_tvalid[port_index]      ),
      .i_chdr_tready(dut_out_tready[port_index]      ),
      .o_chdr_tdata (chdr_from_dut[port_index].tdata ),
      .o_chdr_tuser (                                ),
      .o_chdr_tlast (chdr_from_dut[port_index].tlast ),
      .o_chdr_tvalid(chdr_from_dut[port_index].tvalid),
      .o_chdr_tready(chdr_from_dut[port_index].tready)
    );
  end


  //---------------------------------------------------------------------------
  // Crossbar Configuration
  //---------------------------------------------------------------------------

  task cfg_write(shortint unsigned addr, int unsigned data);
    $display("Writing route 0x%X to addr 0x%X", data, addr);
    @(posedge clk);
    ext_rtcfg_stb  <= 1'b1;
    ext_rtcfg_addr <= addr;
    ext_rtcfg_data <= data;
    @(posedge clk);
    ext_rtcfg_stb  <= 1'b0;
    ext_rtcfg_addr <= 'X;
    ext_rtcfg_data <= 'X;
    @(negedge ext_rtcfg_ack);
  endtask : cfg_write


  task automatic configure_crossbar();
    bit [15:0] epid;

    // Configure an EPID for every possible crossbar route. This will allow us
    // to use the EPID to see indicate the intended source port and intended
    // destination port. The left 8 bits of EPID will be the source port and
    // the right 8 bits will be the destination port.
    $display("Initializing crossbar");

    if (USE_MGMT_PORTS) begin
      // Configure using the management ports
      $fatal(1, "Configuration of crossbar via management ports is NOT supported yet");
    end else begin
      // Configure using the external configuration port
      for (int out_port = 0; out_port < NUM_PORTS; out_port++) begin
        for (int in_port = 0; in_port < NUM_PORTS; in_port++) begin
          // To add an entry to the routing table, put the desired output port
          // number in the data field (the width of the data field is
          // clog2(NUM_PORTS)) and the corresponding 16-bit EPID in the address
          // field.
          epid = { in_port[7:0], out_port[7:0] };
          cfg_write(epid, out_port);
       end
      end
    end

    // The routing table can buffer a few write requests, so it takes a little
    // bit of extra time for the last write to update the KV map before we can
    // start routing packets.
    #(1ns * 100*CLK_PERIOD);
  endtask : configure_crossbar


  //---------------------------------------------------------------------------
  // Traffic Consumers
  //---------------------------------------------------------------------------
  //
  // Each output port has its own BFM. Here we generate a consumer for each
  // output port. We use a for-generate statement to avoid having to manage a
  // bunch of threads for the consumers, although we certainly could have done
  // that.
  //
  //---------------------------------------------------------------------------

  for (genvar port_num = 0; port_num < NUM_PORTS; port_num++) begin : gen_consumers
    initial begin
      int expected_pkts;
      forever begin
        @(start_consumer);
        $display("Consumer started for port %0d", port_num);
        mb_num_pkts[port_num].get(expected_pkts);
        $display("Consumer %0d: Expecting %0d packets", port_num, expected_pkts);

        repeat(expected_pkts) begin
          shortint epid;
          int data_length;
          int start_val;

          // Get the next packet
          ChdrPacket #(PORT_W) pkt;
          chdr_bfm[port_num].get_chdr(pkt);
          epid = pkt.header.dst_epid;
          { data_length, start_val } = pkt.metadata[0];
          if (DEBUG) begin
            $display(
              "Consumer %0d: Received packet %0d -> %0d, EPID: %X, StartVal: 0x%02X, Length: %0d (%0d)",
              port_num, epid[15:8], epid[7:0], epid, start_val, data_length, pkt.data.size()
            );
          end

          // Check the EPID
          `ASSERT_ERROR(
            epid[7:0] == port_num,
            $sformatf(
              "Consumer %0d: Received EPID %X. Expected EPID ending in **%X.",
              port_num, epid, byte'(port_num)
            )
          );

          // Check the payload
          begin
            ChdrData #(PORT_W, 8)::item_queue_t data_bytes;
            byte expected;
            data_bytes = ChdrData #(PORT_W, 8)::chdr_to_item(pkt.data, data_length);
            foreach (data_bytes[i]) begin
              expected = start_val + i;
              `ASSERT_ERROR(
                data_bytes[i] == expected,
                $sformatf(
                  "Consumer %0d: Byte %0d of packet is incorrect. Expected 0x%X, found 0x%X.",
                  port_num, i, expected, data_bytes[i]
                )
              );
            end
          end

          // Check that the payload was the expected size
          begin
            int exp_num_words;
            exp_num_words = $ceil(data_length / (PORT_W/8.0));
            `ASSERT_ERROR(
              pkt.data.size() == exp_num_words,
              $sformatf(
                "Consumer %0d: Received %0d words in payload, expected %0d words",
                port_num, pkt.data.size(), exp_num_words
              )
            );
          end

        end

        ports_done.put();
      end
    end
  end : gen_consumers


  //---------------------------------------------------------------------------
  // Traffic Producer
  //---------------------------------------------------------------------------
  //
  // Each input port has its own input BFM. This task will generate the
  // indicated number of random packets for each input port and enqueue them in
  // the associated BFM for each port. The enqueuing is non-blocking, so all
  // packets get enqueued at the same time and will be transmitted by the BFMs
  // in the order provided. The destination output port is randomly selected.
  // All input ports will be receiving in parallel.
  //
  //---------------------------------------------------------------------------

  task automatic gen_traffic(int num_packets);
    int num_pkts [NUM_PORTS];
    int src_port;
    int dst_port;

    // Because the BFM calls are non-blocking, we can enqueue all the packets
    // we want to send, and they will all start sending at time zero.
    for (src_port = 0; src_port < NUM_PORTS; src_port++) begin
      // Skips this input port if it doesn't have any routes
      if (!TEST_BAD_ROUTES && !ROUTES[src_port]) continue;

      // Send num_packets packets on each input port with a random output
      // port as the destination.
      for (int pkt_count = 0; pkt_count < num_packets; pkt_count++) begin
        ChdrPacket #(PORT_W) packet = new();
        chdr_header_t hdr = '0;
        ChdrData #(PORT_W, 8)::item_queue_t data_bytes;
        ChdrData #(PORT_W, 8)::chdr_word_queue_t data_words;
        ChdrData #(PORT_W, 8)::chdr_word_queue_t mdata_words = '{ 0 };
        int data_length;
        int start_val;

        // Generate a random byte payload
        data_length = $urandom_range(1, MAX_PYLD_BYTES);
        start_val = $urandom_range(0, 255);
        for(int i = 0; i < data_length; i++) begin
          data_bytes.push_back(start_val + i);
        end
        data_words = ChdrData #(PORT_W, 8)::item_to_chdr(data_bytes);

        // Choose a random destination port. We allow paths that are disabled
        // and expect these to be ignored.
        do begin
          dst_port = $urandom_range(0, NUM_PORTS-1);
        end while (!TEST_BAD_ROUTES && !ROUTES[src_port][dst_port]);


        // Generate packet. The EPID holds the expected route and the metadata
        // holds the expected start value and length for the data payload.
        hdr.pkt_type = CHDR_DATA_NO_TS;
        hdr.dst_epid = { src_port[7:0], dst_port[7:0] };
        mdata_words[0] = { data_length, start_val };
        packet.write_raw(hdr, data_words, mdata_words);

        // Enqueue the packet
        if (DEBUG) begin
          $display(
            "Producer %0d: Sending packet %0d -> %0d, EPID: %X, StartVal: 0x%02X, Length: %0d (%0d)",
            src_port, src_port, dst_port, packet.header.dst_epid, start_val,
            data_length, packet.data.size()
          );
        end
        chdr_bfm[src_port].put_chdr(packet);

        // Updated the number of expected packets for the selected port.
        if (ROUTES[src_port][dst_port]) begin
          num_pkts[dst_port]++;
        end
      end
    end

    // Let the consumer know how many packets to expect
    for (dst_port = 0; dst_port < NUM_PORTS; dst_port++) begin
      mb_num_pkts[dst_port].put(num_pkts[dst_port]);
    end

    // Signal that the mailboxes are ready to be read
    -> start_consumer;
  endtask : gen_traffic


  //---------------------------------------------------------------------------
  // Test Executor
  //---------------------------------------------------------------------------

  // Tests num_packets on each port using the provided stall rates.
  task automatic test_traffic_pattern(
    int num_packets,
    int input_stall_prob,
    int output_stall_prob
  );
    // Configure the BFM rates
    for (int i = 0; i < NUM_PORTS; i++) begin
      chdr_bfm[i].set_master_stall_prob(input_stall_prob);
      chdr_bfm[i].set_slave_stall_prob(output_stall_prob);
    end

    // Generate the random traffic to transmit
    gen_traffic(num_packets);

    // Wait until all ports have finished receiving
    ports_done.get(NUM_PORTS);
  endtask : test_traffic_pattern


  //---------------------------------------------------------------------------
  // Main
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;
    tb_name = $sformatf(
      "chdr_crossbar_nxn\nNUM_PORTS = %0D\nCHDR_WIDTHS = %p\nROUTES = %b\nNUM_PKTS = %0D",
      NUM_PORTS, CHDR_WIDTHS, ROUTES, NUM_PKTS
    );

    test.start_tb(tb_name);

    // Initialize mailboxes
    for (int port = 0; port < NUM_PORTS; port++) begin
      mb_num_pkts[port] = new();
    end

    // Start the BFMs. Wait a delta cycle to ensure that the BFMs get created
    // before we use them.
    #0ns;
    for (int port = 0; port < NUM_PORTS; port++) begin
      chdr_bfm[port].run();
    end

    // Start the clocks
    clk_gen.start();

    // Reset
    clk_gen.reset();
    @(negedge rst);

    //-------------------------------------------------------------------------
    // Initialize the Crossbar Routing
    //-------------------------------------------------------------------------

    configure_crossbar();

    //-------------------------------------------------------------------------
    // Run Tests
    //-------------------------------------------------------------------------

    test.start_test("Full rate", 10ms);
    test_traffic_pattern(NUM_PKTS, 0, 0);
    test.end_test();

    test.start_test("Three-quarter rate", 10ms);
    test_traffic_pattern(NUM_PKTS, 25, 25);
    test.end_test();

    test.start_test("Back pressure", 10ms);
    test_traffic_pattern(NUM_PKTS, 25, 50);
    test.end_test();

    test.start_test("Underflow", 10ms);
    test_traffic_pattern(NUM_PKTS, 50, 25);
    test.end_test();

    //-------------------------------------------------------------------------
    // Clean Up
    //-------------------------------------------------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    clk_gen.kill();
  end : tb_main

endmodule : chdr_crossbar_nxn_tb


`default_nettype wire
