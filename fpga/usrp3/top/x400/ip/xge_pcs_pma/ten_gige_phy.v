//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ten_gige_phy
//
// Description:
//
//   Wrapper for the Xilinx xxv_ethernet IP (10G/25G Ethernet Subsystem).
//


module ten_gige_phy (
  input wire areset,
  input wire dclk,
  output wire xgmii_clk,

  // Transceiver IO
  output wire txp,
  output wire txn,
  input  wire rxp,
  input  wire rxn,

  // XGMII Interface
  input  wire [63:0] xgmii_txd,
  input  wire [ 7:0] xgmii_txc,
  output wire [63:0] xgmii_rxd,
  output wire [ 7:0] xgmii_rxc,

  // GTYE4_COMMON
  input  wire qpll0_refclk,
  input  wire qpll0_clk,
  input  wire qpll0_lock,
  output wire qpll0_reset,
  input  wire qpll1_refclk,
  input  wire qpll1_clk,
  input  wire qpll1_lock,
  output wire qpll1_reset,

  output wire rxrecclkout,
  output wire [7:0] core_status,
  output reg        reset_done
);

  localparam XGMII_FREQ      = 125_000_000;  // xgmii_clk frequency in Hz
  localparam RX_RST_WAIT     = XGMII_FREQ/2; // Cycles to wait before resetting
  localparam RX_RST_DURATION = 100;          // Duration of reset in cycles
  localparam RX_RST_COUNT_W  = $clog2(RX_RST_WAIT);

  wire rx_serdes_reset;
  wire tx_reset;
  wire rx_reset;

  wire a_gt_reset_tx_done, gt_reset_tx_done;
  wire a_gt_reset_rx_done, gt_reset_rx_done;

  wire stat_rx_status_tmp;
  reg  stat_rx_status;

  reg [RX_RST_COUNT_W-1:0] rst_count;
  reg                      gt_rx_reset_in;

  //---------------------------------------------------------------------------
  // Xilinx 10G/25G IP High Speed Ethernet Subsystem Instance
  //---------------------------------------------------------------------------

  // All connections below follow the Xilinx IP example design, except that
  // rx_core_clk is driven by tx_mii_clk instead of rx_clk_out. This puts the
  // RX and TX interfaces on the same clock domain.

  // gtwiz_reset_qpll1reset_out is not connected to qpll1reset in the example
  // design. Instead, qpll1reset is connected to 0.
  assign qpll1_reset = 1'b0;

  xge_pcs_pma xge_pcs_pma_i (
    .gt_rxp_in_0                         (rxp),
    .gt_rxn_in_0                         (rxn),
    .gt_txp_out_0                        (txp),
    .gt_txn_out_0                        (txn),
    .rx_core_clk_0                       (xgmii_clk),
    .rx_serdes_reset_0                   (rx_serdes_reset),
    .txoutclksel_in_0                    (3'b101),
    .rxoutclksel_in_0                    (3'b101),
    .gt_dmonitorout_0                    (),
    .gt_eyescandataerror_0               (),
    .gt_eyescanreset_0                   (1'b0),
    .gt_eyescantrigger_0                 (1'b0),
    .gt_pcsrsvdin_0                      (16'b0),
    .gt_rxbufreset_0                     (1'b0),
    .gt_rxbufstatus_0                    (),
    .gt_rxcdrhold_0                      (1'b0),
    .gt_rxcommadeten_0                   (1'b0),
    .gt_rxdfeagchold_0                   (1'b0),
    .gt_rxdfelpmreset_0                  (1'b0),
    .gt_rxlatclk_0                       (1'b0),
    .gt_rxlpmen_0                        (1'b0),
    .gt_rxpcsreset_0                     (1'b0),
    .gt_rxpmareset_0                     (1'b0),
    .gt_rxpolarity_0                     (1'b0),
    .gt_rxprbscntreset_0                 (1'b0),
    .gt_rxprbserr_0                      (),
    .gt_rxprbssel_0                      (4'b0),
    .gt_rxrate_0                         (3'b0),
    .gt_rxslide_in_0                     (1'b0),
    .gt_rxstartofseq_0                   (),
    .gt_txbufstatus_0                    (),
    .gt_txdiffctrl_0                     (5'h18),
    .gt_txinhibit_0                      (1'b0),
    .gt_txlatclk_0                       (1'b0),
    .gt_txmaincursor_0                   (7'h50),
    .gt_txpcsreset_0                     (1'b0),
    .gt_txpmareset_0                     (1'b0),
    .gt_txpolarity_0                     (1'b0),
    .gt_txpostcursor_0                   (5'b0),
    .gt_txprbsforceerr_0                 (1'b0),
    .gt_txprbssel_0                      (4'b0),
    .gt_txprecursor_0                    (5'b0),
    .rxrecclkout_0                       (rxrecclkout),
    .gt_drpclk_0                         (dclk),
    .gt_drpdo_0                          (),
    .gt_drprdy_0                         (),
    .gt_drpen_0                          (1'b0),
    .gt_drpwe_0                          (1'b0),
    .gt_drpaddr_0                        (10'b0),
    .gt_drpdi_0                          (16'b0),
    .sys_reset                           (areset),
    .dclk                                (dclk),
    .tx_mii_clk_0                        (xgmii_clk),
    .rx_clk_out_0                        (),
    .gtpowergood_out_0                   (),
    .qpll0clk_in                         (qpll0_clk),
    .qpll0refclk_in                      (qpll0_refclk),
    .qpll1clk_in                         (qpll1_clk),
    .qpll1refclk_in                      (qpll1_refclk),
    .gtwiz_reset_qpll0lock_in            (qpll0_lock),
    .gtwiz_reset_qpll1lock_in            (qpll1_lock),
    .gtwiz_reset_qpll0reset_out          (qpll0_reset),
    .gtwiz_reset_qpll1reset_out          (),
    .gt_reset_tx_done_out_0              (a_gt_reset_tx_done),
    .gt_reset_rx_done_out_0              (a_gt_reset_rx_done),
    .gt_reset_all_in_0                   (areset),
    .gt_tx_reset_in_0                    (1'b0),
    .gt_rx_reset_in_0                    (gt_rx_reset_in),
    .rx_reset_0                          (rx_reset),
    .rx_mii_d_0                          (xgmii_rxd),
    .rx_mii_c_0                          (xgmii_rxc),
    .ctl_rx_test_pattern_0               (1'b0),
    .ctl_rx_test_pattern_enable_0        (1'b0),
    .ctl_rx_data_pattern_select_0        (1'b0),
    .ctl_rx_prbs31_test_pattern_enable_0 (1'b0),
    .stat_rx_framing_err_0               (),
    .stat_rx_framing_err_valid_0         (),
    .stat_rx_local_fault_0               (),
    .stat_rx_block_lock_0                (),
    .stat_rx_valid_ctrl_code_0           (),
    .stat_rx_status_0                    (stat_rx_status_tmp),  // rx_core_clk_0 domain
    .stat_rx_hi_ber_0                    (),
    .stat_rx_bad_code_0                  (),
    .stat_rx_bad_code_valid_0            (),
    .stat_rx_error_0                     (),
    .stat_rx_error_valid_0               (),
    .stat_rx_fifo_error_0                (),
    .tx_reset_0                          (tx_reset),
    .tx_mii_d_0                          (xgmii_txd),
    .tx_mii_c_0                          (xgmii_txc),
    .stat_tx_local_fault_0               (),
    .ctl_tx_test_pattern_0               (1'b0),
    .ctl_tx_test_pattern_enable_0        (1'b0),
    .ctl_tx_test_pattern_select_0        (1'b0),
    .ctl_tx_data_pattern_select_0        (1'b0),
    .ctl_tx_test_pattern_seed_a_0        (58'b0),
    .ctl_tx_test_pattern_seed_b_0        (58'b0),
    .ctl_tx_prbs31_test_pattern_enable_0 (1'b0),
    .gt_loopback_in_0                    (3'b0)
  );


  //---------------------------------------------------------------------------
  // Status
  //---------------------------------------------------------------------------

  assign core_status[7:1] = 0;                // Unused
  assign core_status[0]   = stat_rx_status;   // Link status

  // Safely combine the RX and TX reset done signals into a single glitch-free
  // signal.

  synchronizer sync_reset_tx_done (
    .clk (xgmii_clk),
    .rst (1'b0),
    .in  (a_gt_reset_tx_done),
    .out (gt_reset_tx_done)
  );

  synchronizer sync_reset_rx_done (
    .clk (xgmii_clk),
    .rst (1'b0),
    .in  (a_gt_reset_rx_done),
    .out (gt_reset_rx_done)
  );

  always @(posedge xgmii_clk) begin : ResetDoneProc
    reset_done <= gt_reset_tx_done & gt_reset_rx_done;
  end


  //---------------------------------------------------------------------------
  // Reset Logic
  //---------------------------------------------------------------------------

  // The reset synchronization below is taken from the example design, except
  // that rx_clk_out was replaced by tx_mii_clk (xgmii_clk).

  synchronizer sync_rx_serdes_reset (
    .clk (xgmii_clk),
    .rst (1'b0),
    .in  (~a_gt_reset_rx_done),
    .out (rx_serdes_reset)
  );

  synchronizer sync_tx_reset (
    .clk (xgmii_clk),
    .rst (1'b0),
    .in  (~a_gt_reset_tx_done),
    .out (tx_reset)
  );

  synchronizer sync_rx_reset (
    .clk (xgmii_clk),
    .rst (1'b0),
    .in  (~a_gt_reset_rx_done),
    .out (rx_reset)
  );

  // This state machine resets the RX GT part of the core periodically when
  // the link is down. This is necessary due to a bug in the Xilinx IP.
  always @(posedge xgmii_clk, posedge areset) begin
      if (areset) begin
        gt_rx_reset_in <= 0;
        rst_count      <= 0;
        stat_rx_status <= 0;
      end else begin
        stat_rx_status <= stat_rx_status_tmp;

        // Periodically reset until link is up
        if (!stat_rx_status) begin
          rst_count <= rst_count + 1;

          if (!gt_rx_reset_in) begin
            // We're not in reset. Wait until RX_RST_WAIT cycles have elapsed,
            // then reset.
            if (rst_count == RX_RST_WAIT-1) begin
              rst_count     <= 0;
              gt_rx_reset_in <= 1;
            end
          end else begin
            // We're in reset. Wait until RX_RST_DURATION cycles have elapsed
            // before deasserting reset.
            if (rst_count == RX_RST_DURATION-1) begin
              rst_count      <= 0;
              gt_rx_reset_in <= 0;
            end
          end

        // Currently linked, so all is well
        end else begin
          rst_count      <= 0;
          gt_rx_reset_in <= 0;
        end
      end
    end

endmodule
