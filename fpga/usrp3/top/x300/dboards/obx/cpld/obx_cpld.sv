//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-late
//
// Module: obx_cpld
//
// Description:
//
//   Top level file for the OBX CPLD. This file includes:
//     - SPI pass-through for the LOs
//     - Register decoding for front-end control.
//

`default_nettype none

module obx_cpld(

  //MCTRL Inputs
  input wire         aMCTRL_RESET_N,
  input wire         mMCTRL_SEN, mMCTRL_SDI, MCTRL_SCLK,
  input wire         aMCTRL_ATR_RX_N, aMCTRL_ATR_TX_N,
  input wire         aMCTRL_RX2_EN,
  input wire [2:0]   aMCTRL_ADDR,
  //MCTRL Outputs
  output wire        mMCTRL_SDO,

  //LO Config Inputs
  input wire         aTXLO1_LD, aTXLO2_LD, aRXLO1_LD, aRXLO2_LD,
  //LO Config Outpus
  output wire        aLED_LOCKD, aLED_TXD, aLED_RXD1, aLED_RXD2,
  output wire        aLOCKD_RX, aLOCKD_TX,
  output wire        mTXLO1_DATA, mTXLO1_DCLK, mTXLO1_DLE, aTXLO1_NPDRF,
  output wire        mTXLO2_DATA, mTXLO2_DCLK, mTXLO2_DLE, aTXLO2_NPDRF,
  output wire        mRXLO1_DATA, mRXLO1_DCLK, mRXLO1_DLE, aRXLO1_NPDRF,
  output wire        mRXLO2_DATA, mRXLO2_DCLK, mRXLO2_DLE, aRXLO2_NPDRF,
  output wire        aTXLO1_FSEL1, aTXLO1_FSEL2, aTXLO1_FSEL3, aTXLO1_FSEL4,
  output wire        aRXLO1_FSEL1, aRXLO1_FSEL2, aRXLO1_FSEL3, aRXLO1_FSEL4,

  //RF Config Outputs
  output wire        aTXLB_SEL, aTXLB_SEL2, aTXHB_SEL, aTXHB_SEL2,
  output wire        aRXLB_SEL, aRXLB_SEL2, aRXHB_SEL, aRXHB_SEL2,
  output wire        aPWREN_TXDRV, aPWREN_TXMOD_N, aPWREN_TXLBMIX,
  output wire        aPWREN_RXDEMOD_N, aPWREN_RXLBMIX, aPWREN_RXAMP,
  output wire        aPWREN_LNA1, aPWREN_LNA2,
  //RX DRV?

  output wire        aFE_SEL_CAL_RX2,
  output wire        aFE_SEL_CAL_TX2,
  output wire        aPWEN_RX_DOUBLER,
  output wire        aPWEN_TX_DOUBLER,

  //FE Config Outputs
  output wire        aFE_SEL_RX_LNA1,
  output wire        aFE_SEL_TRX_RX2,
  output wire        aFE_SEL_TRX_TX
);

  `include "regmap/obx_cpld_regs_utils.vh"

  // Input decoding
  logic   [2:0] ss_addr;
  logic         atr_rx_active;
  logic         atr_tx_active;
  logic         atr_rx2;

  // Asynchronous controls from FPGA (ATR and SPI endpoint selection)
  always_comb begin
    ss_addr        = aMCTRL_ADDR[2:0];
    atr_rx_active  = ~aMCTRL_ATR_RX_N;
    atr_tx_active  = ~aMCTRL_ATR_TX_N;
    atr_rx2        = aMCTRL_RX2_EN;
  end

  //--------------------------
  // LO SPI Pass-throughs
  //--------------------------
  localparam logic [2:0] SPI_DISABLED = {1'b0, 1'b0, 1'b1};
  logic internal_spi_access;

  // SPI pass-throughs for the LOs. They forward the MCTRL SPI signals to the
  // appropriate LO SPI interface based on the slave select address. Note that
  // trasactions targeted to the CPLD SPI address are not passed through,
  // and have their spi slave address verified within the CPLD.
  assign {mTXLO1_DATA, mTXLO1_DCLK, mTXLO1_DLE} =
    (ss_addr == TX_LO1_PASSTHROUGH) ? {mMCTRL_SDI, MCTRL_SCLK, mMCTRL_SEN} : SPI_DISABLED;
  assign {mTXLO2_DATA, mTXLO2_DCLK, mTXLO2_DLE} =
    (ss_addr == TX_LO2_PASSTHROUGH) ? {mMCTRL_SDI, MCTRL_SCLK, mMCTRL_SEN} : SPI_DISABLED;
  assign {mRXLO1_DATA, mRXLO1_DCLK, mRXLO1_DLE} =
    (ss_addr == RX_LO1_PASSTHROUGH) ? {mMCTRL_SDI, MCTRL_SCLK, mMCTRL_SEN} : SPI_DISABLED;
  assign {mRXLO2_DATA, mRXLO2_DCLK, mRXLO2_DLE} =
    (ss_addr == RX_LO2_PASSTHROUGH) ? {mMCTRL_SDI, MCTRL_SCLK, mMCTRL_SEN} : SPI_DISABLED;
  assign internal_spi_access = (ss_addr == CPLD_INTERNAL_SPI);
  //--------------------------
  //CPLD SPI Access
  //--------------------------

  // Control vectors from register endpoints
  logic [23:0] tx_fe_ctrl;
  logic [23:0] rx_fe_ctrl;

  obx_register_endpoints obx_reg_endpoints (
    .reset_n(aMCTRL_RESET_N),
    .spi_cs(mMCTRL_SEN),
    .spi_sdi(mMCTRL_SDI),
    .spi_clk(MCTRL_SCLK),
    .spi_sdo(mMCTRL_SDO),
    .internal_spi_access(internal_spi_access),
    .tx_fe_ctrl(tx_fe_ctrl),
    .rx_fe_ctrl(rx_fe_ctrl)
  );

  // Front-end control intermediate signals
  logic  sel_lna1, sel_lna2;
  logic  rx_doubler_force_on, rx_lna2_force_on, rx_lna1_force_on, rx_amp_force_on,
         rx_mixer_force_on, rx_demod_force_on, rx_lo2_force_on,
         rx_lo1_force_on;
  logic  tx_doubler_force_on, tx_drv_force_on, tx_mixer_force_on, tx_mod_force_on,
         tx_lo2_force_on, tx_lo1_force_on;

  //-----------------------------------------
  // Front-end control register breakout
  //-----------------------------------------
  // TX Control
  // --------------------
  always_comb begin
    // Intermediate signal assigments.
    tx_doubler_force_on = tx_fe_ctrl[TX_CTRL_TXDOUBLER_FORCEON];
    tx_drv_force_on     = tx_fe_ctrl[TX_CTRL_TXDRV_FORCEON];
    tx_mixer_force_on   = tx_fe_ctrl[TX_CTRL_TXMIXER_FORCEON];
    tx_mod_force_on     = tx_fe_ctrl[TX_CTRL_TXMOD_FORCEON];
    tx_lo2_force_on     = tx_fe_ctrl[TX_CTRL_TXLO2_FORCEON];
    tx_lo1_force_on     = tx_fe_ctrl[TX_CTRL_TXLO1_FORCEON];
  end

  // Direct to output assignments
  assign aFE_SEL_CAL_TX2   = tx_fe_ctrl[TX_CTRL_FE_SEL_CAL_TX2];
  assign aTXLO1_FSEL4      = tx_fe_ctrl[TX_CTRL_TXLO1_FSEL4];
  assign aTXLO1_FSEL3      = tx_fe_ctrl[TX_CTRL_TXLO1_FSEL3];
  assign aTXLO1_FSEL2      = tx_fe_ctrl[TX_CTRL_TXLO1_FSEL2];
  assign aTXLO1_FSEL1      = tx_fe_ctrl[TX_CTRL_TXLO1_FSEL1];
  assign aTXLB_SEL         = tx_fe_ctrl[TX_CTRL_TXLB_SEL];
  assign aTXLB_SEL2        = tx_fe_ctrl[TX_CTRL_TXLB_SEL2];
  assign aTXHB_SEL         = tx_fe_ctrl[TX_CTRL_TXHB_SEL];
  assign aTXHB_SEL2        = tx_fe_ctrl[TX_CTRL_TXHB_SEL2];

  // RX Control
  // --------------------
  always_comb begin
    // Intermediate signal assignments.
    rx_doubler_force_on = rx_fe_ctrl[RX_CTRL_RXDOUBLER_FORCEON];
    rx_lna2_force_on    = rx_fe_ctrl[RX_CTRL_RXLNA2_FORCEON];
    rx_lna1_force_on    = rx_fe_ctrl[RX_CTRL_RXLNA1_FORCEON];
    rx_amp_force_on     = rx_fe_ctrl[RX_CTRL_RXAMP_FORCEON];
    rx_mixer_force_on   = rx_fe_ctrl[RX_CTRL_RXMIXER_FORCEON];
    rx_demod_force_on   = rx_fe_ctrl[RX_CTRL_RXDEMOD_FORCEON];
    rx_lo2_force_on     = rx_fe_ctrl[RX_CTRL_RXLO2_FORCEON];
    rx_lo1_force_on     = rx_fe_ctrl[RX_CTRL_RXLO1_FORCEON];
    sel_lna2            = rx_fe_ctrl[RX_CTRL_SEL_LNA2];
    sel_lna1            = rx_fe_ctrl[RX_CTRL_SEL_LNA1];
  end

  // Direct to output assignments
  assign aFE_SEL_CAL_RX2   = rx_fe_ctrl[RX_CTRL_FE_SEL_CAL_RX2];
  assign aRXLO1_FSEL4      = rx_fe_ctrl[RX_CTRL_RXLO1_FSEL4];
  assign aRXLO1_FSEL3      = rx_fe_ctrl[RX_CTRL_RXLO1_FSEL3];
  assign aRXLO1_FSEL2      = rx_fe_ctrl[RX_CTRL_RXLO1_FSEL2];
  assign aRXLO1_FSEL1      = rx_fe_ctrl[RX_CTRL_RXLO1_FSEL1];
  assign aRXLB_SEL         = rx_fe_ctrl[RX_CTRL_RXLB_SEL];
  assign aRXLB_SEL2        = rx_fe_ctrl[RX_CTRL_RXLB_SEL2];
  assign aRXHB_SEL         = rx_fe_ctrl[RX_CTRL_RXHB_SEL];
  assign aRXHB_SEL2        = rx_fe_ctrl[RX_CTRL_RXHB_SEL2];

  //-----------------------------------------
  // Output control
  //-----------------------------------------
  // The Analog Front-End (AFE) control signals are generated based on the
  // active TX and RX paths, as well as the control register settings.

  // TX Side Outputs
  //--------------------
  assign aLED_TXD = atr_tx_active;

  // LO Power Enable Controls
  assign aTXLO1_NPDRF =  atr_tx_active | tx_lo1_force_on;
  // Enable LO2 only when the low-band path is selected
  assign aTXLO2_NPDRF = (atr_tx_active | tx_lo2_force_on) & aTXLB_SEL & (~aTXHB_SEL);

  assign aPWREN_TXMOD_N = ~(atr_tx_active | tx_mod_force_on);
  assign aPWREN_TXLBMIX = (atr_tx_active | tx_mixer_force_on) & aTXLB_SEL & (~aTXHB_SEL);

  assign aPWREN_TXDRV     = atr_tx_active | tx_drv_force_on;
  assign aPWEN_TX_DOUBLER = atr_tx_active | tx_doubler_force_on;
  // Asynchronous output connected to FPGA
  assign aLOCKD_TX  = (aTXLB_SEL && (!aTXHB_SEL)) ? (aTXLO1_LD & aTXLO2_LD) : aTXLO1_LD;

  // RX Side Outputs
  //--------------------
  assign aLED_RXD2 = atr_rx_active & (~atr_tx_active) & (~atr_rx2);
  assign aLED_RXD1 = atr_rx_active & atr_rx2;

  assign aRXLO1_NPDRF = atr_rx_active | rx_lo1_force_on;
  assign aRXLO2_NPDRF = atr_rx_active | rx_lo2_force_on;

  assign aPWREN_RXDEMOD_N = ~(atr_rx_active | rx_demod_force_on);
  // Enable RXLBMIX only when the low-band path is selected
  assign aPWREN_RXLBMIX   = (atr_rx_active | rx_mixer_force_on) &
                            ~aRXLB_SEL & aRXHB_SEL & aRXLB_SEL2 & ~aRXHB_SEL2;

  assign aPWREN_RXAMP     = atr_rx_active | rx_amp_force_on;
  assign aPWEN_RX_DOUBLER = atr_rx_active | rx_doubler_force_on;
  // Asynchronous output connected to FPGA
  assign aLOCKD_RX  = (aRXLB_SEL && (!aRXHB_SEL)) ? (aRXLO1_LD & aRXLO2_LD) : aRXLO1_LD;

  // LNA path selection
  // This LNA path selector should be high when LNA1 is powered on.
  assign aFE_SEL_RX_LNA1 = sel_lna1 & (~sel_lna2);
  // Power enable controls. NOTE: The power enable lines are flipped i.r.t. the
  // LNA selection lines.
  assign aPWREN_LNA1 = (atr_rx_active | rx_lna1_force_on) & (~sel_lna1) & sel_lna2;
  assign aPWREN_LNA2 = (atr_rx_active | rx_lna2_force_on) & (~sel_lna2) & sel_lna1;

  // Front-end Outputs
  assign aFE_SEL_TRX_TX  = ~atr_tx_active & atr_rx2;
  assign aFE_SEL_TRX_RX2 = atr_rx2;

  // Lock detect LED
  assign aLED_LOCKD = (atr_rx_active) ? aLOCKD_RX : aLOCKD_TX;

endmodule

`default_nettype wire
