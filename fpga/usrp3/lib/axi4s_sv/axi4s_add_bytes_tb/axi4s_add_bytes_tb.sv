//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_add_bytes_tb
//
// Description: Testbench for axi_add_bytes
//

module axi4s_add_bytes_tb #(
  parameter TEST_NAME = "axi_add_bytes",
  WIDTH=32,ADD_START=0,ADD_BYTES=7
);
  // Include macros and time declarations for use with PkgTestExec
  // To change the name of the TestExec object being used by the assertion
  // macros, `define TEST_EXEC_OBJ before including this file and `undef it at
  // the end of your testbench. Otherwise, it defaults to the shared object
  // "PkgTestExec::test".
  `define TEST_EXEC_OBJ test
  `include "test_exec.svh"
  import PkgAxiStreamBfm::*;
  import PkgTestExec::*;
  import PkgEthernet::*;

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------
  localparam UWIDTH = $clog2((WIDTH/8)+1);
  localparam MAX_PACKET_BYTES = 16*1024;  
  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit clk;
  bit reset;

  sim_clock_gen #(.PERIOD(5.0), .AUTOSTART(1))
    clk_gen (.clk(clk), .rst(reset));

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------
  TestExec test = new();
  AxiStreamIf #(.DATA_WIDTH(WIDTH),.USER_WIDTH(UWIDTH),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES)) 
    i (clk, reset);
  AxiStreamIf #(.DATA_WIDTH(WIDTH),.USER_WIDTH(UWIDTH),.TKEEP(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES)) 
    o (clk, reset);

  // Bus functional model for a axi_stream controller
  AxiStreamBfm #(.DATA_WIDTH(WIDTH),.USER_WIDTH(UWIDTH),.TKEEP(0),
                 .MAX_PACKET_BYTES(MAX_PACKET_BYTES)) 
    axis = new(.master(i), .slave(o));


  //----------------------------------------------------
  // Instantiate DUT
  //----------------------------------------------------
  axi4s_add_bytes #(.ADD_START(ADD_START),.ADD_BYTES(ADD_BYTES))
    DUT (.*);

  //---------------------------------------------------------------------------
  // Reset
  //---------------------------------------------------------------------------

  task test_reset();
    test.start_test("Wait for Reset", 10us);
    clk_gen.reset();
    wait(!reset);
    repeat (10) @(posedge clk);
    test.end_test();
  endtask : test_reset

  //---------------------------------------------------------------------------
  // Ethernet to CPU test
  //---------------------------------------------------------------------------
  typedef AxiStreamPacket #(WIDTH, UWIDTH) AxisPacket_t;
  typedef XportStreamPacket #(WIDTH) XportPacket_t;

  task test_samples(int num_samples);
    raw_pkt_t    raw;

    localparam PKTS_TO_SEND = 10;
    automatic XportPacket_t send[$];
    automatic XportPacket_t expected[$];

    test.start_test({TEST_NAME,"::Remove Bytes"}, 10us);

    for (int i= 0 ; i < PKTS_TO_SEND; i++) begin

      expected[i] = new;
      send[i] = new;

      get_ramp_raw_pkt(.num_samps(num_samples),.ramp_start((i*num_samples)%256),.ramp_inc(1),.pkt(raw),.SWIDTH(8));
      send[i].push_bytes(raw);
      // Add the bytes to the raw packet to build expected.
      for (int i = 0 ; i < ADD_BYTES; ++i) begin
         raw.insert(ADD_START,0);
      end
      expected[i].push_bytes(raw);
      send[i].tkeep_to_tuser(.ERROR_PROB(10));
      expected[i].tkeep_to_tuser();
      if (send[i].has_error()) expected[i].set_error();
    end

    fork
      begin // Send Thread
        foreach(send[i])begin
           axis.put(send[i]);
        end
      end
      begin // Expected Thread
        foreach(expected[i]) begin
          automatic string str;
          automatic AxisPacket_t  actual_a = new();
          automatic XportPacket_t actual   = new();
          axis.get(actual_a);
          actual.import_axis(actual_a);
          actual.tuser_to_tkeep();
          str = $sformatf("ADD_START=%3d ADD_BYTES=%3d",ADD_START,ADD_BYTES);
          `ASSERT_ERROR(!actual.compare_w_error(expected[i]),str);
        end
      end
    join


    test.end_test();

  endtask : test_samples


  //----------------------------------------------------
  // Main test loop
  //----------------------------------------------------
  initial begin : tb_main
   integer min_length;

   test.tb_name = TEST_NAME;
   min_length = ADD_START+1;


   axis.run();

   test_reset();

   // Test with default holdoff.
   for (int i=1 ; i < (WIDTH/8)*3 ; ++i) begin
     test_samples(min_length+i);
   end

   // repeat back to back
   axis.set_slave_stall_prob(0);
   axis.set_master_stall_prob(0);
   for (int i=1 ; i < (WIDTH/8)*3 ; ++i) begin
     test_samples(min_length+i);
   end


   // End the TB, but don't $finish, since we don't want to kill other
   // instances of this testbench that may be running.
   test.end_tb(0);

   // Kill the clocks to end this instance of the testbench
   clk_gen.kill();

  end // initial begin

endmodule
`undef TEST_EXEC_OBJ
