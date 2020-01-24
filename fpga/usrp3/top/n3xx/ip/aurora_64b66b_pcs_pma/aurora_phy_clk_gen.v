//
// Copyright 2016 Ettus Research LLC
//

module aurora_phy_clk_gen 
(
   input refclk_ibuf,
   output clk156,
   output init_clk
);

   wire init_clk_buf;

   BUFG clk156_bufg_inst (
      .I (refclk_ibuf),
      .O (clk156) 
   );

   // Divding independent clock by 2 as source for DRP clock
   BUFR # (
      .BUFR_DIVIDE ("2")
   ) dclk_divide_by_2_buf (
      .I   (clk156),
      .O   (init_clk_buf),
      .CE  (1'b1),
      .CLR (1'b0)
   );

   BUFG dclk_bufg_i (
      .I (init_clk_buf),
      .O (init_clk)
   );

endmodule



