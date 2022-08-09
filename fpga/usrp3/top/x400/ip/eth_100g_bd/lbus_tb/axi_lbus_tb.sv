//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_lbus_tb
//
// Description:
//
//   Testbench for eth_100g_axi2lbus.
//

module axi_lbus_tb #(
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
  localparam SEG_BYTES       = SEG_DATA_WIDTH/8;

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

  //----------------------------------------------------
  // DUT
  //----------------------------------------------------
  lbus_t lbus_out [NUM_SEG-1:0];
  lbus_t lbus_g   [NUM_SEG-1:0];
  logic lbus_rdy = 1;

  eth_100g_axi2lbus #(.FIFO_DEPTH(5),.NUM_SEG(NUM_SEG)) DUT (
    .axis(axis),
    .lbus_rdy(lbus_rdy),
    .lbus_out(lbus_out)
  );

  //----------------------------------------------------
  // Xilinx golden model
  // When Xilinx is generated with an AXI interface xilinx prints
  // an axi2lbus converter that has trouble meeting timing
  // this is preserved to allow comparison to that
  // TODO: remove when timing is passing and refactor is done
  //----------------------------------------------------
/*
  eth_100g_bd_cmac_usplus_0_0_axis2lbus_segmented_top  GOLD (
    .core_clk(clk),
    .core_rst(rst),

    // AXIS IF
    .axis_tvalid(axis.tvalid),
    .axis_tready(),
    .axis_tdata(axis.tdata),
    .axis_tlast(axis.tlast),
    .axis_tkeep(axis.tkeep),
    .axis_tuser(1'b0),
    // LBUS IF
    .lbus_rdyout(lbus_rdy),
    .lbus_ovfout(1'b0),
    .lbus_unfout(1'b0),
    // Segment 0
    .lbus_ena0(lbus_g[0].ena),
    .lbus_data0(lbus_g[0].data),
    .lbus_sop0(lbus_g[0].sop),
    .lbus_eop0(lbus_g[0].eop),
    .lbus_mty0(lbus_g[0].mty),
    .lbus_err0(lbus_g[0].err),
    // Segment 1
    .lbus_ena1(lbus_g[1].ena),
    .lbus_data1(lbus_g[1].data),
    .lbus_sop1(lbus_g[1].sop),
    .lbus_eop1(lbus_g[1].eop),
    .lbus_mty1(lbus_g[1].mty),
    .lbus_err1(lbus_g[1].err),
    // Segment 2
    .lbus_ena2(lbus_g[2].ena),
    .lbus_data2(lbus_g[2].data),
    .lbus_sop2(lbus_g[2].sop),
    .lbus_eop2(lbus_g[2].eop),
    .lbus_mty2(lbus_g[2].mty),
    .lbus_err2(lbus_g[2].err),
    // Segment 3
    .lbus_ena3(lbus_g[3].ena),
    .lbus_data3(lbus_g[3].data),
    .lbus_sop3(lbus_g[3].sop),
    .lbus_eop3(lbus_g[3].eop),
    .lbus_mty3(lbus_g[3].mty),
    .lbus_err3(lbus_g[3].err)
  );
*/
  //----------------------------------------------------
  //BFMS
  //----------------------------------------------------

  TestExec test = new();
  AxiStreamBfm #(.DATA_WIDTH(DATA_WIDTH),.USER_WIDTH(USER_WIDTH)) axi =
    new(.slave(null),.master(axis));

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


  task automatic check_lbus(AxisPacket_t packets[$]);

    int idle_insert = 0;
    int byte_count = 0;
    int first_clk   = 1;

    // loop over packets
    foreach(packets[i]) begin
      automatic raw_pkt_t pay;
      pay = packets[i].dump_bytes;

      first_clk=1;
      while (pay.size() > 0) begin
        @(posedge clk);
        if (lbus_rdy) begin
          if (lbus_out[0].ena) begin
            for (int s=0; s < NUM_SEG; s++) begin
              byte_count = 0;
              for (int b = SEG_DATA_WIDTH/8-1; b >= 0; b--) begin
                if (pay.size() > 0) begin
                  assert (lbus_out[s].data[b*8 +: 8]  == pay.pop_front()) else $error("Data Mismatch");
                  byte_count++;
                end
              end
              assert (lbus_out[s].ena == (byte_count > 0) ) else $error("Ena Mismatch");
              if (lbus_out[s].ena) begin
                if (first_clk && s==0) begin
                  assert (lbus_out[s].sop == 1) else $error("Sop not set");
                end else begin
                  assert (lbus_out[s].sop == 0) else $error("Sop Set");
                end
                assert (lbus_out[s].mty == (SEG_BYTES-byte_count) % SEG_BYTES) else $error("Mty Mismatch");
                assert (lbus_out[s].eop != (pay.size() > 0) ) else $error("Eop Mismatch");
                assert (lbus_out[s].err == 0) else $error("Err Mismatch"); // not checking error yet
              end else begin
                assert (lbus_out[s].sop == 0) else $error("Sop Set while disabled");
                assert (lbus_out[s].mty == 0) else $error("Mty set while disabled");
                assert (lbus_out[s].eop == 0) else $error("Eop set while disabled");
                assert (lbus_out[s].err == 0) else $error("Err set while disabled");
              end
            end // segment loop
            first_clk=0;
          end // first segment is enabled

        end else begin //not lbus_rdy
           for (int s=0; s < NUM_SEG; s++) begin
             assert (lbus_out[s].ena == 0) else $error("Idle Ena set");
             assert (lbus_out[s].err == 0) else $error("Idle Err set");
             assert (lbus_out[s].sop == 0) else $error("Idle Sop set");
             assert (lbus_out[s].eop == 0) else $error("Idle Eop set");
             assert (lbus_out[s].mty == 0) else $error("Idle Mty non zero");
           end
        end
      end // while pay.size > 0
    end // foreach packet

  endtask : check_lbus;

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
        foreach (send[i]) begin
          axi.put(send[i]);
        end
      end
      begin //rx_thread
        check_lbus(expected);
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
   axi.set_master_stall_prob(0);
   axi.run();
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
