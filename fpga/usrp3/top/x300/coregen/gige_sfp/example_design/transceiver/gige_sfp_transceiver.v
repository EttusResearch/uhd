//------------------------------------------------------------------------------
// Title      : Top-level Transceiver GT wrapper for Ethernet
// Project    : Ethernet 1000BASE-X PCS/PMA or SGMII LogiCORE
// File       : gige_sfp_transceiver.v
// Author     : Xilinx
//------------------------------------------------------------------------------
// (c) Copyright 2009 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES. 
//
//
//------------------------------------------------------------------------------
// Description:  This is the top-level Transceiver GT wrapper. It
//               instantiates the lower-level wrappers produced by
//               the Series-7 FPGA Transceiver GT Wrapper Wizard.
//------------------------------------------------------------------------------

`timescale 1 ps / 1 ps

module gige_sfp_transceiver (
   input            encommaalign,
   input            loopback,
   input            powerdown,
   input            usrclk,
   input            usrclk2,
   input            data_valid,
   input            independent_clock,
   input            txreset,
   input      [7:0] txdata,
   input            txchardispmode,
   input            txchardispval,
   input            txcharisk,
   input            rxreset,
   output reg       rxchariscomma,
   output reg       rxcharisk,
   output reg [2:0] rxclkcorcnt,
   output reg [7:0] rxdata,
   output reg       rxdisperr,
   output reg       rxnotintable,
   output reg       rxrundisp,
   output reg       rxbuferr,
   output reg       txbuferr,
   output           plllkdet,
   output           txoutclk,
   output           rxelecidle,
   output           txn,
   output           txp,
   input            rxn,
   input            rxp,
   input            gtrefclk,
   input            pmareset,
   input            mmcm_locked,
   output           resetdone

);


  //----------------------------------------------------------------------------
  // Signal declarations
  //----------------------------------------------------------------------------

   wire             cplllock;
   wire             gt_reset_rx;
   wire             gt_reset_tx;
   wire             resetdone_tx;
   wire             resetdone_rx;
   wire             pcsreset;
   reg              data_valid_reg;
   wire             data_valid_reg2;

   wire      [2:0]  rxbufstatus;
   wire      [1:0]  txbufstatus;
   reg       [2:0]  rxbufstatus_reg;
   reg       [1:0]  txbufstatus_reg;
   wire      [1:0]  rxclkcorcnt_int;
   reg              txpowerdown_reg = 1'b0;
   reg              txpowerdown_double = 1'b0;
   reg              txpowerdown = 1'b0;
   wire      [1:0]  txpowerdown_int;

   // signal used to control sampling during bus width conversions
   reg              toggle;

   // signals reclocked onto the 62.5MHz userclk source of the GT transceiver
   wire             encommaalign_int;
   wire             txreset_int;
   wire             rxreset_int;

   // Register transmitter signals from the core
   reg        [7:0] txdata_reg;
   reg              txchardispmode_reg;
   reg              txchardispval_reg;
   reg              txcharisk_reg;

   // Signals for data bus width doubling on the transmitter path from the core
   // to the GT transceiver
   reg       [15:0] txdata_double;
   reg        [1:0] txchardispmode_double;
   reg        [1:0] txchardispval_double;
   reg        [1:0] txcharisk_double;

   // Double width signals reclocked onto the 62.5MHz userclk source of the GT
   // transceiver
   reg       [15:0] txdata_int;
   reg        [1:0] txchardispmode_int;
   reg        [1:0] txchardispval_int;
   reg        [1:0] txcharisk_int;

   // Double width signals output from the GT transceiver on the 62.5MHz clock
   // source
   wire       [1:0] rxchariscomma_int;
   wire       [1:0] rxcharisk_int;
   wire      [15:0] rxdata_int;
   wire       [1:0] rxdisperr_int;
   wire       [1:0] rxnotintable_int;
   wire       [1:0] rxrundisp_int;

   // Double width signals reclocked on the GT's 62.5MHz clock source
   reg        [1:0] rxchariscomma_reg;
   reg        [1:0] rxcharisk_reg;
   reg       [15:0] rxdata_reg;
   reg        [1:0] rxdisperr_reg;
   reg        [1:0] rxnotintable_reg;
   reg        [1:0] rxrundisp_reg;
   reg              rxpowerdown_reg = 1'b0;

   // Double width signals reclocked onto the 125MHz clock source
   reg        [1:0] rxchariscomma_double;
   reg        [1:0] rxcharisk_double;
   reg       [15:0] rxdata_double;
   reg        [1:0] rxdisperr_double;
   reg        [1:0] rxnotintable_double;
   reg        [1:0] rxrundisp_double;
   reg              rxpowerdown_double = 1'b0;

   reg              rxpowerdown = 1'b0;
   wire       [1:0] rxpowerdown_int;


   assign txpowerdown_int = {2{txpowerdown}};
   assign rxpowerdown_int = {2{rxpowerdown}};

   //---------------------------------------------------------------------------
   // The core works from a 125MHz clock source, the GT transceiver fabric
   // interface works from a 62.5MHz clock source.  The following signals
   // sourced by the core therefore need to be reclocked onto the 62.5MHz
   // clock
   //---------------------------------------------------------------------------

   // Reclock encommaalign
   gige_sfp_reset_sync reclock_encommaalign
   (
      .clk       (usrclk),
      .reset_in  (encommaalign),
      .reset_out (encommaalign_int)
   );


   // Reclock txreset
   gige_sfp_reset_sync reclock_txreset
   (
      .clk       (usrclk),
      .reset_in  (txreset),
      .reset_out (txreset_int)
   );


   // Reclock rxreset
   gige_sfp_reset_sync reclock_rxreset
   (
      .clk       (usrclk),
      .reset_in  (rxreset),
      .reset_out (rxreset_int)
   );


   //---------------------------------------------------------------------------
   // toggle signal used to control sampling during bus width conversions
   //---------------------------------------------------------------------------

   always @(posedge usrclk2)
   begin
     if (txreset) begin
       toggle <= 1'b0;
     end
     else begin
       toggle <= !toggle;
     end
   end


   //---------------------------------------------------------------------------
   // The core works from a 125MHz clock source, the GT transceiver fabric
   // interface works from a 62.5MHz clock source.  The following signals
   // sourced by the core therefore need to be converted to double width, then
   // resampled on the GT's 62.5MHz clock
   //---------------------------------------------------------------------------

   // Reclock the transmitter signals
   always @(posedge usrclk2)
   begin
     if (txreset) begin
       txdata_reg         <= 8'b0;
       txchardispmode_reg <= 1'b0;
       txchardispval_reg  <= 1'b0;
       txcharisk_reg      <= 1'b0;
       txpowerdown_reg    <= 1'b0;
     end
     else begin
       txdata_reg         <= txdata;
       txchardispmode_reg <= txchardispmode;
       txchardispval_reg  <= txchardispval;
       txcharisk_reg      <= txcharisk;
       txpowerdown_reg    <= powerdown;
     end
   end


   // Double the data width
   always @(posedge usrclk2)
   begin
     if (txreset) begin
       txdata_double                <= 16'b0;
       txchardispmode_double        <= 2'b0;
       txchardispval_double         <= 2'b0;
       txcharisk_double             <= 2'b0;
       txpowerdown_double           <= 1'b0;
     end
     else begin
       if (!toggle) begin
         txdata_double[7:0]         <= txdata_reg;
         txchardispmode_double[0]   <= txchardispmode_reg;
         txchardispval_double[0]    <= txchardispval_reg;
         txcharisk_double[0]        <= txcharisk_reg;
         txdata_double[15:8]        <= txdata;
         txchardispmode_double[1]   <= txchardispmode;
         txchardispval_double[1]    <= txchardispval;
         txcharisk_double[1]        <= txcharisk;
       end
       txpowerdown_double         <= txpowerdown_reg;
     end
   end


   // Cross the clock domain
   always @(posedge usrclk)
   begin
     txdata_int         <= txdata_double;
     txchardispmode_int <= txchardispmode_double;
     txchardispval_int  <= txchardispval_double;
     txcharisk_int      <= txcharisk_double;
     txbufstatus_reg    <= txbufstatus;
     txpowerdown        <= txpowerdown_double;
   end



   //---------------------------------------------------------------------------
   // The core works from a 125MHz clock source, the GT transceiver fabric
   // interface works from a 62.5MHz clock source.  The following signals
   // sourced by the GT transceiver therefore need to converted to half width
   //---------------------------------------------------------------------------

  // Sample the double width received data from the GT transsciever on the GT's
  // 62.5MHz clock
  always @(posedge usrclk)
  begin
    rxchariscomma_reg  <= rxchariscomma_int;
    rxcharisk_reg      <= rxcharisk_int;
    rxdata_reg         <= rxdata_int;
    rxdisperr_reg      <= rxdisperr_int;
    rxnotintable_reg   <= rxnotintable_int;
    rxrundisp_reg      <= rxrundisp_int;
    rxbufstatus_reg    <= rxbufstatus;
    rxpowerdown        <= rxpowerdown_reg;
  end


  // Reclock the double width received data from the GT transsciever onto the
  // 125MHz clock source.   Both clock domains are frequency related and are
  // derived from the same MMCM: the Xilinx tools will accont for this.

  always @(posedge usrclk2)
  begin
    if (rxreset) begin
      rxchariscomma_double  <= 2'b0;
      rxcharisk_double      <= 2'b0;
      rxdata_double         <= 16'b0;
      rxdisperr_double      <= 2'b0;
      rxnotintable_double   <= 2'b0;
      rxrundisp_double      <= 2'b0;
      rxpowerdown_double    <= 1'b0;
    end
    else if (toggle) begin
      rxchariscomma_double  <= rxchariscomma_reg;
      rxcharisk_double      <= rxcharisk_reg;
      rxdata_double         <= rxdata_reg;
      rxdisperr_double      <= rxdisperr_reg;
      rxnotintable_double   <= rxnotintable_reg;
      rxrundisp_double      <= rxrundisp_reg;
    end
    rxpowerdown_double      <= powerdown;
  end


  // Halve the bus width
  always @(posedge usrclk2)
  begin
    if (rxreset) begin
      rxchariscomma    <= 1'b0;
      rxcharisk        <= 1'b0;
      rxdata           <= 8'b0;
      rxdisperr        <= 1'b0;
      rxnotintable     <= 1'b0;
      rxrundisp        <= 1'b0;
      rxpowerdown_reg  <= 1'b0;
    end
    else begin
      if (!toggle) begin
        rxchariscomma  <= rxchariscomma_double[0];
        rxcharisk      <= rxcharisk_double[0];
        rxdata         <= rxdata_double[7:0];
        rxdisperr      <= rxdisperr_double[0];
        rxnotintable   <= rxnotintable_double[0];
        rxrundisp      <= rxrundisp_double[0];
      end
      else begin
        rxchariscomma  <= rxchariscomma_double[1];
        rxcharisk      <= rxcharisk_double[1];
        rxdata         <= rxdata_double[15:8];
        rxdisperr      <= rxdisperr_double[1];
        rxnotintable   <= rxnotintable_double[1];
        rxrundisp      <= rxrundisp_double[1];
      end
      rxpowerdown_reg  <= rxpowerdown_double;
    end
  end


   //---------------------------------------------------------------------------
   // Instantiate the Series-7 GTX
   //---------------------------------------------------------------------------
   // Direct from the Transceiver Wizard output
   gige_sfp_GTWIZARD_init #
   (
        .EXAMPLE_SIM_GTRESET_SPEEDUP     ("TRUE")
   )
   gtwizard_inst
   (
       .SYSCLK_IN (independent_clock),
       .SOFT_RESET_IN (pmareset),
       .GT0_TX_FSM_RESET_DONE_OUT (),
       .GT0_RX_FSM_RESET_DONE_OUT (),
       .GT0_DATA_VALID_IN (data_valid_reg2),
        //----------------------- Channel - Ref Clock Ports //------------------
        .GT0_GTREFCLK0_IN                (gtrefclk),
        //------------------------------ Channel PLL //-------------------------
        .GT0_CPLLFBCLKLOST_OUT           (),
        .GT0_CPLLLOCK_OUT                (cplllock),
        .GT0_CPLLLOCKDETCLK_IN           (1'b1),
        //.GT0_CPLLREFCLKLOST_OUT          (),
        .GT0_CPLLRESET_IN                (pmareset),
        //----------------------------- Eye Scan Ports //-----------------------
        .GT0_EYESCANDATAERROR_OUT        (),
        //---------------------- Loopback and Powerdown Ports //----------------
        .GT0_LOOPBACK_IN                 (3'b0),
        .GT0_RXPD_IN                     (rxpowerdown_int),
        .GT0_TXPD_IN                     (txpowerdown_int),
        //----------------------------- Receive Ports --------------------------
        .GT0_RXUSERRDY_IN                (mmcm_locked),
        //--------------------- Receive Ports - 8b10b Decoder //----------------
        .GT0_RXCHARISCOMMA_OUT           (rxchariscomma_int),
        .GT0_RXCHARISK_OUT               (rxcharisk_int),
        .GT0_RXDISPERR_OUT               (rxdisperr_int),
        .GT0_RXNOTINTABLE_OUT            (rxnotintable_int),
        //----------------- Receive Ports - Clock Correction Ports //-----------
        .GT0_RXCLKCORCNT_OUT             (rxclkcorcnt_int),
        //------------- Receive Ports - Comma Detection and Alignment //--------
        .GT0_RXMCOMMAALIGNEN_IN          (encommaalign_int),
        .GT0_RXPCOMMAALIGNEN_IN          (encommaalign_int),
        //----------------- Receive Ports - RX Data Path interface //-----------
        .GT0_GTRXRESET_IN                (gt_reset_rx),
//        .GT0_GTRXRESET_IN                (rxreset_int),
        .GT0_RXDATA_OUT                  (rxdata_int),
        .GT0_RXOUTCLK_OUT                (),
        //.GT0_RXPCSRESET_IN               (pcsreset),
        .GT0_RXUSRCLK_IN                 (usrclk),
        .GT0_RXUSRCLK2_IN                (usrclk),
        //----- Receive Ports - RX Driver),OOB signalling),Coupling and Eq.),CDR //
        .GT0_GTXRXN_IN                   (rxn),
        .GT0_GTXRXP_IN                   (rxp),
        .GT0_RXCDRLOCK_OUT               (),
        .GT0_RXELECIDLE_OUT              (rxelecidle),
        //------ Receive Ports - RX Elastic Buffer and Phase Alignment Ports //-
        .GT0_RXBUFRESET_IN               (rxreset_int),
        .GT0_RXBUFSTATUS_OUT             (rxbufstatus),
        //---------------------- Receive Ports - RX PLL Ports //----------------
        .GT0_RXRESETDONE_OUT             (resetdone_rx),
        //----------------------------- Transmit Ports -------------------------
        .GT0_TXUSERRDY_IN                (mmcm_locked),
        //-------------- Transmit Ports - 8b10b Encoder Control Ports //--------
        .GT0_TXCHARDISPMODE_IN           (txchardispmode_int),
        .GT0_TXCHARDISPVAL_IN            (txchardispval_int),
        .GT0_TXCHARISK_IN                (txcharisk_int),
        //---------------- Transmit Ports - TX Data Path interface //-----------
        .GT0_GTTXRESET_IN                (gt_reset_tx),
//        .GT0_GTTXRESET_IN                (txreset_int),
        .GT0_TXDATA_IN                   (txdata_int),
        .GT0_TXOUTCLK_OUT                (txoutclk),
        .GT0_TXOUTCLKFABRIC_OUT          (),
        .GT0_TXOUTCLKPCS_OUT             (),
        //.GT0_TXPCSRESET_IN               (pcsreset),
        .GT0_TXUSRCLK_IN                 (usrclk),
        .GT0_TXUSRCLK2_IN                (usrclk),
        //-------------- Transmit Ports - TX Driver and OOB signaling //--------
        .GT0_GTXTXN_OUT                  (txn),
        .GT0_GTXTXP_OUT                  (txp),
        //--------- Transmit Ports - TX Elastic Buffer and Phase Alignment //---
        .GT0_TXBUFSTATUS_OUT             (txbufstatus),
        //--------------------- Transmit Ports - TX PLL Ports //----------------
        .GT0_TXRESETDONE_OUT             (resetdone_tx)
        //----------- Transmit Ports - TX Ports for PCI Express ----------------
        //.GT0_TXELECIDLE_IN               (txpowerdown)
   );


   // Hold the transmitter and receiver paths of the GT transceiver in reset
   // until the PLL has locked.
   assign gt_reset_rx = !cplllock || (rxreset_int & resetdone_rx);
   assign gt_reset_tx = !cplllock || (txreset_int & resetdone_tx);


   // Output the PLL locked status
   assign plllkdet = cplllock;


   // Report overall status for both transmitter and receiver reset done signals
   assign resetdone = cplllock ;


   // reset to PCS part of GT
   assign pcsreset = !mmcm_locked;

   // temporary
   assign rxrundisp_int = 2'b0;


   // Decode the GT transceiver buffer status signals
  always @(posedge usrclk2)
  begin
    rxbuferr    <= rxbufstatus_reg[2];
    txbuferr    <= txbufstatus_reg[1];
    rxclkcorcnt <= {1'b0, rxclkcorcnt_int};
  end

   //---------------------------------------------------------------------------
   // The core works from a 125MHz clock source userclk2, the init statemachines 
   // work at 200 MHz. 
   //---------------------------------------------------------------------------

   // Cross the clock domain
   always @(posedge usrclk2)
   begin
     data_valid_reg    <= data_valid;
   end


   gige_sfp_sync_block sync_block_data_valid
          (
             .clk             (independent_clock),
             .data_in         (data_valid_reg),
             .data_out        (data_valid_reg2)
          );



endmodule
