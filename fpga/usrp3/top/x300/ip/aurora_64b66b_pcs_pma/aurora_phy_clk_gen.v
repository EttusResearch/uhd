//
// Copyright 2016 Ettus Research LLC
//

module aurora_phy_clk_gen 
(
   input  areset,
   input  refclk_p,
   input  refclk_n,

   output refclk,
   output clk156,
   output init_clk
);

   wire clk156_buf;
   wire init_clk_buf;
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
      .O   (init_clk_buf),
      .CE  (1'b1),
      .CLR (1'b0)
   );

   BUFG dclk_bufg_i (
      .I (init_clk_buf),
      .O (init_clk)
   );

endmodule



