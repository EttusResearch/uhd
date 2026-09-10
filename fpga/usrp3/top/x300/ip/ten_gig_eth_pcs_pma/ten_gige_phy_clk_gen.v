//
// Copyright 2008-2013 Ettus Research LLC
//

module ten_gige_phy_clk_gen
(
   input  areset,
   input  refclk_p,
   input  refclk_n,

   output refclk,
   output clk156,
   output dclk
);

   wire clk156_buf;
   wire dclk_buf;
   wire clkfbout;
   wire clkfb;
   wire refclk_buf;

   IBUFDS_GTE2 ibufds_inst (
      .O     (refclk),
      .ODIV2 (),
      .CEB   (1'b0),
      .I     (refclk_p),
      .IB    (refclk_n)
   );

   // required buffer to get differential refclock from clock region without PLL instance to another
   // one
   BUFG refclk_bufg_inst (
      .I (refclk),
      .O (refclk_buf)
   );

   // Clock cleanup required with Vivado 2025.2 to remove hold timing violations downstream
   PLLE2_BASE #(
      .BANDWIDTH("HIGH"),
      .CLKFBOUT_MULT(10),
      .CLKFBOUT_PHASE(0.0),
      .CLKIN1_PERIOD(6.4),
      .CLKOUT0_DIVIDE(10),
      .CLKOUT1_DIVIDE(20),
      .DIVCLK_DIVIDE(1),
      .REF_JITTER1(0.010),
      .STARTUP_WAIT("FALSE")
   ) pll_clk156_inst (
      .CLKOUT0(clk156_buf),
      .CLKOUT1(dclk_buf),
      .CLKOUT2(),
      .CLKOUT3(),
      .CLKOUT4(),
      .CLKOUT5(),
      .CLKFBOUT(clkfbout),
      .LOCKED(),
      .CLKIN1(refclk_buf),
      .PWRDWN(1'b0),
      .RST(areset),
      .CLKFBIN(clkfb)
   );

   // PLL feedback clock buffer
   BUFG clkfb_bufg_inst (
      .I (clkfbout),
      .O (clkfb)
   );

   // buffer for generated clocks
   BUFG clk156_bufg_inst (
      .I (clk156_buf),
      .O (clk156)
   );
   BUFG dclk_bufg_i (
      .I (dclk_buf),
      .O (dclk)
   );

endmodule



