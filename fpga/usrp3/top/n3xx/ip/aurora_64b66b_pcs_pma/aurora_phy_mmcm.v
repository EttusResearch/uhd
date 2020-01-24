//
// Copyright 2017 Ettus Research LLC
//

module aurora_phy_mmcm 
(
   input  aurora_tx_clk_unbuf,
   input  mmcm_reset,
   output user_clk,
   output sync_clk,
   output mmcm_locked
);

  wire mmcm_fb_clk;
  wire user_clk_i;
  wire sync_clk_i;
  wire aurora_tx_clk;

  BUFG txout_clock_net_i (
    .I(aurora_tx_clk_unbuf),
    .O(aurora_tx_clk)
  );

  localparam MULT        = 10;
  localparam DIVIDE      = 5;
  localparam CLK_PERIOD  = 3.103;
  localparam OUT0_DIVIDE = 4;
  localparam OUT1_DIVIDE = 2;
  localparam OUT2_DIVIDE = 6;
  localparam OUT3_DIVIDE = 8;

  MMCME2_ADV #(
    .BANDWIDTH            ("OPTIMIZED"),
    .CLKOUT4_CASCADE      ("FALSE"),
    .COMPENSATION         ("ZHOLD"),
    .STARTUP_WAIT         ("FALSE"),
    .DIVCLK_DIVIDE        (DIVIDE),
    .CLKFBOUT_MULT_F      (MULT),
    .CLKFBOUT_PHASE       (0.000),
    .CLKFBOUT_USE_FINE_PS ("FALSE"),
    .CLKOUT0_DIVIDE_F     (OUT0_DIVIDE),
    .CLKOUT0_PHASE        (0.000),
    .CLKOUT0_DUTY_CYCLE   (0.500),
    .CLKOUT0_USE_FINE_PS  ("FALSE"),
    .CLKIN1_PERIOD        (CLK_PERIOD),
    .CLKOUT1_DIVIDE       (OUT1_DIVIDE),
    .CLKOUT1_PHASE        (0.000),
    .CLKOUT1_DUTY_CYCLE   (0.500),
    .CLKOUT1_USE_FINE_PS  ("FALSE"),
    .CLKOUT2_DIVIDE       (OUT2_DIVIDE),
    .CLKOUT2_PHASE        (0.000),
    .CLKOUT2_DUTY_CYCLE   (0.500),
    .CLKOUT2_USE_FINE_PS  ("FALSE"),
    .CLKOUT3_DIVIDE       (OUT3_DIVIDE),
    .CLKOUT3_PHASE        (0.000),
    .CLKOUT3_DUTY_CYCLE   (0.500),
    .CLKOUT3_USE_FINE_PS  ("FALSE"),
    .REF_JITTER1          (0.010)
  ) mmcm_adv_inst (
    .CLKFBOUT            (mmcm_fb_clk),
    .CLKFBOUTB           (),
    .CLKOUT0             (user_clk_i),
    .CLKOUT0B            (),
    .CLKOUT1             (sync_clk_i),
    .CLKOUT1B            (),
    .CLKOUT2             (),
    .CLKOUT2B            (),
    .CLKOUT3             (),
    .CLKOUT3B            (),
    .CLKOUT4             (),
    .CLKOUT5             (),
    .CLKOUT6             (),
     // Input clock control
    .CLKFBIN             (mmcm_fb_clk),
    .CLKIN1              (aurora_tx_clk),
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
    .LOCKED              (mmcm_locked),
    .CLKINSTOPPED        (),
    .CLKFBSTOPPED        (),
    .PWRDWN              (1'b0),
    .RST                 (mmcm_reset)
  );

  BUFG user_clk_net_i (
     .I(user_clk_i),
     .O(user_clk)
  );
  BUFG sync_clock_net_i (
     .I(sync_clk_i),
     .O(sync_clk)
  );
endmodule
