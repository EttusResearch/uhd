//----------------------------------------------------------------------------
// Title : Demo Testbench
// Project : 10 Gigabit Ethernet PCS/PMA
//----------------------------------------------------------------------------
// File : demo_tb.v
//----------------------------------------------------------------------------
// (c) Copyright 2009 - 2012 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and 
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//----------------------------------------------------------------------------
// Description :
// This test-fixture performs the following operations on the 10GBASE-R core:
//----------------------------------------------------------------------------

`timescale 1ps / 1ps

`define FRAME_TYP [32*32+32*4+16:1]
// refclk Period in ps - in reality this should be 6206 ps nominal, but 66*95
`define BITPERIOD 98 // Closest even number to 96.9696...
`define PERIOD156 66*98 // this is the clock for 156.25MHz based on bit period of 98.
// qplllockdetclk runs at 156.25MHz/2
`define DPERIOD 66*98*2
// MDC clock runs at 2.5MHz
`define MDCPERIOD 400000
`define LOCK_INIT 0
`define RESET_CNT 1
`define TEST_SH   2

module frame_typten_gig_eth_pcs_pma ;
    // This module abstracts the frame data for simpler manipulation
    reg [31:0] data [0:31];
    reg [ 3:0] ctrl [0:31];
    reg [15:0] length;
    reg `FRAME_TYP bits;

    function `FRAME_TYP tobits;
    input dummy;
    begin
        bits = {data[ 0], data[ 1], data[ 2], data[ 3], data[ 4],
                data[ 5], data[ 6], data[ 7], data[ 8], data[ 9],
                data[10], data[11], data[12], data[13], data[14],
                data[15], data[16], data[17], data[18], data[19],
                data[20], data[21], data[22], data[23], data[24],
                data[25], data[26], data[27], data[28], data[29],
                data[30], data[31], ctrl[ 0], ctrl[ 1], ctrl[ 2],
                ctrl[ 3], ctrl[ 4], ctrl[ 5], ctrl[ 6], ctrl[ 7],
                ctrl[ 8], ctrl[ 9], ctrl[10], ctrl[11], ctrl[12],
                ctrl[13], ctrl[14], ctrl[15], ctrl[16], ctrl[17],
                ctrl[18], ctrl[19], ctrl[20], ctrl[21], ctrl[22],
                ctrl[23], ctrl[24], ctrl[25], ctrl[26], ctrl[27],
                ctrl[28], ctrl[29], ctrl[30], ctrl[31], length};
                tobits = bits;
    end
    endfunction // tobits

    task frombits;
    input `FRAME_TYP frame;
    begin
        bits = frame;
        {data[ 0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5],
         data[ 6], data[ 7], data[ 8], data[ 9], data[10], data[11],
         data[12], data[13], data[14], data[15], data[16], data[17],
         data[18], data[19], data[20], data[21], data[22], data[23],
         data[24], data[25], data[26], data[27], data[28], data[29],
         data[30], data[31], ctrl[ 0], ctrl[ 1], ctrl[ 2], ctrl[ 3],
         ctrl[ 4], ctrl[ 5], ctrl[ 6], ctrl[ 7], ctrl[ 8], ctrl[ 9],
         ctrl[10], ctrl[11], ctrl[12], ctrl[13], ctrl[14], ctrl[15],
         ctrl[16], ctrl[17], ctrl[18], ctrl[19], ctrl[20], ctrl[21],
         ctrl[22], ctrl[23], ctrl[24], ctrl[25], ctrl[26], ctrl[27],
         ctrl[28], ctrl[29], ctrl[30], ctrl[31], length} = bits;
    end
    endtask // frombits
endmodule // frame_typ

//-----------------------------------------------------------------------------
// Module declaration of Design Under Test.
//-----------------------------------------------------------------------------

