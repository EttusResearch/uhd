//
// Copyright 2008-2013 Ettus Research LLC
//

module ten_gige_phy_clk_gen
(
   input refclk_ibuf,
   output clk156,
   output dclk
);

   wire dclk_buf;

   BUFG clk156_bufg_inst (
      .I (refclk_ibuf),
      .O (clk156)
   );

   // Dividing independent clock by 2 as source for DRP clock
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



