/////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Ettus Research, A National Instruments Company
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: n3xx_clocking.v
//
// Purpose:
//
// First, instantiate clock input buffers on all clocks to provide termination
// for the PCB traces. This file also includes the MMCM for generating meas_clk at
// specific rates and a global buffer for the reference clock to be able to use it
// directly within and outside of this module.
//
// Second, PPS inputs from the back panel (called external) and the GPSDO are captured by
// the Reference Clock. Selection is performed amongst these and the internally-generated
// options.
//
// NOTE: BUFGs are NOT instantiated on the following clocks, denoted by the _buf suffix:
//   wr_refclk_buf, netclk_buf, gige_refclk_buf, xgige_refclk_buf
//
//////////////////////////////////////////////////////////////////////

module n3xx_clocking (
  // Input buffers for clocks
  input  enable_ref_clk_async, // enables the ref_clk BUFG (driven async to ref_clk)
  input  FPGA_REFCLK_P, FPGA_REFCLK_N,
  output ref_clk,

  input  WB_20MHz_P, WB_20MHz_N,
  output wr_refclk_buf,

  input  NETCLK_REF_P, NETCLK_REF_N,
  output netclk_buf,

  input  NETCLK_P, NETCLK_N,
  output gige_refclk_buf,

  input  MGT156MHZ_CLK1_P, MGT156MHZ_CLK1_N,
  output xgige_refclk_buf,

  // Measurement Clock Generation
  input  misc_clks_ref,
  output meas_clk,
  output ddr3_dma_clk,
  input  misc_clks_reset,
  output misc_clks_locked,

  // PPS Capture & Selection
  input       ext_pps_from_pin,
  input       gps_pps_from_pin,
  input [3:0] pps_select,
  output reg  pps_refclk
  );

  // Clock Buffering and Generation : ///////////////////////////////////////////////////
  //
  // Manually instantiate input buffers on all clocks, and a global buffer on the
  // Reference Clock for use in the rest of the design. All other clocks must have
  // global buffers other places, since the declarations here are for SI purposes.
  //
  ///////////////////////////////////////////////////////////////////////////////////////

  wire ref_clk_buf;

  // FPGA Reference Clock Buffering
  //
  // Only require an IBUF and BUFG here, since an MMCM is (thankfully) not needed
  // to meet timing with the PPS signal.
  IBUFGDS ref_clk_ibuf (
    .O(ref_clk_buf),
    .I(FPGA_REFCLK_P),
    .IB(FPGA_REFCLK_N)
  );

  // BUFG ref_clk_bufg (
    // .I(ref_clk_buf),
    // .O(ref_clk)
  // );
  WrapBufg #(
     .kEnableIsAsync(1'b1)
  ) ref_clk_bufg (
     .ClkIn(ref_clk_buf),
     .aCe(enable_ref_clk_async),
     .ClkOut(ref_clk)
  );

  // Buffers for SI Purposes
  //
  // Instantiate buffers on each of these differential clock inputs with DONT_TOUCH
  // attributes in order to preserve the internal termination regardless of whether
  // these clocks are used in the design.  The lack of termination would place the
  // voltage swings for these pins outside the acceptable range for the FPGA inputs.
  (* dont_touch = "true" *) IBUFGDS wr_refclk_ibuf (
    .I (WB_20MHz_P),
    .IB(WB_20MHz_N),
    .O (wr_refclk_buf)
  );

  (* dont_touch = "true" *) IBUFGDS netclk_ref_ibuf (
    .I (NETCLK_REF_P),
    .IB(NETCLK_REF_N),
    .O (netclk_buf)
  );

  // Same deal for the MGT reference clock buffers.
  (* dont_touch = "true" *) IBUFDS_GTE2 gige_refclk_ibuf (
    .ODIV2(),
    .CEB  (1'b0),
    .I (NETCLK_P),
    .IB(NETCLK_N),
    .O (gige_refclk_buf)
  );

  (* dont_touch = "true" *) IBUFDS_GTE2 ten_gige_refclk_ibuf (
    .ODIV2(),
    .CEB  (1'b0),
    .I (MGT156MHZ_CLK1_P),
    .IB(MGT156MHZ_CLK1_N),
    .O (xgige_refclk_buf)
  );

  // Measurement Clock MMCM Instantiation
  //
  // This must be an MMCM to hit the weird rates we need for meas_clk. It takes the
  // 166.6667 MHz clock from the PS and provides the correct meas_clk rate for the TDC.
  // BUFG is embedded in the MMCM files.
  //----------------------------------------------------------------------------
  //  Output     Output      Phase    Duty Cycle   Pk-to-Pk     Phase
  //   Clock     Freq (MHz)  (degrees)    (%)     Jitter (ps)  Error (ps)
  //----------------------------------------------------------------------------
  //     meas_clk___198.413______0.000______50.0______113.755____141.292
  // ddr3_dma_clk___303.819______0.000______50.0______105.705____141.292
  //
  //----------------------------------------------------------------------------
  // Input Clock   Freq (MHz)    Input Jitter (UI)
  //----------------------------------------------------------------------------
  // __primary________166.666667____________0.010

  misc_clock_gen misc_clock_gen_i (
    .clk_in       (misc_clks_ref),
    .meas_clk     (meas_clk),
    .ddr3_dma_clk (ddr3_dma_clk),
    .reset        (misc_clks_reset),
    .locked       (misc_clks_locked)
  );


  // PPS Capture and Generation : ///////////////////////////////////////////////////////
  //
  // The following shows the support matrix for PPS with respect to the
  // reference clock source and rate.
  //               _______________________________
  //  ____________|              PPS              |
  // |   Clocks   | External | FPGA  | GPSDO | WR |
  // |--------------------------------------------|
  // |External 10 |    x     |  x    |       |    |
  // |Internal 25 |          |  x    |       | x  |
  // |GPSDO    20 |          |       |   x   |    |
  // |--------------------------------------------|
  //
  ///////////////////////////////////////////////////////////////////////////////////////

  wire pps_ext_refclk;
  wire pps_gps_refclk;
  wire [3:0] pps_select_refclk;

  // Generate two internal PPS signals, each with a 25% duty cycle, based on
  // 10 MHz and 25 MHz Reference Clock rates. Only one will be used at a time.
  wire int_pps_10mhz_refclk;
  pps_generator #(
     .CLK_FREQ(32'd10_000_000), .DUTY_CYCLE(25)
  ) pps_gen_10 (
     .clk(ref_clk), .reset(1'b0), .pps(int_pps_10mhz_refclk)
  );
  wire int_pps_25mhz_refclk;
  pps_generator #(
     .CLK_FREQ(32'd25_000_000), .DUTY_CYCLE(25)
  ) pps_gen_25 (
     .clk(ref_clk), .reset(1'b0), .pps(int_pps_25mhz_refclk)
  );

  // Capture the external PPSs with a FF before sending them to the mux. To be safe,
  // we double-synchronize the external signals. If we meet timing (which we should)
  // then this is a two-cycle delay. If we don't meet timing, then it's 1-2 cycles
  // and our system timing is thrown off--but at least our downstream logic doesn't
  // go metastable!
  synchronizer #(
    .FALSE_PATH_TO_IN(0)
  ) ext_pps_dsync (
    .clk(ref_clk), .rst(1'b0), .in(ext_pps_from_pin), .out(pps_ext_refclk)
  );
  // Same deal with the GPSDO PPS input. Double-sync, then use it.
  synchronizer #(
    .FALSE_PATH_TO_IN(0)
  ) gps_pps_dsync (
    .clk(ref_clk), .rst(1'b0), .in(gps_pps_from_pin), .out(pps_gps_refclk)
  );

  // Synchronize the select bits over to the reference clock as well. Note that this is
  // a vector, so we could have some non-one-hot values creep through when changing.
  // See the note below as to why this is safe.
  synchronizer #(
    .FALSE_PATH_TO_IN(1),
    .WIDTH(4)
  ) pps_select_dsync (
    .clk(ref_clk), .rst(1'b0), .in(pps_select), .out(pps_select_refclk)
  );

  // Bit locations for the pps_select vector.
  localparam BIT_PPS_SEL_INT_10 = 0;
  localparam BIT_PPS_SEL_INT_25 = 1;
  localparam BIT_PPS_SEL_EXT    = 2;
  localparam BIT_PPS_SEL_GPSDO  = 3;

  // PPS MUX - selects internal or external PPS.
  always @(posedge ref_clk) begin

    // Encoding is one-hot on these bits. It is possible when the vector is being double-
    // synchronized to the reference clock domain that there could be multiple bits
    // asserted simultaneously. This is not problematic because the order of operations
    // in the following selection mux should take over and only one PPS should win.
    // This could result in glitches, but that is expected during ANY PPS switchover
    // since the switch is performed asynchronously to the PPS signal.
    if (pps_select_refclk[BIT_PPS_SEL_INT_10])
      pps_refclk <= int_pps_10mhz_refclk;
    else if (pps_select_refclk[BIT_PPS_SEL_INT_25])
      pps_refclk <= int_pps_25mhz_refclk;
    else if (pps_select_refclk[BIT_PPS_SEL_EXT])
      pps_refclk <= pps_ext_refclk;
    else if (pps_select_refclk[BIT_PPS_SEL_GPSDO])
      pps_refclk <= pps_gps_refclk;
    else
      pps_refclk <= pps_ext_refclk; // Compatibility with old SW stacks, pps_select_refclk = 0 = external

  end

endmodule
