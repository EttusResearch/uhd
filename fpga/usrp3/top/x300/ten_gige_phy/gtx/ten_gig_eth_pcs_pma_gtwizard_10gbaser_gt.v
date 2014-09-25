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


`default_nettype wire

`timescale 1ns / 1ps


//***************************** Entity Declaration ****************************

module ten_gig_eth_pcs_pma_gtwizard_10gbaser_GT #
(
    // Simulation attributes
    parameter   GT_SIM_GTRESET_SPEEDUP   =   "false",       // Set to 1 to speed up sim reset;
    parameter   RX_DFE_KL_CFG2_IN        =   32'h3008E56A,
    parameter   PMA_RSV_IN               =   32'h00000000,
    parameter   PCS_RSVD_ATTR_IN         =   48'h000000000000,
    parameter   SIM_VERSION              =  ("4.0")
)
(
    //-------------------------------- Channel ---------------------------------
    input           QPLLCLK_IN,
    input           QPLLREFCLK_IN,
    //-------------- Channel - Dynamic Reconfiguration Port (DRP) --------------
    input   [8:0]   DRPADDR_IN,
    input           DRPCLK_IN,
    input   [15:0]  DRPDI_IN,
    output  [15:0]  DRPDO_OUT,
    input           DRPEN_IN,
    output          DRPRDY_OUT,
    input           DRPWE_IN,
    //----------------------------- Eye Scan Ports -----------------------------
    output          EYESCANDATAERROR_OUT,
    //---------------------- Loopback and Powerdown Ports ----------------------
    input   [2:0]   LOOPBACK_IN,
    //----------------------------- Receive Ports ------------------------------
    input           RXUSERRDY_IN,
    //------------ Receive Ports - 64b66b and 64b67b Gearbox Ports -------------
    output          RXDATAVALID_OUT,
    input           RXGEARBOXSLIP_IN,
    output  [1:0]   RXHEADER_OUT,
    output          RXHEADERVALID_OUT,
    //--------------------- Receive Ports - PRBS Detection ---------------------
    input           RXPRBSCNTRESET_IN,
    output          RXPRBSERR_OUT,
    input   [2:0]   RXPRBSSEL_IN,
    //----------------- Receive Ports - RX Data Path interface -----------------
    input           GTRXRESET_IN,
    output  [31:0]  RXDATA_OUT,
    output          RXOUTCLK_OUT,
    input           RXPCSRESET_IN,
    input           RXUSRCLK_IN,
    input           RXUSRCLK2_IN,
    //----- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
    input           GTXRXN_IN,
    input           GTXRXP_IN,
    output          RXCDRLOCK_OUT,
    output          RXELECIDLE_OUT,
    //------ Receive Ports - RX Elastic Buffer and Phase Alignment Ports -------
    input           RXBUFRESET_IN,
    output  [2:0]   RXBUFSTATUS_OUT,
    //---------------------- Receive Ports - RX Equalizer ----------------------
    input           RXLPMEN_IN,
    //---------------------- Receive Ports - RX PLL Ports ----------------------
    output          RXRESETDONE_OUT,
    //----------------------------- Transmit Ports -----------------------------
    input           TXUSERRDY_IN,
    //------------ Transmit Ports - 64b66b and 64b67b Gearbox Ports ------------
    input   [1:0]   TXHEADER_IN,
    input   [6:0]   TXSEQUENCE_IN,
    //---------------- Transmit Ports - TX Data Path interface -----------------
    input           GTTXRESET_IN,
    input   [31:0]  TXDATA_IN,
    output          TXOUTCLK_OUT,
    output          TXOUTCLKFABRIC_OUT,
    output          TXOUTCLKPCS_OUT,
    input           TXPCSRESET_IN,
    input           TXUSRCLK_IN,
    input           TXUSRCLK2_IN,
    //-------------- Transmit Ports - TX Driver and OOB signaling --------------
    output          GTXTXN_OUT,
    output          GTXTXP_OUT,
    input           TXINHIBIT_IN,
    input  [4:0]    TXPRECURSOR_IN,
    input  [4:0]    TXPOSTCURSOR_IN,
    input  [6:0]    TXMAINCURSOR_IN,
    //--------------------- Transmit Ports - TX PLL Ports ----------------------
    output          TXRESETDONE_OUT,
    //------------------- Transmit Ports - TX PRBS Generator -------------------
    input   [2:0]   TXPRBSSEL_IN


);


