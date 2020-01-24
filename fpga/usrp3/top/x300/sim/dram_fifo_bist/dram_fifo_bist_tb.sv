//
// Copyright 2015 Ettus Research LLC
//


`timescale 1ns/1ps
`define SIM_RUNTIME_US  3000
`define NS_PER_TICK     1
`define NUM_TEST_CASES  25

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"
`include "sim_cvita_lib.svh"
`include "sim_axi4_lib.svh"
`include "sim_set_rb_lib.svh"

//`define USE_SRAM_FIFO     //Use an AXI-Stream SRAM FIFO (for testing)
`define USE_SRAM_MIG 1      //Use the DMA engine from the DRAM FIFO but SRAM as the base memory
`define USE_BD_INTERCON 1     //Use the Block Design Axi Interconnect


module dram_fifo_bist_tb();
  `TEST_BENCH_INIT("dram_fifo_bist_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(sys_clk, 10, 50)            //100MHz sys_clk to generate DDR3 clocking
  `DEFINE_CLK(bus_clk, 1000/166.6667, 50) //166MHz bus_clk
  `DEFINE_RESET(bus_rst, 0, 100)          //100ns for GSR to deassert
  `DEFINE_RESET(sys_rst, 0, 100)          //100ns for GSR to deassert

  // Initialize DUT
  wire            calib_complete;
  wire            running, done;
  wire [1:0]      error;
  wire [31:0]     rb_data;
  reg [63:0]      forced_bit_err;

  settings_bus_master #(.SR_AWIDTH(8),.SR_DWIDTH(32)) tst_set (.clk(bus_clk));
  cvita_master  cvita_fifo_in   (.clk(bus_clk));
  cvita_slave   cvita_fifo_out  (.clk(bus_clk));
  
  // AXI DRAM FIFO Topology (Inline production BIST for DRAM FIFO):
  //
  // User Data ====> |---------|       |---------------|       |-----------| ====> User Data Out
  //                 | AXI MUX | ====> | AXI DRAM FIFO | ====> | AXI DEMUX |
  // BIST Data ====> |---------|       |---------------|       |-----------| ====> BIST Data Out
  //                                          ||
  //                                   |--------------|
  //                                   | MIG (D/S)RAM |
  //                                   |--------------|

  localparam SR_FIFO_BASE = 0;
  localparam SR_BIST_BASE = SR_FIFO_BASE + 4;

  axis_dram_fifo_single #(
  .USE_SRAM_MEMORY(`USE_SRAM_MIG),
  .USE_BD_INTERCON(`USE_BD_INTERCON), 
  .SR_BASE(SR_FIFO_BASE)
  ) dut_single (
    .bus_clk(bus_clk),
    .bus_rst(bus_rst),
    .sys_clk(sys_clk),
    .sys_rst(sys_rst),
    
    .i_tdata(cvita_fifo_in.axis.tdata),
    .i_tlast(cvita_fifo_in.axis.tlast),
    .i_tvalid(cvita_fifo_in.axis.tvalid),
    .i_tready(cvita_fifo_in.axis.tready),

    .o_tdata(cvita_fifo_out.axis.tdata),
    .o_tlast(cvita_fifo_out.axis.tlast),
    .o_tvalid(cvita_fifo_out.axis.tvalid),
    .o_tready(cvita_fifo_out.axis.tready),
    
    .set_stb(tst_set.settings_bus.set_stb),
    .set_addr(tst_set.settings_bus.set_addr),
    .set_data(tst_set.settings_bus.set_data),
    .rb_data(rb_data),

    .forced_bit_err(forced_bit_err),
    .init_calib_complete(calib_complete)
  );
  
  assign {error, done, running} = rb_data[3:0];

  //Testbench variables
  cvita_hdr_t   header;
  cvita_pkt_t   pkt_out;
  integer       i;
  integer single_run_time;
  integer xfer_cnt, cyc_cnt;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------
  initial begin : tb_main
    string s;

    `TEST_CASE_START("Wait for reset");
    while (bus_rst) @(posedge bus_clk);
    while (sys_rst) @(posedge sys_clk);
    `TEST_CASE_DONE(~bus_rst & ~sys_rst);
    
    forced_bit_err <= 64'h0;
    repeat (200) @(posedge sys_clk);

    `TEST_CASE_START("Wait for initial calibration to complete");
    while (calib_complete !== 1'b1) @(posedge bus_clk);
    `TEST_CASE_DONE(calib_complete);

    `TEST_CASE_START("Clear FIFO");
    tst_set.write(SR_FIFO_BASE + 1, {16'h0, 12'd280, 2'b00, 1'b0, 1'b1});
    repeat (200) @(posedge bus_clk);
    tst_set.write(SR_FIFO_BASE + 1, {16'h0, 12'd280, 2'b00, 1'b0, 1'b0});
    repeat (200) @(posedge bus_clk);
    `TEST_CASE_DONE(1);

    //Select BIST status as the readback output
    tst_set.write(SR_FIFO_BASE + 0, 3'd1);

    header = '{
      pkt_type:DATA, has_time:0, eob:0, seqnum:12'h666,
      length:0, src_sid:$random, dst_sid:$random, timestamp:64'h0};

    `TEST_CASE_START("Fill up empty FIFO then drain (long packet)");
    cvita_fifo_in.push_ramp_pkt(100, 64'd0, 64'h100, header);
    cvita_fifo_out.pull_pkt(pkt_out);
    $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",100,pkt_out.payload.size());
    `ASSERT_ERROR(pkt_out.payload.size()==100, s);
    $sformat(s, "Bad packet: Wrong SID. Expected: %08x, Actual: %08x",
      {header.src_sid,header.dst_sid},{pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid});
    `ASSERT_ERROR({header.src_sid,header.dst_sid}=={pkt_out.hdr.src_sid,pkt_out.hdr.dst_sid}, s);
    i = 0;
    repeat (100) begin
      $sformat(s, "Bad packet: Wrong payload. Index: %d, Expected: %08x, Actual: %08x",
        i,(i * 64'h100),pkt_out.payload[i]);
      `ASSERT_ERROR(pkt_out.payload[i]==(i * 64'h100), s);
    end
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Setup BIST: 10 x 40byte packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h01234567);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b0, 13'd40, 18'd10});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd3, 2'd0, 1'b0, 1'b1});
    while (~done) @(posedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Run BIST ... again (should fail)");
    forced_bit_err <= 64'h8000000000000000;
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b1});
    while (~done) @(posedge bus_clk);
    `ASSERT_ERROR(error==2'b01, "BIST passed when it should have failed!");
    forced_bit_err <= 64'h0;
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Run BIST ... and again (should pass)");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 0, {2'd2, 2'd0, 1'b0, 1'b1});
    while (~done) @(posedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Setup BIST: 8000 x 40byte ramping packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h01234567);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b1, 13'd40, 18'd8000});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd3, 2'd0, 1'b0, 1'b1});
    while (~done) @(posedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Setup BIST: 256 x 1000byte packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h0ABCDEF0);
    tst_set.write(SR_BIST_BASE + 2, {8'd4, 16'd256});
    tst_set.write(SR_BIST_BASE + 1, {1'b0, 13'd1000, 18'd256});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd1, 2'd0, 1'b0, 1'b1});
    while (~done) @(negedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("User Data: Concurrent read and write");
    cvita_fifo_out.axis.tready = 1;
    fork
      begin
        cvita_fifo_in.push_ramp_pkt(20, 64'd0, 64'h100, header);
      end
      begin
        cvita_fifo_out.pull_pkt(pkt_out);
      end
    join
    $sformat(s, "Bad packet: Length mismatch. Expected: %0d, Actual: %0d",20,pkt_out.payload.size());
    `ASSERT_ERROR(pkt_out.payload.size()==20, s);
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Setup BIST: 256 x 600byte ramping packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h01234567);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b1, 13'd600, 18'd256});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b1});
    while (~done) @(posedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Setup BIST: 30 x 8000byte packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h0ABCDEF0);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b0, 13'd8000, 18'd30});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd1, 2'd0, 1'b0, 1'b1});
    while (~done) @(negedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Setup BIST: 100 x 8000byte ramping packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'h0ABCDEF0);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b1, 13'd8000, 18'd100});
    `TEST_CASE_DONE(~done & ~running);

    `TEST_CASE_START("Run BIST");
    tst_set.write(SR_BIST_BASE + 0, {2'd1, 2'd0, 1'b0, 1'b1});
    while (~done) @(negedge bus_clk);
    `ASSERT_ERROR(error==2'b00, "BIST failed!");
    @(posedge bus_clk);
    `TEST_CASE_DONE(done & ~running);
    
    `TEST_CASE_START("Validate Throughput");
    tst_set.write(SR_FIFO_BASE + 0, 3'd2);
    xfer_cnt = rb_data;
    tst_set.write(SR_FIFO_BASE + 0, 3'd3);
    cyc_cnt = rb_data;
    `ASSERT_ERROR(xfer_cnt>0, "Transfer count was not >0");
    `ASSERT_ERROR(cyc_cnt>0, "Cycle count was not >0");
    $display("Measured Throughput = %0d%% of bus_clk throughput", ((xfer_cnt*100)/cyc_cnt));
    `ASSERT_ERROR(((xfer_cnt*100)/cyc_cnt)>80, "Throughput was less than 80%%");
    tst_set.write(SR_FIFO_BASE + 0, 3'd1);  //Restore
    `TEST_CASE_DONE(done & ~running);

    `TEST_CASE_START("Setup BIST: 10 x 256byte packets");
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    tst_set.write(SR_BIST_BASE + 3, 32'hFFFFFFFF);
    tst_set.write(SR_BIST_BASE + 2, {8'd0, 16'd0});
    tst_set.write(SR_BIST_BASE + 1, {1'b0, 13'd256, 18'd30});
    `TEST_CASE_DONE(~done & ~running);

    fork
      begin
        integer curr_time = $time;
        `TEST_CASE_START("Run BIST Continuous (Early interrupt)");
        tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
        tst_set.write(SR_BIST_BASE + 0, {2'd2, 2'd0, 1'b1, 1'b1});
        while (~done) @(negedge bus_clk);
        `ASSERT_ERROR(error==2'b00, "BIST failed!");
        @(posedge bus_clk);
        single_run_time = $time - curr_time;
        `TEST_CASE_DONE(done & ~running);
      end
      begin
        //Wait then clear
        #2000;
        tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
      end
    join

    fork
      begin
        `TEST_CASE_START("Run BIST Continuous (Force error)");
        tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
        tst_set.write(SR_BIST_BASE + 0, {2'd2, 2'd0, 1'b1, 1'b1});
        while (~done) @(negedge bus_clk);
        `ASSERT_ERROR(error==2'b01, "BIST passed when it should have failed!");
        @(posedge bus_clk);
        `TEST_CASE_DONE(done & ~running);
      end
      begin
        //Wait then force error
        #10000;
        forced_bit_err <= 64'h1;
      end
    join
    //Recover from failure
    forced_bit_err <= 64'h0;
    tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
    repeat (2000) @(posedge bus_clk);

    fork
      begin
        integer curr_time = $time;
        `TEST_CASE_START("Run BIST Continuous");
        tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
        tst_set.write(SR_BIST_BASE + 0, {2'd2, 2'd0, 1'b1, 1'b1});
        while (~done) @(negedge bus_clk);
        `ASSERT_ERROR(error==2'b00, "BIST failed!");
        @(posedge bus_clk);
        `ASSERT_ERROR((($time - curr_time) > 2 * single_run_time), "Continuous test most likely stopped early!");
        `TEST_CASE_DONE(done & ~running);
      end
      begin
        //Wait then clear
        #100000;
        tst_set.write(SR_BIST_BASE + 0, {2'd0, 2'd0, 1'b0, 1'b0});
      end
    join

    `TEST_CASE_START("Validate Throughput");
    tst_set.write(SR_FIFO_BASE + 0, 3'd2);
    xfer_cnt = rb_data;
    tst_set.write(SR_FIFO_BASE + 0, 3'd3);
    cyc_cnt = rb_data;
    `ASSERT_ERROR(xfer_cnt>0, "Transfer count was not >0");
    `ASSERT_ERROR(cyc_cnt>0, "Cycle count was not >0");
    $display("Measured Throughput = %0d%% of bus_clk throughput", ((xfer_cnt*100)/cyc_cnt));
    `ASSERT_ERROR(((xfer_cnt*100)/cyc_cnt)>80, "Throughput was less than 80%%");
    tst_set.write(SR_FIFO_BASE + 0, 3'd1);  //Restore
    `TEST_CASE_DONE(done & ~running);
    `TEST_BENCH_DONE;

  end
endmodule
