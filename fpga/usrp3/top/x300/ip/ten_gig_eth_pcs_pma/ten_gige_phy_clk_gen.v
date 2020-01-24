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
   
   IBUFDS_GTE2 ibufds_inst (
      .O     (refclk),
      .ODIV2 (),
      .CEB   (1'b0),
      .I     (refclk_p),
      .IB    (refclk_n)
   );

   BUFG clk156_bufg_inst (
      .I (refclk),
      .O (clk156) 
   );

   // Divding independent clock by 2 as source for DRP clock
   BUFR # (
      .BUFR_DIVIDE ("2")
   ) dclk_divide_by_2_buf (
      .I   (clk156),
      .O   (dclk_buf),
      .CE  (1'b1),
      .CLR (1'b0)
   );

   BUFG dclk_bufg_i (
      .I (dclk_buf),
      .O (dclk)
   );

endmodule



