
module gige_phy_mdio
  (input reset, 
   input independent_clock,
   input sfp_clk,
   input SFP_RX_p, 
   input SFP_RX_n,
   output SFP_TX_p, 
   output SFP_TX_n,
   output gmii_clk,
   input  [7:0] gmii_txd, 
   input gmii_tx_en, 
   input gmii_tx_er,
   output [7:0] gmii_rxd, 
   output gmii_rx_dv, 
   output gmii_rx_er,
   output [31:0] misc_debug,
   output [15:0] int_data,
   output [15:0] status_vector,
   // MDIO signals
   input [4:0] prtad,
   input mdc,
   input mdio_i,
   output mdio_o,
   output mdio_t
   );


   wire   mmcm_locked, mmcm_reset, resetdone, clkfbout;
   wire   userclk, userclk2;
   wire   txoutclk, txoutclk_bufg;
   
   assign gmii_clk = userclk2; // 125 MHz
   
   // Route txoutclk input through a BUFG
   // FIXME is this really necessary?  It seems wasteful.
   BUFG  bufg_txoutclk (.I (txoutclk), .O (txoutclk_bufg));

   // This 62.5MHz clock is placed onto global clock routing and is then used
   // for tranceiver TXUSRCLK/RXUSRCLK.
   BUFG bufg_userclk (.I (clkout1), .O (userclk));

   // This 125MHz clock is placed onto global clock routing and is then used
   // to clock all Ethernet core logic.
   BUFG bufg_userclk2 (.I (clkout0), .O (userclk2));

   // The GT transceiver provides a 62.5MHz clock to the FPGA fabric.  This is 
   // routed to an MMCM module where it is used to create phase and frequency
   // related 62.5MHz and 125MHz clock sources
   MMCME2_ADV # 
     (.BANDWIDTH            ("OPTIMIZED"),
      .CLKOUT4_CASCADE      ("FALSE"),
      .COMPENSATION         ("ZHOLD"),
      .STARTUP_WAIT         ("FALSE"),
      .DIVCLK_DIVIDE        (1),
      .CLKFBOUT_MULT_F      (16.000),
      .CLKFBOUT_PHASE       (0.000),
      .CLKFBOUT_USE_FINE_PS ("FALSE"),
      .CLKOUT0_DIVIDE_F     (8.000),
      .CLKOUT0_PHASE        (0.000),
      .CLKOUT0_DUTY_CYCLE   (0.5),
      .CLKOUT0_USE_FINE_PS  ("FALSE"),
      .CLKOUT1_DIVIDE       (16),
      .CLKOUT1_PHASE        (0.000),
      .CLKOUT1_DUTY_CYCLE   (0.5),
      .CLKOUT1_USE_FINE_PS  ("FALSE"),
      .CLKIN1_PERIOD        (16.0),
      .REF_JITTER1          (0.010)
      ) mmcm_adv_inst 
       (// Output clocks
	.CLKFBOUT             (clkfbout),
	.CLKFBOUTB            (),
	.CLKOUT0              (clkout0),
	.CLKOUT0B             (),
	.CLKOUT1              (clkout1),
	.CLKOUT1B             (),
	.CLKOUT2              (),
	.CLKOUT2B             (),
	.CLKOUT3              (),
	.CLKOUT3B             (),
	.CLKOUT4              (),
	.CLKOUT5              (),
	.CLKOUT6              (),
	// Input clock control
	.CLKFBIN              (clkfbout),
	.CLKIN1               (txoutclk_bufg),
	.CLKIN2               (1'b0),
	// Tied to always select the primary input clock
	.CLKINSEL             (1'b1),
	// Ports for dynamic reconfiguration
	.DADDR                (7'h0),
	.DCLK                 (1'b0),
	.DEN                  (1'b0),
	.DI                   (16'h0),
	.DO                   (),
	.DRDY                 (),
	.DWE                  (1'b0),
	// Ports for dynamic phase shift
	.PSCLK                (1'b0),
	.PSEN                 (1'b0),
	.PSINCDEC             (1'b0),
	.PSDONE               (),
	// Other control and status signals
	.LOCKED               (mmcm_locked),
	.CLKINSTOPPED         (),
	.CLKFBSTOPPED         (),
	.PWRDWN               (1'b0),
	.RST                  (mmcm_reset)
	);

   assign mmcm_reset = reset | ~resetdone;


   gige_sfp_mdio_block gige_sfp_mdio_block 
     (
      .gtrefclk              (sfp_clk),
      .txp                   (SFP_TX_p),
      .txn                   (SFP_TX_n),
      .rxp                   (SFP_RX_p),
      .rxn                   (SFP_RX_n),
      .txoutclk              (txoutclk),
      .resetdone             (resetdone),
      .mmcm_locked           (mmcm_locked),
      .userclk               (userclk),
      .userclk2              (userclk2),
      .independent_clock_bufg(independent_clock),
      .pma_reset             (reset),
      .gmii_txd              (gmii_txd),
      .gmii_tx_en            (gmii_tx_en),
      .gmii_tx_er            (gmii_tx_er),
      .gmii_rxd              (gmii_rxd),
      .gmii_rx_dv            (gmii_rx_dv),
      .gmii_rx_er            (gmii_rx_er),
      .gmii_isolate          (), // Unused
      .mdc                   (mdc),
      .mdio_i                (mdio_i),
      .mdio_o                (mdio_o),
      .mdio_t                (mdio_t),
      .phyad                 (prtad),
      .configuration_vector  (5'b00000),
      .configuration_valid   (1'b1), //default
      .status_vector         (status_vector),
      .reset                 (reset),
      .signal_detect         (1'b1)
      );


endmodule // gige_phy
