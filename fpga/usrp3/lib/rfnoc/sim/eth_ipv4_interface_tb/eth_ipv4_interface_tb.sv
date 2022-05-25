//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_interface_tb
//
// Description:
//
//   Testbench for eth_ipv4_interface. It uses the eth_ipv4_interface_wrapper
//   to instantiate it to test the wrapper also.
//
// Parameters:
//
//   ENET_W     : Ethernet MAC data port width
//   CPU_W      : CPU data port width
//   CHDR_W     : CHDR width and bus width on the RFNoC CHDR interface
//   NET_CHDR_W : CHDR width of network traffic (as generated on the host; may
//                be different from the bus width).
//   EN_RAW_UDP : Enable raw UDP testing
//

`default_nettype none


module eth_ipv4_interface_tb #(
  parameter int ENET_W     = 64,
  parameter int CPU_W      = 64,
  parameter int CHDR_W     = 64,
  parameter int NET_CHDR_W = CHDR_W,
  parameter bit EN_RAW_UDP = 1
);

  //---------------------------------------------------------------------------
  // Includes
  //---------------------------------------------------------------------------

  `include "test_exec.svh"

  // Eth parameters must be defined before including eth_regs.vh
  localparam int BASE       = 0;
  localparam int REG_AWIDTH = 14;
  `include "../../xport_sv/eth_regs.vh"

  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgChdrIfaceBfm::*;
  import PkgAxiStreamBfm::*;
  import PkgChdrBfm::*;
  import PkgEthernet::*;
  import PkgRandom::*;


  //---------------------------------------------------------------------------
  // Testbench Parameters
  //---------------------------------------------------------------------------

  localparam real ETH_CLK_PERIOD      = 8.0;
  localparam real BUS_CLK_PERIOD      = 5.0;
  localparam real CPU_CLK_PERIOD      = 25.0;
  localparam int  ITEM_W              = 8;     // Use bytes for this simulation
  localparam int  MAX_ITEM_PER_PACKET = 8192;

  // The DUT's MAC, IP, UDP info
  localparam bit [47:0] MAC_ADDR = 48'hFEEDEADDBEEF;
  localparam bit [31:0] IP_ADDR  = { 8'd192, 8'd168, 8'd10, 8'd2 };
  localparam bit [15:0] UDP_PORT = 49153;

  // RFNoC device and node instance
  localparam bit [15:0] DEVICE_ID = 16'h1234;
  localparam bit [31:0] NODE_INST = 32'hF1B0412A;

  // Enable debug prints
  localparam bit DEBUG = 0;

  // Number of unique endpoints to simulate
  localparam int NUM_EPID = 8;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit bus_clk, eth_clk, cpu_clk;
  bit bus_rst, eth_rst, cpu_rst;

  sim_clock_gen #(.PERIOD(BUS_CLK_PERIOD), .AUTOSTART(0))
    bus_clk_gen (.clk(bus_clk), .rst(bus_rst));

  sim_clock_gen #(.PERIOD(ETH_CLK_PERIOD), .AUTOSTART(0))
    eth_clk_gen (.clk(eth_clk), .rst(eth_rst));

  sim_clock_gen #(.PERIOD(CPU_CLK_PERIOD), .AUTOSTART(0))
    cpu_clk_gen (.clk(cpu_clk), .rst(cpu_rst));


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  localparam int       ENET_USER_W  =
    eth_ipv4_interface_wrapper_dut.ENET_USER_W;
  localparam int       CPU_USER_W   =
    eth_ipv4_interface_wrapper_dut.CPU_USER_W;
  localparam bit [7:0] COMPAT_MAJOR =
    eth_ipv4_interface_wrapper_dut.eth_ipv4_interface_i.COMPAT_MAJOR;
  localparam bit [7:0] COMPAT_MINOR =
    eth_ipv4_interface_wrapper_dut.eth_ipv4_interface_i.COMPAT_MINOR;

  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W))
    eth_rx (eth_clk, eth_rst);
  AxiStreamIf #(.DATA_WIDTH(ENET_W), .USER_WIDTH(ENET_USER_W))
    eth_tx (eth_clk, eth_rst);
  logic [ENET_W/8-1:0] eth_tx_tkeep;

  AxiStreamIf #(.DATA_WIDTH(CHDR_W))
    v2e (bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W))
    e2v (bus_clk, bus_rst);

  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W))
    c2e (cpu_clk, cpu_rst);
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W))
    e2c (cpu_clk, cpu_rst);

  regport_if #(REG_AWIDTH) regport (bus_clk, bus_rst);

  ChdrIfaceBfm #(CHDR_W, ITEM_W) chdr_bfm =
    new(v2e, e2v, MAX_ITEM_PER_PACKET*ITEM_W/8, CHDR_W/ITEM_W);

  AxiStreamBfm #(ENET_W, ENET_USER_W) eth_bfm = new(eth_rx, eth_tx);

  typedef ChdrData #(CHDR_W, ITEM_W)::chdr_word_t chdr_word_t;
  typedef ChdrData #(CHDR_W, ITEM_W)::item_t      item_t;

  typedef AxiStreamPacket #(ENET_W, ENET_USER_W)::AxisPacket_t AxisEthPacket_t;

  // Drive unused ports
  assign c2e.tvalid = 1'b0;
  assign e2c.tready = 1'b1;


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  // DUT parameters
  localparam logic [15:0] PROTOVER         = {8'd1, 8'd0};
  localparam int          CPU_FIFO_SIZE    = $clog2(8*1024);
  localparam int          CHDR_FIFO_SIZE   = $clog2(8*1024);
  localparam int          RT_TBL_SIZE      = 6;
  localparam bit          DROP_UNKNOWN_MAC = 0;
  localparam bit          DROP_MIN_PACKET  = 0;
  localparam int          PREAMBLE_BYTES   = 6;
  localparam bit          ADD_SOF          = 1;
  localparam bit          SYNC             = 0;
  localparam bit          PAUSE_EN         = 0;
  localparam bit          EN_RX_KV_MAP_CFG = EN_RAW_UDP;
  localparam bit          EN_RX_RAW_PYLD   = EN_RAW_UDP;

  logic [47:0] my_mac;
  logic [31:0] my_ip;
  logic [15:0] my_udp_chdr_port;

  eth_ipv4_interface_wrapper #(
    .PROTOVER         (PROTOVER        ),
    .CPU_FIFO_SIZE    (CPU_FIFO_SIZE   ),
    .CHDR_FIFO_SIZE   (CHDR_FIFO_SIZE  ),
    .NODE_INST        (NODE_INST       ),
    .RT_TBL_SIZE      (RT_TBL_SIZE     ),
    .REG_AWIDTH       (REG_AWIDTH      ),
    .BASE             (BASE            ),
    .DROP_UNKNOWN_MAC (DROP_UNKNOWN_MAC),
    .DROP_MIN_PACKET  (DROP_MIN_PACKET ),
    .PREAMBLE_BYTES   (PREAMBLE_BYTES  ),
    .ADD_SOF          (ADD_SOF         ),
    .SYNC             (SYNC            ),
    .PAUSE_EN         (PAUSE_EN        ),
    .ENET_W           (ENET_W          ),
    .CPU_W            (CPU_W           ),
    .CHDR_W           (CHDR_W          ),
    .NET_CHDR_W       (NET_CHDR_W      ),
    .EN_RX_KV_MAP_CFG (EN_RX_KV_MAP_CFG),
    .EN_RX_RAW_PYLD   (EN_RX_RAW_PYLD  )
  ) eth_ipv4_interface_wrapper_dut (
    .bus_clk         (bus_clk         ),
    .bus_rst         (bus_rst         ),
    .device_id       (DEVICE_ID       ),
    .reg_wr_req      (regport.wr_req  ),
    .reg_wr_addr     (regport.wr_addr ),
    .reg_wr_data     (regport.wr_data ),
    .reg_rd_req      (regport.rd_req  ),
    .reg_rd_addr     (regport.rd_addr ),
    .reg_rd_resp     (regport.rd_resp ),
    .reg_rd_data     (regport.rd_data ),
    .my_mac          (my_mac          ),
    .my_ip           (my_ip           ),
    .my_udp_chdr_port(my_udp_chdr_port),
    .eth_clk         (eth_clk         ),
    .eth_rst         (eth_rst         ),
    .eth_pause_req   (                ),
    .eth_tx_tdata    (eth_tx.tdata    ),
    .eth_tx_tuser    (eth_tx.tuser    ),
    .eth_tx_tkeep    (eth_tx_tkeep    ),
    .eth_tx_tlast    (eth_tx.tlast    ),
    .eth_tx_tvalid   (eth_tx.tvalid   ),
    .eth_tx_tready   (eth_tx.tready   ),
    .eth_rx_tdata    (eth_rx.tdata    ),
    .eth_rx_tuser    (eth_rx.tuser    ),
    .eth_rx_tlast    (eth_rx.tlast    ),
    .eth_rx_tvalid   (eth_rx.tvalid   ),
    .eth_rx_tready   (eth_rx.tready   ),
    .e2v_tdata       (e2v.tdata       ),
    .e2v_tlast       (e2v.tlast       ),
    .e2v_tvalid      (e2v.tvalid      ),
    .e2v_tready      (e2v.tready      ),
    .v2e_tdata       (v2e.tdata       ),
    .v2e_tlast       (v2e.tlast       ),
    .v2e_tvalid      (v2e.tvalid      ),
    .v2e_tready      (v2e.tready      ),
    .cpu_clk         (cpu_clk         ),
    .cpu_rst         (cpu_rst         ),
    .e2c_tdata       (e2c.tdata       ),
    .e2c_tuser       (e2c.tuser       ),
    .e2c_tlast       (e2c.tlast       ),
    .e2c_tvalid      (e2c.tvalid      ),
    .e2c_tready      (e2c.tready      ),
    .c2e_tdata       (c2e.tdata       ),
    .c2e_tuser       (c2e.tuser       ),
    .c2e_tlast       (c2e.tlast       ),
    .c2e_tvalid      (c2e.tvalid      ),
    .c2e_tready      (c2e.tready      )
  );


  //---------------------------------------------------------------------------
  // TKEEP/TUSER Checker
  //---------------------------------------------------------------------------
  //
  // The DUT has both a TKEEP on eth_tx as well as TUSER, which gives the
  // number of valid trailing bytes. Devices can use either. This testbench
  // uses TUSER, so here we confirm that TKEEP matches TKEEP at all times.
  //
  //---------------------------------------------------------------------------

  always @(posedge eth_clk) begin
    if (eth_tx.tvalid && eth_tx.tready) begin
      // TUSER will be 0 if all the bytes are valid, which is the most common.
      if (eth_tx.tuser == 0) begin
        `ASSERT_ERROR(eth_tx_tkeep == '1, "TKEEP should be all ones");
      end else begin
        // Count the number of consecutive TKEEP bits
        automatic int valid_bytes = 0;
        for (int i = 0; i < eth_tx.DATA_WIDTH/8; i++) begin
          if (eth_tx_tkeep[i]) valid_bytes += 1;
          else break;
        end

        // Make sure there are no other bits set
        `ASSERT_ERROR(eth_tx_tkeep == 2**valid_bytes-1, "eth_tx.TKEEP ones are not contiguous");

        // Make sure TKEEP matches TUSER
        `ASSERT_ERROR(valid_bytes == eth_tx.tuser, "eth_tx.TKEEP/TUSER mismatch");

        // Make sure TUSER does not exceed its range (it should be 0 when all
        // bytes are valid).
        `ASSERT_ERROR(eth_tx.tuser < eth_tx.DATA_WIDTH/8, "eth_tx.TUSER exceeds range")
      end
    end
  end


  //---------------------------------------------------------------------------
  // Helper Tasks
  //---------------------------------------------------------------------------

  // 4-state byte queue data type
  typedef logic [7:0] logic_byte_queue_t[$];

  // Structure to describe a stream configuration
  typedef struct packed {
    bit [15:0] dst_epid;
    bit [47:0] eth_mac;
    bit [15:0] udp_port;
    bit [31:0] ip_addr;
    bit        raw_udp;
  } stream_cfg_t;

  // Queue of stream configurations to use for the simulation
  stream_cfg_t stream_cfgs[$];


  // Convert 2-state byte queue to 4-state byte queue
  function automatic logic_byte_queue_t bytes_to_logic_queue(byte bytes[$]);
    logic [7:0] logic_bytes[$];
    foreach(bytes[i]) logic_bytes[i] = bytes[i];
    return logic_bytes;
  endfunction : bytes_to_logic_queue


  // Setup a stream route through the transport adapter. This can be CHDR or
  // raw UDP.
  task automatic config_stream(input stream_cfg_t cfg);
    logic [31:0] val;

    // Poll on BUSY bit
    do begin
      regport.read(REG_XPORT_KV_CFG, val);
    end while(val & (1 << 31));

    // Add route to KV map
    regport.write(REG_XPORT_KV_MAC_LO, cfg.eth_mac & 32'hFFFFFFFF);
    regport.write(REG_XPORT_KV_MAC_HI, (cfg.eth_mac >> 32) & 32'hFFFFFFFF);
    regport.write(REG_XPORT_KV_IP,     cfg.ip_addr);
    regport.write(REG_XPORT_KV_UDP,    cfg.udp_port);
    regport.write(REG_XPORT_KV_CFG,    {cfg.raw_udp, cfg.dst_epid});

    // Make sure config is complete before attempting to use it
    do begin
      regport.read(REG_XPORT_KV_CFG, val);
    end while(val & (1 << 31));
  endtask : config_stream


  // Generate the list streams to use for simulation and program stream
  // configurations into the DUT's KV map.
  task automatic config_test_streams();
    for (int count=0; count < NUM_EPID; count++) begin
      stream_cfg_t cfg;
      // Stream configuration to use for this test
      cfg.eth_mac  = Rand#(48)::rand_bit();
      cfg.ip_addr  = Rand#(32)::rand_bit();
      cfg.udp_port = Rand#(16)::rand_bit();
      cfg.dst_epid = Rand#(16)::rand_bit();
      // If raw UDP is enabled, make half the streams CHDR and half raw UDP
      cfg.raw_udp  = (count % 2 == 0);
      if (cfg.raw_udp) $display("Adding raw UDP route");
      else $display("Adding normal CHDR route");

      // Setup route for this data stream
      config_stream(cfg);

      // Save configuration in the list of streams to refer to later
      stream_cfgs.push_back(cfg);
    end
  endtask : config_test_streams


  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  task automatic test_reset();
    test.start_test("Reset", 1us);
    bus_clk_gen.reset();
    eth_clk_gen.reset();
    cpu_clk_gen.reset();
    if (bus_rst) @bus_rst;
    if (eth_rst) @eth_rst;
    if (cpu_rst) @cpu_rst;
    test.end_test();
  endtask : test_reset

  // Initialize all the registers for the testbench to run, testing them in
  // the process.
  task automatic test_regs();
    logic [31:0] val;

    test.start_test("Initialize registers", 10us);

    // Configure the registers
    regport.write(REG_MAC_LSB, MAC_ADDR);
    regport.write(REG_MAC_MSB, MAC_ADDR >> 32);
    regport.write(REG_IP, IP_ADDR);
    regport.write(REG_UDP, UDP_PORT);
    // Give the bridge different values to make sure they're unique
    regport.write(REG_BRIDGE_MAC_LSB, MAC_ADDR+1);
    regport.write(REG_BRIDGE_MAC_MSB, (MAC_ADDR >> 32)+1);
    regport.write(REG_BRIDGE_IP, IP_ADDR+1);
    regport.write(REG_BRIDGE_UDP, UDP_PORT+1);
    regport.write(REG_BRIDGE_ENABLE, 1'b1);

    // Verify the registers
    regport.read(REG_MAC_LSB, val);
    `ASSERT_ERROR(val == (MAC_ADDR & 32'hFFFFFFFF), "REG_MAC_LSB does not match");
    regport.read(REG_MAC_MSB, val);
    `ASSERT_ERROR(val == ((MAC_ADDR >> 32) & 32'hFFFFFFFF), "REG_MAC_MSB does not match");
    regport.read(REG_IP, val);
    `ASSERT_ERROR(val == IP_ADDR, "REG_IP does not match");
    regport.read(REG_UDP, val);
    `ASSERT_ERROR(val == UDP_PORT, "REG_UDP does not match");
    regport.read(REG_BRIDGE_MAC_LSB, val);
    `ASSERT_ERROR(val == (MAC_ADDR & 32'hFFFFFFFF)+1, "REG_BRIDGE_MAC_LSB does not match");
    regport.read(REG_BRIDGE_MAC_MSB, val);
    `ASSERT_ERROR(val == ((MAC_ADDR >> 32) & 32'hFFFFFFFF)+1, "REG_BRIDGE_MAC_MSB does not match");
    regport.read(REG_BRIDGE_IP, val);
    `ASSERT_ERROR(val == IP_ADDR+1, "REG_BRIDGE_IP does not match");
    regport.read(REG_BRIDGE_UDP, val);
    `ASSERT_ERROR(val == UDP_PORT+1, "REG_BRIDGE_UDP does not match");
    regport.read(REG_BRIDGE_ENABLE, val);
    `ASSERT_ERROR(val == 1'b1, "REG_BRIDGE_ENABLE does not match");
    regport.read(REG_XPORT_COMPAT, val);
    `ASSERT_ERROR(val == (COMPAT_MAJOR << 8) | (COMPAT_MINOR << 0),
      "REG_XPORT_COMPAT does not match");
    regport.read(REG_XPORT_INFO, val);
    `ASSERT_ERROR(val == (EN_RX_RAW_PYLD << 1) | (EN_RX_KV_MAP_CFG << 0),
      "REG_XPORT_INFO does not match");
    regport.read(REG_XPORT_NODE_INST, val);
    `ASSERT_ERROR(val == NODE_INST, "REG_XPORT_NODE_INST does not match");

    // Disable the bridge for now
    regport.write(REG_BRIDGE_ENABLE, 0);
    regport.read(REG_BRIDGE_ENABLE, val);
    `ASSERT_ERROR(val == 1'b0, "REG_BRIDGE_ENABLE does not match");

    // Check the ports that mirror registers
    `ASSERT_ERROR(my_mac == MAC_ADDR, "my_mac does not match");
    `ASSERT_ERROR(my_ip == IP_ADDR, "my_ip does not match");
    `ASSERT_ERROR(my_udp_chdr_port == UDP_PORT, "my_udp_chdr_port does not match");

    test.end_test();
  endtask : test_regs

  // Do a constrained random test using a mixture of CHDR and raw UDP packets
  // in the RX direction (RFNoC to Ethernet).
  //
  //   num_pkts         : Number of random packets to generate and test
  //   eth_stall_prob   : Ethernet stall probability
  //   chdr_stall_prob  : RFNoC stall probability
  //   max_length       : Maximum payload length in bytes
  //
  task automatic test_rand_raw_udp_rx(
    int num_pkts         = 100,
    int eth_stall_prob   = 50,
    int chdr_stall_prob  = 50,
    int max_length       = 512
  );
    mailbox #(logic_byte_queue_t) mb_payloads;
    mailbox #(stream_cfg_t) mb_stream_cfgs;
    logic [31:0] val;
    string test_str;

    mb_payloads = new();
    mb_stream_cfgs = new();

    test_str = $sformatf(
      "Test rand raw UDP (RX)\nArgs: num_pkts=%0d, eth_stall_prob=%0d, chdr_stall_prob=%0d, max_length=%0d",
      num_pkts, eth_stall_prob, chdr_stall_prob, max_length
    );
    test.start_test(test_str, 10us*max_length);

    // Set the stall probabilities
    eth_bfm.set_slave_stall_prob(eth_stall_prob);
    chdr_bfm.set_master_stall_prob(chdr_stall_prob);

    fork
      begin : packet_generation_process
        int iter = 0;
        repeat(num_pkts) begin : send_packet
          item_t items[$];
          packet_info_t pkt_info = 0;
          stream_cfg_t stream_cfg;
          int length;

          // Choose a random stream to use for this packet
          stream_cfg = stream_cfgs[$urandom_range(stream_cfgs.size()-1)];

          // Generate a random packet
          length = $urandom_range(1, max_length);
          items = {};
          for (int i=0; i < length; i++) begin
            items.push_back(i);
          end
          chdr_bfm.dst_epid = stream_cfg.dst_epid;

          // Put packet info into mailboxes
          mb_payloads.put(items);
          mb_stream_cfgs.put(stream_cfg);

          // Send the packet
          chdr_bfm.send_items(items);

          iter++;
        end : send_packet
      end : packet_generation_process

      begin : packet_checker_process
        int iter = 0;
        repeat(num_pkts) begin : recv_packet
          item_t          sent_items[$], rcvd_items[$];
          stream_cfg_t    stream_cfg;
          int             trailing_bytes;
          int             udp_payload_length;
          AxisEthPacket_t axis_eth_pkt;
          raw_pkt_t       raw_pkt;
          raw_pkt_t       payload;
          eth_hdr_t       eth_hdr;
          ipv4_hdr_t      ipv4_hdr;
          udp_hdr_t       udp_hdr;

          // Get packet info from mailboxes
          mb_payloads.get(sent_items);
          mb_stream_cfgs.get(stream_cfg);

          // Grab the next packet
          eth_bfm.get(axis_eth_pkt);
          raw_pkt = axis_eth_pkt.dump_bytes();

          // Figure out how many bytes of the last word are valid
          if (axis_eth_pkt.user[$] > 0) begin
            trailing_bytes = ENET_W/8 - axis_eth_pkt.user[$];
          end else begin
            trailing_bytes = 0;
          end

          // Extract the packet info
          raw_pkt = raw_pkt[PREAMBLE_BYTES:$-trailing_bytes];
          decode_udp_pkt(raw_pkt, eth_hdr, ipv4_hdr, udp_hdr, payload);
          udp_payload_length = udp_hdr.length - 8;
          rcvd_items = bytes_to_logic_queue(payload);
          if (stream_cfg.raw_udp) begin
            // Drop the trailing bytes
            rcvd_items = rcvd_items[0:(udp_payload_length-1)];
          end else begin
            // Drop the CHDR header and trailing bytes
            rcvd_items = rcvd_items[NET_CHDR_W/8:(udp_payload_length-1)];
          end

          // Debug prints
          if (DEBUG) begin
            $display("UDP payload length is %0d", udp_payload_length);
            $display("Expecting is_raw=%0b, sent_items=%p", stream_cfg.raw_udp, sent_items);
            $display("Received rcvd_items=%p", rcvd_items);
            $display("Finished iteration %0d", iter);
          end
          iter++;

          // Check the UDP/IP/Eth layers for correct information
          `ASSERT_ERROR(eth_hdr.dest_mac  == stream_cfg.eth_mac,  "Incorrect dest MAC addr");
          `ASSERT_ERROR(ipv4_hdr.dest_ip  == stream_cfg.ip_addr,  "Incorrect dest IP addr");
          `ASSERT_ERROR(udp_hdr.dest_port == stream_cfg.udp_port, "Incorrect dest UDP port");

          // Check the payload
          `ASSERT_ERROR(
            ChdrData #(CHDR_W, ITEM_W)::item_equal(sent_items, rcvd_items),
            "UDP payload doesn't match packet sent"
          );

        end : recv_packet
      end : packet_checker_process
    join

    test.end_test();

  endtask : test_rand_raw_udp_rx


  //---------------------------------------------------------------------------
  // Main
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string tb_name;

    //----------
    // TB Setup
    //----------

    tb_name = $sformatf(
      "eth_ipv4_interface_tb\nENET_W = %0D, CPU_W = %0D, CHDR_W = %0D, NET_CHDR_W = %0D",
      ENET_W, CPU_W, CHDR_W, NET_CHDR_W
    );

    test.start_tb(tb_name);

    // Initialize the regport bus
    regport.init();

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    bus_clk_gen.start();
    eth_clk_gen.start();
    cpu_clk_gen.start();

    // Start the BFMs running
    chdr_bfm.run();
    eth_bfm.run();

    //-----------
    // Run tests
    //-----------

    test_reset();
    test_regs();

    if (EN_RAW_UDP) begin
      // Configure streams for testing of raw UDP feature
      config_test_streams();
      test_rand_raw_udp_rx(500,  0,  0);  // No back-pressure
      test_rand_raw_udp_rx(500, 75,  0);  // Back-pressure
      test_rand_raw_udp_rx(500,  0, 75);  // Underflow
      test_rand_raw_udp_rx(500, 50, 50);  // Random-balanced
    end

    //--------------------
    // Finish
    //--------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    bus_clk_gen.kill();
    eth_clk_gen.kill();
    cpu_clk_gen.kill();

  end : tb_main

endmodule : eth_ipv4_interface_tb


`default_nettype wire
