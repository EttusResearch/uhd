//-----------------------------------------------------------------------------
// Title      : Block level wrapper                                             
// Project    : 10GBASE-R                                                      
//-----------------------------------------------------------------------------
// File       : ten_gig_eth_pcs_pma_block.v                                          
//-----------------------------------------------------------------------------
// Description: This file is a wrapper for the 10GBASE-R core. It contains the 
// 10GBASE-R core, the transceivers and some transceiver logic.                
//-----------------------------------------------------------------------------
// (c) Copyright 2009 - 2012 Xilinx, Inc. All rights reserved.
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

module ten_gig_eth_pcs_pma_block #
  (
    parameter   EXAMPLE_SIM_GTRESET_SPEEDUP = "FALSE"
  )
  (
//  input           refclk_n,
//  input           refclk_p,
  input refclk156,
  input refclk156_buf, 
  output          clk156,
  output          txclk322,
  output          rxclk322,
  output          dclk,
  input           areset,
  input           reset,
  input           txreset322,
  input           rxreset322,
  input           dclk_reset,
  output          txp,
  output          txn,
  input           rxp,
  input           rxn,
  input  [63:0]   xgmii_txd,
  input  [7:0]    xgmii_txc,
  output [63:0]   xgmii_rxd,
  output [7:0]    xgmii_rxc,
  input           mdc,
  input           mdio_in,
  output          mdio_out,
  output          mdio_tri,
  input  [4 : 0]  prtad,
  output [7 : 0]  core_status,
  output          tx_resetdone,
  output          rx_resetdone,
  input           signal_detect,
  input           tx_fault,
  output          tx_disable);   
    
  //  Static signal Assigments    
  wire            tied_to_ground_i;
  wire    [63:0]  tied_to_ground_vec_i;
  wire            tied_to_vcc_i;
  wire    [7:0]   tied_to_vcc_vec_i;
  assign tied_to_ground_i             = 1'b0;
  assign tied_to_ground_vec_i         = 64'h0000000000000000;
  assign tied_to_vcc_i                = 1'b1;
  assign tied_to_vcc_vec_i            = 8'hff;
  

  wire [31:0]     gt_txd;
  wire [7:0]      gt_txc;

  wire [31:0]     gt_rxd;
  wire [7:0]      gt_rxc;
  
  reg [31:0]     gt_rxd_d1;
  reg [7:0]      gt_rxc_d1;  

  wire [15:0]     gt0_drpdi_i;
  wire [15:0]     gt0_drpaddr_i;
  wire [15:0]     gt0_drpdo_i;
  
  wire gt0_rxgearboxslip_i;
  wire  drp_gnt;
  wire  drp_req;
  
  wire [2:0] gt0_loopback_i;
  wire gt0_clear_rx_prbs_err_count_i;
  
  wire gt0_qplllock_i;
  wire gt0_rxusrclk_i;
  wire gt0_txusrclk_i;
  wire gt0_gtrxreset_i;
  wire gt0_gttxreset_i;

  reg pma_resetout_reg;
  wire pma_resetout_rising;
  reg pcs_resetout_reg;
  wire pcs_resetout_rising;
  
  wire pma_resetout;
  wire pcs_resetout;
  
  wire clk156_buf;
  wire dclk_buf;

  wire gt0_rxuserrdy_i;
  wire gt0_txuserrdy_i;
  reg gt0_rxuserrdy_r = 1'b0;
  reg gt0_txuserrdy_r = 1'b0;
  wire GTTXRESET_IN;
  wire GTRXRESET_IN;
  wire QPLLRESET_IN;
  reg [7:0] reset_counter = 8'h00;
  reg [3:0] reset_pulse;
  wire mmcm_locked;

  reg [19:0] rxuserrdy_counter = 20'h0;
  // Nominal wait time of 50000 UI = 757 cyles of 156.25MHz clock
  localparam [19:0] RXRESETTIME_NOM = 20'h002F5; 
  // Maximum wait time of 37x10^6 UI = 560782 cycles of 156.25MHz clock
  localparam [19:0] RXRESETTIME_MAX = 20'h89000;
  
  // Set this according to requirements
  wire [19:0] RXRESETTIME = RXRESETTIME_NOM;

  // Aid the detection of a cable/board being pulled
  reg [3:0] rx_sample = 4'b0000; // Used to monitor RX data for a cable pull 
  reg [3:0] rx_sample_prev = 4'b0000; // Used to monitor RX data for a cable pull 
  reg [19:0] cable_pull_watchdog = 20'h20000; // 128K cycles 
  reg [1:0] cable_pull_watchdog_event = 2'b00; // Count events which suggest no cable pull
  reg cable_pull_reset = 1'b0;  // This is set when the watchdog above gets to 0.
  (* ASYNC_REG = "TRUE" *)
  reg cable_pull_reset_reg = 1'b0;  // This is set when the watchdog above gets to 0.
  (* ASYNC_REG = "TRUE" *)
  reg cable_pull_reset_reg_reg = 1'b0;  
  reg cable_pull_reset_rising = 1'b0;  
  reg cable_pull_reset_rising_reg = 1'b0;  
  
  // Aid the detection of a cable/board being plugged back in
  reg cable_unpull_enable = 1'b0;
  reg [19:0] cable_unpull_watchdog = 20'h20000;
  reg [10:0] cable_unpull_watchdog_event = 11'b0;
  reg cable_unpull_reset = 1'b0;
  (* ASYNC_REG = "TRUE" *)
  reg cable_unpull_reset_reg = 1'b0;
  (* ASYNC_REG = "TRUE" *)
  reg cable_unpull_reset_reg_reg = 1'b0;
  reg cable_unpull_reset_rising = 1'b0;
  reg cable_unpull_reset_rising_reg = 1'b0;
  
  wire signal_detect_comb;
  wire cable_is_pulled;


  // If no arbitration is required on the GT DRP ports then connect REQ to GNT...
  assign drp_gnt = drp_req;
          
  ten_gig_eth_pcs_pma 
     ten_gig_eth_pcs_pma_core (
      .reset(reset), 
      .txreset322(txreset322),
      .rxreset322(rxreset322),
      .dclk_reset(dclk_reset),
      .pma_resetout(pma_resetout),
      .pcs_resetout(pcs_resetout),
      .clk156(clk156), 
      .txusrclk2(txclk322),
      .rxusrclk2(rxclk322),
      .dclk(dclk),      
      .xgmii_txd(xgmii_txd),
      .xgmii_txc(xgmii_txc),
      .xgmii_rxd(xgmii_rxd),
      .xgmii_rxc(xgmii_rxc),
      .mdc(mdc),
      .mdio_in(mdio_in),
      .mdio_out(mdio_out),
      .mdio_tri(mdio_tri),
      .prtad(prtad),
      .core_status(core_status), 
      .pma_pmd_type(3'b101),
      .drp_req(drp_req),
      .drp_gnt(drp_gnt),                            
      .drp_den(gt0_drpen_i),                                   
      .drp_dwe(gt0_drpwe_i),
      .drp_daddr(gt0_drpaddr_i),                 
      .drp_di(gt0_drpdi_i),                  
      .drp_drdy(gt0_drprdy_i),               
      .drp_drpdo(gt0_drpdo_i),
      .resetdone(resetdone),
      .gt_txd(gt_txd),
      .gt_txc(gt_txc),
      .gt_rxd(gt_rxd_d1),
      .gt_rxc(gt_rxc_d1),
      .gt_slip(gt0_rxgearboxslip_i),
      .signal_detect(signal_detect_comb),
      .tx_fault(tx_fault),
      .tx_disable(tx_disable),
      .tx_prbs31_en(tx_prbs31_en),
      .rx_prbs31_en(rx_prbs31_en),
      .clear_rx_prbs_err_count(gt0_clear_rx_prbs_err_count_i),
      .loopback_ctrl(gt0_loopback_i));

  // Make the GT Wizard output connect to the core and top level i/f
  //assign Q1_CLK0_GTREFCLK_PAD_N_IN = refclk_n;
  //assign Q1_CLK0_GTREFCLK_PAD_P_IN = refclk_p;
  
  wire gt0_txusrclk2_i;
  wire gt0_rxusrclk2_i;
  wire gt0_drpclk_i;
  wire clkfbout;

  //
  // Single ended 156MHz reference clock brought in from upper hierarchy now.
  //
  wire q1_clk0_refclk_i = refclk156;
   
  wire q1_clk0_refclk_i_bufh = refclk156_buf;
  
  assign txclk322 = gt0_txusrclk2_i;
  assign rxclk322 = gt0_rxusrclk2_i;
  assign gt0_drpclk_i = dclk;

  assign RXN_IN = rxn;
  assign RXP_IN = rxp;
  
  wire TXN_OUT;
  wire TXP_OUT;
  
  assign txn = TXN_OUT;
  assign txp = TXP_OUT;
  
  wire gt0_txresetdone_i;
  wire gt0_rxresetdone_i;
  
  (* ASYNC_REG = "TRUE" *)
  reg gt0_txresetdone_i_rega = 1'b0;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_txresetdone_i_reg = 1'b0;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_rxresetdone_i_rega = 1'b0;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_rxresetdone_i_reg = 1'b0;
  
  reg gt0_rxresetdone_i_regrx322 = 1'b0;
  
  always @(posedge clk156)
  begin
    if(mmcm_locked == 1'b1) begin
      gt0_txresetdone_i_rega <= gt0_txresetdone_i;
      gt0_txresetdone_i_reg <= gt0_txresetdone_i_rega;
      gt0_rxresetdone_i_rega <= gt0_rxresetdone_i;
      gt0_rxresetdone_i_reg <= gt0_rxresetdone_i_rega;
    end
  end
  
  assign resetdone = gt0_txresetdone_i_reg && gt0_rxresetdone_i_reg;
  assign tx_resetdone = gt0_txresetdone_i_reg && mmcm_locked;
  assign rx_resetdone = gt0_rxresetdone_i_reg && mmcm_locked;
  
  wire [1:0] gt0_txheader_i;
  wire [6:0] gt0_txsequence_i;
  wire [31:0] gt0_txdata_i;
  
  reg gt0_rxbufreset_i = 1'b0;
  wire [2:0] gt0_rxbufstatus_i;
  
  assign gt0_txdata_i[0 ] = gt_txd[31];
  assign gt0_txdata_i[1 ] = gt_txd[30];
  assign gt0_txdata_i[2 ] = gt_txd[29];
  assign gt0_txdata_i[3 ] = gt_txd[28];
  assign gt0_txdata_i[4 ] = gt_txd[27];
  assign gt0_txdata_i[5 ] = gt_txd[26];
  assign gt0_txdata_i[6 ] = gt_txd[25];
  assign gt0_txdata_i[7 ] = gt_txd[24];
  assign gt0_txdata_i[8 ] = gt_txd[23];
  assign gt0_txdata_i[9 ] = gt_txd[22];
  assign gt0_txdata_i[10] = gt_txd[21];
  assign gt0_txdata_i[11] = gt_txd[20];
  assign gt0_txdata_i[12] = gt_txd[19];
  assign gt0_txdata_i[13] = gt_txd[18];
  assign gt0_txdata_i[14] = gt_txd[17];
  assign gt0_txdata_i[15] = gt_txd[16];
  assign gt0_txdata_i[16] = gt_txd[15];
  assign gt0_txdata_i[17] = gt_txd[14];
  assign gt0_txdata_i[18] = gt_txd[13];
  assign gt0_txdata_i[19] = gt_txd[12];
  assign gt0_txdata_i[20] = gt_txd[11];
  assign gt0_txdata_i[21] = gt_txd[10];
  assign gt0_txdata_i[22] = gt_txd[9 ];
  assign gt0_txdata_i[23] = gt_txd[8 ];
  assign gt0_txdata_i[24] = gt_txd[7 ];
  assign gt0_txdata_i[25] = gt_txd[6 ];
  assign gt0_txdata_i[26] = gt_txd[5 ];
  assign gt0_txdata_i[27] = gt_txd[4 ];
  assign gt0_txdata_i[28] = gt_txd[3 ];
  assign gt0_txdata_i[29] = gt_txd[2 ];
  assign gt0_txdata_i[30] = gt_txd[1 ];
  assign gt0_txdata_i[31] = gt_txd[0 ];
  assign gt0_txheader_i[0] = gt_txc[1];
  assign gt0_txheader_i[1] = gt_txc[0];
  assign gt0_txsequence_i = {1'b0, gt_txc[7:2]};
  
  wire [31:0] gt0_rxdata_i;
  wire [1:0] gt0_rxheader_i;
  wire gt0_rxheadervalid_i;
  wire gt0_rxdatavalid_i;
  
  assign gt_rxd[0 ] = gt0_rxdata_i[31];
  assign gt_rxd[1 ] = gt0_rxdata_i[30];
  assign gt_rxd[2 ] = gt0_rxdata_i[29];
  assign gt_rxd[3 ] = gt0_rxdata_i[28];
  assign gt_rxd[4 ] = gt0_rxdata_i[27];
  assign gt_rxd[5 ] = gt0_rxdata_i[26];
  assign gt_rxd[6 ] = gt0_rxdata_i[25];
  assign gt_rxd[7 ] = gt0_rxdata_i[24];
  assign gt_rxd[8 ] = gt0_rxdata_i[23];
  assign gt_rxd[9 ] = gt0_rxdata_i[22];
  assign gt_rxd[10] = gt0_rxdata_i[21];
  assign gt_rxd[11] = gt0_rxdata_i[20];
  assign gt_rxd[12] = gt0_rxdata_i[19];
  assign gt_rxd[13] = gt0_rxdata_i[18];
  assign gt_rxd[14] = gt0_rxdata_i[17];
  assign gt_rxd[15] = gt0_rxdata_i[16];
  assign gt_rxd[16] = gt0_rxdata_i[15];
  assign gt_rxd[17] = gt0_rxdata_i[14];
  assign gt_rxd[18] = gt0_rxdata_i[13];
  assign gt_rxd[19] = gt0_rxdata_i[12];
  assign gt_rxd[20] = gt0_rxdata_i[11];
  assign gt_rxd[21] = gt0_rxdata_i[10];
  assign gt_rxd[22] = gt0_rxdata_i[9 ];
  assign gt_rxd[23] = gt0_rxdata_i[8 ];
  assign gt_rxd[24] = gt0_rxdata_i[7 ];
  assign gt_rxd[25] = gt0_rxdata_i[6 ];
  assign gt_rxd[26] = gt0_rxdata_i[5 ];
  assign gt_rxd[27] = gt0_rxdata_i[4 ];
  assign gt_rxd[28] = gt0_rxdata_i[3 ];
  assign gt_rxd[29] = gt0_rxdata_i[2 ];
  assign gt_rxd[30] = gt0_rxdata_i[1 ];
  assign gt_rxd[31] = gt0_rxdata_i[0 ];
  assign gt_rxc = {4'b0000, gt0_rxheadervalid_i,gt0_rxdatavalid_i, gt0_rxheader_i[0], gt0_rxheader_i[1]};

  always @(posedge rxclk322) begin
    gt_rxc_d1 <= gt_rxc;
    gt_rxd_d1 <= gt_rxd;
    gt0_rxresetdone_i_regrx322 <= gt0_rxresetdone_i;
  end
    
  // Asynch reset synchronizer registers
  (* ASYNC_REG = "TRUE" *)
  reg areset_q1_clk0_refclk_i_bufh_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg areset_q1_clk0_refclk_i_bufh;
  (* ASYNC_REG = "TRUE" *)
  reg areset_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg areset_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg areset_clk156_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg areset_clk156;
  (* ASYNC_REG = "TRUE" *)
  reg cable_pull_reset_rising_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg cable_pull_reset_rising_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg cable_unpull_reset_rising_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg cable_unpull_reset_rising_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg pma_resetout_rising_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg pma_resetout_rising_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_qplllock_i_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_qplllock_i_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_qplllock_i_gt0_txusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_qplllock_i_gt0_txusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg mmcm_locked_clk156_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg mmcm_locked_clk156;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_gtrxreset_i_gt0_rxusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_gtrxreset_i_gt0_rxusrclk2_i;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_gttxreset_i_gt0_txusrclk2_i_tmp;
  (* ASYNC_REG = "TRUE" *)
  reg gt0_gttxreset_i_gt0_txusrclk2_i;
  
  // Asynch reset synchronizers
  always @(posedge areset or posedge q1_clk0_refclk_i_bufh)
  begin
    if(areset)
    begin
      areset_q1_clk0_refclk_i_bufh_tmp <= 1'b1;
      areset_q1_clk0_refclk_i_bufh <= 1'b1;
    end
    else
    begin
      areset_q1_clk0_refclk_i_bufh_tmp <= 1'b0;
      areset_q1_clk0_refclk_i_bufh <= areset_q1_clk0_refclk_i_bufh_tmp;
    end
  end  
   
  always @(posedge areset or posedge gt0_rxusrclk2_i)
  begin
    if(areset)
    begin
      areset_gt0_rxusrclk2_i_tmp <= 1'b1;
      areset_gt0_rxusrclk2_i <= 1'b1;
    end
    else
    begin
      areset_gt0_rxusrclk2_i_tmp <= 1'b0;
      areset_gt0_rxusrclk2_i <= areset_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(posedge areset or posedge clk156)
  begin
    if(areset)
    begin
      areset_clk156_tmp <= 1'b1;
      areset_clk156 <= 1'b1;
    end
    else
    begin
      areset_clk156_tmp <= 1'b0;
      areset_clk156 <= areset_clk156_tmp;
    end
  end  
   
  always @(posedge cable_pull_reset_rising or posedge gt0_rxusrclk2_i)
  begin
    if(cable_pull_reset_rising)
    begin
      cable_pull_reset_rising_gt0_rxusrclk2_i_tmp <= 1'b1;
      cable_pull_reset_rising_gt0_rxusrclk2_i <= 1'b1;
    end
    else
    begin
      cable_pull_reset_rising_gt0_rxusrclk2_i_tmp <= 1'b0;
      cable_pull_reset_rising_gt0_rxusrclk2_i <= cable_pull_reset_rising_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(posedge cable_unpull_reset_rising or posedge gt0_rxusrclk2_i)
  begin
    if(cable_unpull_reset_rising)
    begin
      cable_unpull_reset_rising_gt0_rxusrclk2_i_tmp <= 1'b1;
      cable_unpull_reset_rising_gt0_rxusrclk2_i <= 1'b1;
    end
    else
    begin
      cable_unpull_reset_rising_gt0_rxusrclk2_i_tmp <= 1'b0;
      cable_unpull_reset_rising_gt0_rxusrclk2_i <= cable_unpull_reset_rising_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(posedge pma_resetout_rising or posedge gt0_rxusrclk2_i)
  begin
    if(pma_resetout_rising)
    begin
      pma_resetout_rising_gt0_rxusrclk2_i_tmp <= 1'b1;
      pma_resetout_rising_gt0_rxusrclk2_i <= 1'b1;
    end
    else
    begin
      pma_resetout_rising_gt0_rxusrclk2_i_tmp <= 1'b0;
      pma_resetout_rising_gt0_rxusrclk2_i <= pma_resetout_rising_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(negedge gt0_qplllock_i or posedge gt0_rxusrclk2_i)
  begin
    if(!gt0_qplllock_i)
    begin
      gt0_qplllock_i_gt0_rxusrclk2_i_tmp <= 1'b0;
      gt0_qplllock_i_gt0_rxusrclk2_i <= 1'b0;
    end
    else
    begin
      gt0_qplllock_i_gt0_rxusrclk2_i_tmp <= 1'b1;
      gt0_qplllock_i_gt0_rxusrclk2_i <= gt0_qplllock_i_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(negedge gt0_qplllock_i or posedge gt0_txusrclk2_i)
  begin
    if(!gt0_qplllock_i)
    begin
      gt0_qplllock_i_gt0_txusrclk2_i_tmp <= 1'b0;
      gt0_qplllock_i_gt0_txusrclk2_i <= 1'b0;
    end
    else
    begin
      gt0_qplllock_i_gt0_txusrclk2_i_tmp <= 1'b1;
      gt0_qplllock_i_gt0_txusrclk2_i <= gt0_qplllock_i_gt0_txusrclk2_i_tmp;
    end
  end  
   
  always @(negedge mmcm_locked or posedge clk156)
  begin
    if(!mmcm_locked)
    begin
      mmcm_locked_clk156_tmp <= 1'b0;
      mmcm_locked_clk156 <= 1'b0;
    end
    else
    begin
      mmcm_locked_clk156_tmp <= 1'b1;
      mmcm_locked_clk156 <= mmcm_locked_clk156_tmp;
    end
  end       
    
  always @(posedge gt0_gtrxreset_i or posedge gt0_rxusrclk2_i)
  begin
    if(gt0_gtrxreset_i)
    begin
      gt0_gtrxreset_i_gt0_rxusrclk2_i_tmp <= 1'b1;
      gt0_gtrxreset_i_gt0_rxusrclk2_i <= 1'b1;
    end
    else
    begin
      gt0_gtrxreset_i_gt0_rxusrclk2_i_tmp <= 1'b0;
      gt0_gtrxreset_i_gt0_rxusrclk2_i <= gt0_gtrxreset_i_gt0_rxusrclk2_i_tmp;
    end
  end  
   
  always @(posedge gt0_gttxreset_i or posedge gt0_txusrclk2_i)
  begin
    if(gt0_gttxreset_i)
    begin
      gt0_gttxreset_i_gt0_txusrclk2_i_tmp <= 1'b1;
      gt0_gttxreset_i_gt0_txusrclk2_i <= 1'b1;
    end
    else
    begin
      gt0_gttxreset_i_gt0_txusrclk2_i_tmp <= 1'b0;
      gt0_gttxreset_i_gt0_txusrclk2_i <= gt0_gttxreset_i_gt0_txusrclk2_i_tmp;
    end
  end  
   
  // Reset logic from the gtwizard top level output file....
  // Adapt the reset_counter to count clk156 ticks.
  // 128 ticks at 6.4ns period will be >> 500 ns.
  // Removed all 'after DLY' text.

  always @(posedge q1_clk0_refclk_i_bufh or posedge areset_q1_clk0_refclk_i_bufh)
  begin
    if (areset_q1_clk0_refclk_i_bufh == 1'b1)
      reset_counter <= 8'b0;
    else if (!reset_counter[7])
      reset_counter   <=   reset_counter + 1'b1;   
    else
      reset_counter   <=   reset_counter;
  end

  always @(posedge q1_clk0_refclk_i_bufh)
  begin
    if(!reset_counter[7])
      reset_pulse   <=   4'b1110;
    else
      reset_pulse   <=   {1'b0, reset_pulse[3:1]};
  end

  // Delay the assertion of RXUSERRDY by the given amount
  always @(posedge gt0_rxusrclk2_i or posedge gt0_gtrxreset_i_gt0_rxusrclk2_i or negedge gt0_qplllock_i_gt0_rxusrclk2_i)
  begin
     if(!gt0_qplllock_i_gt0_rxusrclk2_i || gt0_gtrxreset_i_gt0_rxusrclk2_i)
       rxuserrdy_counter <= 20'h0;
     else if (!(rxuserrdy_counter == RXRESETTIME))
        rxuserrdy_counter   <=   rxuserrdy_counter + 1'b1;       
     else
        rxuserrdy_counter   <=   rxuserrdy_counter;
  end

  assign   GTTXRESET_IN  =     reset_pulse[0];
  assign   GTRXRESET_IN  =     reset_pulse[0];

  assign   QPLLRESET_IN  =     reset_pulse[0];

  assign gt0_rxuserrdy_i = gt0_rxuserrdy_r;
  assign gt0_txuserrdy_i = gt0_txuserrdy_r;

  always @(posedge gt0_rxusrclk2_i or posedge gt0_gtrxreset_i_gt0_rxusrclk2_i)
  begin
     if(gt0_gtrxreset_i_gt0_rxusrclk2_i)
       gt0_rxuserrdy_r <= 1'b0;
     else if(rxuserrdy_counter == RXRESETTIME)
       gt0_rxuserrdy_r <= 1'b1;
     else
       gt0_rxuserrdy_r <= gt0_rxuserrdy_r;
  end
  
  always @(posedge gt0_txusrclk2_i or posedge gt0_gttxreset_i_gt0_txusrclk2_i)
  begin
     if(gt0_gttxreset_i_gt0_txusrclk2_i)
       gt0_txuserrdy_r <= 1'b0;
     else
       gt0_txuserrdy_r <= gt0_qplllock_i_gt0_txusrclk2_i;
  end

  // Create a watchdog which samples 4 bits from the gt_rxd vector and checks that it does
  // vary from a 1010 or 0101 or 0000 pattern. If not then there may well have been a cable pull
  // and the gt rx side needs to be reset.
  always @(posedge gt0_rxusrclk2_i or posedge cable_pull_reset_rising_gt0_rxusrclk2_i)
  begin
    if(cable_pull_reset_rising_gt0_rxusrclk2_i)
    begin
      cable_pull_watchdog_event <= 2'b00;
      cable_pull_watchdog <= 20'h20000; // reset the watchdog
      cable_pull_reset <= 1'b0; 
      rx_sample <= 4'b0;
      rx_sample_prev <= 4'b0;
    end
    else
    begin
      // Sample 4 bits of the gt_rxd vector
      rx_sample <= gt_rxd[7:4];
      rx_sample_prev <= rx_sample;
      
      if(!cable_pull_reset && !cable_is_pulled && gt0_rxresetdone_i_regrx322)
      begin
        // If those 4 bits do not look like the cable-pull behaviour, increment the event counter
        if(!(rx_sample == 4'b1010) && !(rx_sample == 4'b0101) && !(rx_sample == 4'b0000) && !(rx_sample == rx_sample_prev))  // increment the event counter
          cable_pull_watchdog_event <= cable_pull_watchdog_event + 1;
        else // we are seeing what may be a cable pull
          cable_pull_watchdog_event <= 2'b00;
        
        
        if(cable_pull_watchdog_event == 2'b10) // Two consecutive events which look like the cable is attached
        begin
          cable_pull_watchdog <= 20'h20000; // reset the watchdog
          cable_pull_watchdog_event <= 2'b00;
        end
        else
          cable_pull_watchdog <= cable_pull_watchdog - 1;
        
                        
        if(~|cable_pull_watchdog) 
          cable_pull_reset <= 1'b1; // Hit GTRXRESET! 
        else
          cable_pull_reset <= 1'b0;
      end
    end
  end 

  always @(posedge clk156)
  begin
    if(mmcm_locked == 1'b1) begin
      cable_pull_reset_reg <= cable_pull_reset;
      cable_pull_reset_reg_reg <= cable_pull_reset_reg;
      cable_pull_reset_rising <= cable_pull_reset_reg && !cable_pull_reset_reg_reg;  
      cable_pull_reset_rising_reg <= cable_pull_reset_rising;  
    end
  end

  always @(posedge gt0_rxusrclk2_i or posedge areset_gt0_rxusrclk2_i or posedge pma_resetout_rising_gt0_rxusrclk2_i)
  begin
    if(areset_gt0_rxusrclk2_i || pma_resetout_rising_gt0_rxusrclk2_i)
      cable_unpull_enable <= 1'b0;
    else if(cable_pull_reset) // Cable pull has been detected - enable cable unpull counter
      cable_unpull_enable <= 1'b1;
    else if(cable_unpull_reset) // Cable has been detected as being plugged in again
      cable_unpull_enable <= 1'b0;
    else
      cable_unpull_enable <= cable_unpull_enable;
  end
  
  // Look for data on the line which does NOT look like the cable is still pulled
  // a set of 1024 non-1010 or 0101 or 0000 samples within 128k samples suggests that the cable is in.
  always @(posedge gt0_rxusrclk2_i or posedge cable_unpull_reset_rising_gt0_rxusrclk2_i)
  begin
    if(cable_unpull_reset_rising_gt0_rxusrclk2_i)
    begin
      cable_unpull_reset <= 1'b0; 
      cable_unpull_watchdog_event <= 11'b0; // reset the event counter
      cable_unpull_watchdog <= 20'h20000; // reset the watchdog window
    end
    else
    begin
      if(!cable_unpull_reset && cable_is_pulled && gt0_rxresetdone_i_regrx322)
      begin
        // If those 4 bits do not look like the cable-pull behaviour, increment the event counter
        if(!(rx_sample == 4'b1010) && !(rx_sample == 4'b0101) && !(rx_sample == 4'b0000) && !(rx_sample == rx_sample_prev))  // increment the event counter
          cable_unpull_watchdog_event <= cable_unpull_watchdog_event + 1;
        
        
        if(cable_unpull_watchdog_event[10] == 1'b1) // Detected 1k 'valid' rx data words within 128k words
        begin
          cable_unpull_reset <= 1'b1; // Hit GTRXRESET again!
          cable_unpull_watchdog <= 20'h20000; // reset the watchdog window
        end
        else
          cable_unpull_watchdog <= cable_unpull_watchdog - 1;    
                            
        if(~|cable_unpull_watchdog) 
        begin 
          cable_unpull_watchdog <= 20'h20000; // reset the watchdog window
          cable_unpull_watchdog_event <= 11'b0; // reset the event counter
        end
      end
    end
  end 
  
  always @(posedge clk156)
  begin
    if(mmcm_locked == 1'b1) begin
      cable_unpull_reset_reg <= cable_unpull_reset;
      cable_unpull_reset_reg_reg <= cable_unpull_reset_reg;
      cable_unpull_reset_rising <= cable_unpull_reset_reg && !cable_unpull_reset_reg_reg;  
      cable_unpull_reset_rising_reg <= cable_unpull_reset_rising;  
    end
  end

  // Create the local cable_is_pulled signal
  assign cable_is_pulled = cable_unpull_enable;

  // Create the signal_detect signal as an AND of the external signal and (not) the local cable_is_pulled
  assign signal_detect_comb = signal_detect && !cable_is_pulled;


  always @(posedge areset_clk156 or posedge clk156 or negedge mmcm_locked_clk156)
  begin
    if(areset_clk156 || !mmcm_locked_clk156)
      pma_resetout_reg <= 1'b0;
    else
      pma_resetout_reg <= pma_resetout;
  end
  
  assign pma_resetout_rising = pma_resetout && !pma_resetout_reg;
  
  always @(posedge areset_clk156 or posedge clk156 or negedge mmcm_locked_clk156)
  begin
    if(areset_clk156 || !mmcm_locked_clk156)
      pcs_resetout_reg <= 1'b0;
    else
      pcs_resetout_reg <= pcs_resetout;
  end
  
  assign pcs_resetout_rising = pcs_resetout && !pcs_resetout_reg;
  
  
  // Incorporate the pma_resetout_rising and cable_pull/unpull_reset_rising bits generated in code below.
  assign  gt0_gtrxreset_i = (GTRXRESET_IN || !gt0_qplllock_i || pma_resetout_rising ||
                             cable_pull_reset_rising_reg || cable_unpull_reset_rising_reg) && reset_counter[7];
 	assign  gt0_gttxreset_i = (GTTXRESET_IN || !gt0_qplllock_i || pma_resetout_rising) && reset_counter[7];
  assign  gt0_qpllreset_i = QPLLRESET_IN;

  assign  gt0_rxpcsreset_i = pcs_resetout_rising;
  assign  gt0_txpcsreset_i = pcs_resetout_rising;

  // reset the GT RX Buffer when over/underflowing
  always @(posedge gt0_rxusrclk2_i)
  begin
    if(gt0_rxbufstatus_i[2] == 1'b1 && gt0_rxresetdone_i_regrx322)
      gt0_rxbufreset_i <= 1'b1;
    else
      gt0_rxbufreset_i <= 1'b0;
  end    

  // As generated by the GT Wizard - cut from _top level in eg design dir
  ten_gig_eth_pcs_pma_GT_USRCLK_SOURCE gt_usrclk_source
  (
   // IJB. Remove IBUFDS_GTE2 from 10G PHY hierarchy so that it can be shared.
   //
   // .Q1_CLK0_GTREFCLK_PAD_N_IN  (Q1_CLK0_GTREFCLK_PAD_N_IN),
   // .Q1_CLK0_GTREFCLK_PAD_P_IN  (Q1_CLK0_GTREFCLK_PAD_P_IN),
   // .Q1_CLK0_GTREFCLK_OUT       (q1_clk0_refclk_i),

    .GT0_TXUSRCLK_OUT    (gt0_txusrclk_i),
    .GT0_TXUSRCLK2_OUT   (gt0_txusrclk2_i),
    .GT0_TXOUTCLK_IN     (gt0_txoutclk_i),
    .GT0_RXUSRCLK_OUT    (gt0_rxusrclk_i),
    .GT0_RXUSRCLK2_OUT   (gt0_rxusrclk2_i),
    .GT0_RXOUTCLK_IN     (gt0_rxoutclk_i),
    .DRPCLK_IN           (tied_to_ground_i),
    .DRPCLK_OUT          () 
  );
   
  // MMCM to generate both clk156 and dclk
  MMCME2_BASE #
  (
    .BANDWIDTH            ("OPTIMIZED"),
    .STARTUP_WAIT         ("FALSE"),
    .DIVCLK_DIVIDE        (1),
    .CLKFBOUT_MULT_F      (4.0),
    .CLKFBOUT_PHASE       (0.000),
    .CLKOUT0_DIVIDE_F     (4.000),
    .CLKOUT0_PHASE        (0.000),
    .CLKOUT0_DUTY_CYCLE   (0.500),
    .CLKIN1_PERIOD        (6.400),
    .CLKOUT1_DIVIDE       (8),
    .CLKOUT1_PHASE        (0.000),
    .CLKOUT1_DUTY_CYCLE   (0.500),
    .REF_JITTER1          (0.010)
  )
  clkgen_i
  (
    .CLKFBIN(clkfbout),
    .CLKIN1(q1_clk0_refclk_i_bufh),
    .PWRDWN(1'b0),
    .RST(!gt0_qplllock_i),
    .CLKFBOUT(clkfbout),
    .CLKOUT0(clk156_buf),
    .CLKOUT1(dclk_buf),
    .LOCKED(mmcm_locked)
  );

/* -----\/----- EXCLUDED -----\/-----
  BUFG bufg_inst 
  (
 //     .CE                             (tied_to_vcc_i),
      .I                              (q1_clk0_refclk_i),
      .O                              (q1_clk0_refclk_i_bufh) 
  );
 -----/\----- EXCLUDED -----/\----- */

  BUFG clk156_bufg_inst 
  (
      .I                              (clk156_buf),
      .O                              (clk156) 
  );

  BUFG dclk_bufg_inst 
  (
      .I                              (dclk_buf),
      .O                              (dclk) 
  );  

  // As generated by the GT Wizard - cut from _top level in eg design dir
  // Use this example as a template for any updates - some signal names in
  // port mappings may have been changed from the GT wizard output
  ten_gig_eth_pcs_pma_gtwizard_10gbaser #
   (
       .WRAPPER_SIM_GTRESET_SPEEDUP   (EXAMPLE_SIM_GTRESET_SPEEDUP)
   )
    gtwizard_10gbaser_i
   (
        //_____________________________________________________________________
        //_____________________________________________________________________
        //

        //-------------- Channel - Dynamic Reconfiguration Port (DRP) --------------
        .GT0_DRPADDR_IN                 (gt0_drpaddr_i[8:0]),
        .GT0_DRPCLK_IN                  (gt0_drpclk_i),
        .GT0_DRPDI_IN                   (gt0_drpdi_i),
        .GT0_DRPDO_OUT                  (gt0_drpdo_i),
        .GT0_DRPEN_IN                   (gt0_drpen_i),
        .GT0_DRPRDY_OUT                 (gt0_drprdy_i),
        .GT0_DRPWE_IN                   (gt0_drpwe_i),
        //----------------------------- Eye Scan Ports -----------------------------
        .GT0_EYESCANDATAERROR_OUT       (gt0_eyescandataerror_i),
        //---------------------- Loopback and Powerdown Ports ----------------------
        .GT0_LOOPBACK_IN                (gt0_loopback_i),
        //----------------------------- Receive Ports ------------------------------
        .GT0_RXUSERRDY_IN               (gt0_rxuserrdy_i),
        //------------ Receive Ports - 64b66b and 64b67b Gearbox Ports -------------
        .GT0_RXDATAVALID_OUT            (gt0_rxdatavalid_i),
        .GT0_RXGEARBOXSLIP_IN           (gt0_rxgearboxslip_i),
        .GT0_RXHEADER_OUT               (gt0_rxheader_i),
        .GT0_RXHEADERVALID_OUT          (gt0_rxheadervalid_i),
        //--------------------- Receive Ports - PRBS Detection ---------------------
        .GT0_RXPRBSCNTRESET_IN          (gt0_clear_rx_prbs_err_count_i),
        .GT0_RXPRBSERR_OUT              (),
        .GT0_RXPRBSSEL_IN               ({rx_prbs31_en,2'b00}),
        //----------------- Receive Ports - RX Data Path interface -----------------
        .GT0_GTRXRESET_IN               (gt0_gtrxreset_i),
        .GT0_RXDATA_OUT                 (gt0_rxdata_i),
        .GT0_RXOUTCLK_OUT               (gt0_rxoutclk_i),
        .GT0_RXPCSRESET_IN              (gt0_rxpcsreset_i),
        .GT0_RXUSRCLK_IN                (gt0_rxusrclk_i),
        .GT0_RXUSRCLK2_IN               (gt0_rxusrclk2_i),
        //----- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
        .GT0_GTXRXN_IN                  (RXN_IN),
        .GT0_GTXRXP_IN                  (RXP_IN),
        .GT0_RXCDRLOCK_OUT              (),
        .GT0_RXELECIDLE_OUT             (),
        .GT0_RXLPMEN_IN                 (1'b0),
        //------ Receive Ports - RX Elastic Buffer and Phase Alignment Ports -------
        .GT0_RXBUFRESET_IN              (gt0_rxbufreset_i),
        .GT0_RXBUFSTATUS_OUT            (gt0_rxbufstatus_i),
        //---------------------- Receive Ports - RX PLL Ports ----------------------
        .GT0_RXRESETDONE_OUT            (gt0_rxresetdone_i),
        //----------------------------- Transmit Ports -----------------------------
        .GT0_TXUSERRDY_IN               (gt0_txuserrdy_i),
        //------------ Transmit Ports - 64b66b and 64b67b Gearbox Ports ------------
        .GT0_TXHEADER_IN                (gt0_txheader_i),
        .GT0_TXSEQUENCE_IN              (gt0_txsequence_i),
        //---------------- Transmit Ports - TX Data Path interface -----------------
        .GT0_GTTXRESET_IN               (gt0_gttxreset_i),
        .GT0_TXDATA_IN                  (gt0_txdata_i),
        .GT0_TXOUTCLK_OUT               (gt0_txoutclk_i),
        .GT0_TXOUTCLKFABRIC_OUT         (gt0_txoutclkfabric_i),
        .GT0_TXOUTCLKPCS_OUT            (gt0_txoutclkpcs_i),
        .GT0_TXPCSRESET_IN              (gt0_txpcsreset_i),
        .GT0_TXUSRCLK_IN                (gt0_txusrclk_i),
        .GT0_TXUSRCLK2_IN               (gt0_txusrclk2_i),
        //-------------- Transmit Ports - TX Driver and OOB signaling --------------
        .GT0_GTXTXN_OUT                 (TXN_OUT),
        .GT0_GTXTXP_OUT                 (TXP_OUT),
        .GT0_TXINHIBIT_IN               (tx_disable),
        .GT0_TXPRECURSOR_IN             (5'b0),
        .GT0_TXPOSTCURSOR_IN            (5'b0),
        .GT0_TXMAINCURSOR_IN            (7'b0),
        //--------------------- Transmit Ports - TX PLL Ports ----------------------
        .GT0_TXRESETDONE_OUT            (gt0_txresetdone_i),
        //------------------- Transmit Ports - TX PRBS Generator -------------------
        .GT0_TXPRBSSEL_IN               ({tx_prbs31_en,2'b00}),



    //____________________________COMMON PORTS________________________________
        //-------------------- Common Block  - Ref Clock Ports ---------------------
        .GT0_GTREFCLK0_COMMON_IN        (q1_clk0_refclk_i),
        //----------------------- Common Block - QPLL Ports ------------------------
        .GT0_QPLLLOCK_OUT               (gt0_qplllock_i),
        .GT0_QPLLLOCKDETCLK_IN          (tied_to_ground_i),
        .GT0_QPLLREFCLKLOST_OUT         (),
        .GT0_QPLLRESET_IN               (gt0_qpllreset_i)

    );
   

endmodule



