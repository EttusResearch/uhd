/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: e320_clocking.v
//
// Purpose:
//
// TODO: First, instantiate clock input buffers on all clocks to provide termination
// for the PCB traces.
//
// Second, PPS inputs from the back panel (called external) and the GPSDO are captured by
// the Reference Clock. Selection is performed amongst these and the internally-generated
// options.
//
//////////////////////////////////////////////////////////////////////

module e320_clocking (
  input global_rst,

  // Reference Clk
  input ref_clk_from_pin,
  output ref_clk,

  // Input clocks
  input clk156,    // 156.25 MHz

  // Output clocks
  output ddr3_dma_clk,
  output reg clocks_locked = 1'b0,

  // PPS Capture & Selection
  input       ext_pps_from_pin,
  input       gps_pps_from_pin,
  input [1:0] pps_select,
  output reg  pps_refclk
);

  //TODO: Code is same as n3xx, try reusing it.

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
  IBUFG ref_clk_ibuf (
    .O(ref_clk_buf),
    .I(ref_clk_from_pin)
  );

  BUFG ref_clk_bufg (
    .I(ref_clk_buf),
    .O(ref_clk)
  );

  wire pps_ext_refclk;
  wire pps_gps_refclk;
  wire [1:0] pps_select_refclk;

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
    .WIDTH(2)
  ) pps_select_dsync (
    .clk(ref_clk), .rst(1'b0), .in(pps_select), .out(pps_select_refclk)
  );

  // Bit locations for the pps_select vector.
  localparam BIT_PPS_SEL_INT = 0;
  localparam BIT_PPS_SEL_EXT = 1;

  // PPS MUX - selects internal/gpsdo or external PPS.
  always @(posedge ref_clk) begin

    // Encoding is one-hot on these bits. It is possible when the vector is being double-
    // synchronized to the reference clock domain that there could be multiple bits
    // asserted simultaneously. This is not problematic because the order of operations
    // in the following selection mux should take over and only one PPS should win.
    // This could result in glitches, but that is expected during ANY PPS switchover
    // since the switch is performed asynchronously to the PPS signal.
    if (pps_select_refclk[BIT_PPS_SEL_INT]) begin
      pps_refclk <= pps_gps_refclk;
    end else if (pps_select_refclk[BIT_PPS_SEL_EXT]) begin
      pps_refclk <= pps_ext_refclk;
    end else begin
      pps_refclk <= pps_gps_refclk;
    end
  end

  //---------------------------------------------------------------------------
  // Clock Generation
  //---------------------------------------------------------------------------

  MMCME2_ADV #(
    .BANDWIDTH            ("OPTIMIZED"),
    .CLKOUT4_CASCADE      ("FALSE"),
    .COMPENSATION         ("ZHOLD"),
    .STARTUP_WAIT         ("FALSE"),
    .DIVCLK_DIVIDE        (1),
    .CLKFBOUT_MULT_F      (6.000),
    .CLKFBOUT_PHASE       (0.000),
    .CLKFBOUT_USE_FINE_PS ("FALSE"),
    .CLKOUT0_DIVIDE_F     (3.125),
    .CLKOUT0_PHASE        (0.000),
    .CLKOUT0_DUTY_CYCLE   (0.500),
    .CLKOUT0_USE_FINE_PS  ("FALSE"),
    .CLKIN1_PERIOD        (6.400))
  mmcm_adv_inst (
    .CLKFBOUT            (clkfbout),
    .CLKFBOUTB           (),
    .CLKOUT0             (ddr3_dma_clk_raw),
    .CLKOUT0B            (),
    .CLKOUT1             (),
    .CLKOUT1B            (),
    .CLKOUT2             (),
    .CLKOUT2B            (),
    .CLKOUT3             (),
    .CLKOUT3B            (),
    .CLKOUT4             (),
    .CLKOUT5             (),
    .CLKOUT6             (),
     // Input clock control
    .CLKFBIN             (clkfbout),
    .CLKIN1              (clk156),
    .CLKIN2              (1'b0),
     // Tied to always select the primary input clock
    .CLKINSEL            (1'b1),
    // Ports for dynamic reconfiguration
    .DADDR               (7'h0),
    .DCLK                (1'b0),
    .DEN                 (1'b0),
    .DI                  (16'h0),
    .DO                  (),
    .DRDY                (),
    .DWE                 (1'b0),
    // Ports for dynamic phase shift
    .PSCLK               (1'b0),
    .PSEN                (1'b0),
    .PSINCDEC            (1'b0),
    .PSDONE              (),
    // Other control and status signals
    .LOCKED              (locked_raw),
    .CLKINSTOPPED        (),
    .CLKFBSTOPPED        (),
    .PWRDWN              (1'b0),
    .RST                 (global_rst));

  BUFG clk300_bufg
     (.O   (ddr3_dma_clk),
      .I   (ddr3_dma_clk_raw));


  //---------------------------------------------------------------------------
  // Lock Signal
  //---------------------------------------------------------------------------
  //
  // We assume that the LOCKED signal from the MMCM is not necessarily a clean
  // asynchronous signal, so we want to make sure that the MMCM is really
  // locked before we assert our clocks_locked output.
  //
  //---------------------------------------------------------------------------

  reg [9:0] locked_count = ~0;

  synchronizer lock_sync_i (
    .clk(clk156), .rst(1'b0), .in(locked_raw), .out(locked_sync)
  );

  // Filter the locked signal
  always @(posedge clk156 or posedge global_rst)
  begin
    if (global_rst) begin
      locked_count  <= ~0;
      clocks_locked <=  0;
    end else begin
      if (~locked_sync) begin
        locked_count  <= ~0;
        clocks_locked <= 1'b0;
      end else begin
        if (locked_count == 0) begin
          clocks_locked <= 1'b1;
        end else begin
          clocks_locked <= 1'b0;
          locked_count <= locked_count - 1;
        end
      end
    end
  end

endmodule
