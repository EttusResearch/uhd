//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: lbus_axi_tb
//
// Description:
//
//   Testbench for eth_100g_lbus2axi.
//

module lbus_axi_tb #(
  parameter TEST_NAME  = ""
)(
  /* no IO */
);
  // Include macros and time declarations for use with PkgTestExec
  `define TEST_EXEC_OBJ test
  `include "test_exec.svh"
  import PkgAxiStreamBfm::*;
  import PkgEthernet::*;
  import PkgTestExec::*;
  import PkgEth100gLbus::*;

  localparam DATA_WIDTH      = 512;
  localparam USER_WIDTH      = $clog2(DATA_WIDTH/8)+1;
  localparam NUM_SEG         = 4;
  localparam SEG_DATA_WIDTH  = DATA_WIDTH/NUM_SEG;
  localparam SEG_BYTES      = SEG_DATA_WIDTH/8;

  //----------------------------------------------------
  // clocks
  //----------------------------------------------------
  logic clk;
  logic rst;

  sim_clock_gen #(.PERIOD(5), .AUTOSTART(1))
    clk_gen (.clk(clk), .rst(rst));

  //----------------------------------------------------
  //interfaces
  //----------------------------------------------------
  AxiStreamIf #(.DATA_WIDTH(DATA_WIDTH),.USER_WIDTH(USER_WIDTH))
    axis    (clk, rst);

  AxiStreamIf #(.DATA_WIDTH(DATA_WIDTH),.USER_WIDTH(USER_WIDTH))
    axig    (clk, rst);

  //----------------------------------------------------
  // DUT
  //----------------------------------------------------
  lbus_t lbus_in [NUM_SEG-1:0];

  eth_100g_lbus2axi #(.FIFO_DEPTH(5),.NUM_SEG(NUM_SEG)) DUT (
    .axis(axis),
    .lbus_in(lbus_in)
  );

  //----------------------------------------------------
  // Xilinx golden model
  // When Xilinx is generated with an AXI interface xilinx prints
  // a lbus2axis converter that has trouble meeting timing
  // this is preserved to allow comparison to that
  // TODO: remove when timing is passing and refactor is done
  //----------------------------------------------------