module demo_tb;

    // Frame data....
    frame_typten_gig_eth_pcs_pma frame0();
    frame_typten_gig_eth_pcs_pma frame1();
    frame_typten_gig_eth_pcs_pma frame2();
    frame_typten_gig_eth_pcs_pma frame3();

    frame_typten_gig_eth_pcs_pma tx_stimulus_working_frame();
    frame_typten_gig_eth_pcs_pma tx_monitor_working_frame();
    frame_typten_gig_eth_pcs_pma rx_stimulus_working_frame();
    frame_typten_gig_eth_pcs_pma rx_monitor_working_frame();

    // Store the frame data etc....
    initial
    begin
        // Frame 0...
        frame0.data[0]  = 32'h040302FB;
        frame0.data[1]  = 32'h02020605;
        frame0.data[2]  = 32'h06050403;
        frame0.data[3]  = 32'h55AA2E00;
        frame0.data[4]  = 32'hAA55AA55;
        frame0.data[5]  = 32'h55AA55AA;
        frame0.data[6]  = 32'hAA55AA55;
        frame0.data[7]  = 32'h55AA55AA;
        frame0.data[8]  = 32'hAA55AA55;
        frame0.data[9]  = 32'h55AA55AA;
        frame0.data[10] = 32'hAA55AA55;
        frame0.data[11] = 32'h55AA55AA;
        frame0.data[12] = 32'hAA55AA55;
        frame0.data[13] = 32'h55AA55AA;
        frame0.data[14] = 32'hFD55AA55;
        frame0.data[15] = 32'h07070707;
        frame0.data[16] = 32'h07070707;
        frame0.data[17] = 32'h07070707;
        frame0.data[18] = 32'h07070707;
        frame0.data[19] = 32'h07070707;
        frame0.data[20] = 32'h07070707;
        frame0.data[21] = 32'h07070707;
        frame0.data[22] = 32'h07070707;
        frame0.data[23] = 32'h07070707;
        frame0.data[24] = 32'h07070707;
        frame0.data[25] = 32'h07070707;
        frame0.data[26] = 32'h07070707;
        frame0.data[27] = 32'h07070707;
        frame0.data[28] = 32'h07070707;
        frame0.data[29] = 32'h07070707;
        frame0.data[30] = 32'h07070707;
        frame0.data[31] = 32'h07070707;
        frame0.ctrl[0]  = 4'b0001;
        frame0.ctrl[1]  = 4'b0000;
        frame0.ctrl[2]  = 4'b0000;
        frame0.ctrl[3]  = 4'b0000;
        frame0.ctrl[4]  = 4'b0000;
        frame0.ctrl[5]  = 4'b0000;
        frame0.ctrl[6]  = 4'b0000;
        frame0.ctrl[7]  = 4'b0000;
        frame0.ctrl[8]  = 4'b0000;
        frame0.ctrl[9]  = 4'b0000;
        frame0.ctrl[10] = 4'b0000;
        frame0.ctrl[11] = 4'b0000;
        frame0.ctrl[12] = 4'b0000;
        frame0.ctrl[13] = 4'b0000;
        frame0.ctrl[14] = 4'b1000;
        frame0.ctrl[15] = 4'b1111;
        frame0.ctrl[16] = 4'b1111;
        frame0.ctrl[17] = 4'b1111;
        frame0.ctrl[18] = 4'b1111;
        frame0.ctrl[19] = 4'b1111;
        frame0.ctrl[20] = 4'b1111;
        frame0.ctrl[21] = 4'b1111;
        frame0.ctrl[22] = 4'b1111;
        frame0.ctrl[23] = 4'b1111;
        frame0.ctrl[24] = 4'b1111;
        frame0.ctrl[25] = 4'b1111;
        frame0.ctrl[26] = 4'b1111;
        frame0.ctrl[27] = 4'b1111;
        frame0.ctrl[28] = 4'b1111;
        frame0.ctrl[29] = 4'b1111;
        frame0.ctrl[30] = 4'b1111;
        frame0.ctrl[31] = 4'b1111;
        frame0.length   = 15;

        //Frame 1
        frame1.data[0]  = 32'h030405FB;
        frame1.data[1]  = 32'h05060102;
        frame1.data[2]  = 32'h02020304;
        frame1.data[3]  = 32'hEE110080;
        frame1.data[4]  = 32'h11EE11EE;
        frame1.data[5]  = 32'hEE11EE11;
        frame1.data[6]  = 32'h11EE11EE;
        frame1.data[7]  = 32'hEE11EE11;
        frame1.data[8]  = 32'h11EE11EE;
        frame1.data[9]  = 32'hEE11EE11;
        frame1.data[10] = 32'h11EE11EE;
        frame1.data[11] = 32'hEE11EE11;
        frame1.data[12] = 32'h11EE11EE;
        frame1.data[13] = 32'hEE11EE11;
        frame1.data[14] = 32'h11EE11EE;
        frame1.data[15] = 32'hEE11EE11;
        frame1.data[16] = 32'h11EE11EE;
        frame1.data[17] = 32'hEE11EE11;
        frame1.data[18] = 32'h11EE11EE;
        frame1.data[19] = 32'hEE11EE11;
        frame1.data[20] = 32'h11EE11EE;
        frame1.data[21] = 32'h07FDEE11;
        frame1.data[22] = 32'h07070707;
        frame1.data[23] = 32'h07070707;
        frame1.data[24] = 32'h07070707;
        frame1.data[25] = 32'h07070707;
        frame1.data[26] = 32'h07070707;
        frame1.data[27] = 32'h07070707;
        frame1.data[28] = 32'h07070707;
        frame1.data[29] = 32'h07070707;
        frame1.data[30] = 32'h07070707;
        frame1.data[31] = 32'h07070707;
        frame1.ctrl[0]  = 4'b0001;
        frame1.ctrl[1]  = 4'b0000;
        frame1.ctrl[2]  = 4'b0000;
        frame1.ctrl[3]  = 4'b0000;
        frame1.ctrl[4]  = 4'b0000;
        frame1.ctrl[5]  = 4'b0000;
        frame1.ctrl[6]  = 4'b0000;
        frame1.ctrl[7]  = 4'b0000;
        frame1.ctrl[8]  = 4'b0000;
        frame1.ctrl[9]  = 4'b0000;
        frame1.ctrl[10] = 4'b0000;
        frame1.ctrl[11] = 4'b0000;
        frame1.ctrl[12] = 4'b0000;
        frame1.ctrl[13] = 4'b0000;
        frame1.ctrl[14] = 4'b0000;
        frame1.ctrl[15] = 4'b0000;
        frame1.ctrl[16] = 4'b0000;
        frame1.ctrl[17] = 4'b0000;
        frame1.ctrl[18] = 4'b0000;
        frame1.ctrl[19] = 4'b0000;
        frame1.ctrl[20] = 4'b0000;
        frame1.ctrl[21] = 4'b1100;
        frame1.ctrl[22] = 4'b1111;
        frame1.ctrl[23] = 4'b1111;
        frame1.ctrl[24] = 4'b1111;
        frame1.ctrl[25] = 4'b1111;
        frame1.ctrl[26] = 4'b1111;
        frame1.ctrl[27] = 4'b1111;
        frame1.ctrl[28] = 4'b1111;
        frame1.ctrl[29] = 4'b1111;
        frame1.ctrl[30] = 4'b1111;
        frame1.ctrl[31] = 4'b1111;
        frame1.length   = 22;

        //Frame 2
        frame2.data[0]  = 32'h040302FB;
        frame2.data[1]  = 32'h02020605;
        frame2.data[2]  = 32'h06050403;
        frame2.data[3]  = 32'h55AA2E80;
        frame2.data[4]  = 32'hAA55AA55;
        frame2.data[5]  = 32'h55AA55AA;
        frame2.data[6]  = 32'hAA55AA55;
        frame2.data[7]  = 32'h55AA55AA;
        frame2.data[8]  = 32'hAA55AA55;
        frame2.data[9]  = 32'h55AA55AA;
        frame2.data[10] = 32'hAA55AA55;
        frame2.data[11] = 32'h55AA55AA;
        frame2.data[12] = 32'hAA55AA55;
        frame2.data[13] = 32'h55AA55AA;
        frame2.data[14] = 32'hAA55AA55;
        frame2.data[15] = 32'h55AA55AA;
        frame2.data[16] = 32'hAA55AA55;
        frame2.data[17] = 32'h55AA55AA;
        frame2.data[18] = 32'hAA55AA55;
        frame2.data[19] = 32'h55AA55AA;
        frame2.data[20] = 32'h0707FDAA;
        frame2.data[21] = 32'h07070707;
        frame2.data[22] = 32'h07070707;
        frame2.data[23] = 32'h07070707;
        frame2.data[24] = 32'h07070707;
        frame2.data[25] = 32'h07070707;
        frame2.data[26] = 32'h07070707;
        frame2.data[27] = 32'h07070707;
        frame2.data[28] = 32'h07070707;
        frame2.data[29] = 32'h07070707;
        frame2.data[30] = 32'h07070707;
        frame2.data[31] = 32'h07070707;
        frame2.ctrl[0]  = 4'b0001;
        frame2.ctrl[1]  = 4'b0000;
        frame2.ctrl[2]  = 4'b0000;
        frame2.ctrl[3]  = 4'b0000;
        frame2.ctrl[4]  = 4'b0000;
        frame2.ctrl[5]  = 4'b0000;
        frame2.ctrl[6]  = 4'b0000;
        frame2.ctrl[7]  = 4'b0000;
        frame2.ctrl[8]  = 4'b0000;
        frame2.ctrl[9]  = 4'b0000;
        frame2.ctrl[10] = 4'b0000;
        frame2.ctrl[11] = 4'b0000;
        frame2.ctrl[12] = 4'b0000;
        frame2.ctrl[13] = 4'b0000;
        frame2.ctrl[14] = 4'b0000;
        frame2.ctrl[15] = 4'b0000;
        frame2.ctrl[16] = 4'b0000;
        frame2.ctrl[17] = 4'b0000;
        frame2.ctrl[18] = 4'b0000;
        frame2.ctrl[19] = 4'b0000;
        frame2.ctrl[20] = 4'b1110;
        frame2.ctrl[21] = 4'b1111;
        frame2.ctrl[22] = 4'b1111;
        frame2.ctrl[23] = 4'b1111;
        frame2.ctrl[24] = 4'b1111;
        frame2.ctrl[25] = 4'b1111;
        frame2.ctrl[26] = 4'b1111;
        frame2.ctrl[27] = 4'b1111;
        frame2.ctrl[28] = 4'b1111;
        frame2.ctrl[29] = 4'b1111;
        frame2.ctrl[30] = 4'b1111;
        frame2.ctrl[31] = 4'b1111;
        frame2.length   = 21;

        //Frame 3
        frame3.data[0]  = 32'h030405FB;
        frame3.data[1]  = 32'h05060102;
        frame3.data[2]  = 32'h02020304;
        frame3.data[3]  = 32'hEE110080;
        frame3.data[4]  = 32'h11EE11EE;
        frame3.data[5]  = 32'hEE11EE11;
        frame3.data[6]  = 32'h11EE11EE;
        frame3.data[7]  = 32'hEE11EE11;
        frame3.data[8]  = 32'h11EE11EE;
        frame3.data[9]  = 32'h070707FD;
        frame3.data[10] = 32'h07070707;
        frame3.data[11] = 32'h07070707;
        frame3.data[12] = 32'h07070707;
        frame3.data[13] = 32'h07070707;
        frame3.data[14] = 32'h07070707;
        frame3.data[15] = 32'h07070707;
        frame3.data[16] = 32'h07070707;
        frame3.data[17] = 32'h07070707;
        frame3.data[18] = 32'h07070707;
        frame3.data[19] = 32'h07070707;
        frame3.data[20] = 32'h07070707;
        frame3.data[21] = 32'h07070707;
        frame3.data[22] = 32'h07070707;
        frame3.data[23] = 32'h07070707;
        frame3.data[24] = 32'h07070707;
        frame3.data[25] = 32'h07070707;
        frame3.data[26] = 32'h07070707;
        frame3.data[27] = 32'h07070707;
        frame3.data[28] = 32'h07070707;
        frame3.data[29] = 32'h07070707;
        frame3.data[30] = 32'h07070707;
        frame3.data[31] = 32'h07070707;
        frame3.ctrl[0]  = 4'b0001;
        frame3.ctrl[1]  = 4'b0000;
        frame3.ctrl[2]  = 4'b0000;
        frame3.ctrl[3]  = 4'b0000;
        frame3.ctrl[4]  = 4'b0000;
        frame3.ctrl[5]  = 4'b0000;
        frame3.ctrl[6]  = 4'b0000;
        frame3.ctrl[7]  = 4'b0000;
        frame3.ctrl[8]  = 4'b0000;
        frame3.ctrl[9]  = 4'b1111;
        frame3.ctrl[10] = 4'b1111;
        frame3.ctrl[11] = 4'b1111;
        frame3.ctrl[12] = 4'b1111;
        frame3.ctrl[13] = 4'b1111;
        frame3.ctrl[14] = 4'b1111;
        frame3.ctrl[15] = 4'b1111;
        frame3.ctrl[16] = 4'b1111;
        frame3.ctrl[17] = 4'b1111;
        frame3.ctrl[18] = 4'b1111;
        frame3.ctrl[19] = 4'b1111;
        frame3.ctrl[20] = 4'b1111;
        frame3.ctrl[21] = 4'b1111;
        frame3.ctrl[22] = 4'b1111;
        frame3.ctrl[23] = 4'b1111;
        frame3.ctrl[24] = 4'b1111;
        frame3.ctrl[25] = 4'b1111;
        frame3.ctrl[26] = 4'b1111;
        frame3.ctrl[27] = 4'b1111;
        frame3.ctrl[28] = 4'b1111;
        frame3.ctrl[29] = 4'b1111;
        frame3.ctrl[30] = 4'b1111;
        frame3.ctrl[31] = 4'b1111;
        frame3.length   = 10;
    end // initialise the frame contents

    reg            reset;
    wire           core_clk156_out;

    reg  [63 : 0]  xgmii_txd;
    reg  [7 : 0]   xgmii_txc;
    wire           xgmii_rx_clk;
    wire [63 : 0]  xgmii_rxd;
    wire [7 : 0]   xgmii_rxc;

    reg            refclk_p;
    reg            refclk_n;
    reg            qplllockdetclk;
    reg            bitclk;

    wire           txp;
    wire           txn;
    reg            rxp;
    wire           rxn;

    reg             mdc;
    wire            mdio_in;
    wire            mdio_out;
    wire            mdio_tri;
    wire   [4 : 0]  prtad;

    wire [7 : 0]   core_status;
    wire           resetdone;
    wire           signal_detect;
    wire           tx_fault;
    wire           tx_disable;

    wire           simulation_finished;
    reg            rx_simulation_finished = 1'b0;
    reg            tx_simulation_finished = 1'b0;
    reg            sampleclk;

    reg          block_lock;
    reg          test_sh = 0;
    reg          slip = 0;
    integer      BLSTATE;
    integer      next_blstate;
    reg [65:0]   RxD;
    reg [65:0]   RxD_aligned;
    integer      nbits = 0;
    integer      sh_cnt;
    integer      sh_invalid_cnt;
    integer      i;

    // To aid the TX data checking code
    reg in_a_frame = 0;

    assign prtad    = 5'b00000;


