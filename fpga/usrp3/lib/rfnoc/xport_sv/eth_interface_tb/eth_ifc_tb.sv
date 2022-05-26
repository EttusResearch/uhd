//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ifc_tb
//
// Description:  Testbench for eth_interface
//

module eth_ifc_tb #(
  parameter TEST_NAME  = "eth_ifc_tb",
  parameter SV_ETH_IFC = 1,
  parameter ENET_W     = 64,
  parameter CPU_W      = 64,
  parameter CHDR_W     = 64
)(
  /* no IO */
);
  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgAxiStreamBfm::*;
  import PkgTestExec::*;
  import PkgChdrUtils::*;
  import PkgChdrBfm::*;
  import PkgEthernet::*;

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------
  localparam ENET_USER_W = $clog2(ENET_W/8)+1;
  localparam CPU_USER_W  = $clog2(CPU_W/8)+1;
  localparam CHDR_USER_W  = $clog2(CHDR_W/8)+1;
  localparam        REG_AWIDTH = 14;
  localparam [15:0] PROTOVER    = {8'd1, 8'd0};
  localparam        MTU         = 10;
  localparam        NODE_INST   = 0;
  localparam        RT_TBL_SIZE = 6;
  localparam        REG_BASE_ETH_SWITCH  = 14'h1000;
  localparam        BASE  = REG_BASE_ETH_SWITCH;
  localparam        DROP_MIN_PACKET = 0;
  localparam        SYNC = (ENET_W==512) ? 0:1;
  localparam        ETH_PERIOD = (ENET_W==512) ? 3.1:5.0;
  // can set PREAMBLE_BYTES to 0 or 6 if SV_ETH_IFC, but otherwise
  // it's hard-coded to 6. (0 is normal for 100G)(6 is normal for old Xge)
  localparam PREAMBLE_BYTES = SV_ETH_IFC ? 0 : 6;
  // Include for register offsets
  `include "../eth_regs.vh"
  // allows the DUT to push full words and tb does not check tuser/tkeep of packets it's transmitting
  localparam IGNORE_EXTRA_DATA = 0;

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  // ModelSim should initialize the fields of ipv4_hdr to their default values,
  // but there's a bug where it doesn't in some cases. This constant is used to
  // initialize them to work around the bug.
  localparam ipv4_hdr_t DEFAULT_IPV4_HDR = '{
    header_length : 4'd5,
    version       : 4'd4,
    dscp          : 6'b0000_00,
    ecn           : 2'b00,
    length        : 16'hXXXX,
    identification: 16'h462E,
    rsv_zero      : 1'b0,
    dont_frag     : 1'b1,
    more_frag     : 1'b0,
    frag_offset   : 16'd0,
    time_to_live  : 16'd64,
    protocol      : UDP,
    checksum      : 16'hXXXX,
    src_ip        : DEF_SRC_IP_ADDR,
    dest_ip       : DEF_DEST_IP_ADDR
  };

  bit clk;
  bit reset;

  sim_clock_gen #(.PERIOD(5.0), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(reset));
  sim_clock_gen #(.PERIOD(ETH_PERIOD), .AUTOSTART(0))
    eth_clk_gen (.clk(eth_clk), .rst(eth_reset));

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  localparam MAX_PACKET_BYTES = 2**16;
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    eth_tx (eth_clk, eth_reset);
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    eth_rx (eth_clk, eth_reset);

  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(CHDR_USER_W),.TKEEP(0),.TUSER(0))
    v2e    (clk, reset);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(CHDR_USER_W),.TKEEP(0),.TUSER(0))
    e2v    (clk, reset);

  AxiStreamIf #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),.TKEEP(0))
    c2e    (clk, reset);
  AxiStreamIf #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),.TKEEP(0))
    e2c    (clk, reset);

  // Bus functional model for a axi_stream controller
  AxiStreamBfm #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                 .MAX_PACKET_BYTES(MAX_PACKET_BYTES)) eth =
    new(.master(eth_rx), .slave(eth_tx));
  AxiStreamBfm #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(CHDR_USER_W),.TKEEP(0),.TUSER(0)) v =
    new(.master(v2e), .slave(e2v));
  AxiStreamBfm #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),.TKEEP(0)) cpu =
    new(.master(c2e), .slave(e2c));

  //----------------------------------------------------
  // Instantiate DUT
  //----------------------------------------------------

  reg                  reg_wr_req  = 1'b0;
  reg [REG_AWIDTH-1:0] reg_wr_addr = 0;
  reg [31:0]           reg_wr_data = 32'd0;

  reg                  reg_rd_req  = 1'b0;
  reg [REG_AWIDTH-1:0] reg_rd_addr = 0;
  reg                  reg_rd_resp;
  reg [31:0]           reg_rd_data;

  reg [3:0]            eth_tx_tuser = 4'd0;
  reg [3:0]            eth_rx_tuser = 4'd0;
  reg [3:0]            e2c_tuser    = 4'd0;
  reg [3:0]            c2e_tuser    = 4'd0;

  reg [15:0]           device_id    =16'd0;

  reg [47:0]           my_mac;
  reg [31:0]           my_ip;
  reg [15:0]           my_udp_chdr_port;


  if (SV_ETH_IFC) begin : gen_new_dut

    eth_ipv4_interface #(
     .PREAMBLE_BYTES(PREAMBLE_BYTES),
     .CPU_FIFO_SIZE(MTU),
     .CHDR_FIFO_SIZE(MTU),
     .PROTOVER(PROTOVER),
     .NODE_INST(NODE_INST),
     .REG_AWIDTH(REG_AWIDTH),
     .RT_TBL_SIZE(RT_TBL_SIZE),
     .BASE(BASE),.SYNC(SYNC),
     .ENET_W(ENET_W),.CPU_W(CPU_W),.CHDR_W(CHDR_W)
    ) eth_interface (
     .bus_clk(clk),.bus_rst(reset),.eth_pause_req(),.*
    );
  end else begin : gen_old_dut
    logic [63:0] eth_tx_tdata;
    logic [3:0]  eth_tx_tuser;
    logic        eth_tx_tlast;
    logic        eth_tx_tvalid;
    logic        eth_tx_tready;

    logic [63:0] eth_rx_tdata;
    logic [3:0]  eth_rx_tuser;
    logic        eth_rx_tlast;
    logic        eth_rx_tvalid;
    logic        eth_rx_tready;

    logic [63:0] e2v_tdata;
    logic        e2v_tlast;
    logic        e2v_tvalid;
    logic        e2v_tready;

    logic [63:0] v2e_tdata;
    logic        v2e_tlast;
    logic        v2e_tvalid;
    logic        v2e_tready;

    logic [63:0] e2c_tdata;
    logic [3:0]  e2c_tuser;
    logic        e2c_tlast;
    logic        e2c_tvalid;
    logic        e2c_tready;

    logic [63:0] c2e_tdata;
    logic [3:0]  c2e_tuser;
    logic        c2e_tlast;
    logic        c2e_tvalid;
    logic        c2e_tready;

    always_comb begin
     eth_tx.tdata  = eth_tx_tdata;
     eth_tx.tuser  = eth_tx_tuser;
     eth_tx.tkeep  = eth_tx.trailing2keep(eth_tx_tuser);
     eth_tx.tlast  = eth_tx_tlast;
     eth_tx.tvalid = eth_tx_tvalid;
     eth_tx_tready = eth_tx.tready;

     eth_rx_tdata  = eth_rx.tdata;
     eth_rx_tuser  = eth_rx.tuser;
     eth_rx_tlast  = eth_rx.tlast;
     eth_rx_tvalid = eth_rx.tvalid;
     eth_rx.tready = eth_rx_tready;

     e2v.tdata     = e2v_tdata;
     e2v.tlast     = e2v_tlast;
     e2v.tvalid    = e2v_tvalid;
     e2v_tready    = e2v.tready;

     v2e_tdata     = v2e.tdata;
     v2e_tlast     = v2e.tlast;
     v2e_tvalid    = v2e.tvalid;
     v2e.tready    = v2e_tready;

     e2c.tdata     = e2c_tdata;
     e2c.tuser     = e2c_tuser;
     e2c.tlast     = e2c_tlast;
     e2c.tvalid    = e2c_tvalid;
     e2c_tready    = e2c.tready;

     c2e_tdata     = c2e.tdata;
     c2e_tuser     = c2e.tuser;
     c2e_tlast     = c2e.tlast;
     c2e_tvalid    = c2e.tvalid;
     c2e.tready    = c2e_tready;
    end

    eth_interface #(
     .PROTOVER(PROTOVER), .NODE_INST(NODE_INST), .MTU(MTU),
     .REG_AWIDTH(REG_AWIDTH), .RT_TBL_SIZE(RT_TBL_SIZE),
     .BASE(BASE)
    ) eth_interface (
     .*,
     .my_udp_port   (my_udp_chdr_port)
    );
  end

  task automatic reg_wr (
    // Register port: Write port (domain: clk)
    input int   addr,
    input int   data);
  begin
    @(posedge clk);
    reg_wr_req = 1'b1;
    reg_wr_addr = addr;
    reg_wr_data = data;
    @(posedge clk);
    reg_wr_req = 1'b0;
  end
  endtask

  task automatic reg_rd_check (
    // Register port: Write port (domain: clk)
    input int   addr,
    input int   data);
  begin
    @(posedge clk);
    reg_rd_req = 1'b1; // drive at posedge
    reg_rd_addr = addr;
    @(negedge clk); // check at negedge
    if (SV_ETH_IFC) begin
      assert(reg_rd_resp==1'b0) else $error("resp set early");
    end else begin
      // The original doesn't initialize reg_rd_resp so it comes back as X on first read
      assert(reg_rd_resp==1'b0 || $isunknown(reg_rd_resp)) else $error("resp set early");
    end
    @(posedge clk);
    reg_rd_req = 1'b0;
    @(negedge clk);
    assert(data==reg_rd_data) else $error("read data didn't match");
    assert(reg_rd_resp==1'b1) else $error("resp didn't pulse");
    @(posedge clk);
    @(negedge clk);
    assert(reg_rd_resp==1'b0) else $error("resp set late");
  end
  endtask


  //---------------------------------------------------------------------------
  // Reset
  //---------------------------------------------------------------------------

  task test_reset();
    wait(!reset);
    repeat (10) @(posedge clk);
    test.end_test();
  endtask : test_reset

  //---------------------------------------------------------------------------
  // Test Registers
  //---------------------------------------------------------------------------

  task test_registers();
    test.start_test({TEST_NAME,"::Test/Setup Registers"}, 10us);
    // DEF_DEST_MAC/IP/UDP are defined in the
    // sim_ethernet_lib.svh, as the destination
    // addresses. Using the defaults means
    // if I don't change the dest address on
    // a packet it will go to the CHDR
    reg_wr(REG_MAC_LSB,DEF_DEST_MAC_ADDR[31:0]);
    reg_wr(REG_MAC_MSB,DEF_DEST_MAC_ADDR[47:32]);
    reg_wr(REG_IP,DEF_DEST_IP_ADDR);
    reg_wr(REG_UDP,DEF_DEST_UDP_PORT);

    repeat (3) @(posedge clk);

    `ASSERT_ERROR(my_mac==DEF_DEST_MAC_ADDR,     "my mac mismatched!");
    `ASSERT_ERROR(my_ip==DEF_DEST_IP_ADDR,       "my ip mismatched!");
    `ASSERT_ERROR(my_udp_chdr_port==DEF_DEST_UDP_PORT,"my udp mismatched!");

    reg_wr(REG_BRIDGE_ENABLE,1);
    reg_wr(REG_BRIDGE_MAC_LSB,DEF_BRIDGE_MAC_ADDR[31:0]);
    reg_wr(REG_BRIDGE_MAC_MSB,DEF_BRIDGE_MAC_ADDR[47:32]);
    reg_wr(REG_BRIDGE_IP,DEF_BRIDGE_IP_ADDR);
    reg_wr(REG_BRIDGE_UDP,DEF_BRIDGE_UDP_PORT);

    repeat (3) @(posedge clk);
    `ASSERT_ERROR(my_mac==DEF_BRIDGE_MAC_ADDR,     "my mac mismatched!");
    `ASSERT_ERROR(my_ip==DEF_BRIDGE_IP_ADDR,       "my ip mismatched!");
    `ASSERT_ERROR(my_udp_chdr_port==DEF_BRIDGE_UDP_PORT,"my udp mismatched!");

    reg_wr(REG_BRIDGE_ENABLE,0);

    // Readback the values
    reg_rd_check(REG_MAC_LSB,DEF_DEST_MAC_ADDR[31:0]);
    reg_rd_check(REG_MAC_MSB,DEF_DEST_MAC_ADDR[47:32]);
    reg_rd_check(REG_IP,DEF_DEST_IP_ADDR);
    reg_rd_check(REG_UDP,DEF_DEST_UDP_PORT);
    reg_rd_check(REG_BRIDGE_ENABLE,0);
    reg_rd_check(REG_BRIDGE_MAC_LSB,DEF_BRIDGE_MAC_ADDR[31:0]);
    reg_rd_check(REG_BRIDGE_MAC_MSB,DEF_BRIDGE_MAC_ADDR[47:32]);
    reg_rd_check(REG_BRIDGE_IP,DEF_BRIDGE_IP_ADDR);
    reg_rd_check(REG_BRIDGE_UDP,DEF_BRIDGE_UDP_PORT);
    if (SV_ETH_IFC) begin
      reg_rd_check(REG_CHDR_DROPPED,0);
      reg_rd_check(REG_CPU_DROPPED,0);
    end
    test.end_test();
  endtask : test_registers

  //---------------------------------------------------------------------------
  // Ethernet to CPU test
  //---------------------------------------------------------------------------
    typedef ChdrData #(CHDR_W)::chdr_word_t chdr_word_t;
    typedef chdr_word_t word_queue_t[$];

    typedef XportStreamPacket #(ENET_W)             EthXportPacket_t;
    typedef AxiStreamPacket   #(ENET_W,ENET_USER_W) EthAxisPacket_t;

    typedef XportStreamPacket #(CPU_W)            CpuXportPacket_t;
    typedef AxiStreamPacket   #(CPU_W,CPU_USER_W) CpuAxisPacket_t;

    typedef XportStreamPacket #(CHDR_W) ChdrXportPacket_t;
    typedef AxiStreamPacket   #(CHDR_W,CHDR_USER_W) ChdrAxisPacket_t;
    typedef ChdrPacket        #(CHDR_W,CHDR_USER_W) ChdrPacket_t;

  task automatic test_ethcpu(int num_samples[$], int ERROR_PROB=2, int EXPECT_DROPS=0);
    TestExec test_e2c = new();
    automatic EthXportPacket_t send[$];
    automatic CpuXportPacket_t expected[$];
    automatic int sample_sum = 0;

    test_e2c.start_test({TEST_NAME,"::Ethernet to CPU"}, 60us);
    // This path is
    //   eth_rx -> s_mac(eth_adapter) -> s_mac(eth_dispatch) ->
    ////   in_reg(AXI_FIFO)(SIZE=1)
    //   (eth_dispatch) in -> STATMACHINE (Dispatch) + cpu ->
    ////   out_reg_cpu(AXI_FIFO)(SIZE=1)
    //   (eth_dispatch) o_cpu ->
    ////   cpu_out_gate(AXI_GATE)(SIZE=11)
    //   (eth_dispatch) m_cpu -> (eth_adapter) e2c_chdr -> e2c_fifo
    ////   cpu_fifo(AXI_FIFO)(SIZE=CPU_FIFO_SIZE)
    //   (eth_adapater) m_cpu -> e2c

    foreach (num_samples[i]) begin
      automatic eth_hdr_t    eth_hdr;
      automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
      automatic udp_hdr_t    udp_hdr;
      automatic raw_pkt_t    pay,udp_raw;
      automatic int          preamble;

      if      (PREAMBLE_BYTES == 6) preamble = NORMAL_PREAMBLE;
      else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
      else    $fatal(1, "Invalid PREAMBLE_BYTES");

      expected[i] = new;
      send[i] = new;

      udp_hdr.dest_port = 0; //change dest port from default so it goes to cpu
      get_ramp_raw_pkt(.num_samps(num_samples[i]),.ramp_start((sample_sum)%256),
                       .ramp_inc(1),.pkt(pay),.SWIDTH(8));
      sample_sum += num_samples[i];
      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,pay,
                              .preamble(preamble));
      send[i].push_bytes(udp_raw);
      send[i].tkeep_to_tuser(.ERROR_PROB(ERROR_PROB));

      // rebuild the expected packet for comparison without the preamble
      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,pay,
                              .preamble(NO_PREAMBLE));
      expected[i].push_bytes(udp_raw);
      expected[i].tkeep_to_tuser();

    end

    // iterate in descending order so deleting doesn't shift down
    // the packets in future loop iterations.
    for (int i=  num_samples.size()-1 ; i >= 0; i--) begin
      // original only checks for errors in last word.
      if (!SV_ETH_IFC && send[i].has_error()) send[i].set_error();
      // If a packet has an error it shouldn't make it through
      if (send[i].has_error()) expected.delete(i); // MAC ERROR
      else if (DROP_MIN_PACKET) begin
        if (send[i].byte_length()-PREAMBLE_BYTES < 64) begin
           // short packet rejection feature broken in original
           if (SV_ETH_IFC) expected.delete(i); // TOO SHORT
        end
      end

    end

    fork
      begin // tx_thread
        foreach(send[i])begin
          #1 eth.put(send[i]);
        end
      end
      begin //rx_thread
        if (EXPECT_DROPS > 0) begin
          automatic int pkt_num = 0;
          automatic int drop_count = 0;
          while (expected.size() > 0) begin
            automatic CpuAxisPacket_t  actual_a;
            automatic CpuXportPacket_t actual = new();
            cpu.get(actual_a);
            actual.import_axis(actual_a);
            actual.tuser_to_tkeep();
            while (expected.size > 0 && actual.compare_no_user(expected[0],.PRINT_LVL(0))) begin
              void'(expected.pop_front());
              ++drop_count;
              ++pkt_num;
               $display("Droped packet %d",pkt_num);
              `ASSERT_ERROR(drop_count < EXPECT_DROPS,"Exceeded anticipated number of dropped packets e2c");
            end
            if (expected.size() > 0) begin
              ++pkt_num;
              $display("Rcvd packet   %d",pkt_num);
              void'(expected.pop_front());
            end
          end
          if (SV_ETH_IFC) begin
            $display("Verify drop count is %d",drop_count);
            reg_rd_check(REG_CPU_DROPPED,drop_count);
          end
        end else begin
          foreach(expected[i]) begin
            automatic CpuAxisPacket_t  actual_a;
            automatic CpuXportPacket_t actual = new();
            cpu.get(actual_a);
            actual.import_axis(actual_a);
            actual.tuser_to_tkeep();
            `ASSERT_ERROR(!actual.compare_w_sof(expected[i]),"failed to send packet to e2c");
          end
        end
      end
    join

    test_e2c.end_test();
  endtask : test_ethcpu

  task automatic wait_for_udp_packets(int udp_dest_port);
    automatic EthAxisPacket_t  actual_a;
    automatic raw_pkt_t        rcv_raw,rcv_pay;
    automatic udp_hdr_t        rcv_udp;
    automatic eth_hdr_t        rcv_eth;
    automatic ipv4_hdr_t       rcv_ip = DEFAULT_IPV4_HDR;
    automatic int              try_count = 0;

    do begin
      ++try_count;
      // check if packet is for our port
      #100;
      eth.peek(actual_a);
      rcv_raw = actual_a.dump_bytes();
      repeat(PREAMBLE_BYTES) rcv_raw.delete(0); // strip preamble
      decode_udp_pkt(rcv_raw,rcv_eth,rcv_ip,rcv_udp,rcv_pay);
      `ASSERT_ERROR(try_count != 100,"unclaimed packet on c2e");
      end while (rcv_udp.dest_port != udp_dest_port);

  endtask : wait_for_udp_packets


  task automatic test_cpueth(int num_samples[$]);
    TestExec test_c2e = new();
    automatic CpuXportPacket_t send[$];
    automatic EthXportPacket_t expected[$];
    automatic int sample_sum = 0;

    test_c2e.start_test({TEST_NAME,"::CPU to Ethernet"}, 60us);
    // This path is
    //   c2e -> (eth_adapter) s_cpu ->
    ////  (ARM_DEFRAMER)(IF ARM)
    //   (eth_adapater) c2e ->
    ////  (ETH_MUX)(SIZE=2)
    //   (eth_adapater) m_mac -> eth_tx

    foreach (num_samples[i]) begin
      automatic eth_hdr_t    eth_hdr;
      automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
      automatic udp_hdr_t    udp_hdr;
      automatic raw_pkt_t    pay,udp_raw;
      automatic int          preamble;

      expected[i] = new;
      send[i] = new;

      if      (PREAMBLE_BYTES == 6) preamble = ZERO_PREAMBLE;
      else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
      else    $fatal(1, "Invalid PREAMBLE_BYTES");

      get_ramp_raw_pkt(.num_samps(num_samples[i]),.ramp_start((sample_sum)%256),
                       .ramp_inc(1),.pkt(pay),.SWIDTH(8));
      sample_sum += num_samples[i];
      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,pay,
                              .preamble(NO_PREAMBLE));
      send[i].push_bytes(udp_raw);
      send[i].tkeep_to_tuser();

      // rebuild the expected packet for comparison with a zero preamble
      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,pay,
                              .preamble(preamble));
      if (SV_ETH_IFC) begin
        while (udp_raw.size < 64) begin
          udp_raw.push_back(0);
        end
      end;
      expected[i].push_bytes(udp_raw);
      expected[i].tkeep_to_tuser();
    end

    fork
      begin // tx_thread
        foreach(send[i])begin
          cpu.put(send[i]);
        end
      end
      begin //rx_thread
        foreach(expected[i]) begin
          automatic EthAxisPacket_t  actual_a;
          automatic EthXportPacket_t actual = new();
          automatic raw_pkt_t        rcv_raw,rcv_pay;
          automatic udp_hdr_t        rcv_udp;
          automatic eth_hdr_t        rcv_eth;
          automatic ipv4_hdr_t       rcv_ip = DEFAULT_IPV4_HDR;
          automatic int              try_count = 0;

          wait_for_udp_packets(DEF_DEST_UDP_PORT);
          eth.get(actual_a);
          actual.import_axis(actual_a);
          if (!SV_ETH_IFC) begin
            actual.tuser_to_tkeep();
          end
         `ASSERT_ERROR(!actual.compare_w_pad(expected[i],!SV_ETH_IFC),"failed to send packet to c2e");
        end
      end
    join
    test_c2e.end_test();

  endtask : test_cpueth
  //---------------------------------------------------------------------------
  // Ethernet to CHDR test
  //---------------------------------------------------------------------------

  function automatic word_queue_t bytes_to_words(raw_pkt_t pay);
    automatic ChdrXportPacket_t axis_pkt = new();

    axis_pkt.push_bytes(pay);
    return axis_pkt.data;

  endfunction : bytes_to_words;

  function automatic raw_pkt_t flatten_chdr(ChdrPacket_t chdr_pkt);
    automatic ChdrAxisPacket_t axis_chdr;
    automatic ChdrXportPacket_t xport_chdr = new();
    axis_chdr = chdr_pkt.chdr_to_axis();
    foreach (axis_chdr.data[i]) begin
      axis_chdr.keep[i] = '1;
      axis_chdr.user[i] = '0;
    end
    xport_chdr.import_axis(axis_chdr);
    return xport_chdr.dump_bytes();
  endfunction : flatten_chdr

  function automatic ChdrPacket_t unflatten_chdr(raw_pkt_t chdr_raw);
    automatic ChdrXportPacket_t xport_chdr = new();
    automatic ChdrPacket_t chdr_pkt = new();
    xport_chdr.push_bytes(chdr_raw);
    foreach (xport_chdr.data[i]) begin
      xport_chdr.keep[i] = '1;
      xport_chdr.user[i] = '0;
    end
    chdr_pkt.axis_to_chdr(xport_chdr);
    return chdr_pkt;
  endfunction : unflatten_chdr

  task automatic test_ethchdr(int num_samples[$], int ERROR_PROB=2, int EXPECT_DROPS=0);
    TestExec test_e2v = new();
    automatic EthXportPacket_t send[$];
    automatic ChdrXportPacket_t expected[$];
    automatic int sample_sum = 0;

    test_e2v.start_test({TEST_NAME,"::Ethernet to CHDR"}, 60us);
    // This path is
    //   eth_rx -> s_mac(eth_adapter) -> s_mac(eth_dispatch) ->
    ////   in_reg(AXI_FIFO)(SIZE=1)
    //   (eth_dispatch) in -> STATMACHINE (Dispatch) + chdr ->
    ////   chdr_user_fifo(AXI_FIFO)(SIZE=8) (capture eth header)
    ////   chdr_out_gate(AXI_GATE)(SIZE=11)
    //   (eth_dispatch) o_chdr ->
    ////   chdr_trim(CHDR_TRIM_PAYLOAD)
    //   (eth_dispatch) m_chdr -> (eth_adapater) e2x_chdr -> (xport_adapter_gen) s_axis_xport
    ////   xport_in_swap (AXIS_DATA_SWAP)
    //   (xport_adapter_gen) i_xport ->
    ////   mgmt_ep(CHDR_MGMT_PKT_HANDLER)
    //   (xport_adapter_gen) x2d ->
    ////   rtn_demux(AXI_SWITCH)  x2x(loopback) or m_axis_rfnoc
    //   (xport_adapter_gen) m_axis_rfnoc -> (eth_adapter) e2x_fifo
    ////   chdr_fifo(AXI_FIFO)(SIZE=MTU)
    //   (eth_adapater) m_chdr -> e2v

    foreach (num_samples[i]) begin
      automatic eth_hdr_t    eth_hdr;
      automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
      automatic udp_hdr_t    udp_hdr;
      automatic raw_pkt_t    pay,udp_raw,chdr_raw;

      automatic ChdrPacket_t chdr_pkt = new();
      automatic chdr_header_t chdr_hdr;
      automatic chdr_word_t chdr_ts;
      automatic chdr_word_t chdr_mdata[$];
      automatic chdr_word_t chdr_data[$];
      automatic int preamble;

      expected[i] = new;
      send[i] = new;

      if      (PREAMBLE_BYTES == 6) preamble = NORMAL_PREAMBLE;
      else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
      else    $fatal(1, "Invalid PREAMBLE_BYTES");

      // build a payload
      get_ramp_raw_pkt(.num_samps(num_samples[i]),.ramp_start((sample_sum)%256),
                       .ramp_inc(1),.pkt(pay),.SWIDTH(8));
      sample_sum += num_samples[i];
      // Fill data in the chdr packet
      chdr_hdr = '{
        vc        : 0,
        dst_epid  : 0,
        seq_num   : 0,
        pkt_type  : CHDR_DATA_NO_TS,
        num_mdata : 0,
        default   : 0
      };
      chdr_ts = 0;         // no timestamp
      chdr_mdata.delete(); // not adding meta data
      chdr_data = bytes_to_words(pay);

      chdr_pkt.write_raw(chdr_hdr, chdr_data, chdr_mdata, chdr_ts);
      chdr_raw =  flatten_chdr(chdr_pkt);

      //build a udp packet
      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,chdr_raw,
                              .preamble(preamble));
      send[i].push_bytes(udp_raw);
      send[i].tkeep_to_tuser(.ERROR_PROB(ERROR_PROB));

      // expect just the chdr packet (UDP stripped)
      expected[i].push_bytes(chdr_raw);
      expected[i].tkeep_to_tuser();

    end

    // iterate in descending order so deleting doesn't shift down
    // the packets in future loop iterations.
    for (int i=  num_samples.size()-1 ; i >= 0; i--) begin
      // original only checks for errors in last word.
      if (!SV_ETH_IFC && send[i].has_error()) send[i].set_error();
      // If a packet has an error it shouldn't make it through
      if (send[i].has_error()) expected.delete(i);//MAC ERROR
      else if (DROP_MIN_PACKET) begin
        if (send[i].byte_length()-PREAMBLE_BYTES < 64) begin
           // short packet rejection feature broken in original
           if (SV_ETH_IFC) expected.delete(i); // TOO SHORT
        end
      end
    end

    fork
      begin // tx_thread
        foreach(send[i])begin
          #1 eth.put(send[i]);
        end
      end
      begin //rx_thread
        if (EXPECT_DROPS > 0) begin
          automatic int pkt_num = 0;
          automatic int drop_count = 0;
          while (expected.size() > 0) begin
            automatic ChdrAxisPacket_t  actual_a;
            automatic ChdrXportPacket_t actual = new();
            v.get(actual_a);
            actual.import_axis(actual_a);
            actual.tuser_to_tkeep();
            while (expected.size > 0 && actual.compare_no_user(expected[0],.PRINT_LVL(0))) begin
              void'(expected.pop_front());
              ++drop_count;
              ++pkt_num;
               $display("Dropped packet %d",pkt_num);
              `ASSERT_ERROR(drop_count < EXPECT_DROPS,"Exceeded anticipated number of dropped packets e2v");
            end
            if (expected.size() > 0) begin
              ++pkt_num;
              $display("Rcvd packet   %d",pkt_num);
              void'(expected.pop_front());
            end
          end
          if (SV_ETH_IFC) begin
            $display("Verify drop count is %d",drop_count);
            reg_rd_check(REG_CHDR_DROPPED,drop_count);
          end
        end else begin
          foreach(expected[i]) begin
            automatic ChdrAxisPacket_t  actual_a;
            automatic ChdrXportPacket_t actual = new();
            v.get(actual_a);
            actual.import_axis(actual_a);
            actual.tuser_to_tkeep();
           `ASSERT_ERROR(!actual.compare_no_user(expected[i]),"failed to send packet e2v");
          end
        end
      end
    join
    test_e2v.end_test();

  endtask : test_ethchdr;

  task automatic test_chdreth(int num_samples[$]);
    TestExec test_v2e = new();
    automatic ChdrXportPacket_t send[$];
    automatic EthXportPacket_t expected[$];
    automatic int sample_sum = 0;

    test_v2e.start_test({TEST_NAME,"::CHDR to Ethernet"}, 60us);
    // This path is
    //   v2e -> s_chdr(eth_adapter) -> s_axis_rfnoc (xport_adapter_gen) ->
    ////   axi_demux_mgmt_filter (AXI_DEMUX) (IF ALLOW_DISC) (discards discovery packets)
    //   (xport_adapter_gen) f2m ->
    ////   rtn_mux(AXI_MUX) between x2x and f2m
    //   (xport_adapter_gen) m2x ->
    ////   data_fifo/lookup_fifo (AXI_FIFO_SHORT)
    ////   LOOKUP LOGIC (lookup_fifo,data_fifo,results)
    //   (xport_adapter_gen) o_xport ->
    ////   xport_out_swap (AXIS_DATA_SWAP)
    //   (xport_adapter_gen) m_axis_xport -> (eth_adapater) x2e_chdr ->
    ////   ENET_HDR_LOGIC (frame_state)
    //   (eth_adapater) frame -> (eth_adapater) x2e_framed
    ////  (ETH_MUX)(SIZE=2)
    //   (eth_adapater) m_mac -> eth_tx

    foreach (num_samples[i]) begin
      automatic eth_hdr_t    eth_hdr;
      automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
      automatic udp_hdr_t    udp_hdr;
      automatic raw_pkt_t    pay,udp_raw,chdr_raw;

      automatic ChdrPacket_t chdr_pkt = new();
      automatic chdr_header_t chdr_hdr;
      automatic chdr_word_t chdr_ts;
      automatic chdr_word_t chdr_mdata[$];
      automatic chdr_word_t chdr_data[$];
      automatic int preamble;

      expected[i] = new;
      send[i] = new;

      if      (PREAMBLE_BYTES == 6) preamble = ZERO_PREAMBLE;
      else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
      else    $fatal(1, "Invalid PREAMBLE_BYTES");

      // build a payload
      get_ramp_raw_pkt(.num_samps(num_samples[i]),.ramp_start((sample_sum)%256),
                       .ramp_inc(1),.pkt(pay),.SWIDTH(8));
      sample_sum += num_samples[i];

      // Fill data in the chdr packet
      chdr_hdr = '{
        vc        : 0,
        dst_epid  : 0,
        seq_num   : 0,
        pkt_type  : CHDR_DATA_NO_TS,
        num_mdata : 0,
        default   : 0
      };
      chdr_ts = 0;         // no timestamp
      chdr_mdata.delete(); // not adding meta data
      chdr_data = bytes_to_words(pay);

      chdr_pkt.write_raw(chdr_hdr, chdr_data, chdr_mdata, chdr_ts);
      chdr_raw =  flatten_chdr(chdr_pkt);

      // send the raw chedar packet
      send[i].push_bytes(chdr_raw);
      send[i].tkeep_to_tuser();

      //build a udp packet
      // modify as the EthInterface does
      udp_hdr.src_port = DEF_DEST_UDP_PORT;
      udp_hdr.dest_port = 0; // Extract from router lookup results (Default)
      udp_hdr.checksum = 0;  // Checksum not calculated at this point
      ipv4_hdr.src_ip = DEF_DEST_IP_ADDR;
      ipv4_hdr.dest_ip = 0; // Extract from router lookup results (Default)
      ipv4_hdr.dscp      = 0; // hardcoded
      ipv4_hdr.dont_frag = 1; // hardcoded
      ipv4_hdr.identification = 0; // hardcoded
      ipv4_hdr.time_to_live = 8'h10; //hardcoded
      eth_hdr.src_mac = DEF_DEST_MAC_ADDR;
      eth_hdr.dest_mac = 0; // Extract from router lookup results (Default)

      udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,chdr_raw,
                              .preamble(preamble));

      // expect udp wrapped chdr
      expected[i].push_bytes(udp_raw);
      if (IGNORE_EXTRA_DATA) begin
        expected[i].clear_user(); // expect all full words!
        expected[i].tuser_to_tkeep();
      end else begin
        expected[i].tkeep_to_tuser();
      end
    end

    fork
      begin // tx_thread
        foreach(send[i])begin
          v.put(send[i]);
        end
      end
      begin //rx_thread
        foreach(expected[i]) begin
          automatic EthAxisPacket_t  actual_a;
          automatic EthXportPacket_t actual = new();
          automatic eth_hdr_t    eth_hdr;
          automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
          automatic udp_hdr_t    udp_hdr;
          automatic raw_pkt_t    chdr_raw,actual_raw;
          automatic ChdrPacket_t chdr_pkt;
          automatic integer chdr_len;
          localparam UDP_LEN = 8/*udp*/+20/*ipv4*/+14/*eth-no vlan*/;

          wait_for_udp_packets(.udp_dest_port(0));
          eth.get(actual_a);
          actual.import_axis(actual_a);
          actual_raw = actual.dump_bytes();
          repeat(PREAMBLE_BYTES) void'(actual_raw.pop_front());
          decode_udp_pkt(actual_raw,eth_hdr,ipv4_hdr,udp_hdr,chdr_raw);
          chdr_pkt = unflatten_chdr(chdr_raw);
          // fills remainder of packet with zeros
          if (IGNORE_EXTRA_DATA) begin
            for (int w=chdr_pkt.header.length+UDP_LEN;w <actual_raw.size();w++) begin
              actual_raw[w] = '0;
            end
            repeat(PREAMBLE_BYTES) actual_raw.push_front(0);
            actual.empty();
            actual.push_bytes(actual_raw);
          end
          `ASSERT_ERROR(!actual.compare_w_error(expected[i]),"failed to send packet v2e");
        end
      end
    join
    test_v2e.end_test();

  endtask : test_chdreth

  integer cached_mgmt_seqnum = 0;

  `include "../../core/rfnoc_chdr_utils.vh"
  `include "../../core/rfnoc_chdr_internal_utils.vh"
  `include "../../xport/rfnoc_xport_types.vh"
  task automatic test_chdr_endpoint();
    TestExec test_e2v = new();
    automatic EthXportPacket_t send[$];
    automatic EthXportPacket_t expected[$];
    automatic int sample_sum = 0;

    automatic eth_hdr_t    eth_hdr;
    automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
    automatic udp_hdr_t    udp_hdr;
    automatic raw_pkt_t    pay,udp_raw,chdr_raw;

    automatic ChdrPacket_t chdr_pkt = new();
    automatic chdr_header_t  chdr_hdr;
    automatic chdr_mgmt_t    mgmt_pl;
    automatic chdr_mgmt_op_t exp_mgmt_op;

    automatic chdr_word_t chdr_ts;
    automatic chdr_word_t chdr_mdata[$];
    automatic chdr_word_t chdr_data[$];
    automatic logic [47:0] node_info;
    automatic int preamble;
    localparam NODE_INST=0;

    test_e2v.start_test({TEST_NAME,"::ChdrEndpoint"}, 60us);


    expected[0] = new;
    send[0] = new;

    if      (PREAMBLE_BYTES == 6) preamble = NORMAL_PREAMBLE;
    else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
    else    $fatal(1, "Invalid PREAMBLE_BYTES");

    // Generic management header
    mgmt_pl.header = '{
      default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:1
    };
    // Send a node info request to the crossbar
    mgmt_pl.header.num_hops = 2;
    mgmt_pl.ops.delete();
    mgmt_pl.ops[0] = '{  // Hop 1: Send node info
      op_payload:48'h0, op_code:MGMT_OP_INFO_REQ, ops_pending:8'd1};
    mgmt_pl.ops[1] = '{  // Hop 1: Return
      op_payload:48'h0, op_code:MGMT_OP_RETURN, ops_pending:8'd0};
    mgmt_pl.ops[2] = '{  // Hop 2: Nop for return
      op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd0};
    chdr_hdr = '{
      pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum, dst_epid:16'h0, default:'0};
    chdr_pkt.write_mgmt(chdr_hdr, mgmt_pl);
    chdr_raw =  flatten_chdr(chdr_pkt);

    //build a udp packet
    udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,chdr_raw,
                            .preamble(preamble));
    send[0].push_bytes(udp_raw);
    send[0].tkeep_to_tuser();

   // build expected response
   // Generic management header
    if      (PREAMBLE_BYTES == 6) preamble = ZERO_PREAMBLE;
    else if (PREAMBLE_BYTES == 0) preamble = NO_PREAMBLE;
    else    $fatal(1, "Invalid PREAMBLE_BYTES");

    mgmt_pl.header = '{
      default:'0, prot_ver:PROTOVER, chdr_width:translate_chdr_w(CHDR_W), src_epid:0
    };
    // Send a node info request to the crossbar
    mgmt_pl.header.num_hops = 1;
    mgmt_pl.ops.delete();
    mgmt_pl.ops[0] = '{  // Hop 2: Nop for return
      op_payload:48'h0, op_code:MGMT_OP_NOP, ops_pending:8'd1};

    node_info = chdr_mgmt_build_node_info({ 10'h0, NODE_SUBTYPE_XPORT_IPV4_CHDR64},
                                           NODE_INST, NODE_TYPE_TRANSPORT, device_id);
    mgmt_pl.ops[1] = '{op_payload:node_info,
                       op_code:MGMT_OP_INFO_RESP, ops_pending:8'd0};

    chdr_hdr = '{
      pkt_type:CHDR_MANAGEMENT, seq_num:cached_mgmt_seqnum++, dst_epid:16'h1, default:'0};
    chdr_pkt.write_mgmt(chdr_hdr, mgmt_pl);
    chdr_raw =  flatten_chdr(chdr_pkt);

    // build a udp packet
    // modify as the EthInterface does
    udp_hdr.src_port = DEF_DEST_UDP_PORT;
    udp_hdr.dest_port = DEF_SRC_UDP_PORT; // Extract from router lookup results (Default)
    udp_hdr.checksum = 0;  // Checksum not calculated at this point
    ipv4_hdr.src_ip = DEF_DEST_IP_ADDR;
    ipv4_hdr.dest_ip = DEF_SRC_IP_ADDR; // Extract from router lookup results (Default)
    ipv4_hdr.dscp      = 0; // hardcoded
    ipv4_hdr.dont_frag = 1; // hardcoded
    ipv4_hdr.identification = 0; // hardcoded
    ipv4_hdr.time_to_live = 8'h10; //hardcoded
    eth_hdr.src_mac = DEF_DEST_MAC_ADDR;
    eth_hdr.dest_mac = DEF_SRC_MAC_ADDR; // Extract from router lookup results (Default)

    udp_raw = build_udp_pkt(eth_hdr,ipv4_hdr,udp_hdr,chdr_raw,
                            .preamble(preamble));

    expected[0].push_bytes(udp_raw);
    if (IGNORE_EXTRA_DATA) begin
      expected[0].clear_user(); // expect all full words!
      expected[0].tuser_to_tkeep();
    end else begin
      expected[0].tkeep_to_tuser();
    end

    fork
      begin // tx_thread
        foreach(send[i])begin
          #1 eth.put(send[i]);
        end
      end
      begin //rx_thread
        foreach(expected[i]) begin
          automatic EthAxisPacket_t  actual_a;
          automatic EthXportPacket_t actual = new();
          automatic eth_hdr_t    eth_hdr;
          automatic ipv4_hdr_t   ipv4_hdr = DEFAULT_IPV4_HDR;
          automatic udp_hdr_t    udp_hdr;
          automatic raw_pkt_t    chdr_raw,actual_raw;
          automatic ChdrPacket_t chdr_pkt;
          automatic integer chdr_len;
          localparam UDP_LEN = 8/*udp*/+20/*ipv4*/+14/*eth-no vlan*/;

          eth.get(actual_a);
          actual.import_axis(actual_a);
          actual_raw = actual.dump_bytes();
          repeat(PREAMBLE_BYTES) void'(actual_raw.pop_front());
          decode_udp_pkt(actual_raw,eth_hdr,ipv4_hdr,udp_hdr,chdr_raw);
          chdr_pkt = unflatten_chdr(chdr_raw);
          // fills remainder of packet with zeros
          if (IGNORE_EXTRA_DATA) begin
            for (int w=chdr_pkt.header.length+UDP_LEN;w <actual_raw.size();w++) begin
              actual_raw[w] = '0;
            end
            repeat(PREAMBLE_BYTES) actual_raw.push_front(0);
            actual.empty();
            actual.push_bytes(actual_raw);
         end
         `ASSERT_ERROR(!actual.compare_no_user(expected[i]),"failed to get node info");
        end
      end
    join
    test_e2v.end_test();

  endtask : test_chdr_endpoint
  //----------------------------------------------------
  // Main test loop
  //----------------------------------------------------
  initial begin : tb_main
   automatic int num_samples[$];
   automatic int cpu_num_samples[$];
   automatic int expected_drops;
   localparam QUICK = 0;

   test.start_tb(TEST_NAME);

   test.start_test({TEST_NAME,"::Wait for Reset"}, 10us);
   clk_gen.start();
   eth_clk_gen.start();
   clk_gen.reset();
   eth_clk_gen.reset();

   eth.run();
   cpu.run();
   v.run();


   test_reset();
   test_registers();

   test_chdr_endpoint();

   // Check what happens if the input bandwidth exceeds
   // the devices ability to consume packets
   // This can happen in matched bandwidth cases
   // if there is hold off from upstream
   // Dropped packets exceed the drop count cause an error
   // The actual dropped count is compared versus the real count
   test.start_test({TEST_NAME,"::Input overrun"}, 200us);

   eth.set_master_stall_prob(0);
   eth.set_slave_stall_prob(0);
   cpu.set_master_stall_prob(0);
   cpu.set_slave_stall_prob(0);
   v.set_master_stall_prob(0);
   v.set_slave_stall_prob(0);

   num_samples = {7936,7936,7936,7936,7936,320,
                  7936,7936,7936,7936,7936,320};

   // The actual number of expected drops depends on the
   // bus width difference between ENET_W and CHDR/CPU_W

   // in this SIM unlimited Ethernet bandwidth is coming in at over 300 MHZ
   // and output runs at 200 MHZ.  This causes excess BW on transmitter even when matched.
   expected_drops = 9;

   test_ethchdr(num_samples,.EXPECT_DROPS(expected_drops),.ERROR_PROB(0));
   test_ethcpu(num_samples,.EXPECT_DROPS(expected_drops),.ERROR_PROB(0));
   test.end_test();

   eth.set_master_stall_prob(38);
   eth.set_slave_stall_prob(38);
   cpu.set_master_stall_prob(38);
   cpu.set_slave_stall_prob(38);
   v.set_master_stall_prob(38);
   v.set_slave_stall_prob(38);

   num_samples = {1,2,3,4,5,6,7,8,
                  ENET_W/8-1,ENET_W/8,ENET_W/8+1,
                  2*ENET_W/8-1,2*ENET_W/8,2*ENET_W/8+1,
                  CPU_W/8-1,CPU_W/8,CPU_W/8+1,
                  2*CPU_W/8-1,2*CPU_W/8,2*CPU_W/8+1,
                  CHDR_W/8-1,CHDR_W/8,CHDR_W/8+1,
                  2*CHDR_W/8-1,2*CHDR_W/8,2*CHDR_W/8+1
                 };
   // add some extra samples for CPU packets to try to get above the min
   //packet size of 64. (just the headers makes up 42 bytes)
   //this way we still have some short packets to drop, but not as many.
   foreach (num_samples[i]) cpu_num_samples[i] = num_samples[i]+20;
   test.start_test({TEST_NAME,"::PacketW Combos NO Errors"}, 10us);
   fork // run in parallel
     // ethrx
     test_ethcpu(cpu_num_samples,.ERROR_PROB(0));
     test_ethchdr(num_samples,.ERROR_PROB(0));
     // ethtx
     test_chdreth(num_samples);
     test_cpueth(num_samples);
   join
   test.end_test();

   if (!QUICK) begin

     test.start_test({TEST_NAME,"::PacketW Combos Errors"}, 10us);
     fork // run in parallel
       // ethrx
       test_ethcpu(cpu_num_samples,.ERROR_PROB(2));
       test_ethchdr(num_samples,.ERROR_PROB(2));
       // ethtx
       test_chdreth(num_samples);
       test_cpueth(num_samples);
     join
     test.end_test();

     num_samples = {16,32,64,128,256,512,1024,1500,1522,9000};
     test.start_test({TEST_NAME,"::Pwr2 NoErrors"}, 60us);
     fork // run in parallel
       // ethrx
       test_ethcpu(cpu_num_samples,.ERROR_PROB(0));
       test_ethchdr(num_samples,.ERROR_PROB(0));
       // ethtx
       test_chdreth(num_samples);
       test_cpueth(num_samples);
    join
     test.end_test();
   end

   eth.set_master_stall_prob(0);
   eth.set_slave_stall_prob(0);
   cpu.set_master_stall_prob(0);
   cpu.set_slave_stall_prob(0);
   v.set_master_stall_prob(0);
   v.set_slave_stall_prob(0);

   num_samples = {1,2,3,4,5,6,7,8,
                  ENET_W/8-1,ENET_W/8,ENET_W/8+1,
                  2*ENET_W/8-1,2*ENET_W/8,2*ENET_W/8+1,
                  CPU_W/8-1,CPU_W/8,CPU_W/8+1,
                  2*CPU_W/8-1,2*CPU_W/8,2*CPU_W/8+1,
                  CHDR_W/8-1,CHDR_W/8,CHDR_W/8+1,
                  2*CHDR_W/8-1,2*CHDR_W/8,2*CHDR_W/8+1
                 };
   test.start_test({TEST_NAME,"::Pktw NoStall+Error"}, 10us);
   fork // run in parallel
     // ethrx
     test_ethcpu(cpu_num_samples,.ERROR_PROB(2));
     test_ethchdr(num_samples,.ERROR_PROB(2));
     // ethtx
     test_chdreth(num_samples);
     test_cpueth(num_samples);
   join
   test.end_test();

   // repeat with back to back cpu/chdr packets
   test.start_test({TEST_NAME,"::Serial Pktw NoStall+Error"}, 10us);
   fork // run in parallel
     // ethrx
     begin
       test_ethcpu(cpu_num_samples,.ERROR_PROB(2));
       test_ethchdr(num_samples,.ERROR_PROB(2));
     end
     // ethtx
     begin
       test_chdreth(num_samples);
       test_cpueth(num_samples);
     end
   join
   test.end_test();

   // End the TB, but don't $finish, since we don't want to kill other
   // instances of this testbench that may be running.
   test.end_tb(0);

   // Kill the clocks to end this instance of the testbench
   clk_gen.kill();
   eth_clk_gen.kill();
  end // initial begin

endmodule
