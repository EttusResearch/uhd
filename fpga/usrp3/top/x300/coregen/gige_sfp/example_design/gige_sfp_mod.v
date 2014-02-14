//------------------------------------------------------------------------------
// File       : gige_sfp_mod.v
// Author     : Xilinx Inc.
//------------------------------------------------------------------------------
// (c) Copyright 2002-2008 Xilinx, Inc. All rights reserved.
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
// Description: This package holds the top level component declaration
// for the Ethernet 1000BASE-X PCS/PMA core.



module gige_sfp
   (
      // Core <=> tranceiver Interface
      //------------------------------

      output       mgt_rx_reset,          // tranceiver connection: reset for the receiver half of the tranceiver
      output       mgt_tx_reset,          // tranceiver connection: reset for the transmitter half of the tranceiver
      input        userclk,               // Routed to TXUSERCLK and RXUSERCLK of Transceiver.
      input        userclk2,              // Routed to TXUSERCLK2 and RXUSERCLK2 of Transceiver.
      input        dcm_locked,            // LOCKED signal from DCM.

      input [1:0]  rxbufstatus,           // tranceiver connection: Elastic Buffer Status.
      input        rxchariscomma,         // tranceiver connection: Comma detected in RXDATA.
      input        rxcharisk,             // tranceiver connection: K character received (or extra data bit) in RXDATA.
      input [2:0]  rxclkcorcnt,           // tranceiver connection: Indicates clock correction.
      input [7:0]  rxdata,                // tranceiver connection: Data after 8B/10B decoding.
      input        rxdisperr,             // tranceiver connection: Disparity-error in RXDATA.
      input        rxnotintable,          // tranceiver connection: Non-existent 8B/10 code indicated.
      input        rxrundisp,             // tranceiver connection: Running Disparity of RXDATA (or extra data bit).
      input        txbuferr,              // tranceiver connection: TX Buffer error (overflow or underflow).

      output       powerdown,             // tranceiver connection: Powerdown the tranceiver
      output       txchardispmode,        // tranceiver connection: Set running disparity for current byte.
      output       txchardispval,         // tranceiver connection: Set running disparity value.
      output       txcharisk,             // tranceiver connection: K character transmitted in TXDATA.
      output [7:0] txdata,                // tranceiver connection: Data for 8B/10B encoding.
      output       enablealign,           // Allow the transceivers to serially realign to a comma character.

      // GMII Interface (MAC <=> PCS)
      //-----------------------------

      input [7:0]  gmii_txd,              // Transmit data from client MAC.
      input        gmii_tx_en,            // Transmit control signal from client MAC.
      input        gmii_tx_er,            // Transmit control signal from client MAC.
      output [7:0] gmii_rxd,              // Received Data to client MAC.
      output       gmii_rx_dv,            // Received control signal to client MAC.
      output       gmii_rx_er,            // Received control signal to client MAC.
      output       gmii_isolate,          // Tristate control to electrically isolate GMII.

      // Alternative to MDIO Interface
      //------------------------------

      input [4:0]  configuration_vector,  // Alternative to MDIO interface.

      // General IO's
      //-------------
      output [15:0] status_vector,         // Core status.
      input        reset,                 // Asynchronous reset for entire core.
      input        signal_detect          // Input from PMD to indicate presence of optical input.
   );

endmodule // gige_sfp

