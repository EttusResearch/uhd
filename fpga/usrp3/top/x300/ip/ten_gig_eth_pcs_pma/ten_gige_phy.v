//
// Copyright 2008-2013 Ettus Research LLC
//

module ten_gige_phy
(
   input             refclk,
   input             clk156,
   input             dclk,
   input             areset,
   input             sim_speedup_control,
   input      [63:0] xgmii_txd,
   input      [7:0]  xgmii_txc,
   output reg [63:0] xgmii_rxd,
   output reg [7:0]  xgmii_rxc,
   output            txp,
   output            txn,
   input             rxp,
   input             rxn,
   input             mdc,
   input             mdio_in,
   output reg        mdio_out,
   output reg        mdio_tri,
   input      [4:0]  prtad,
   output     [7:0]  core_status,
   output            resetdone,
   input             signal_detect,
   input             tx_fault,
   output            tx_disable
);

   reg [63:0]  xgmii_txd_reg;
   reg [7:0]   xgmii_txc_reg;
   wire [63:0] xgmii_rxd_int;
   wire [7:0]  xgmii_rxc_int;

   // Add a pipeline to the xmgii_tx inputs, to aid timing closure
   always @(posedge clk156)
   begin
      xgmii_txd_reg <= xgmii_txd;
      xgmii_txc_reg <= xgmii_txc;
   end

   // Add a pipeline to the xmgii_rx outputs, to aid timing closure
   always @(posedge clk156)
   begin
      xgmii_rxd <= xgmii_rxd_int;
      xgmii_rxc <= xgmii_rxc_int;
   end

   wire  mdio_out_int;
   wire  mdio_tri_int;
   reg   mdc_reg;
   reg   mdio_in_reg;

   // Add a pipeline to the mdio in/outputs, to aid timing closure
   // This is safe because the mdio clock is running so slowly
   always @(posedge clk156)
   begin
      mdio_out <= mdio_out_int;
      mdio_tri <= mdio_tri_int;
      mdc_reg <= mdc;
      mdio_in_reg <= mdio_in;
   end

   // Signal declarations
   wire        txclk322;
   wire        qplloutclk;
   wire        qplloutrefclk;
   wire        qplllock;

   wire        drp_gnt;
   wire        drp_req;
   wire        drp_den_o;
   wire        drp_dwe_o;
   wire [15:0] drp_daddr_o;
   wire [15:0] drp_di_o;
   wire        drp_drdy_o;
   wire [15:0] drp_drpdo_o;
   wire        drp_den_i;
   wire        drp_dwe_i;
   wire [15:0] drp_daddr_i;
   wire [15:0] drp_di_i;
   wire        drp_drdy_i;
   wire [15:0] drp_drpdo_i;

   wire        tx_resetdone_int;
   wire        rx_resetdone_int;

   wire        areset_clk156;
   wire        gttxreset;
   wire        gtrxreset;
   wire        qpllreset;
   wire        qplllock_txusrclk2;
   wire        gttxreset_txusrclk2;
   wire        reset_counter_done;
   wire        txusrclk;
   wire        txusrclk2;
   reg         txuserrdy;

   assign resetdone = tx_resetdone_int && rx_resetdone_int;

   // If no arbitration is required on the GT DRP ports then connect REQ to GNT
   // and connect other signals i <= o;
   assign drp_gnt       = drp_req;
   assign drp_den_i     = drp_den_o;
   assign drp_dwe_i     = drp_dwe_o;
   assign drp_daddr_i   = drp_daddr_o;
   assign drp_di_i      = drp_di_o;
   assign drp_drdy_i    = drp_drdy_o;
   assign drp_drpdo_i   = drp_drpdo_o;

   // Instantiate the 10GBASER/KR GT Common block
   ten_gig_eth_pcs_pma_gt_common # (
      .WRAPPER_SIM_GTRESET_SPEEDUP("TRUE") //Does not affect hardware
   )  ten_gig_eth_pcs_pma_gt_common_block (
      .refclk(refclk),
      .qpllreset(qpllreset),
      .qplllock(qplllock),
      .qplloutclk(qplloutclk),
      .qplloutrefclk(qplloutrefclk)
   );

   // Asynch reset synchronizers...
   ten_gig_eth_pcs_pma_ff_synchronizer_rst2 #(
      .C_NUM_SYNC_REGS(4),
      .C_RVAL(1'b1)
   ) areset_clk156_sync_i (
      .clk(clk156),
      .rst(areset),
      .data_in(1'b0),
      .data_out(areset_clk156)
   );

   ten_gig_eth_pcs_pma_ff_synchronizer_rst2 #(
      .C_NUM_SYNC_REGS(4),
      .C_RVAL(1'b0)
   ) qplllock_txusrclk2_sync_i (
      .clk(txusrclk2),
      .rst(!qplllock),
      .data_in(1'b1),
      .data_out(qplllock_txusrclk2)
   );

   reg [7:0] reset_counter = 8'h00;
   reg [3:0] reset_pulse = 4'b1110;
   assign reset_counter_done = reset_counter[7];

   // Hold off the GT resets until 500ns after configuration.
   // 128 ticks at 6.4ns period will be >> 500 ns.
   always @(posedge clk156)
   begin
      if (!reset_counter[7])
         reset_counter <= reset_counter + 1'b1;   
      else
         reset_counter <= reset_counter;
   end

   always @(posedge clk156)
   begin
      if (areset_clk156 == 1'b1)  
         reset_pulse <= 4'b1110;
      else if(reset_counter[7])
         reset_pulse <= {1'b0, reset_pulse[3:1]};
   end

   assign qpllreset  = reset_pulse[0];
   assign gttxreset  = reset_pulse[0];
   assign gtrxreset  = reset_pulse[0];  

   ten_gig_eth_pcs_pma_ff_synchronizer_rst2 #(
      .C_NUM_SYNC_REGS(4),
      .C_RVAL(1'b1)
   ) gttxreset_txusrclk2_sync_i (
      .clk(txusrclk2),
      .rst(gttxreset),
      .data_in(1'b0),
      .data_out(gttxreset_txusrclk2)
   );

   always @(posedge txusrclk2 or posedge gttxreset_txusrclk2)
   begin
      if(gttxreset_txusrclk2)
         txuserrdy <= 1'b0;
      else
         txuserrdy <= qplllock_txusrclk2;
   end

   BUFG tx322clk_bufg_i (
      .I (txclk322),
      .O (txusrclk)
   );
   
   assign txusrclk2 = txusrclk;

   // Instantiate the 10GBASER/KR Block Level
   ten_gig_eth_pcs_pma ten_gig_eth_pcs_pma_i (
      .coreclk(clk156),
      .dclk(dclk),
      .txusrclk(txusrclk),
      .txusrclk2(txusrclk2),
      .txoutclk(txclk322),
      .areset_coreclk(areset_clk156),
      .txuserrdy(txuserrdy),
      .areset(areset),
      .gttxreset(gttxreset),
      .gtrxreset(gtrxreset),
      .sim_speedup_control(sim_speedup_control),
      .qplllock(qplllock),
      .qplloutclk(qplloutclk),
      .qplloutrefclk(qplloutrefclk),
      .reset_counter_done(reset_counter_done),
      .xgmii_txd(xgmii_txd_reg),
      .xgmii_txc(xgmii_txc_reg),
      .xgmii_rxd(xgmii_rxd_int),
      .xgmii_rxc(xgmii_rxc_int),
      .txp(txp),
      .txn(txn),
      .rxp(rxp),
      .rxn(rxn),
      .mdc(mdc_reg),
      .mdio_in(mdio_in_reg),
      .mdio_out(mdio_out_int),
      .mdio_tri(mdio_tri_int),
      .prtad(prtad),
      .core_status(core_status),
      .tx_resetdone(tx_resetdone_int),
      .rx_resetdone(rx_resetdone_int),
      .signal_detect(signal_detect),
      .tx_fault(tx_fault),
      .drp_req(drp_req),
      .drp_gnt(drp_gnt),
      .drp_den_o(drp_den_o),
      .drp_dwe_o(drp_dwe_o),
      .drp_daddr_o(drp_daddr_o),
      .drp_di_o(drp_di_o),
      .drp_drdy_o(drp_drdy_o),
      .drp_drpdo_o(drp_drpdo_o),
      .drp_den_i(drp_den_i),
      .drp_dwe_i(drp_dwe_i),
      .drp_daddr_i(drp_daddr_i),
      .drp_di_i(drp_di_i),
      .drp_drdy_i(drp_drdy_i),
      .drp_drpdo_i(drp_drpdo_i),
      .pma_pmd_type(3'b101),
      .tx_disable(tx_disable)
   );

endmodule