//-----------------------------------------------------------------------------
// Connect the Design Under Test to the signals in the test-fixture.
//-----------------------------------------------------------------------------
  ten_gig_eth_pcs_pma_example_design DUT(
    .reset(reset),
    .core_clk156_out(core_clk156_out),
    .xgmii_txd(xgmii_txd),
    .xgmii_txc(xgmii_txc),
    .xgmii_rx_clk(xgmii_rx_clk),
    .xgmii_rxd(xgmii_rxd),
    .xgmii_rxc(xgmii_rxc),
    .refclk_p(refclk_p),
    .refclk_n(refclk_n),
//-----------------------------------------------------------------------------
// Serial Interface
//-----------------------------------------------------------------------------
    .txp(txp),
    .txn(txn),
    .rxp(rxp),
    .rxn(rxn),
    .resetdone(resetdone),
    .signal_detect(signal_detect),
    .tx_fault(tx_fault),
    .tx_disable(tx_disable),
    .core_status(core_status),
//-----------------------------------------------------------------------------
// MDIO Interface
//-----------------------------------------------------------------------------
    .mdc(mdc),
    .mdio_in(mdio_in),
    .mdio_out(mdio_out),
    .mdio_tri(mdio_tri),
    .prtad(prtad)
  );

  assign signal_detect = 1'b1;
  assign tx_fault = 1'b0;
