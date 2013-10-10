

module catcodec_ddr_cmos
(
   //output source sync clock for baseband data
   output radio_clk,

   //async reset for clocking
   input arst,

   //control mimo mode
   input mimo,

   //baseband sample interface
   output reg [31:0] rx1,
   output reg [31:0] rx2,
   input [31:0] tx1,
   input [31:0] tx2,

   //capture interface
   input rx_clk,
   input rx_frame,
   input [11:0] rx_d,

   //generate interface
   output tx_clk,
   output tx_frame,
   output [11:0] tx_d
);

   //rx_clk to DCM - creates codec_clk and codec_clk/2
   wire clk0, clkdv;
   wire locked;
   wire codec_rst = !locked;

   DCM_SP #(
        .CLKDV_DIVIDE(2),
        .CLK_FEEDBACK("1X")
   ) DCM_SP_codec_clk
   (
        .RST(arst),
        .CLKIN(rx_clk), .CLKFB(clk0),
        .CLK0(clk0), .CLKDV(clkdv),
        .LOCKED(locked)
   );

   wire codec_clk, half_clk;
   BUFG BUFG_codec_clk(.I(clk0), .O(codec_clk));
   BUFG BUFG_half_clk(.I(clkdv), .O(half_clk));
   BUFGMUX BUFGMUX_radio_clk (.I0(codec_clk), .I1(half_clk), .S(mimo), .O(radio_clk));

   //make codec clock domain mimo mode signal
   reg mimo_r;
   always @(posedge codec_clk) mimo_r <= mimo;

   //assign baseband sample interfaces
   //all samples are registered on strobe
   wire rx_strobe, tx_strobe;
   wire [11:0] rx_i0, rx_q0, rx_i1, rx_q1;
   reg [11:0] tx_i0, tx_q0, tx_i1, tx_q1;
   //tx mux to feed single channel mode from either input
   wire [31:0] txm = (mimo_r || (tx1 != 32'b0))? tx1: tx2;
   always @(posedge codec_clk) begin
        if (rx_strobe) rx2 <= {rx_i1, 4'b0, rx_q1, 4'b0};
        if (rx_strobe) rx1 <= {rx_i0, 4'b0, rx_q0, 4'b0};
        if (tx_strobe) {tx_i0, tx_q0} <= {txm[31:20], txm[15:4]};
        if (tx_strobe) {tx_i1, tx_q1} <= {tx2[31:20], tx2[15:4]};
   end

   // CMOS Data interface to catalina, ignore _n pins
   catcap_ddr_cmos catcap
     (.data_clk(codec_clk), .reset(codec_rst), .mimo(mimo_r),
      .rx_frame(rx_frame), .rx_d(rx_d),
      .rx_clk(/*out*/), .rx_strobe(rx_strobe),
      .i0(rx_i0), .q0(rx_q0),
      .i1(rx_i1), .q1(rx_q1));

   catgen_ddr_cmos catgen
     (.data_clk(tx_clk), .reset(codec_rst), .mimo(mimo_r),
      .tx_frame(tx_frame), .tx_d(tx_d),
      .tx_clk(codec_clk), .tx_strobe(tx_strobe),
      .i0(tx_i0), .q0(tx_q0),
      .i1(tx_i1), .q1(tx_q1));

endmodule // catcodec_ddr_cmos