//***************************** Wire Declarations *****************************

    // ground and vcc signals
    wire            tied_to_ground_i;
    wire    [63:0]  tied_to_ground_vec_i;
    wire            tied_to_vcc_i;
    wire    [63:0]  tied_to_vcc_vec_i;


    //RX Datapath signals
    wire    [63:0]  rxdata_i;
    wire    [5:0]   rxchariscomma_float_i;
    wire    [5:0]   rxcharisk_float_i;
    wire    [5:0]   rxdisperr_float_i;
    wire    [5:0]   rxnotintable_float_i;
    wire    [5:0]   rxrundisp_float_i;


    //TX Datapath signals
    wire    [63:0]  txdata_i;           
    wire    [5:0]   txkerr_float_i;
    wire    [5:0]   txrundisp_float_i;
        
// 
//********************************* Main Body of Code**************************
                       
    //-------------------------  Static signal Assigments ---------------------   

    assign tied_to_ground_i             = 1'b0;
    assign tied_to_ground_vec_i         = 64'h0000000000000000;
    assign tied_to_vcc_i                = 1'b1;
    assign tied_to_vcc_vec_i            = 64'hffffffffffffffff;
    
    //-------------------  GT Datapath byte mapping  -----------------

    //The GT deserializes the rightmost parallel bit (LSb) first
    assign  RXDATA_OUT    =   rxdata_i[31:0];

    //The GT serializes the rightmost parallel bit (LSb) first
            assign txdata_i =   {tied_to_ground_vec_i[31:0], TXDATA_IN};
    



    //------------------------- GT Instantiations  --------------------------
        GTXE2_CHANNEL #
        (
            //_______________________ Simulation-Only Attributes __________________
    
            .SIM_RECEIVER_DETECT_PASS   ("TRUE"),
            .SIM_TX_EIDLE_DRIVE_LEVEL   ("X"), 
            .SIM_RESET_SPEEDUP          (GT_SIM_GTRESET_SPEEDUP),
            .SIM_CPLLREFCLK_SEL         (3'b001),
            .SIM_VERSION                (SIM_VERSION),
            

           //----------------RX Byte and Word Alignment Attributes---------------
            .ALIGN_COMMA_DOUBLE                     ("FALSE"),
            .ALIGN_COMMA_ENABLE                     (10'b1111111111),
            .ALIGN_COMMA_WORD                       (1),
            .ALIGN_MCOMMA_DET                       ("FALSE"),
            .ALIGN_MCOMMA_VALUE                     (10'b1010000011),
            .ALIGN_PCOMMA_DET                       ("FALSE"),
            .ALIGN_PCOMMA_VALUE                     (10'b0101111100),
            .SHOW_REALIGN_COMMA                     ("TRUE"),
            .RXSLIDE_AUTO_WAIT                      (7),
            .RXSLIDE_MODE                           ("OFF"),
            .RX_SIG_VALID_DLY                       (10),

           //----------------RX 8B/10B Decoder Attributes---------------
            .RX_DISPERR_SEQ_MATCH                   ("TRUE"),
            .DEC_MCOMMA_DETECT                      ("FALSE"),
            .DEC_PCOMMA_DETECT                      ("FALSE"),
            .DEC_VALID_COMMA_ONLY                   ("FALSE"),

           //----------------------RX Clock Correction Attributes----------------------
            .CBCC_DATA_SOURCE_SEL                   ("DECODED"),
            .CLK_COR_SEQ_2_USE                      ("FALSE"),
            .CLK_COR_KEEP_IDLE                      ("FALSE"),
            .CLK_COR_MAX_LAT                        (19),
            .CLK_COR_MIN_LAT                        (15),
            .CLK_COR_PRECEDENCE                     ("TRUE"),
            .CLK_COR_REPEAT_WAIT                    (0),
            .CLK_COR_SEQ_LEN                        (1),
            .CLK_COR_SEQ_1_ENABLE                   (4'b1111),
            .CLK_COR_SEQ_1_1                        (10'b0000000000),
            .CLK_COR_SEQ_1_2                        (10'b0000000000),
            .CLK_COR_SEQ_1_3                        (10'b0000000000),
            .CLK_COR_SEQ_1_4                        (10'b0000000000),
            .CLK_CORRECT_USE                        ("FALSE"),
            .CLK_COR_SEQ_2_ENABLE                   (4'b1111),
            .CLK_COR_SEQ_2_1                        (10'b0000000000),
            .CLK_COR_SEQ_2_2                        (10'b0000000000),
            .CLK_COR_SEQ_2_3                        (10'b0000000000),
            .CLK_COR_SEQ_2_4                        (10'b0000000000),

           //----------------------RX Channel Bonding Attributes----------------------
            .CHAN_BOND_KEEP_ALIGN                   ("FALSE"),
            .CHAN_BOND_MAX_SKEW                     (1),
            .CHAN_BOND_SEQ_LEN                      (1),
            .CHAN_BOND_SEQ_1_1                      (10'b0000000000),
            .CHAN_BOND_SEQ_1_2                      (10'b0000000000),
            .CHAN_BOND_SEQ_1_3                      (10'b0000000000),
            .CHAN_BOND_SEQ_1_4                      (10'b0000000000),
            .CHAN_BOND_SEQ_1_ENABLE                 (4'b1111),
            .CHAN_BOND_SEQ_2_1                      (10'b0000000000),
            .CHAN_BOND_SEQ_2_2                      (10'b0000000000),
            .CHAN_BOND_SEQ_2_3                      (10'b0000000000),
            .CHAN_BOND_SEQ_2_4                      (10'b0000000000),
            .CHAN_BOND_SEQ_2_ENABLE                 (4'b1111),
            .CHAN_BOND_SEQ_2_USE                    ("FALSE"),
            .FTS_DESKEW_SEQ_ENABLE                  (4'b1111),
            .FTS_LANE_DESKEW_CFG                    (4'b1111),
            .FTS_LANE_DESKEW_EN                     ("FALSE"),

           //-------------------------RX Margin Analysis Attributes----------------------------
            .ES_CONTROL                             (6'b000000),
            .ES_ERRDET_EN                           ("FALSE"),
            .ES_EYE_SCAN_EN                         ("TRUE"),
            .ES_HORZ_OFFSET                         (12'h000),
            .ES_PMA_CFG                             (10'b0000000000),
            .ES_PRESCALE                            (5'b00000),
            .ES_QUALIFIER                           (80'h00000000000000000000),
            .ES_QUAL_MASK                           (80'h00000000000000000000),
            .ES_SDATA_MASK                          (80'h00000000000000000000),
            .ES_VERT_OFFSET                         (9'b000000000),

           //-----------------------FPGA RX Interface Attributes-------------------------
            .RX_DATA_WIDTH                          (32),

           //-------------------------PMA Attributes----------------------------
            .OUTREFCLK_SEL_INV                      (2'b11),
            .PMA_RSV                                (PMA_RSV_IN),
            .PMA_RSV2                               (16'h2050),
            .PMA_RSV3                               (2'b00),
            .PMA_RSV4                               (32'h00000000),
            .RX_BIAS_CFG                            (12'b000000000100),
            .DMONITOR_CFG                           (24'h000A00),
            .RX_CM_SEL                              (2'b11),
            .RX_CM_TRIM                             (3'b010),
            .RX_DEBUG_CFG                           (12'b000000000000),
            .RX_OS_CFG                              (13'b0000010000000),
            .TERM_RCAL_CFG                          (5'b10000),
            .TERM_RCAL_OVRD                         (1'b0),
            .TST_RSV                                (32'h00000000),
            .RX_CLK25_DIV                           (7),
            .TX_CLK25_DIV                           (7),
            .UCODEER_CLR                            (1'b0),

           //-------------------------PCI Express Attributes----------------------------
            .PCS_PCIE_EN                            ("FALSE"),

           //-------------------------PCS Attributes----------------------------
            .PCS_RSVD_ATTR                          (PCS_RSVD_ATTR_IN),

           //-----------RX Buffer Attributes------------
            .RXBUF_ADDR_MODE                        ("FAST"),
            .RXBUF_EIDLE_HI_CNT                     (4'b1000),
            .RXBUF_EIDLE_LO_CNT                     (4'b0000),
            .RXBUF_EN                               ("TRUE"),
            .RX_BUFFER_CFG                          (6'b000000),
            .RXBUF_RESET_ON_CB_CHANGE               ("TRUE"),
            .RXBUF_RESET_ON_COMMAALIGN              ("FALSE"),
            .RXBUF_RESET_ON_EIDLE                   ("FALSE"),
            .RXBUF_RESET_ON_RATE_CHANGE             ("TRUE"),
            .RXBUFRESET_TIME                        (5'b00001),
            .RXBUF_THRESH_OVFLW                     (61),
            .RXBUF_THRESH_OVRD                      ("FALSE"),
            .RXBUF_THRESH_UNDFLW                    (4),
            .RXDLY_CFG                              (16'h001F),
            .RXDLY_LCFG                             (9'h030),
            .RXDLY_TAP_CFG                          (16'h0000),
            .RXPH_CFG                               (24'h000000),
            .RXPHDLY_CFG                            (24'h084020),
            .RXPH_MONITOR_SEL                       (5'b00000),
            .RX_XCLK_SEL                            ("RXREC"),
            .RX_DDI_SEL                             (6'b000000),
            .RX_DEFER_RESET_BUF_EN                  ("TRUE"),

           //---------------------CDR Attributes-------------------------
            .RXCDR_CFG                              (72'h0b000023ff10400020),
            .RXCDR_FR_RESET_ON_EIDLE                (1'b0),
            .RXCDR_HOLD_DURING_EIDLE                (1'b0),
            .RXCDR_PH_RESET_ON_EIDLE                (1'b0),
            .RXCDR_LOCK_CFG                         (6'b010101),

           //-----------------RX Initialization and Reset Attributes-------------------
            .RXCDRFREQRESET_TIME                    (5'b00001),
            .RXCDRPHRESET_TIME                      (5'b00001),
            .RXISCANRESET_TIME                      (5'b00001),
            .RXPCSRESET_TIME                        (5'b00001),
            .RXPMARESET_TIME                        (5'b00011),

           //-----------------RX OOB Signaling Attributes-------------------
            .RXOOB_CFG                              (7'b0000110),

           //-----------------------RX Gearbox Attributes---------------------------
            .RXGEARBOX_EN                           ("TRUE"),
            .GEARBOX_MODE                           (3'b001),

           //-----------------------PRBS Detection Attribute-----------------------
            .RXPRBS_ERR_LOOPBACK                    (1'b0),

           //-----------Power-Down Attributes----------
            .PD_TRANS_TIME_FROM_P2                  (12'h03c),
            .PD_TRANS_TIME_NONE_P2                  (8'h19),
            .PD_TRANS_TIME_TO_P2                    (8'h64),

           //-----------RX OOB Signaling Attributes----------
            .SAS_MAX_COM                            (64),
            .SAS_MIN_COM                            (36),
            .SATA_BURST_SEQ_LEN                     (4'b1111),
            .SATA_BURST_VAL                         (3'b100),
            .SATA_EIDLE_VAL                         (3'b100),
            .SATA_MAX_BURST                         (8),
            .SATA_MAX_INIT                          (21),
            .SATA_MAX_WAKE                          (7),
            .SATA_MIN_BURST                         (4),
            .SATA_MIN_INIT                          (12),
            .SATA_MIN_WAKE                          (4),

           //-----------RX Fabric Clock Output Control Attributes----------
            .TRANS_TIME_RATE                        (8'h0E),

           //------------TX Buffer Attributes----------------
            .TXBUF_EN                               ("TRUE"),
            .TXBUF_RESET_ON_RATE_CHANGE             ("TRUE"),
            .TXDLY_CFG                              (16'h001F),
            .TXDLY_LCFG                             (9'h030),
            .TXDLY_TAP_CFG                          (16'h0000),
            .TXPH_CFG                               (16'h0780),
            .TXPHDLY_CFG                            (24'h084020),
            .TXPH_MONITOR_SEL                       (5'b00000),
            .TX_XCLK_SEL                            ("TXOUT"),

           //-----------------------FPGA TX Interface Attributes-------------------------
            .TX_DATA_WIDTH                          (32),

           //-----------------------TX Configurable Driver Attributes-------------------------
            .TX_DEEMPH0                             (5'b00000),
            .TX_DEEMPH1                             (5'b00000),
            .TX_EIDLE_ASSERT_DELAY                  (3'b110),
            .TX_EIDLE_DEASSERT_DELAY                (3'b100),
            .TX_LOOPBACK_DRIVE_HIZ                  ("FALSE"),
            .TX_MAINCURSOR_SEL                      (1'b0),
            .TX_DRIVE_MODE                          ("DIRECT"),
            .TX_MARGIN_FULL_0                       (7'b1001110),
            .TX_MARGIN_FULL_1                       (7'b1001001),
            .TX_MARGIN_FULL_2                       (7'b1000101),
            .TX_MARGIN_FULL_3                       (7'b1000010),
            .TX_MARGIN_FULL_4                       (7'b1000000),
            .TX_MARGIN_LOW_0                        (7'b1000110),
            .TX_MARGIN_LOW_1                        (7'b1000100),
            .TX_MARGIN_LOW_2                        (7'b1000010),
            .TX_MARGIN_LOW_3                        (7'b1000000),
            .TX_MARGIN_LOW_4                        (7'b1000000),

           //-----------------------TX Gearbox Attributes--------------------------
            .TXGEARBOX_EN                           ("TRUE"),

           //-----------------------TX Initialization and Reset Attributes--------------------------
            .TXPCSRESET_TIME                        (5'b00001),
            .TXPMARESET_TIME                        (5'b00001),

           //-----------------------TX Receiver Detection Attributes--------------------------
            .TX_RXDETECT_CFG                        (14'h1832),
            .TX_RXDETECT_REF                        (3'b100),

           //--------------------------CPLL Attributes----------------------------
            .CPLL_CFG                               (24'hBC07DC),
            .CPLL_FBDIV                             (4),
            .CPLL_FBDIV_45                          (5),
            .CPLL_INIT_CFG                          (24'h00001E),
            .CPLL_LOCK_CFG                          (16'h01E8),
            .CPLL_REFCLK_DIV                        (1),
            .RXOUT_DIV                              (1),
            .TXOUT_DIV                              (1),
            .SATA_CPLL_CFG                          ("VCO_3000MHZ"),

           //------------RX Initialization and Reset Attributes-------------
            .RXDFELPMRESET_TIME                     (7'b0001111),

           //------------RX Equalizer Attributes-------------
            .RXLPM_HF_CFG                           (14'b00000011110000),
            .RXLPM_LF_CFG                           (14'b00000011110000),
            .RX_DFE_GAIN_CFG                        (23'h020FEA),
            .RX_DFE_H2_CFG                          (12'b000000000000),
            .RX_DFE_H3_CFG                          (12'b000001000000),
            .RX_DFE_H4_CFG                          (11'b00011110000),
            .RX_DFE_H5_CFG                          (11'b00011100000),
            .RX_DFE_KL_CFG                          (13'b0000011111110),
            .RX_DFE_LPM_CFG                         (16'h0954),
            .RX_DFE_LPM_HOLD_DURING_EIDLE           (1'b0),
            .RX_DFE_UT_CFG                          (17'b10001111000000000),
            .RX_DFE_VP_CFG                          (17'b00011111100000011),

           //-----------------------Power-Down Attributes-------------------------
            .RX_CLKMUX_PD                           (1'b1),
            .TX_CLKMUX_PD                           (1'b1),

           //-----------------------FPGA RX Interface Attribute-------------------------
            .RX_INT_DATAWIDTH                       (1),

           //-----------------------FPGA TX Interface Attribute-------------------------
            .TX_INT_DATAWIDTH                       (1),

           //----------------TX Configurable Driver Attributes---------------
            .TX_QPI_STATUS_EN                       (1'b0),

           //-----------------------RX Equalizer Attributes--------------------------
            .RX_DFE_KL_CFG2                         (RX_DFE_KL_CFG2_IN),
            .RX_DFE_XYD_CFG                         (13'b0000000000000),

           //-----------------------TX Configurable Driver Attributes--------------------------
            .TX_PREDRIVER_MODE                      (1'b0)

            
        ) 
        gtxe2_i 
        (
        
        //-------------------------------- Channel ---------------------------------
        .CFGRESET                       (tied_to_ground_i),
        .CLKRSVD                        (4'b0000),
        .DMONITOROUT                    (),
        .GTRESETSEL                     (tied_to_ground_i),
        .GTRSVD                         (16'b0000000000000000),
        .QPLLCLK                        (QPLLCLK_IN),
        .QPLLREFCLK                     (QPLLREFCLK_IN),
        .RESETOVRD                      (tied_to_ground_i),
        //-------------- Channel - Dynamic Reconfiguration Port (DRP) --------------
        .DRPADDR                        (DRPADDR_IN),
        .DRPCLK                         (DRPCLK_IN),
        .DRPDI                          (DRPDI_IN),
        .DRPDO                          (DRPDO_OUT),
        .DRPEN                          (DRPEN_IN),
        .DRPRDY                         (DRPRDY_OUT),
        .DRPWE                          (DRPWE_IN),
        //----------------------- Channel - Ref Clock Ports ------------------------
        .GTGREFCLK                      (tied_to_ground_i),
        .GTNORTHREFCLK0                 (tied_to_ground_i),
        .GTNORTHREFCLK1                 (tied_to_ground_i),
        .GTREFCLK0                      (tied_to_ground_i),
        .GTREFCLK1                      (tied_to_ground_i),
        .GTREFCLKMONITOR                (),
        .GTSOUTHREFCLK0                 (tied_to_ground_i),
        .GTSOUTHREFCLK1                 (tied_to_ground_i),
        //------------------------------ Channel PLL -------------------------------
        .CPLLFBCLKLOST                  (),
        .CPLLLOCK                       (),
        .CPLLLOCKDETCLK                 (tied_to_ground_i),
        .CPLLLOCKEN                     (tied_to_vcc_i),
        .CPLLPD                         (tied_to_vcc_i),
        .CPLLREFCLKLOST                 (),
        .CPLLREFCLKSEL                  (3'b001),
        .CPLLRESET                      (tied_to_ground_i),
        //----------------------------- Eye Scan Ports -----------------------------
        .EYESCANDATAERROR               (EYESCANDATAERROR_OUT),
        .EYESCANMODE                    (tied_to_ground_i),
        .EYESCANRESET                   (tied_to_ground_i),
        .EYESCANTRIGGER                 (tied_to_ground_i),
        //---------------------- Loopback and Powerdown Ports ----------------------
        .LOOPBACK                       (LOOPBACK_IN),
        .RXPD                           (2'b00),
        .TXPD                           (2'b00),
        //--------------------------- PCS Reserved Ports ---------------------------
        .PCSRSVDIN                      (16'b0000000000000000),
        .PCSRSVDIN2                     (5'b00000),
        .PCSRSVDOUT                     (),
        //--------------------------- PMA Reserved Ports ---------------------------
        .PMARSVDIN                      (5'b00000),
        .PMARSVDIN2                     (5'b00000),
        //----------------------------- Receive Ports ------------------------------
        .RXQPIEN                        (tied_to_ground_i),
        .RXQPISENN                      (),
        .RXQPISENP                      (),
        .RXSYSCLKSEL                    (2'b11),
        .RXUSERRDY                      (RXUSERRDY_IN),
        //------------ Receive Ports - 64b66b and 64b67b Gearbox Ports -------------
        .RXDATAVALID                    (RXDATAVALID_OUT),
        .RXGEARBOXSLIP                  (RXGEARBOXSLIP_IN),
        .RXHEADER                       ({rxheader_float_i,RXHEADER_OUT}),
        .RXHEADERVALID                  (RXHEADERVALID_OUT),
        .RXSTARTOFSEQ                   (),
        //--------------------- Receive Ports - 8b10b Decoder ----------------------
        .RX8B10BEN                      (tied_to_ground_i),
        .RXCHARISCOMMA                  (),
        .RXCHARISK                      (),
        .RXDISPERR                      (),
        .RXNOTINTABLE                   (),
        //----------------- Receive Ports - Channel Bonding Ports ------------------
        .RXCHANBONDSEQ                  (),
        .RXCHBONDEN                     (tied_to_ground_i),
        .RXCHBONDI                      (5'b00000),
        .RXCHBONDLEVEL                  (tied_to_ground_vec_i[2:0]),
        .RXCHBONDMASTER                 (tied_to_ground_i),
        .RXCHBONDO                      (),
        .RXCHBONDSLAVE                  (tied_to_ground_i),
        //----------------- Receive Ports - Channel Bonding Ports  -----------------
        .RXCHANISALIGNED                (),
        .RXCHANREALIGN                  (),
        //----------------- Receive Ports - Clock Correction Ports -----------------
        .RXCLKCORCNT                    (),
        //------------- Receive Ports - Comma Detection and Alignment --------------
        .RXBYTEISALIGNED                (),
        .RXBYTEREALIGN                  (),
        .RXCOMMADET                     (),
        .RXCOMMADETEN                   (tied_to_ground_i),
        .RXMCOMMAALIGNEN                (tied_to_ground_i),
        .RXPCOMMAALIGNEN                (tied_to_ground_i),
        .RXSLIDE                        (RXGEARBOXSLIP_IN),
        //--------------------- Receive Ports - PRBS Detection ---------------------
        .RXPRBSCNTRESET                 (RXPRBSCNTRESET_IN),
        .RXPRBSERR                      (RXPRBSERR_OUT),
        .RXPRBSSEL                      (RXPRBSSEL_IN),
        //----------------- Receive Ports - RX Data Path interface -----------------
        .GTRXRESET                      (GTRXRESET_IN),
        .RXDATA                         (rxdata_i),
        .RXOUTCLK                       (RXOUTCLK_OUT),
        .RXOUTCLKFABRIC                 (),
        .RXOUTCLKPCS                    (),
        .RXOUTCLKSEL                    (3'b010),
        .RXPCSRESET                     (RXPCSRESET_IN),
        .RXPMARESET                     (tied_to_ground_i),
        .RXUSRCLK                       (RXUSRCLK_IN),
        .RXUSRCLK2                      (RXUSRCLK2_IN),
        //---------- Receive Ports - RX Decision Feedback Equalizer(DFE) -----------
        .RXDFEAGCHOLD                   (tied_to_ground_i),
        .RXDFEAGCOVRDEN                 (tied_to_ground_i),
        .RXDFECM1EN                     (tied_to_ground_i),
        .RXDFELFHOLD                    (tied_to_ground_i),
        .RXDFELFOVRDEN                  (tied_to_vcc_i),
        .RXDFELPMRESET                  (tied_to_ground_i),
        .RXDFETAP2HOLD                  (tied_to_ground_i),
        .RXDFETAP2OVRDEN                (tied_to_ground_i),
        .RXDFETAP3HOLD                  (tied_to_ground_i),
        .RXDFETAP3OVRDEN                (tied_to_ground_i),
        .RXDFETAP4HOLD                  (tied_to_ground_i),
        .RXDFETAP4OVRDEN                (tied_to_ground_i),
        .RXDFETAP5HOLD                  (tied_to_ground_i),
        .RXDFETAP5OVRDEN                (tied_to_ground_i),
        .RXDFEUTHOLD                    (tied_to_ground_i),
        .RXDFEUTOVRDEN                  (tied_to_ground_i),
        .RXDFEVPHOLD                    (tied_to_ground_i),
        .RXDFEVPOVRDEN                  (tied_to_ground_i),
        .RXDFEVSEN                      (tied_to_ground_i),
        .RXDFEXYDEN                     (tied_to_ground_i),
        .RXDFEXYDHOLD                   (tied_to_ground_i),
        .RXDFEXYDOVRDEN                 (tied_to_ground_i),
        .RXMONITOROUT                   (),
        .RXMONITORSEL                   (2'b00),
        .RXOSHOLD                       (tied_to_ground_i),
        .RXOSOVRDEN                     (tied_to_ground_i),
        //----- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
        .GTXRXN                         (GTXRXN_IN),
        .GTXRXP                         (GTXRXP_IN),
        .RXCDRFREQRESET                 (tied_to_ground_i),
        .RXCDRHOLD                      (tied_to_ground_i),
        .RXCDRLOCK                      (RXCDRLOCK_OUT),
        .RXCDROVRDEN                    (tied_to_ground_i),
        .RXCDRRESET                     (tied_to_ground_i),
        .RXCDRRESETRSV                  (tied_to_ground_i),
        .RXELECIDLE                     (RXELECIDLE_OUT),
        .RXELECIDLEMODE                 (2'b11),
        .RXLPMHFHOLD                    (tied_to_ground_i),
        .RXLPMHFOVRDEN                  (tied_to_ground_i),
        .RXLPMLFHOLD                    (tied_to_ground_i),
        .RXLPMLFKLOVRDEN                (tied_to_ground_i),
        .RXOOBRESET                     (tied_to_ground_i),
        //------ Receive Ports - RX Elastic Buffer and Phase Alignment Ports -------
        .RXBUFRESET                     (RXBUFRESET_IN),
        .RXBUFSTATUS                    (RXBUFSTATUS_OUT),
        .RXDDIEN                        (tied_to_ground_i),
        .RXDLYBYPASS                    (tied_to_vcc_i),
        .RXDLYEN                        (tied_to_ground_i),
        .RXDLYOVRDEN                    (tied_to_ground_i),
        .RXDLYSRESET                    (tied_to_ground_i),
        .RXDLYSRESETDONE                (),
        .RXPHALIGN                      (tied_to_ground_i),
        .RXPHALIGNDONE                  (),
        .RXPHALIGNEN                    (tied_to_ground_i),
        .RXPHDLYPD                      (tied_to_ground_i),
        .RXPHDLYRESET                   (tied_to_ground_i),
        .RXPHMONITOR                    (),
        .RXPHOVRDEN                     (tied_to_ground_i),
        .RXPHSLIPMONITOR                (),
        .RXSTATUS                       (),
        //---------------------- Receive Ports - RX Equalizer ----------------------
        .RXLPMEN                        (RXLPMEN_IN),
        //---------------------- Receive Ports - RX PLL Ports ----------------------
        .RXRATE                         (tied_to_ground_vec_i[2:0]),
        .RXRATEDONE                     (),
        .RXRESETDONE                    (RXRESETDONE_OUT),
        //------------ Receive Ports - RX Pipe Control for PCI Express -------------
        .PHYSTATUS                      (),
        .RXVALID                        (),
        //--------------- Receive Ports - RX Polarity Control Ports ----------------
        .RXPOLARITY                     (tied_to_ground_i),
        //------------------- Receive Ports - RX Ports for SATA --------------------
        .RXCOMINITDET                   (),
        .RXCOMSASDET                    (),
        .RXCOMWAKEDET                   (),
        //----------------------------- Transmit Ports -----------------------------
        .SETERRSTATUS                   (tied_to_ground_i),
        .TSTIN                          (20'b11111111111111111111),
        .TSTOUT                         (),
        .TXPHDLYTSTCLK                  (tied_to_ground_i),
        .TXPOSTCURSOR                   (TXPOSTCURSOR_IN),
        .TXPOSTCURSORINV                (tied_to_ground_i),
        .TXPRECURSOR                    (TXPRECURSOR_IN),
        .TXPRECURSORINV                 (tied_to_ground_i),
        .TXQPIBIASEN                    (tied_to_ground_i),
        .TXQPISENN                      (),
        .TXQPISENP                      (),
        .TXQPISTRONGPDOWN               (tied_to_ground_i),
        .TXQPIWEAKPUP                   (tied_to_ground_i),
        .TXSYSCLKSEL                    (2'b11),
        .TXUSERRDY                      (TXUSERRDY_IN),
        //------------ Transmit Ports - 64b66b and 64b67b Gearbox Ports ------------
        .TXGEARBOXREADY                 (),
        .TXHEADER                       ({tied_to_ground_i,TXHEADER_IN}),
        .TXSEQUENCE                     (TXSEQUENCE_IN),
        .TXSTARTSEQ                     (tied_to_ground_i),
        //-------------- Transmit Ports - 8b10b Encoder Control Ports --------------
        .TX8B10BBYPASS                  (tied_to_ground_vec_i[7:0]),
        .TX8B10BEN                      (tied_to_ground_i),
        .TXCHARDISPMODE                 (tied_to_ground_vec_i[7:0]),
        .TXCHARDISPVAL                  (tied_to_ground_vec_i[7:0]),
        .TXCHARISK                      (tied_to_ground_vec_i[7:0]),
        //---------- Transmit Ports - TX Buffer and Phase Alignment Ports ----------
        .TXBUFSTATUS                    (),
        .TXDLYBYPASS                    (tied_to_vcc_i),
        .TXDLYEN                        (tied_to_ground_i),
        .TXDLYHOLD                      (tied_to_ground_i),
        .TXDLYOVRDEN                    (tied_to_ground_i),
        .TXDLYSRESET                    (tied_to_ground_i),
        .TXDLYSRESETDONE                (),
        .TXDLYUPDOWN                    (tied_to_ground_i),
        .TXPHALIGN                      (tied_to_ground_i),
        .TXPHALIGNDONE                  (),
        .TXPHALIGNEN                    (tied_to_ground_i),
        .TXPHDLYPD                      (tied_to_ground_i),
        .TXPHDLYRESET                   (tied_to_ground_i),
        .TXPHINIT                       (tied_to_ground_i),
        .TXPHINITDONE                   (),
        .TXPHOVRDEN                     (tied_to_ground_i),
        //---------------- Transmit Ports - TX Data Path interface -----------------
        .GTTXRESET                      (GTTXRESET_IN),
        .TXDATA                         (txdata_i),
        .TXOUTCLK                       (TXOUTCLK_OUT),
        .TXOUTCLKFABRIC                 (TXOUTCLKFABRIC_OUT),
        .TXOUTCLKPCS                    (TXOUTCLKPCS_OUT),
        .TXOUTCLKSEL                    (3'b010),
        .TXPCSRESET                     (TXPCSRESET_IN),
        .TXPMARESET                     (tied_to_ground_i),
        .TXUSRCLK                       (TXUSRCLK_IN),
        .TXUSRCLK2                      (TXUSRCLK2_IN),
        //-------------- Transmit Ports - TX Driver and OOB signaling --------------
        .GTXTXN                         (GTXTXN_OUT),
        .GTXTXP                         (GTXTXP_OUT),
        .TXBUFDIFFCTRL                  (3'b100),
        .TXDIFFCTRL                     (4'b1110),
        .TXDIFFPD                       (tied_to_ground_i),
        .TXINHIBIT                      (TXINHIBIT_IN),
        .TXMAINCURSOR                   (TXMAINCURSOR_IN),
        .TXPDELECIDLEMODE               (tied_to_ground_i),
        .TXPISOPD                       (tied_to_ground_i),
        //--------------------- Transmit Ports - TX PLL Ports ----------------------
        .TXRATE                         (tied_to_ground_vec_i[2:0]),
        .TXRATEDONE                     (),
        .TXRESETDONE                    (TXRESETDONE_OUT),
        //------------------- Transmit Ports - TX PRBS Generator -------------------
        .TXPRBSFORCEERR                 (tied_to_ground_i),
        .TXPRBSSEL                      (TXPRBSSEL_IN),
        //------------------ Transmit Ports - TX Polarity Control ------------------
        .TXPOLARITY                     (tied_to_ground_i),
        //--------------- Transmit Ports - TX Ports for PCI Express ----------------
        .TXDEEMPH                       (tied_to_ground_i),
        .TXDETECTRX                     (tied_to_ground_i),
        .TXELECIDLE                     (tied_to_ground_i),
        .TXMARGIN                       (tied_to_ground_vec_i[2:0]),
        .TXSWING                        (tied_to_ground_i),
        //------------------- Transmit Ports - TX Ports for SATA -------------------
        .TXCOMFINISH                    (),
        .TXCOMINIT                      (tied_to_ground_i),
        .TXCOMSAS                       (tied_to_ground_i),
        .TXCOMWAKE                      (tied_to_ground_i)

     );
     
endmodule     

 
