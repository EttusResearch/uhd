//
// Copyright 2016 Ettus Research
//


`timescale 1ns/1ps
`define SIM_TIMEOUT_US 120
`define NS_PER_TICK 1
`define NUM_TEST_CASES 7

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_cvita_lib.svh"
`include "sim_axi4_lib.svh"
`include "sim_set_rb_lib.svh"

module dram_fifo_tb();
  `TEST_BENCH_INIT("dram_fifo_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_DIFF_CLK(sys_clk_p, sys_clk_n, 10, 50)  //100MHz differential sys_clk to generate DDR3 clocking
  `DEFINE_CLK(bus_clk, 1000/200, 50)              //200MHz bus_clk
  `DEFINE_CLK(dma_engine_clk, 1000.0/305.0, 50)   //305MHz dma_engine_clk
  `DEFINE_RESET(bus_rst, 0, 100)                  //100ns for GSR to deassert
  `DEFINE_RESET_N(sys_rst_n, 0, 100)              //100ns for GSR to deassert

  settings_bus_master #(.SR_AWIDTH(8),.SR_DWIDTH(32)) tst_set (.clk(bus_clk));
  cvita_master chdr_i (.clk(bus_clk));
  cvita_slave chdr_o (.clk(bus_clk));

  // Initialize DUT
  wire calib_complete;

  axis_dram_fifo_single dut_single (
    .bus_clk(bus_clk),
    .bus_rst(bus_rst),
    .sys_clk_p(sys_clk_p),//use differential clock on N310
    .sys_clk_n(sys_clk_n),
    .sys_rst_n(sys_rst_n),
    .dma_engine_clk(dma_engine_clk),
    
    .i_tdata(chdr_i.axis.tdata),
    .i_tlast(chdr_i.axis.tlast),
    .i_tvalid(chdr_i.axis.tvalid),
    .i_tready(chdr_i.axis.tready),
  
    .o_tdata(chdr_o.axis.tdata),
    .o_tlast(chdr_o.axis.tlast),
    .o_tvalid(chdr_o.axis.tvalid),
    .o_tready(chdr_o.axis.tready),
    
    .set_stb(tst_set.settings_bus.set_stb),
    .set_addr(tst_set.settings_bus.set_addr),
    .set_data(tst_set.settings_bus.set_data),
    .rb_data(),

    .forced_bit_err(64'h0),
    .init_calib_complete(calib_complete)
  );
  
  //Testbench variables
  cvita_hdr_t   header;
  cvita_pkt_t   pkt_out;
  integer       i;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------
  initial begin : tb_main
    string s;

    `TEST_CASE_START("Wait for reset");
      while (bus_rst) @(posedge bus_clk);
      while (~sys_rst_n) @(posedge sys_clk_p);
    `TEST_CASE_DONE((~bus_rst & sys_rst_n));

    `TEST_CASE_START("Wait for initial calibration to complete");
      while (calib_complete !== 1'b1) @(posedge bus_clk);
    `TEST_CASE_DONE(calib_complete);

    `TEST_CASE_START("Clear FIFO");
      tst_set.write(1, {16'h0, 12'd280, 2'b00, 1'b0, 1'b1});
      repeat (200) @(posedge bus_clk);
      tst_set.write(1, {16'h0, 12'd280, 2'b00, 1'b0, 1'b0});
      repeat (200) @(posedge bus_clk);
    `TEST_CASE_DONE(1);

    header = '{
      pkt_type:DATA, has_time:0, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Fill up empty FIFO then drain (short packet)");
      chdr_i.push_ramp_pkt(16, 64'd0, 64'h100, header);
      chdr_o.pull_pkt(pkt_out);
      $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",16,pkt_out.payload.size());
      `ASSERT_ERROR(pkt_out.payload.size()==16, s);
      $sformat(s, "Bad packet: Wrong SID. Expected: %08x, Actual: %08x",
        {header.src_sid,header.dst_sid},{pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid});
      `ASSERT_ERROR({header.src_sid,header.dst_sid}=={pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid}, s);
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Fill up empty FIFO then drain (long packet)");
      chdr_i.push_ramp_pkt(1024, 64'd0, 64'h100, header);
      chdr_o.pull_pkt(pkt_out);
      $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",1024,pkt_out.payload.size());
      `ASSERT_ERROR(pkt_out.payload.size()==1024, s);
      $sformat(s, "Bad packet: Wrong SID. Expected: %08x, Actual: %08x",
        {header.src_sid,header.dst_sid},{pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid});
      `ASSERT_ERROR({header.src_sid,header.dst_sid}=={pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid}, s);
    `TEST_CASE_DONE(1);

    header = '{
      pkt_type:DATA, has_time:0, eob:0, seqnum:12'h666, 
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Concurrent read and write (single packet)");
      fork
        begin
          chdr_i.push_ramp_pkt(20, 64'd0, 64'h100, header);
        end
        begin
          chdr_o.pull_pkt(pkt_out);
        end
      join
      $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",20,pkt_out.payload.size());
      `ASSERT_ERROR(pkt_out.payload.size()==20, s);
      i = 0;
      repeat (20) begin
        $sformat(s, "Bad packet: Wrong payload. Index: %d, Expected: %08x, Actual: %08x",
          i,(i * 64'h100),pkt_out.payload[i]);
        `ASSERT_ERROR(pkt_out.payload[i]==(i * 64'h100), s);
      end
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Concurrent read and write (multiple packets)");
      fork
        begin
          repeat (10) begin
            chdr_i.push_ramp_pkt(20, 64'd0, 64'h100, header);
            repeat (30) @(posedge bus_clk);
          end
        end
        begin
          repeat (10) begin
            chdr_o.pull_pkt(pkt_out);
            $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",20,pkt_out.payload.size());
            `ASSERT_ERROR(pkt_out.payload.size()==20, s);
            i = 0;
            repeat (20) begin
              $sformat(s, "Bad packet: Wrong payload. Index: %d, Expected: %08x, Actual: %08x",
                i,(i * 64'h100),pkt_out.payload[i]);
              `ASSERT_ERROR(pkt_out.payload[i]==(i * 64'h100), s);
            end
          end
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_BENCH_DONE;

  end

endmodule
