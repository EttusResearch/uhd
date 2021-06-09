//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_remove_bytes_tb
//
// Description: Testbench for axi_remove_bytes
//
// Parameters:
//
//   TEST_NAME : Name of the test instance to run
//   WIDTH     : Packet data width to use
//   REM_START : REM_START parameter to pass to the DUT
//   REM_END   : REM_END parameter to pass to the DUT
//

module axi4s_remove_bytes_tb #(
  parameter TEST_NAME = "axi_remove_bytes",
  parameter WIDTH     = 32,
  parameter REM_START = 0,
  parameter REM_END   = 7
);
  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgAxiStreamBfm::*;
  import PkgTestExec::*;
  import PkgEthernet::*;

  //---------------------------------------------------------------------------
  // Local Parameters
  //---------------------------------------------------------------------------

  localparam UWIDTH = $clog2((WIDTH/8)+1);
  localparam MAX_PACKET_BYTES = 16*1024;

  typedef AxiStreamPacket #(WIDTH, UWIDTH) AxisPacket_t;
  typedef XportStreamPacket #(WIDTH) XportPacket_t;

  //---------------------------------------------------------------------------
  // Clocks
  //---------------------------------------------------------------------------

  bit clk;
  bit reset;

  sim_clock_gen #(.PERIOD(5.0), .AUTOSTART(0))
    clk_gen (.clk(clk), .rst(reset));

  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

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

  axi4s_remove_bytes #(.REM_START(REM_START),.REM_END(REM_END))
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

  task automatic test_samples(int num_samples);
    localparam PKTS_TO_SEND = 10;
    localparam LOW_LIM  = DUT.EXACT?DUT.START_WORD-1:DUT.START_WORD;
    localparam HIGH_LIM = REM_END;

    automatic string       test_str;
    automatic raw_pkt_t    raw;
    automatic XportPacket_t send[$];
    automatic XportPacket_t expected[$];
    automatic integer last_word_index;
    test_str = $sformatf({TEST_NAME,"::Remove Bytes L=%3d"},num_samples);
    test.start_test(test_str, 10us);

    for (int i= 0 ; i < PKTS_TO_SEND; i++) begin

      expected[i] = new;
      send[i] = new;

      get_ramp_raw_pkt(.num_samps(num_samples),.ramp_start((i*num_samples)%256),.ramp_inc(1),.pkt(raw),.SWIDTH(8));
      send[i].push_bytes(raw);
      // Remove the bytes from raw packet to build expected.
      if (REM_END < 0) begin
        raw = raw[0:REM_START-1];
      end else begin
        for (int i = 0 ; i < REM_END-REM_START+1 ; ++i) begin
          if (raw.size()-1 >= REM_START) begin
            raw.delete(REM_START);
          end
        end
      end
      expected[i].push_bytes(raw);
      send[i].tkeep_to_tuser(.ERROR_PROB(10));
      expected[i].tkeep_to_tuser();
      if (send[i].has_error())
        expected[i].set_error();

      // if the incoming packet stops at a word
      // where we have no where to mark an error
      // then we set an error
      last_word_index = send[i].data.size()-1;
      if (last_word_index > LOW_LIM &&
        num_samples-1 <= HIGH_LIM) begin
        if (!DUT.TRUNCATE)
          if (DUT.START_WORD != DUT.END_WORD || DUT.EXACT)
            expected[i].set_error();
      end
    end

    for (int i=  PKTS_TO_SEND-1 ; i >= 0; i--) begin
      if (expected[i].data.size() == 0) expected.delete(i);
    end

    fork
      begin : send_thread
        foreach(send[i])begin
          axis.put(send[i]);
        end
      end
      begin : expected_thread

        if (expected.size() == 0) begin : no_expected
          automatic string str;
          automatic AxisPacket_t  actual_a = new();
          // I need a better way to flush the send queue.
          #(2*WIDTH); // wait a bit for completion
          o.tready = 1;
          axis.wait_send(); // wait complete hung
          #(2*WIDTH); // wait a bit for completion
            str = $sformatf("UNEXPECTED_PACKET L=%3d\nREM_START=%3d:%1d REM_END=%3d:%1d SIZE =%3d WIDTH=%3d\nSEND\n%s",
                            num_samples,REM_START,DUT.START_WORD,REM_END,DUT.END_WORD,send[0].data.size(),WIDTH,
                            send[0].sprint());
          if (axis.try_get(actual_a)) begin
            $display("ACTUAL");
            actual_a.print();
            `ASSERT_ERROR(actual_a.data.size() == 0,str);
          end
          $display("Wait completed");

        end else begin : get_expected
          automatic string str;
          automatic AxisPacket_t  actual_a = new();
          automatic XportPacket_t actual   = new();
          foreach(expected[i]) begin
            axis.get(actual_a);
            actual.import_axis(actual_a);
            actual.tuser_to_tkeep();
            str = $sformatf({"L= %3d REM =%2d:%1d WORD=%1d:%1d ERROR_LIM=%1d<%1d %1d<=%1d WIDTH=%3d","\nSEND\n%s"},
                            num_samples,
                            REM_START,REM_END,
                            DUT.START_WORD,DUT.END_WORD,
                            LOW_LIM,last_word_index,num_samples-1,HIGH_LIM,WIDTH,
                            send[i].sprint());
            `ASSERT_ERROR(!actual.compare_w_error(expected[i],.COMPARE_ERROR_PACKETS(0)),str);
          end
        end
      end
    join
    test.end_test();

  endtask : test_samples


  //----------------------------------------------------
  // Main test loop
  //----------------------------------------------------
  initial begin : tb_main
    automatic integer min_length;

    test.start_tb(TEST_NAME);
    clk_gen.start();

    if (REM_END == -1)
      min_length = REM_START-4;
    else
      min_length = REM_END-4;

    if ( min_length < 1) min_length = 1;

    axis.run();

    test_reset();
    // power of 2 from zero
    for (int i=0 ; i < 6 ; ++i) begin
      test_samples(2**i);
    end

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