/*
  eth_100g_bd_cmac_usplus_0_0_lbus2axis_segmented_top  GOLD (
    .core_clk(clk),
    .core_rst(rst),

    // AXIS IF
    .axis_tvalid(axig.tvalid),
    .axis_tdata(axig.tdata),
    .axis_tlast(axig.tlast),
    .axis_tkeep(axig.tkeep),
    .axis_tuser(),
    // Segment 0
    .lbus_ena0(lbus_in[0].ena),
    .lbus_data0(lbus_in[0].data),
    .lbus_sop0(lbus_in[0].sop),
    .lbus_eop0(lbus_in[0].eop),
    .lbus_mty0(lbus_in[0].mty),
    .lbus_err0(lbus_in[0].err),
    // Segment 1
    .lbus_ena1(lbus_in[1].ena),
    .lbus_data1(lbus_in[1].data),
    .lbus_sop1(lbus_in[1].sop),
    .lbus_eop1(lbus_in[1].eop),
    .lbus_mty1(lbus_in[1].mty),
    .lbus_err1(lbus_in[1].err),
    // Segment 2
    .lbus_ena2(lbus_in[2].ena),
    .lbus_data2(lbus_in[2].data),
    .lbus_sop2(lbus_in[2].sop),
    .lbus_eop2(lbus_in[2].eop),
    .lbus_mty2(lbus_in[2].mty),
    .lbus_err2(lbus_in[2].err),
    // Segment 3
    .lbus_ena3(lbus_in[3].ena),
    .lbus_data3(lbus_in[3].data),
    .lbus_sop3(lbus_in[3].sop),
    .lbus_eop3(lbus_in[3].eop),
    .lbus_mty3(lbus_in[3].mty),
    .lbus_err3(lbus_in[3].err)
  );
*/
  //----------------------------------------------------
  //BFMS
  //----------------------------------------------------

  TestExec test = new();
  AxiStreamBfm #(.DATA_WIDTH(DATA_WIDTH),.USER_WIDTH(USER_WIDTH)) axi =
    new(.slave(axis),.master(null));

  //---------------------------------------------------------------------------
  // Tests
  //---------------------------------------------------------------------------

  // use Test timeout to check reset goes away
  task test_reset();
    test.start_test({TEST_NAME,"Wait for Reset"}, 10us);
    wait(!rst);
    repeat (10) @(posedge clk);
    test.end_test();
  endtask : test_reset

  typedef AxiStreamPacket #(DATA_WIDTH,USER_WIDTH) AxisPacket_t;
  typedef XportStreamPacket #(DATA_WIDTH) XportPacket_t;

  task automatic clear_lbus_in();
    for (int i=0 ; i < NUM_SEG ; ++i) begin
      lbus_in[i].data = 'b0;
      lbus_in[i].mty  = 'b0;
      lbus_in[i].sop  = 1'b0;
      lbus_in[i].eop  = 1'b0;
      lbus_in[i].err  = 1'b0;
      lbus_in[i].ena  = 1'b0;
    end
  endtask : clear_lbus_in;

  task automatic send_lbus(AxisPacket_t packets[$]);

    int seg = 0;
    int b   = 0;
    int idle_insert = 0;

    // loop over packets
    foreach(packets[i]) begin
      automatic raw_pkt_t pay;
      pay = packets[i].dump_bytes;
      // set for the first segment
      lbus_in[seg].sop = 1;
      // empty this packets payload
      while (pay.size() > 0) begin
         lbus_in[seg].ena = 1;
         lbus_in[seg].data |= pay.pop_front()<<(SEG_DATA_WIDTH-b*8-8);
         if (pay.size() == 0) begin
           lbus_in[seg].eop = 1;
           lbus_in[seg].mty = SEG_DATA_WIDTH-1-b;
         end
         if (seg == NUM_SEG-1 &&  b == SEG_DATA_WIDTH/8-1) begin
           @(posedge clk);
           clear_lbus_in();
           idle_insert++;
           // insert period idle period
           if (idle_insert > 2) begin
             @(posedge clk);
             idle_insert =0;
           end
         end
         b = (b+1) % (SEG_DATA_WIDTH/8);
         if (b==0) begin
           seg = (seg+1) % NUM_SEG;
         end
      end // pay.size > 0
      if (b != 0) begin
        seg = (seg+1) % NUM_SEG;
        if (seg == 0) begin
          @(posedge clk);
          clear_lbus_in();
          idle_insert++;
           // insert period idle period
           if (idle_insert > 2) begin
             @(posedge clk);
             idle_insert =0;
           end
        end
        b   = 0;
      end

      // 25% of the time we start a new packet
      if ($urandom_range(99) < 25) begin
        b   = 0;
        seg = 0;
        // 25% of the time add an idle cycle
        if ($urandom_range(99) < 25) begin
          @(posedge clk);
          clear_lbus_in();
          idle_insert++;
           // insert period idle period
           if (idle_insert > 2) begin
             idle_insert =0;
             @(posedge clk);
           end
          @(posedge clk);
        end else begin
          @(posedge clk);
          clear_lbus_in();
          idle_insert++;
           // insert period idle period
           if (idle_insert > 2) begin
             idle_insert =0;
             @(posedge clk);
           end
        end
      end

    end // foreach packet

    @(posedge clk);
    clear_lbus_in();

  endtask : send_lbus;

  task automatic compare_packet(AxisPacket_t actual, expected);

    automatic XportPacket_t actual_copy = new();
    automatic XportPacket_t expected_copy = new();
    actual_copy.import_axis(actual);
    expected_copy.import_axis(expected);

    expected_copy.tkeep_to_tuser();
    actual_copy.clear_unused_bytes();

    if (!expected_copy.equal(actual_copy)) begin
      $display("Expected");
      expected_copy.print();
      $display("Actual");
      actual_copy.print();
      if (!expected_copy.equal(actual_copy))
        $error("ERROR :: packet mismatch");

    end

  endtask : compare_packet;

  task automatic test_transfers(int num_samples[$]);
    automatic AxisPacket_t send[$];
    automatic AxisPacket_t expected[$];
    automatic int sample_sum = 0;

    test.start_test({TEST_NAME,"Test Transfer"}, 200us);

    foreach (num_samples[i]) begin
      automatic raw_pkt_t    pay;

      expected[i] = new;
      send[i] = new;

      get_ramp_raw_pkt(.num_samps(num_samples[i]),.ramp_start((sample_sum)%256),
                       .ramp_inc(1),.pkt(pay),.SWIDTH(8));
      sample_sum += num_samples[i];
      send[i].push_bytes(pay);

      // rebuild the expected packet for comparison without the preamble
      expected[i].push_bytes(pay);
    end

    fork
      begin // tx_thread
        send_lbus(send);
      end
      begin //rx_thread
        foreach(expected[i]) begin
          automatic AxisPacket_t  actual;
          axi.get(actual);
          compare_packet(actual,expected[i]);
        end
      end
    join

    test.end_test();
  endtask : test_transfers

  //----------------------------------------------------
  // Main test loop
  //----------------------------------------------------
  initial begin : tb_main
   automatic int num_samples[$];
   automatic int random_value;
   clk_gen.reset();

   // stalling is not allowed by whoever is reading the MAC
   // i.e. ready is ignored
   axi.slave_tready_init = 1'b1;
   axi.set_master_stall_prob(0);
   axi.set_slave_stall_prob(0);
   axi.run();
   clear_lbus_in();
   test_reset();

   $display("Fixed Sequences");
   num_samples = {64,65,66,67,68,69,70,71};
   test_transfers(num_samples);
   num_samples = {64,128,256,512,256,64,64,64};
   test_transfers(num_samples);
   num_samples = {65,64,64,64,64,64,64,64};
   test_transfers(num_samples);
   num_samples = {65,64,80,80,80,80,80,80};
   test_transfers(num_samples);
   num_samples = {64,64,96,80,96,80,96,80};
   test_transfers(num_samples);
   $display("Tons of 64");
   num_samples.delete();
   repeat(1000) begin
     num_samples.push_back(64);
   end
   test_transfers(num_samples);
   $display("Tons of 65");
   num_samples.delete();
   repeat(1000) begin
     num_samples.push_back(65);
   end
   test_transfers(num_samples);
   $display("Tons of Random");
   num_samples.delete();
   repeat(1000) begin
     random_value = $urandom_range(64,512);
     num_samples.push_back(random_value);
   end
   test_transfers(num_samples);


   // End the TB, but don't $finish, since we don't want to kill other
   // instances of this testbench that may be running.
   test.end_tb(0);

   // Kill the clocks to end this instance of the testbench
   clk_gen.kill();
  end // initial begin

endmodule
