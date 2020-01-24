//
// Copyright 2014 Ettus Research LLC
//

module one_gige_phy 
(
   input            independent_clock,

   // Tranceiver Interface
   //---------------------
   input            gtrefclk,              // Reference clock for MGT: 125MHz, very high quality.
   input            gtrefclk_bufg,         // Reference clock routed through a BUFG
   output           txp,                   // Differential +ve of serial transmission from PMA to PMD.
   output           txn,                   // Differential -ve of serial transmission from PMA to PMD.
   input            rxp,                   // Differential +ve for serial reception from PMD to PMA.
   input            rxn,                   // Differential -ve for serial reception from PMD to PMA.

   // GMII Interface (client MAC <=> PCS)
   //------------------------------------
   output           gmii_clk,              // Receive clock to client MAC.
   input [7:0]      gmii_txd,              // Transmit data from client MAC.
   input            gmii_tx_en,            // Transmit control signal from client MAC.
   input            gmii_tx_er,            // Transmit control signal from client MAC.
   output reg [7:0] gmii_rxd,              // Received Data to client MAC.
   output reg       gmii_rx_dv,            // Received control signal to client MAC.
   output reg       gmii_rx_er,            // Received control signal to client MAC.

   // Management: MDIO Interface
   //---------------------------
   input            mdc,                   // Management Data Clock
   input            mdio_i,                // Management Data In
   output           mdio_o,                // Management Data Out
   output           mdio_t,                // Management Data Tristate
   input [4:0]      configuration_vector,  // Alternative to MDIO interface.
   input            configuration_valid,   // Validation signal for Config vector

   // General IO's
   //-------------
   output [15:0]    status_vector,         // Core status.
   input            reset,                 // Asynchronous reset for entire core.
   input            signal_detect          // Input from PMD to indicate presence of optical input.
);

   wire         resetdone;                // To indicate that the GT transceiver has completed its reset cycle
   wire         userclk;                  // 62.5MHz clock for GT transceiver Tx/Rx user clocks
   wire         userclk2;                 // 125MHz clock for core reference clock.
   wire         rxuserclk2;
   wire         gmii_isolate;             // internal gmii_isolate signal.

   wire [7:0]   gmii_rxd_int;
   wire         gmii_rx_dv_int;
   wire         gmii_rx_er_int;
   
   always @(posedge gmii_clk) begin
      gmii_rxd    <= gmii_rxd_int;
      gmii_rx_dv  <= gmii_rx_dv_int;
      gmii_rx_er  <= gmii_rx_er_int;
   end

   //----------------------------------------------------------------------------
   // Instantiate core wrapper
   //----------------------------------------------------------------------------
   one_gig_eth_pcs_pma_support core_support_i (
      .gtrefclk              (gtrefclk),
      .gtrefclk_bufg         (gtrefclk_bufg),
      .txp                   (txp),
      .txn                   (txn),
      .rxp                   (rxp),
      .rxn                   (rxn),
      .mmcm_locked_out       (),
      .userclk_out           (userclk),
      .userclk2_out          (userclk2),
      .rxuserclk_out         (),
      .rxuserclk2_out        (rxuserclk2),
      .independent_clock_bufg(independent_clock),
      .pma_reset_out         (),
      .resetdone             (resetdone),
      .gmii_txd              (gmii_txd),
      .gmii_tx_en            (gmii_tx_en),
      .gmii_tx_er            (gmii_tx_er),
      .gmii_rxd              (gmii_rxd_int),
      .gmii_rx_dv            (gmii_rx_dv_int),
      .gmii_rx_er            (gmii_rx_er_int),
      .gmii_isolate          (gmii_isolate),
      .mdc                   (mdc),
      .mdio_i                (mdio_i),
      .mdio_o                (mdio_o),
      .mdio_t                (mdio_t),
      .configuration_vector  (configuration_vector),
      .configuration_valid   (configuration_valid),
      .status_vector         (status_vector),
      .reset                 (reset),
      .signal_detect         (signal_detect),
      .gt0_qplloutclk_out    (),
      .gt0_qplloutrefclk_out ()
   );
   
   assign gmii_clk = userclk2;

endmodule // one_gige_phy