//-----------------------------------------------------------------------------
// Clock Drivers
//-----------------------------------------------------------------------------

  // Generate the refclk
  initial
  begin
    refclk_p <= 1'b0;
    refclk_n <= 1'b1;

    forever
    begin
      refclk_p <= 1'b0;
      refclk_n <= 1'b1;
      #(`PERIOD156/2);
      refclk_p <= 1'b1;
      refclk_n <= 1'b0;
      #(`PERIOD156/2);
    end
  end

  // Generate the sampleclk
  initial
  begin
    sampleclk <= 1'b0;

    forever
    begin
      sampleclk <= 1'b0;
      #(`PERIOD156/2);
      sampleclk <= 1'b1;
      #(`PERIOD156/2);
    end
  end

  // Generate the qplllockdetclk
  initial
  begin
    qplllockdetclk <= 1'b0;

    forever
    begin
      qplllockdetclk <= 1'b0;
      #(`DPERIOD/2);
      qplllockdetclk <= 1'b1;
      #(`DPERIOD/2);
    end
  end


  // Generate the bit clock
  initial
  begin
    bitclk <= 1'b0;

    #(`BITPERIOD/4);
    forever
    begin
      bitclk <= 1'b0;
      #(`BITPERIOD/2);
      bitclk <= 1'b1;
      #(`BITPERIOD/2);
    end
  end


  //Generate the mdc management i/f clock
  initial
  begin
    mdc <= 1'b0;
    forever
    begin
      mdc <= 1'b0;
      #(`MDCPERIOD/2);
      mdc <= 1'b1;
      #(`MDCPERIOD/2);
    end
  end

  //----------------------------------------------------------------
  // Global Set/Reset
  //----------------------------------------------------------------

  reg         gsr_r;
  reg         gts_r;

  assign glbl.GSR = gsr_r;
  assign glbl.GTS = gts_r;

  initial
    begin
      gts_r = 1'b0;
      gsr_r = 1'b1;
      #500000;
      gsr_r = 1'b0;
  end

  //Generate the reset.
  initial
    begin
    $display("Resetting the core...");

    reset = 1'b0;
    wait (gsr_r == 1'b1);
    reset = 1'b1;
    wait (gsr_r == 1'b0);
    reset = 1'b0;
    @ ( negedge sampleclk );

    // Wait for GT init to finish
    wait ( DUT.resetdone == 1'b0 );
    wait ( DUT.resetdone == 1'b1 );
    $display ( "INFO:// At %0t: GT initialization done. \n", $time );
    repeat ( 500 ) @ ( negedge sampleclk );
  end

  // Drive MDIO bus during the simulation...
  assign mdio_in = 1'b1;
  assign simulation_finished = rx_simulation_finished & tx_simulation_finished;

  // Main initial block to start and stop simulation
  initial
    begin
      fork : sim_in_progress
        @(posedge simulation_finished) disable sim_in_progress;
        #150000000 disable sim_in_progress;
      join
      if (simulation_finished)
        $display("** Test completed successfully");
      else
        $display("** Error: Testbench timed out");
      $stop;
  end // initial begin

  //----------------------------------------------------------------
  // Transmit Stimulus code...
  //----------------------------------------------------------------

   // Support code for transmitting frames through xgmii
   task tx_stimulus_send_column;
     input [31:0] d;
     input [ 3:0] c;
     reg [31:0] cached_column_data;
     reg [ 3:0] cached_column_ctrl;
     reg        cached_column_valid;
     begin
       if (cached_column_valid)
         begin
           @(posedge sampleclk);
           #3200;
           xgmii_txd[31: 0] <= cached_column_data;
           xgmii_txc[ 3: 0] <= cached_column_ctrl;
           xgmii_txd[63:32] <= d;
           xgmii_txc[ 7: 4] <= c;
           cached_column_valid = 0;
         end
       else
         begin
           cached_column_data  = d;
           cached_column_ctrl  = c;
           cached_column_valid = 1;
         end
     end
   endtask // tx_stimulus_send_column

   task tx_stimulus_send_idle;
     begin
        tx_stimulus_send_column(32'h07070707,4'b1111);
     end
   endtask // tx_stimulus_send_idle

   task tx_stimulus_send_frame;
      input `FRAME_TYP frame;
      integer column_index;
      begin
        tx_stimulus_working_frame.frombits(frame);
        column_index = 0;
        // send columns
        while (column_index < tx_stimulus_working_frame.length)
          begin
            tx_stimulus_send_column(tx_stimulus_working_frame.data[column_index],
                                    tx_stimulus_working_frame.ctrl[column_index]);
            column_index = column_index + 1;
          end

        $display("Transmitter: frame inserted into XGMII interface");
      end
   endtask // tx_stimulus_send_frame

   initial
     begin : p_tx_stimulus
       // wait until the core is ready after reset - this will be indicated
       // by a rising edge on the resetdone signal.
       while (DUT.resetdone !== 1'b1)
         tx_stimulus_send_idle;
       // now wait until the testbench has block_lock on the transmitted idles
       while (block_lock !== 1'b1)
         tx_stimulus_send_idle;

       tx_stimulus_send_frame(frame0.tobits(0));
       tx_stimulus_send_idle;
       tx_stimulus_send_idle;
       tx_stimulus_send_frame(frame1.tobits(0));
       tx_stimulus_send_idle;
       tx_stimulus_send_idle;
       tx_stimulus_send_frame(frame2.tobits(0));
       tx_stimulus_send_idle;
       tx_stimulus_send_idle;
       tx_stimulus_send_frame(frame3.tobits(0));
       while (1)
         tx_stimulus_send_idle;

     end // block: p_tx_stimulus

   //----------------------------------------------------------------
   // Transmit Monitor code.....
   //----------------------------------------------------------------

   // Fill RxD with 66 bits...
   always @(posedge bitclk)
   begin : p_tx_serial_capture
     if(!slip)
     begin // Just grab next 66 bits
       RxD[64:0] <= RxD[65:1];
       RxD[65] <= txp;
       if(nbits < 65)
       begin
         nbits <= nbits + 1;
         test_sh <= 0;
       end
       else
       begin
         nbits <= 0;
         test_sh <= 1;
       end
     end
     else // SLIP!!
     begin // Just grab single bit
       RxD[64:0] <= RxD[65:1];
       RxD[65] <= txp;
       test_sh <= 1;
       nbits <= 0;
     end
   end // p_tx_serial_capture


   // Implement the block lock state machine on serial TX...
   always @(BLSTATE or test_sh or RxD)
     begin : p_tx_block_lock

     case (BLSTATE)
       `LOCK_INIT : begin
         block_lock <= 1'b0;
         next_blstate <= `RESET_CNT;
         slip <= 0;
         sh_cnt <= 0;
         sh_invalid_cnt <= 0;
       end
       `RESET_CNT : begin
         slip <= 0;
         if(test_sh)
           next_blstate <= `TEST_SH;
         else
           next_blstate <= `RESET_CNT;
       end
       `TEST_SH : begin
         slip <= 0;
         next_blstate <= `TEST_SH;
         if(test_sh && (RxD[0] != RxD[1])) // Good sync header candidate
         begin
           sh_cnt <= sh_cnt + 1; // Immediate update!
           if(sh_cnt < 64)
             next_blstate <= `TEST_SH;
           else if(sh_cnt == 64 && sh_invalid_cnt > 0)
           begin
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
           else if(sh_cnt == 64 && sh_invalid_cnt == 0)
           begin
             block_lock <= 1;
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
         end
         else if(test_sh)// Bad sync header
         begin
           sh_cnt <= sh_cnt + 1;
           sh_invalid_cnt <= sh_invalid_cnt + 1;
           if(sh_cnt == 64 && sh_invalid_cnt < 16 && block_lock)
           begin
             next_blstate <= `RESET_CNT;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
           end
           else if(sh_cnt < 64 && sh_invalid_cnt < 16 && test_sh && block_lock)
             next_blstate <= `TEST_SH;
           else if(sh_invalid_cnt == 16 && !block_lock)
           begin
             block_lock <= 0;
             slip <= 1;
             sh_cnt <= 0;
             sh_invalid_cnt <= 0;
             next_blstate <= `RESET_CNT;
           end
         end
       end
     endcase
   end // p_tx_block_lock

   // Implement the block lock state machine on serial TX
   // And capture the aligned 66 bit words....
   always @(posedge bitclk)
     begin : p_tx_block_lock_next_blstate
       if(reset || !resetdone)
         BLSTATE <= `LOCK_INIT;
       else
         BLSTATE <= next_blstate;

       if(test_sh && block_lock)
         RxD_aligned <= RxD;

     end // p_tx_block_lock_next_blstate

    // Descramble the TX serial data
    reg    [57:0] DeScrambler_Register = 58'h3;
    reg    [63:0] RXD_input = 64'h0;
    reg    [1:0]  RX_Sync_header = 2'b01;
    wire   [63:0] DeScr_wire;
    reg    [65:0] DeScr_RXD = 66'h79;

    assign DeScr_wire[0] = RXD_input[0]^DeScrambler_Register[38]^DeScrambler_Register[57];
    assign DeScr_wire[1] = RXD_input[1]^DeScrambler_Register[37]^DeScrambler_Register[56];
    assign DeScr_wire[2] = RXD_input[2]^DeScrambler_Register[36]^DeScrambler_Register[55];
    assign DeScr_wire[3] = RXD_input[3]^DeScrambler_Register[35]^DeScrambler_Register[54];
    assign DeScr_wire[4] = RXD_input[4]^DeScrambler_Register[34]^DeScrambler_Register[53];
    assign DeScr_wire[5] = RXD_input[5]^DeScrambler_Register[33]^DeScrambler_Register[52];
    assign DeScr_wire[6] = RXD_input[6]^DeScrambler_Register[32]^DeScrambler_Register[51];
    assign DeScr_wire[7] = RXD_input[7]^DeScrambler_Register[31]^DeScrambler_Register[50];

    assign  DeScr_wire[8] = RXD_input[8]^DeScrambler_Register[30]^DeScrambler_Register[49];
    assign  DeScr_wire[9] = RXD_input[9]^DeScrambler_Register[29]^DeScrambler_Register[48];
    assign  DeScr_wire[10] = RXD_input[10]^DeScrambler_Register[28]^DeScrambler_Register[47];
    assign  DeScr_wire[11] = RXD_input[11]^DeScrambler_Register[27]^DeScrambler_Register[46];
    assign  DeScr_wire[12] = RXD_input[12]^DeScrambler_Register[26]^DeScrambler_Register[45];
    assign  DeScr_wire[13] = RXD_input[13]^DeScrambler_Register[25]^DeScrambler_Register[44];
    assign  DeScr_wire[14] = RXD_input[14]^DeScrambler_Register[24]^DeScrambler_Register[43];
    assign  DeScr_wire[15] = RXD_input[15]^DeScrambler_Register[23]^DeScrambler_Register[42];

    assign  DeScr_wire[16] = RXD_input[16]^DeScrambler_Register[22]^DeScrambler_Register[41];
    assign  DeScr_wire[17] = RXD_input[17]^DeScrambler_Register[21]^DeScrambler_Register[40];
    assign  DeScr_wire[18] = RXD_input[18]^DeScrambler_Register[20]^DeScrambler_Register[39];
    assign  DeScr_wire[19] = RXD_input[19]^DeScrambler_Register[19]^DeScrambler_Register[38];
    assign  DeScr_wire[20] = RXD_input[20]^DeScrambler_Register[18]^DeScrambler_Register[37];
    assign  DeScr_wire[21] = RXD_input[21]^DeScrambler_Register[17]^DeScrambler_Register[36];
    assign  DeScr_wire[22] = RXD_input[22]^DeScrambler_Register[16]^DeScrambler_Register[35];
    assign  DeScr_wire[23] = RXD_input[23]^DeScrambler_Register[15]^DeScrambler_Register[34];

    assign  DeScr_wire[24] = RXD_input[24]^DeScrambler_Register[14]^DeScrambler_Register[33];
    assign  DeScr_wire[25] = RXD_input[25]^DeScrambler_Register[13]^DeScrambler_Register[32];
    assign  DeScr_wire[26] = RXD_input[26]^DeScrambler_Register[12]^DeScrambler_Register[31];
    assign  DeScr_wire[27] = RXD_input[27]^DeScrambler_Register[11]^DeScrambler_Register[30];
    assign  DeScr_wire[28] = RXD_input[28]^DeScrambler_Register[10]^DeScrambler_Register[29];
    assign  DeScr_wire[29] = RXD_input[29]^DeScrambler_Register[9]^DeScrambler_Register[28];
    assign  DeScr_wire[30] = RXD_input[30]^DeScrambler_Register[8]^DeScrambler_Register[27];
    assign  DeScr_wire[31] = RXD_input[31]^DeScrambler_Register[7]^DeScrambler_Register[26];

    assign  DeScr_wire[32] = RXD_input[32]^DeScrambler_Register[6]^DeScrambler_Register[25];
    assign  DeScr_wire[33] = RXD_input[33]^DeScrambler_Register[5]^DeScrambler_Register[24];
    assign  DeScr_wire[34] = RXD_input[34]^DeScrambler_Register[4]^DeScrambler_Register[23];
    assign  DeScr_wire[35] = RXD_input[35]^DeScrambler_Register[3]^DeScrambler_Register[22];
    assign  DeScr_wire[36] = RXD_input[36]^DeScrambler_Register[2]^DeScrambler_Register[21];
    assign  DeScr_wire[37] = RXD_input[37]^DeScrambler_Register[1]^DeScrambler_Register[20];
    assign  DeScr_wire[38] = RXD_input[38]^DeScrambler_Register[0]^DeScrambler_Register[19];

    assign  DeScr_wire[39] = RXD_input[39]^RXD_input[0]^DeScrambler_Register[18];
    assign  DeScr_wire[40] = RXD_input[40]^RXD_input[1]^DeScrambler_Register[17];
    assign  DeScr_wire[41] = RXD_input[41]^RXD_input[2]^DeScrambler_Register[16];
    assign  DeScr_wire[42] = RXD_input[42]^RXD_input[3]^DeScrambler_Register[15];
    assign  DeScr_wire[43] = RXD_input[43]^RXD_input[4]^DeScrambler_Register[14];
    assign  DeScr_wire[44] = RXD_input[44]^RXD_input[5]^DeScrambler_Register[13];
    assign  DeScr_wire[45] = RXD_input[45]^RXD_input[6]^DeScrambler_Register[12];
    assign  DeScr_wire[46] = RXD_input[46]^RXD_input[7]^DeScrambler_Register[11];
    assign  DeScr_wire[47] = RXD_input[47]^RXD_input[8]^DeScrambler_Register[10];

    assign  DeScr_wire[48] = RXD_input[48]^RXD_input[9]^DeScrambler_Register[9];
    assign  DeScr_wire[49] = RXD_input[49]^RXD_input[10]^DeScrambler_Register[8];
    assign  DeScr_wire[50] = RXD_input[50]^RXD_input[11]^DeScrambler_Register[7];
    assign  DeScr_wire[51] = RXD_input[51]^RXD_input[12]^DeScrambler_Register[6];
    assign  DeScr_wire[52] = RXD_input[52]^RXD_input[13]^DeScrambler_Register[5];
    assign  DeScr_wire[53] = RXD_input[53]^RXD_input[14]^DeScrambler_Register[4];
    assign  DeScr_wire[54] = RXD_input[54]^RXD_input[15]^DeScrambler_Register[3];

    assign  DeScr_wire[55] = RXD_input[55]^RXD_input[16]^DeScrambler_Register[2];
    assign  DeScr_wire[56] = RXD_input[56]^RXD_input[17]^DeScrambler_Register[1];
    assign  DeScr_wire[57] = RXD_input[57]^RXD_input[18]^DeScrambler_Register[0];
    assign  DeScr_wire[58] = RXD_input[58]^RXD_input[19]^RXD_input[0];
    assign  DeScr_wire[59] = RXD_input[59]^RXD_input[20]^RXD_input[1];
    assign  DeScr_wire[60] = RXD_input[60]^RXD_input[21]^RXD_input[2];
    assign  DeScr_wire[61] = RXD_input[61]^RXD_input[22]^RXD_input[3];
    assign  DeScr_wire[62] = RXD_input[62]^RXD_input[23]^RXD_input[4];
    assign  DeScr_wire[63] = RXD_input[63]^RXD_input[24]^RXD_input[5];

    // Synchronous part of descrambler
    always @(posedge core_clk156_out) begin
        RXD_input[63:0] <= RxD_aligned[65:2];
        RX_Sync_header <= RxD_aligned[1:0];
        DeScr_RXD[65:0] <= {DeScr_wire[63:0],RX_Sync_header[1:0]};
        for (i = 0; i < 58; i = i+1) begin
            DeScrambler_Register[i] <= RXD_input[63-i];
        end
    end

    // Decode and check the Descrambled TX data...
    // This is not a complete decoder: It only decodes the
    // block words we expect to see.
    always @(posedge core_clk156_out) begin : check_tx
        integer frame_no;
        integer word_no;

        case (frame_no)
            0 : tx_monitor_working_frame.frombits(frame0.tobits(0));
            1 : tx_monitor_working_frame.frombits(frame1.tobits(0));
            2 : tx_monitor_working_frame.frombits(frame2.tobits(0));
            3 : tx_monitor_working_frame.frombits(frame3.tobits(0));
            default : tx_monitor_working_frame.frombits(frame0.tobits(0));
        endcase

        if(reset || !resetdone || !block_lock) begin
            frame_no <= 0;
            word_no <= 0;
        end
        else if(DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h33)         // Wait for a Start code...
        begin // Start code in byte 4, data in bytes 5, 6, 7
         if(({DeScr_RXD[65:42], 8'hFB} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0001))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {DeScr_RXD[65:42],8'h00});
         else
         begin
            in_a_frame <= 1;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         word_no <= word_no + 1;
       end
       else if(DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h78)
       begin // Start code in byte 0, data on bytes 1..7
         if(({DeScr_RXD[33:10], 8'hFB} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0001))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {DeScr_RXD[33:10],8'h00});
         else
         begin
            in_a_frame <= 1;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         if((DeScr_RXD[65:34] !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      DeScr_RXD[65:34]);
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= word_no + 2;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hFF))
       begin // T code in 7th byte, data in bytes 1..7
         if((DeScr_RXD[41:10] !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      DeScr_RXD[41:10]);
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         if(({8'hFD, DeScr_RXD[65:42]} !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b1000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      {8'h00, DeScr_RXD[65:42]});
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hE1))
       begin // T code in 6th byte, data in bytes 1..6
         if((DeScr_RXD[41:10] !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      DeScr_RXD[41:10]);
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         if(({8'h07, 8'hFD, DeScr_RXD[57:42]} !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b1100))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      {8'h00, 8'h00, DeScr_RXD[57:42]});
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hD2))
       begin // T code, data in bytes 1..5
         if((DeScr_RXD[41:10] !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      DeScr_RXD[41:10]);
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         if(({8'h07, 8'h07, 8'hFD, DeScr_RXD[49:42]} !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b1110))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      {8'h00, 8'h00, 8'h00, DeScr_RXD[49:42]});
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hCC))
       begin // T code, data in bytes 1..4
         if((DeScr_RXD[41:10] !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      DeScr_RXD[41:10]);
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         if(({8'h07, 8'h07, 8'h07, 8'hFD} !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b1111))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      {8'h00, 8'h00, 8'h00, 8'h00});
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hB4))
       begin // T code, data in bytes 1..3
         if(({8'hFD, DeScr_RXD[33:10]} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b1000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {8'h00, DeScr_RXD[33:10]});
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'hAA))
       begin // T code, data in bytes 1..2
         if(({8'h07, 8'hFD, DeScr_RXD[25:10]} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b1100))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {8'h00, 8'h00, DeScr_RXD[25:10]});
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h99))
       begin // T code, data in byte 1
         if(({8'h07, 8'h07, 8'hFD, DeScr_RXD[17:10]} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b1110))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {8'h00, 8'h00, 8'h00, DeScr_RXD[17:10]});
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b01 &&
               DeScr_RXD[9:2] == 8'h87))
       begin // T code, no data
         if(({8'h07, 8'h07, 8'h07, 8'hFD} !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b1111))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      {8'h00, 8'h00, 8'h00, 8'h00});
         else
         begin
            in_a_frame <= 0;
            $display("Tx data check OK!!, frame %d, word %d", frame_no, word_no);
         end
         word_no <= 0;
         frame_no <= frame_no + 1;
       end
       else if(in_a_frame && (DeScr_RXD[1:0] == 2'b10)) // All data
       begin
         if((DeScr_RXD[33:2] !== tx_monitor_working_frame.data[word_no]) ||
            (tx_monitor_working_frame.ctrl[word_no] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no, tx_monitor_working_frame.data[word_no],
                      DeScr_RXD[33:2]);
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no);
         if((DeScr_RXD[65:34] !== tx_monitor_working_frame.data[word_no+1]) ||
            (tx_monitor_working_frame.ctrl[word_no+1] !== 4'b0000))
            $display("Tx data check ERROR!!, frame %d, word %d, StimData = %8x, MonData = %8x",
                      frame_no, word_no+1, tx_monitor_working_frame.data[word_no+1],
                      DeScr_RXD[65:34]);
         else
            $display("Tx data check OK!!, frame %d, word %d",frame_no, word_no+1);
         word_no <= word_no + 2;
        end
        if(frame_no == 4) // We're done!
            tx_simulation_finished <= 1'b1;
     end

   //----------------------------------------------------------------
   // Receive Monitor code.....
   //----------------------------------------------------------------
   reg [63:0] xgmii_rxd_int;
   reg [7:0] xgmii_rxc_int;

   wire [63:0] xgmii_rxd_del;
   wire [7:0] xgmii_rxc_del;

   // Avoid problems wiith delta delays in functional simulation,
   // since the xgmii_rx_clk is generated on the same clock as xgmii_rxd/c
   assign #1 xgmii_rxd_del = xgmii_rxd;
   assign #1 xgmii_rxc_del = xgmii_rxc;

   // Safely capture the data and control from the core onto a register for comparison
   always @(posedge xgmii_rx_clk)
     begin
       xgmii_rxc_int <= xgmii_rxc_del;
       xgmii_rxd_int <= xgmii_rxd_del;
   end

   // Simply compare what arrives on RX with what was Transmitted to core
   always@(xgmii_rx_clk)
     begin : rx_check

       integer rx_frame_no;
       integer rx_word_no;
       reg     rx_half_word;

       case (rx_frame_no)
         0 : rx_monitor_working_frame.frombits(frame0.tobits(0));
         1 : rx_monitor_working_frame.frombits(frame1.tobits(0));
         2 : rx_monitor_working_frame.frombits(frame2.tobits(0));
         3 : rx_monitor_working_frame.frombits(frame3.tobits(0));
         default : rx_monitor_working_frame.frombits(frame0.tobits(0));
       endcase


        if(xgmii_rx_clk == 1'b1)
            rx_half_word = 1'b1;
        else
            rx_half_word = 1'b0;

        if(reset)
        begin
            rx_frame_no <= 0;
            rx_word_no <= 0;
        end
        else if(rx_half_word == 0)
        begin
            if(xgmii_rxc_int[3:0] == 4'b0001 && xgmii_rxd_int[7:0] == 8'hFB) // Frames always begin with this
            begin
                if(xgmii_rxd_int[31:0] !== rx_monitor_working_frame.data[rx_word_no])
                    $display("Rx data check ERROR (/S/)!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_word_no <= rx_word_no + 1;
            end
            else if(rx_word_no > 0 && xgmii_rxc_int[3:0] == 4'b0000) // Data only
            begin
                if(xgmii_rxd_int[31:0] !== rx_monitor_working_frame.data[rx_word_no])
                    $display("Rx data check ERROR!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_word_no <= rx_word_no + 1;
            end
            else if(rx_word_no > 0)  // T code plus 0, 1, 2 or 3 data
            begin
                if(xgmii_rxd_int[31:0] !== rx_monitor_working_frame.data[rx_word_no])
                     $display("Rx data check ERROR (/T/)!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_frame_no <= rx_frame_no + 1;
                rx_word_no <= 0;
            end
        end
        else // Upper half of rx word
        begin
            if(xgmii_rxc_int[7:4] == 4'b0001 && xgmii_rxd_int[39:32] == 8'hFB) // Frames always begin with this
            begin
                if(xgmii_rxd_int[63:32] !== rx_monitor_working_frame.data[rx_word_no])
                    $display("Rx data check ERROR (/S/)!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_word_no <= rx_word_no + 1;
            end
            else if(rx_word_no > 0 && xgmii_rxc_int[7:4] == 4'b0000) // Data only
            begin
                if(xgmii_rxd_int[63:32] !== rx_monitor_working_frame.data[rx_word_no])
                     $display("Rx data check ERROR!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_word_no <= rx_word_no + 1;
            end
            else if(rx_word_no > 0)  // T code plus 0, 1, 2 or 3 data
            begin
                if(xgmii_rxd_int[63:32] !== rx_monitor_working_frame.data[rx_word_no])
                    $display("Rx data check ERROR (/T/)!!, frame %d, word %d", rx_frame_no, rx_word_no);
                else
                    $display("Rx data check OK!!, frame %d, word %d", rx_frame_no, rx_word_no);
                rx_frame_no <= rx_frame_no + 1;
                rx_word_no <= 0;
            end
        end
        if(rx_frame_no == 4) // We're done!
            rx_simulation_finished <= 1'b1;
     end // rx_check

    //----------------------------------------------------------------
    // Receive Stimulus code.....
    //----------------------------------------------------------------

    // Support code for transmitting frames to core rxn/p
    reg [65:0] TxEnc;
    reg [31:0] d0;
    reg [3:0] c0;
    reg [63:0] d;
    reg [7:0] c;
    reg decided_clk_edge = 0;
    reg clk_edge;

    // Encode next 64 bits of frame;
    task rx_stimulus_send_column;
    input [31:0] d1;
    input [ 3:0] c1;
    begin : rx_stimulus_send_column
        @(posedge core_clk156_out or negedge core_clk156_out);
        d0 <= d1;
        c0 <= c1;

        assign d = {d1, d0};
        assign c = {c1, c0};

        // Need to know when to apply the encoded data to the scrambler
        if(!decided_clk_edge && |c0) // Found first full 64 bit word
        begin
          clk_edge <= !core_clk156_out;
          decided_clk_edge <= 1;
        end

        // Detect column of IDLEs vs T code in byte 0
        if(&c && d[7:0] !== 8'hFD) // Column of IDLEs
        begin
            TxEnc[1:0] = 2'b01;
            TxEnc[65:2] = 64'h00000000001E;
        end
        else if(|c) // Control code somewhere
        begin
            TxEnc[1:0] = 2'b01;

            if(c == 8'b00000001) // Start code
            begin
                TxEnc[9:2] = 8'h78;
                TxEnc[65:10] = d[63:8];
            end
            if(c == 8'b00011111) // Start code
            begin
                TxEnc[9:2] = 8'h33;
                TxEnc[41:10] = 32'h00000000;
                TxEnc[65:42] = d[63:40];
            end
            else if(c == 8'b10000000) // End code
            begin
                TxEnc[9:2] = 8'hFF;
                TxEnc[65:10] = d[55:0];
            end
            else if(c == 8'b11000000) // End code
            begin
                TxEnc[9:2] = 8'hE1;
                TxEnc[57:10] = d[47:0];
                TxEnc[65:58] = 8'h00;
            end
            else if(c == 8'b11100000) // End code
            begin
                TxEnc[9:2] = 8'hD2;
                TxEnc[49:10] = d[39:0];
                TxEnc[65:50] = 16'h0000;
            end
            else if(c == 8'b11110000) // End code
            begin
                TxEnc[9:2] = 8'hCC;
                TxEnc[41:10] = d[31:0];
                TxEnc[65:42] = 24'h000000;
            end
            else if(c == 8'b11111000) // End code
            begin
                TxEnc[9:2] = 8'hB4;
                TxEnc[33:10] = d[23:0];
                TxEnc[65:34] = 32'h00000000;
            end
            else if(c == 8'b11111100) // End code
            begin
                TxEnc[9:2] = 8'hAA;
                TxEnc[25:10] = d[15:0];
                TxEnc[65:26] = 40'h0000000000;
            end
            else if(c == 8'b11111110) // End code
            begin
                TxEnc[9:2] = 8'h99;
                TxEnc[17:10] = d[7:0];
                TxEnc[65:18] = 48'h000000000000;
            end
            else if(c == 8'b11111111) // && d0[7:0] == 8'hFD) // End code
            begin
                TxEnc[9:2] = 8'h87;
                TxEnc[65:10] = 56'h00000000000000;
            end
        end
        else // all data
        begin
            TxEnc[1:0] = 2'b10;
            TxEnc[65:2] = d;
        end
    end
    endtask // rx_stimulus_send_column

    task rx_stimulus_send_idle;
    begin
        rx_stimulus_send_column(32'h07070707, 4'b1111);
    end
    endtask // rx_stimulus_send_idle

    task rx_stimulus_send_frame;
    input `FRAME_TYP frame;
    integer column_index;
    begin
        rx_stimulus_working_frame.frombits(frame);
        column_index = 0;
        // send columns
        while (column_index < rx_stimulus_working_frame.length)
            begin
                rx_stimulus_send_column(rx_stimulus_working_frame.data[column_index],
                                    rx_stimulus_working_frame.ctrl[column_index]);
                column_index = column_index + 1;
            end
        $display("Receiver: frame inserted into Serial interface");
    end
    endtask // rx_stimulus_send_frame

    initial
    begin : p_rx_stimulus
        // Wait for the core to come up
        while (core_status[0] !== 1'b1)
            rx_stimulus_send_idle;

        rx_stimulus_send_frame(frame0.tobits(0));
        rx_stimulus_send_idle;
        rx_stimulus_send_idle;
        rx_stimulus_send_frame(frame1.tobits(0));
        rx_stimulus_send_idle;
        rx_stimulus_send_idle;
        rx_stimulus_send_frame(frame2.tobits(0));
        rx_stimulus_send_idle;
        rx_stimulus_send_idle;
        rx_stimulus_send_frame(frame3.tobits(0));
        while (1)
            rx_stimulus_send_idle;
    end // block: p_rx_stimulus

    // Capture the 66 bit data for scrambling...
    reg [65:0] TxEnc_Data = 66'h79;
    wire TxEnc_clock;

    assign TxEnc_clock = clk_edge ? core_clk156_out : !core_clk156_out;
    always @(posedge TxEnc_clock) begin
        TxEnc_Data <= TxEnc;
    end

    reg [65:0] TXD_Scr = 66'h2;

    reg [57:0]    Scrambler_Register = 58'h3;
    reg [63:0]    TXD_input = 0;
    reg [1:0]     Sync_header = 2'b10;
    wire [63:0]   Scr_wire;

    // Scramble the TxEnc_Data before applying to rxn/p
    assign Scr_wire[0] = TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57];
    assign Scr_wire[1] = TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56];
    assign Scr_wire[2] = TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55];
    assign Scr_wire[3] = TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54];
    assign Scr_wire[4] = TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53];
    assign Scr_wire[5] = TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52];
    assign Scr_wire[6] = TXD_input[6]^Scrambler_Register[32]^Scrambler_Register[51];
    assign Scr_wire[7] = TXD_input[7]^Scrambler_Register[31]^Scrambler_Register[50];

    assign Scr_wire[8] = TXD_input[8]^Scrambler_Register[30]^Scrambler_Register[49];
    assign Scr_wire[9] = TXD_input[9]^Scrambler_Register[29]^Scrambler_Register[48];
    assign Scr_wire[10] = TXD_input[10]^Scrambler_Register[28]^Scrambler_Register[47];
    assign Scr_wire[11] = TXD_input[11]^Scrambler_Register[27]^Scrambler_Register[46];
    assign Scr_wire[12] = TXD_input[12]^Scrambler_Register[26]^Scrambler_Register[45];
    assign Scr_wire[13] = TXD_input[13]^Scrambler_Register[25]^Scrambler_Register[44];
    assign Scr_wire[14] = TXD_input[14]^Scrambler_Register[24]^Scrambler_Register[43];
    assign Scr_wire[15] = TXD_input[15]^Scrambler_Register[23]^Scrambler_Register[42];

    assign Scr_wire[16] = TXD_input[16]^Scrambler_Register[22]^Scrambler_Register[41];
    assign Scr_wire[17] = TXD_input[17]^Scrambler_Register[21]^Scrambler_Register[40];
    assign Scr_wire[18] = TXD_input[18]^Scrambler_Register[20]^Scrambler_Register[39];
    assign Scr_wire[19] = TXD_input[19]^Scrambler_Register[19]^Scrambler_Register[38];
    assign Scr_wire[20] = TXD_input[20]^Scrambler_Register[18]^Scrambler_Register[37];
    assign Scr_wire[21] = TXD_input[21]^Scrambler_Register[17]^Scrambler_Register[36];
    assign Scr_wire[22] = TXD_input[22]^Scrambler_Register[16]^Scrambler_Register[35];
    assign Scr_wire[23] = TXD_input[23]^Scrambler_Register[15]^Scrambler_Register[34];

    assign Scr_wire[24] = TXD_input[24]^Scrambler_Register[14]^Scrambler_Register[33];
    assign Scr_wire[25] = TXD_input[25]^Scrambler_Register[13]^Scrambler_Register[32];
    assign Scr_wire[26] = TXD_input[26]^Scrambler_Register[12]^Scrambler_Register[31];
    assign Scr_wire[27] = TXD_input[27]^Scrambler_Register[11]^Scrambler_Register[30];
    assign Scr_wire[28] = TXD_input[28]^Scrambler_Register[10]^Scrambler_Register[29];
    assign Scr_wire[29] = TXD_input[29]^Scrambler_Register[9]^Scrambler_Register[28];
    assign Scr_wire[30] = TXD_input[30]^Scrambler_Register[8]^Scrambler_Register[27];
    assign Scr_wire[31] = TXD_input[31]^Scrambler_Register[7]^Scrambler_Register[26];

    assign Scr_wire[32] = TXD_input[32]^Scrambler_Register[6]^Scrambler_Register[25];
    assign Scr_wire[33] = TXD_input[33]^Scrambler_Register[5]^Scrambler_Register[24];
    assign Scr_wire[34] = TXD_input[34]^Scrambler_Register[4]^Scrambler_Register[23];
    assign Scr_wire[35] = TXD_input[35]^Scrambler_Register[3]^Scrambler_Register[22];
    assign Scr_wire[36] = TXD_input[36]^Scrambler_Register[2]^Scrambler_Register[21];
    assign Scr_wire[37] = TXD_input[37]^Scrambler_Register[1]^Scrambler_Register[20];
    assign Scr_wire[38] = TXD_input[38]^Scrambler_Register[0]^Scrambler_Register[19];
    assign Scr_wire[39] = TXD_input[39]^TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57]^Scrambler_Register[18];
    assign Scr_wire[40] = TXD_input[40]^(TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56])^Scrambler_Register[17];
    assign Scr_wire[41] = TXD_input[41]^(TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55])^Scrambler_Register[16];
    assign Scr_wire[42] = TXD_input[42]^(TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54])^Scrambler_Register[15];
    assign Scr_wire[43] = TXD_input[43]^(TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53])^Scrambler_Register[14];
    assign Scr_wire[44] = TXD_input[44]^(TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52])^Scrambler_Register[13];
    assign Scr_wire[45] = TXD_input[45]^(TXD_input[6]^Scrambler_Register[32]^Scrambler_Register[51])^Scrambler_Register[12];
    assign Scr_wire[46] = TXD_input[46]^(TXD_input[7]^Scrambler_Register[31]^Scrambler_Register[50])^Scrambler_Register[11];
    assign Scr_wire[47] = TXD_input[47]^(TXD_input[8]^Scrambler_Register[30]^Scrambler_Register[49])^Scrambler_Register[10];

    assign Scr_wire[48] = TXD_input[48]^(TXD_input[9]^Scrambler_Register[29]^Scrambler_Register[48])^Scrambler_Register[9];
    assign Scr_wire[49] = TXD_input[49]^(TXD_input[10]^Scrambler_Register[28]^Scrambler_Register[47])^Scrambler_Register[8];
    assign Scr_wire[50] = TXD_input[50]^(TXD_input[11]^Scrambler_Register[27]^Scrambler_Register[46])^Scrambler_Register[7];
    assign Scr_wire[51] = TXD_input[51]^(TXD_input[12]^Scrambler_Register[26]^Scrambler_Register[45])^Scrambler_Register[6];
    assign Scr_wire[52] = TXD_input[52]^(TXD_input[13]^Scrambler_Register[25]^Scrambler_Register[44])^Scrambler_Register[5];
    assign Scr_wire[53] = TXD_input[53]^(TXD_input[14]^Scrambler_Register[24]^Scrambler_Register[43])^Scrambler_Register[4];
    assign Scr_wire[54] = TXD_input[54]^(TXD_input[15]^Scrambler_Register[23]^Scrambler_Register[42])^Scrambler_Register[3];
    assign Scr_wire[55] = TXD_input[55]^(TXD_input[16]^Scrambler_Register[22]^Scrambler_Register[41])^Scrambler_Register[2];

    assign Scr_wire[56] = TXD_input[56]^(TXD_input[17]^Scrambler_Register[21]^Scrambler_Register[40])^Scrambler_Register[1];
    assign Scr_wire[57] = TXD_input[57]^(TXD_input[18]^Scrambler_Register[20]^Scrambler_Register[39])^Scrambler_Register[0];
    assign Scr_wire[58] = TXD_input[58]^(TXD_input[19]^Scrambler_Register[19]^Scrambler_Register[38])^(TXD_input[0]^Scrambler_Register[38]^Scrambler_Register[57]);
    assign Scr_wire[59] = TXD_input[59]^(TXD_input[20]^Scrambler_Register[18]^Scrambler_Register[37])^(TXD_input[1]^Scrambler_Register[37]^Scrambler_Register[56]);
    assign Scr_wire[60] = TXD_input[60]^(TXD_input[21]^Scrambler_Register[17]^Scrambler_Register[36])^(TXD_input[2]^Scrambler_Register[36]^Scrambler_Register[55]);
    assign Scr_wire[61] = TXD_input[61]^(TXD_input[22]^Scrambler_Register[16]^Scrambler_Register[35])^(TXD_input[3]^Scrambler_Register[35]^Scrambler_Register[54]);
    assign Scr_wire[62] = TXD_input[62]^(TXD_input[23]^Scrambler_Register[15]^Scrambler_Register[34])^(TXD_input[4]^Scrambler_Register[34]^Scrambler_Register[53]);
    assign Scr_wire[63] = TXD_input[63]^(TXD_input[24]^Scrambler_Register[14]^Scrambler_Register[33])^(TXD_input[5]^Scrambler_Register[33]^Scrambler_Register[52]);


    always @(posedge TxEnc_clock) begin
        TXD_input[63:0] <= TxEnc_Data[65:2];
        Sync_header[1:0] <= TxEnc_Data[1:0];
        TXD_Scr[65:0] <= {Scr_wire[63:0], Sync_header[1:0]};
        for (i=0; i<58; i=i+1) begin
            Scrambler_Register[i] <= Scr_wire[63-i];
        end
    end

    // Serialize the RX stimulus
    assign rxn = !rxp;

    reg[65:0] serial_word = 66'h0;
    integer rxbitno = 'd0;

    always @(posedge bitclk) begin : rx_serialize
        rxp <= serial_word[rxbitno];
        rxbitno <= (rxbitno + 1) % 66;
        // Pull in the next word when we have sent 66 bits
        if (rxbitno == 'd65) begin
            serial_word <= TXD_Scr;
        end
    end // rx_serialize

endmodule
