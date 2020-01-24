//
// Copyright 2014 Ettus Research LLC
//

module one_gige_phy_clk_gen 
(
   input  refclk_p,
   input  refclk_n,
   output refclk,
   output refclk_bufg
);

   IBUFDS_GTE2 ibufds_inst (
      .O     (refclk),
      .ODIV2 (),
      .CEB   (1'b0),
      .I     (refclk_p),
      .IB    (refclk_n)
   );

   BUFG bufg_gtrefclk_inst (
      .I(refclk),
      .O(refclk_bufg)
   );

endmodule



