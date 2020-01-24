library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library UNISIM;
use UNISIM.VCOMPONENTS.all;

--***************************** Entity Declaration ****************************

entity WHITERABBITGTP_WRAPPER_TILE_SPARTAN6 is
  generic
    (
      TILE_SIM_GTPRESET_SPEEDUP : integer := 0;  -- Set to 1 to speed up sim reset
      TILE_CLK25_DIVIDER_0      : integer := 4;
      TILE_CLK25_DIVIDER_1      : integer := 4;
      TILE_PLL_DIVSEL_FB_0      : integer := 5;
      TILE_PLL_DIVSEL_FB_1      : integer := 5;
      TILE_PLL_DIVSEL_REF_0     : integer := 2;
      TILE_PLL_DIVSEL_REF_1     : integer := 2;
      TILE_PLL_SOURCE_0         : string  := "PLL0";
      TILE_PLL_SOURCE_1         : string  := "PLL1"
      );
  port
    (
      ------------------------ Loopback and Powerdown Ports ----------------------
      LOOPBACK0_IN         : in  std_logic_vector(2 downto 0);
      LOOPBACK1_IN         : in  std_logic_vector(2 downto 0);
      REFCLKOUT0_OUT           : out std_logic;
      REFCLKOUT1_OUT           : out std_logic;
      --------------------------------- PLL Ports --------------------------------
      CLK00_IN             : in  std_logic;
      CLK01_IN             : in  std_logic;
      CLK10_IN             : in  std_logic;
      CLK11_IN             : in  std_logic;
      GTPRESET0_IN         : in  std_logic;
      GTPRESET1_IN         : in  std_logic;
      PLLLKDET0_OUT        : out std_logic;
      PLLLKDET1_OUT        : out std_logic;
      RESETDONE0_OUT       : out std_logic;
      RESETDONE1_OUT       : out std_logic;
      ----------------------- Receive Ports - 8b10b Decoder ----------------------
      RXCHARISK0_OUT       : out std_logic;
      RXCHARISK1_OUT       : out std_logic;
      RXDISPERR0_OUT       : out std_logic;
      RXDISPERR1_OUT       : out std_logic;
      RXNOTINTABLE0_OUT    : out std_logic;
      RXNOTINTABLE1_OUT    : out std_logic;
      --------------- Receive Ports - Comma Detection and Alignment --------------
      RXBYTEISALIGNED0_OUT : out std_logic;
      RXBYTEISALIGNED1_OUT : out std_logic;
      RXCOMMADET0_OUT      : out std_logic;
      RXCOMMADET1_OUT      : out std_logic;
      RXSLIDE0_IN          : in  std_logic;
      RXSLIDE1_IN          : in  std_logic;
      ------------------- Receive Ports - RX Data Path interface -----------------
      RXDATA0_OUT          : out std_logic_vector(7 downto 0);
      RXDATA1_OUT          : out std_logic_vector(7 downto 0);
      RXUSRCLK0_IN         : in  std_logic;
      RXUSRCLK1_IN         : in  std_logic;
      RXUSRCLK20_IN        : in  std_logic;
      RXUSRCLK21_IN        : in  std_logic;
      ------- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
      RXCDRRESET0_IN       : in  std_logic;
      RXCDRRESET1_IN       : in  std_logic;
      RXN0_IN              : in  std_logic;
      RXN1_IN              : in  std_logic;
      RXP0_IN              : in  std_logic;
      RXP1_IN              : in  std_logic;
      ---------------------------- TX/RX Datapath Ports --------------------------
      GTPCLKFBEAST_OUT     : out std_logic_vector(1 downto 0);
      GTPCLKFBWEST_OUT     : out std_logic_vector(1 downto 0);
      GTPCLKOUT0_OUT       : out std_logic_vector(1 downto 0);
      GTPCLKOUT1_OUT       : out std_logic_vector(1 downto 0);
      ------------------- Transmit Ports - 8b10b Encoder Control -----------------
      TXCHARISK0_IN        : in  std_logic;
      TXCHARISK1_IN        : in  std_logic;

      TXCHARDISPMODE0_IN: in std_logic;
      TXCHARDISPMODE1_IN: in std_logic;

      TXCHARDISPVAL0_IN: in std_logic;
      TXCHARDISPVAL1_IN: in std_logic;

      TXRUNDISP0_OUT       : out std_logic_vector(3 downto 0);
      TXRUNDISP1_OUT       : out std_logic_vector(3 downto 0);

      --------------- Transmit Ports - TX Buffer and Phase Alignment -------------
      TXENPMAPHASEALIGN0_IN : in  std_logic;
      TXENPMAPHASEALIGN1_IN : in  std_logic;
      TXPMASETPHASE0_IN     : in  std_logic;
      TXPMASETPHASE1_IN     : in  std_logic;
      ------------------ Transmit Ports - TX Data Path interface -----------------
      TXDATA0_IN            : in  std_logic_vector(7 downto 0);
      TXDATA1_IN            : in  std_logic_vector(7 downto 0);
      TXUSRCLK0_IN          : in  std_logic;
      TXUSRCLK1_IN          : in  std_logic;
      TXUSRCLK20_IN         : in  std_logic;
      TXUSRCLK21_IN         : in  std_logic;
      --------------- Transmit Ports - TX Driver and OOB signalling --------------
      TXN0_OUT              : out std_logic;
      TXN1_OUT              : out std_logic;
      TXP0_OUT              : out std_logic;
      TXP1_OUT              : out std_logic;
      --------------- Transmit Ports - PRBS Generator ----------------------------
      TXENPRBSTST0_IN       : in  std_logic_vector(2 downto 0);
      TXENPRBSTST1_IN       : in  std_logic_vector(2 downto 0)

      );


end WHITERABBITGTP_WRAPPER_TILE_SPARTAN6;

architecture RTL of WHITERABBITGTP_WRAPPER_TILE_SPARTAN6 is

--**************************** Signal Declarations ****************************

  -- ground and tied_to_vcc_i signals
  signal tied_to_ground_i     : std_logic;
  signal tied_to_ground_vec_i : std_logic_vector(63 downto 0);
  signal tied_to_vcc_i        : std_logic;
  signal tied_to_vcc_vec_i    : std_logic_vector(63 downto 0);



  -- RX Datapath signals
  signal rxdata0_i              : std_logic_vector(31 downto 0);
  signal rxchariscomma0_float_i : std_logic_vector(2 downto 0);
  signal rxcharisk0_float_i     : std_logic_vector(2 downto 0);
  signal rxdisperr0_float_i     : std_logic_vector(2 downto 0);
  signal rxnotintable0_float_i  : std_logic_vector(2 downto 0);
  signal rxrundisp0_float_i     : std_logic_vector(2 downto 0);


  -- TX Datapath signals
  signal txdata0_i          : std_logic_vector(31 downto 0);
  signal txkerr0_float_i    : std_logic_vector(2 downto 0);
  signal txrundisp0_float_i : std_logic_vector(2 downto 0);


  -- RX Datapath signals
  signal rxdata1_i              : std_logic_vector(31 downto 0);
  signal rxchariscomma1_float_i : std_logic_vector(2 downto 0);
  signal rxcharisk1_float_i     : std_logic_vector(2 downto 0);
  signal rxdisperr1_float_i     : std_logic_vector(2 downto 0);
  signal rxnotintable1_float_i  : std_logic_vector(2 downto 0);
  signal rxrundisp1_float_i     : std_logic_vector(2 downto 0);


  -- TX Datapath signals
  signal txdata1_i          : std_logic_vector(31 downto 0);
  signal txkerr1_float_i    : std_logic_vector(2 downto 0);
  signal txrundisp1_float_i : std_logic_vector(2 downto 0);

--******************************** Main Body of Code***************************
  
begin

  ---------------------------  Static signal Assignments ---------------------   

  tied_to_ground_i                  <= '0';
  tied_to_ground_vec_i(63 downto 0) <= (others => '0');
  tied_to_vcc_i                     <= '1';
  tied_to_vcc_vec_i(63 downto 0)    <= (others => '1');

  -------------------  GTP Datapath byte mapping  -----------------    

  RXDATA0_OUT <= rxdata0_i(7 downto 0);

  txdata0_i <= (tied_to_ground_vec_i(23 downto 0) & TXDATA0_IN);

  RXDATA1_OUT <= rxdata1_i(7 downto 0);

  txdata1_i <= (tied_to_ground_vec_i(23 downto 0) & TXDATA1_IN);



  ----------------------------- GTPA1_DUAL Instance  --------------------------   

  gtpa1_dual_i : GTPA1_DUAL
    generic map
    (

      --_______________________ Simulation-Only Attributes ___________________

      SIM_RECEIVER_DETECT_PASS => (true),
      SIM_TX_ELEC_IDLE_LEVEL   => ("X"),
      SIM_VERSION              => ("2.0"),
      SIM_REFCLK0_SOURCE       => ("000"),
      SIM_REFCLK1_SOURCE       => ("000"),
      SIM_GTPRESET_SPEEDUP     => (TILE_SIM_GTPRESET_SPEEDUP),
      CLK25_DIVIDER_0          => (TILE_CLK25_DIVIDER_0),
      CLK25_DIVIDER_1          => (TILE_CLK25_DIVIDER_1),
      PLL_DIVSEL_FB_0          => (TILE_PLL_DIVSEL_FB_0),
      PLL_DIVSEL_FB_1          => (TILE_PLL_DIVSEL_FB_1),
      PLL_DIVSEL_REF_0         => (TILE_PLL_DIVSEL_REF_0),
      PLL_DIVSEL_REF_1         => (TILE_PLL_DIVSEL_REF_1),


      --PLL Attributes
      CLKINDC_B_0        => (true),
      CLKRCV_TRST_0      => (true),
      OOB_CLK_DIVIDER_0  => (4),
      PLL_COM_CFG_0      => (x"21680a"),
      PLL_CP_CFG_0       => (x"00"),
      PLL_RXDIVSEL_OUT_0 => (2),
      PLL_SATA_0         => (false),
      PLL_SOURCE_0       => (TILE_PLL_SOURCE_0),
      PLL_TXDIVSEL_OUT_0 => (2),
      PLLLKDET_CFG_0     => ("111"),

      --
      CLKINDC_B_1        => (true),
      CLKRCV_TRST_1      => (true),
      OOB_CLK_DIVIDER_1  => (4),
      PLL_COM_CFG_1      => (x"21680a"),
      PLL_CP_CFG_1       => (x"00"),
      PLL_RXDIVSEL_OUT_1 => (2),
      PLL_SATA_1         => (false),
      PLL_SOURCE_1       => (TILE_PLL_SOURCE_1),
      PLL_TXDIVSEL_OUT_1 => (2),
      PLLLKDET_CFG_1     => ("111"),
      PMA_COM_CFG_EAST   => (x"000008000"),
      PMA_COM_CFG_WEST   => (x"000008000"),
      TST_ATTR_0         => (x"00000000"),
      TST_ATTR_1         => (x"00000000"),

      --TX Interface Attributes
      CLK_OUT_GTP_SEL_0 => ("REFCLKPLL0"),
      TX_TDCC_CFG_0     => ("00"),
      CLK_OUT_GTP_SEL_1 => ("REFCLKPLL1"),
      TX_TDCC_CFG_1     => ("00"),

      --TX Buffer and Phase Alignment Attributes
      PMA_TX_CFG_0    => (x"80082"),
      TX_BUFFER_USE_0 => (false),
      TX_XCLK_SEL_0   => ("TXUSR"),
      TXRX_INVERT_0   => ("111"),
      PMA_TX_CFG_1    => (x"80082"),
      TX_BUFFER_USE_1 => (false),
      TX_XCLK_SEL_1   => ("TXUSR"),
      TXRX_INVERT_1   => ("111"),

      --TX Driver and OOB signalling Attributes
      CM_TRIM_0       => ("00"),
      TX_IDLE_DELAY_0 => ("011"),
      CM_TRIM_1       => ("00"),
      TX_IDLE_DELAY_1 => ("011"),

      --TX PIPE/SATA Attributes
      COM_BURST_VAL_0 => ("1111"),
      COM_BURST_VAL_1 => ("1111"),

      --RX Driver,OOB signalling,Coupling and Eq,CDR Attributes
      AC_CAP_DIS_0          => (true),
      OOBDETECT_THRESHOLD_0 => ("110"),
      PMA_CDR_SCAN_0        => (x"6404040"),
      PMA_RX_CFG_0          => (x"05ce048"),
      PMA_RXSYNC_CFG_0      => (x"00"),
      RCV_TERM_GND_0        => (false),
      RCV_TERM_VTTRX_0      => (false),
      RXEQ_CFG_0            => ("01111011"),
      TERMINATION_CTRL_0    => ("10100"),
      TERMINATION_OVRD_0    => (false),
      TX_DETECT_RX_CFG_0    => (x"1832"),
      AC_CAP_DIS_1          => (true),
      OOBDETECT_THRESHOLD_1 => ("110"),
      PMA_CDR_SCAN_1        => (x"6404040"),
      PMA_RX_CFG_1          => (x"05ce048"),
      PMA_RXSYNC_CFG_1      => (x"00"),
      RCV_TERM_GND_1        => (false),
      RCV_TERM_VTTRX_1      => (false),
      RXEQ_CFG_1            => ("01111011"),
      TERMINATION_CTRL_1    => ("10100"),
      TERMINATION_OVRD_1    => (false),
      TX_DETECT_RX_CFG_1    => (x"1832"),

      --PRBS Detection Attributes
      RXPRBSERR_LOOPBACK_0 => ('0'),
      RXPRBSERR_LOOPBACK_1 => ('0'),

      --Comma Detection and Alignment Attributes
      ALIGN_COMMA_WORD_0     => (1),
      COMMA_10B_ENABLE_0     => ("1111111111"),
      DEC_MCOMMA_DETECT_0    => (false),
      DEC_PCOMMA_DETECT_0    => (false),
      DEC_VALID_COMMA_ONLY_0 => (true),
      MCOMMA_10B_VALUE_0     => ("1010000011"),
      MCOMMA_DETECT_0        => (true),
      PCOMMA_10B_VALUE_0     => ("0101111100"),
      PCOMMA_DETECT_0        => (true),
      RX_SLIDE_MODE_0        => ("PCS"),
      ALIGN_COMMA_WORD_1     => (1),
      COMMA_10B_ENABLE_1     => ("1111111111"),
      DEC_MCOMMA_DETECT_1    => (false),
      DEC_PCOMMA_DETECT_1    => (false),
      DEC_VALID_COMMA_ONLY_1 => (true),
      MCOMMA_10B_VALUE_1     => ("1010000011"),
      MCOMMA_DETECT_1        => (true),
      PCOMMA_10B_VALUE_1     => ("0101111100"),
      PCOMMA_DETECT_1        => (true),
      RX_SLIDE_MODE_1        => ("PCS"),

      --RX Loss-of-sync State Machine Attributes
      RX_LOS_INVALID_INCR_0 => (8),
      RX_LOS_THRESHOLD_0    => (128),
      RX_LOSS_OF_SYNC_FSM_0 => (false),
      RX_LOS_INVALID_INCR_1 => (8),
      RX_LOS_THRESHOLD_1    => (128),
      RX_LOSS_OF_SYNC_FSM_1 => (false),

      --RX Elastic Buffer and Phase alignment Attributes
      RX_BUFFER_USE_0        => (true),
      RX_EN_IDLE_RESET_BUF_0 => (true),
      RX_IDLE_HI_CNT_0       => ("1000"),
      RX_IDLE_LO_CNT_0       => ("0000"),
      RX_XCLK_SEL_0          => ("RXREC"),
      RX_BUFFER_USE_1        => (true),
      RX_EN_IDLE_RESET_BUF_1 => (true),
      RX_IDLE_HI_CNT_1       => ("1000"),
      RX_IDLE_LO_CNT_1       => ("0000"),
      RX_XCLK_SEL_1          => ("RXREC"),

      --Clock Correction Attributes
      CLK_COR_ADJ_LEN_0          => (1),
      CLK_COR_DET_LEN_0          => (1),
      CLK_COR_INSERT_IDLE_FLAG_0 => (false),
      CLK_COR_KEEP_IDLE_0        => (false),
      CLK_COR_MAX_LAT_0          => (18),
      CLK_COR_MIN_LAT_0          => (16),
      CLK_COR_PRECEDENCE_0       => (true),
      CLK_COR_REPEAT_WAIT_0      => (0),
      CLK_COR_SEQ_1_1_0          => ("0100000000"),
      CLK_COR_SEQ_1_2_0          => ("0000000000"),
      CLK_COR_SEQ_1_3_0          => ("0000000000"),
      CLK_COR_SEQ_1_4_0          => ("0000000000"),
      CLK_COR_SEQ_1_ENABLE_0     => ("0000"),
      CLK_COR_SEQ_2_1_0          => ("0100000000"),
      CLK_COR_SEQ_2_2_0          => ("0000000000"),
      CLK_COR_SEQ_2_3_0          => ("0000000000"),
      CLK_COR_SEQ_2_4_0          => ("0000000000"),
      CLK_COR_SEQ_2_ENABLE_0     => ("0000"),
      CLK_COR_SEQ_2_USE_0        => (false),
      CLK_CORRECT_USE_0          => (false),
      RX_DECODE_SEQ_MATCH_0      => (true),
      CLK_COR_ADJ_LEN_1          => (1),
      CLK_COR_DET_LEN_1          => (1),
      CLK_COR_INSERT_IDLE_FLAG_1 => (false),
      CLK_COR_KEEP_IDLE_1        => (false),
      CLK_COR_MAX_LAT_1          => (18),
      CLK_COR_MIN_LAT_1          => (16),
      CLK_COR_PRECEDENCE_1       => (true),
      CLK_COR_REPEAT_WAIT_1      => (0),
      CLK_COR_SEQ_1_1_1          => ("0100000000"),
      CLK_COR_SEQ_1_2_1          => ("0000000000"),
      CLK_COR_SEQ_1_3_1          => ("0000000000"),
      CLK_COR_SEQ_1_4_1          => ("0000000000"),
      CLK_COR_SEQ_1_ENABLE_1     => ("0000"),
      CLK_COR_SEQ_2_1_1          => ("0100000000"),
      CLK_COR_SEQ_2_2_1          => ("0000000000"),
      CLK_COR_SEQ_2_3_1          => ("0000000000"),
      CLK_COR_SEQ_2_4_1          => ("0000000000"),
      CLK_COR_SEQ_2_ENABLE_1     => ("0000"),
      CLK_COR_SEQ_2_USE_1        => (false),
      CLK_CORRECT_USE_1          => (false),
      RX_DECODE_SEQ_MATCH_1      => (true),

      --Channel Bonding Attributes
      CHAN_BOND_1_MAX_SKEW_0   => (1),
      CHAN_BOND_2_MAX_SKEW_0   => (1),
      CHAN_BOND_KEEP_ALIGN_0   => (false),
      CHAN_BOND_SEQ_1_1_0      => ("0000000000"),
      CHAN_BOND_SEQ_1_2_0      => ("0000000000"),
      CHAN_BOND_SEQ_1_3_0      => ("0000000000"),
      CHAN_BOND_SEQ_1_4_0      => ("0000000000"),
      CHAN_BOND_SEQ_1_ENABLE_0 => ("0000"),
      CHAN_BOND_SEQ_2_1_0      => ("0000000000"),
      CHAN_BOND_SEQ_2_2_0      => ("0000000000"),
      CHAN_BOND_SEQ_2_3_0      => ("0000000000"),
      CHAN_BOND_SEQ_2_4_0      => ("0000000000"),
      CHAN_BOND_SEQ_2_ENABLE_0 => ("0000"),
      CHAN_BOND_SEQ_2_USE_0    => (false),
      CHAN_BOND_SEQ_LEN_0      => (1),
      RX_EN_MODE_RESET_BUF_0   => (true),
      CHAN_BOND_1_MAX_SKEW_1   => (1),
      CHAN_BOND_2_MAX_SKEW_1   => (1),
      CHAN_BOND_KEEP_ALIGN_1   => (false),
      CHAN_BOND_SEQ_1_1_1      => ("0000000000"),
      CHAN_BOND_SEQ_1_2_1      => ("0000000000"),
      CHAN_BOND_SEQ_1_3_1      => ("0000000000"),
      CHAN_BOND_SEQ_1_4_1      => ("0000000000"),
      CHAN_BOND_SEQ_1_ENABLE_1 => ("0000"),
      CHAN_BOND_SEQ_2_1_1      => ("0000000000"),
      CHAN_BOND_SEQ_2_2_1      => ("0000000000"),
      CHAN_BOND_SEQ_2_3_1      => ("0000000000"),
      CHAN_BOND_SEQ_2_4_1      => ("0000000000"),
      CHAN_BOND_SEQ_2_ENABLE_1 => ("0000"),
      CHAN_BOND_SEQ_2_USE_1    => (false),
      CHAN_BOND_SEQ_LEN_1      => (1),
      RX_EN_MODE_RESET_BUF_1   => (true),

      --RX PCI Express Attributes
      CB2_INH_CC_PERIOD_0   => (8),
      CDR_PH_ADJ_TIME_0     => ("01010"),
      PCI_EXPRESS_MODE_0    => (false),
      RX_EN_IDLE_HOLD_CDR_0 => (false),
      RX_EN_IDLE_RESET_FR_0 => (true),
      RX_EN_IDLE_RESET_PH_0 => (true),
      RX_STATUS_FMT_0       => ("PCIE"),
      TRANS_TIME_FROM_P2_0  => (x"03c"),
      TRANS_TIME_NON_P2_0   => (x"19"),
      TRANS_TIME_TO_P2_0    => (x"064"),
      CB2_INH_CC_PERIOD_1   => (8),
      CDR_PH_ADJ_TIME_1     => ("01010"),
      PCI_EXPRESS_MODE_1    => (false),
      RX_EN_IDLE_HOLD_CDR_1 => (false),
      RX_EN_IDLE_RESET_FR_1 => (true),
      RX_EN_IDLE_RESET_PH_1 => (true),
      RX_STATUS_FMT_1       => ("PCIE"),
      TRANS_TIME_FROM_P2_1  => (x"03c"),
      TRANS_TIME_NON_P2_1   => (x"19"),
      TRANS_TIME_TO_P2_1    => (x"064"),

      --RX SATA Attributes
      SATA_BURST_VAL_0 => ("100"),
      SATA_IDLE_VAL_0  => ("100"),
      SATA_MAX_BURST_0 => (9),
      SATA_MAX_INIT_0  => (27),
      SATA_MAX_WAKE_0  => (9),
      SATA_MIN_BURST_0 => (5),
      SATA_MIN_INIT_0  => (15),
      SATA_MIN_WAKE_0  => (5),
      SATA_BURST_VAL_1 => ("100"),
      SATA_IDLE_VAL_1  => ("100"),
      SATA_MAX_BURST_1 => (9),
      SATA_MAX_INIT_1  => (27),
      SATA_MAX_WAKE_1  => (9),
      SATA_MIN_BURST_1 => (5),
      SATA_MIN_INIT_1  => (15),
      SATA_MIN_WAKE_1  => (5)


      ) 
    port map
    (
      ------------------------ Loopback and Powerdown Ports ----------------------
      LOOPBACK0                 => LOOPBACK0_IN,
      LOOPBACK1                 => LOOPBACK1_IN,
      RXPOWERDOWN0              => tied_to_ground_vec_i(1 downto 0),
      RXPOWERDOWN1              => tied_to_ground_vec_i(1 downto 0),
      TXPOWERDOWN0              => tied_to_ground_vec_i(1 downto 0),
      TXPOWERDOWN1              => tied_to_ground_vec_i(1 downto 0),
      --------------------------------- PLL Ports --------------------------------
      CLK00                     => CLK00_IN,
      CLK01                     => CLK01_IN,
      CLK10                     => CLK10_IN,
      CLK11                     => CLK11_IN,
      CLKINEAST0                => tied_to_ground_i,
      CLKINEAST1                => tied_to_ground_i,
      CLKINWEST0                => tied_to_ground_i,
      CLKINWEST1                => tied_to_ground_i,
      GCLK00                    => tied_to_ground_i,
      GCLK01                    => tied_to_ground_i,
      GCLK10                    => tied_to_ground_i,
      GCLK11                    => tied_to_ground_i,
      GTPRESET0                 => GTPRESET0_IN,
      GTPRESET1                 => GTPRESET1_IN,
      GTPTEST0                  => "00010000",
      GTPTEST1                  => "00010000",
      INTDATAWIDTH0             => tied_to_vcc_i,
      INTDATAWIDTH1             => tied_to_vcc_i,
      PLLCLK00                  => tied_to_ground_i,
      PLLCLK01                  => tied_to_ground_i,
      PLLCLK10                  => tied_to_ground_i,
      PLLCLK11                  => tied_to_ground_i,
      PLLLKDET0                 => PLLLKDET0_OUT,
      PLLLKDET1                 => PLLLKDET1_OUT,
      PLLLKDETEN0               => tied_to_vcc_i,
      PLLLKDETEN1               => tied_to_vcc_i,
      PLLPOWERDOWN0             => tied_to_ground_i,
      PLLPOWERDOWN1             => tied_to_ground_i,
      REFCLKOUT0                => REFCLKOUT0_OUT,
      REFCLKOUT1                => REFCLKOUT1_OUT,
      REFCLKPLL0                => open,
      REFCLKPLL1                => open,
      REFCLKPWRDNB0             => tied_to_vcc_i,
      REFCLKPWRDNB1             => tied_to_vcc_i,
      REFSELDYPLL0              => tied_to_ground_vec_i(2 downto 0),
      REFSELDYPLL1              => tied_to_ground_vec_i(2 downto 0),
      RESETDONE0                => RESETDONE0_OUT,
      RESETDONE1                => RESETDONE1_OUT,
      TSTCLK0                   => tied_to_ground_i,
      TSTCLK1                   => tied_to_ground_i,
      TSTIN0                    => tied_to_ground_vec_i(11 downto 0),
      TSTIN1                    => tied_to_ground_vec_i(11 downto 0),
      TSTOUT0                   => open,
      TSTOUT1                   => open,
      ----------------------- Receive Ports - 8b10b Decoder ----------------------
      RXCHARISCOMMA0            => open,
      RXCHARISCOMMA1            => open,
      RXCHARISK0(3 downto 1)    => rxcharisk0_float_i,
      RXCHARISK0(0)             => RXCHARISK0_OUT,
      RXCHARISK1(3 downto 1)    => rxcharisk1_float_i,
      RXCHARISK1(0)             => RXCHARISK1_OUT,
      RXDEC8B10BUSE0            => tied_to_vcc_i,
      RXDEC8B10BUSE1            => tied_to_vcc_i,
      RXDISPERR0(3 downto 1)    => rxdisperr0_float_i,
      RXDISPERR0(0)             => RXDISPERR0_OUT,
      RXDISPERR1(3 downto 1)    => rxdisperr1_float_i,
      RXDISPERR1(0)             => RXDISPERR1_OUT,
      RXNOTINTABLE0(3 downto 1) => rxnotintable0_float_i,
      RXNOTINTABLE0(0)          => RXNOTINTABLE0_OUT,
      RXNOTINTABLE1(3 downto 1) => rxnotintable1_float_i,
      RXNOTINTABLE1(0)          => RXNOTINTABLE1_OUT,
      RXRUNDISP0                => open,
      RXRUNDISP1                => open,
      USRCODEERR0               => tied_to_ground_i,
      USRCODEERR1               => tied_to_ground_i,
      ---------------------- Receive Ports - Channel Bonding ---------------------
      RXCHANBONDSEQ0            => open,
      RXCHANBONDSEQ1            => open,
      RXCHANISALIGNED0          => open,
      RXCHANISALIGNED1          => open,
      RXCHANREALIGN0            => open,
      RXCHANREALIGN1            => open,
      RXCHBONDI                 => tied_to_ground_vec_i(2 downto 0),
      RXCHBONDMASTER0           => tied_to_ground_i,
      RXCHBONDMASTER1           => tied_to_ground_i,
      RXCHBONDO                 => open,
      RXCHBONDSLAVE0            => tied_to_ground_i,
      RXCHBONDSLAVE1            => tied_to_ground_i,
      RXENCHANSYNC0             => tied_to_ground_i,
      RXENCHANSYNC1             => tied_to_ground_i,
      ---------------------- Receive Ports - Clock Correction --------------------
      RXCLKCORCNT0              => open,
      RXCLKCORCNT1              => open,
      --------------- Receive Ports - Comma Detection and Alignment --------------
      RXBYTEISALIGNED0          => RXBYTEISALIGNED0_OUT,
      RXBYTEISALIGNED1          => RXBYTEISALIGNED1_OUT,
      RXBYTEREALIGN0            => open,
      RXBYTEREALIGN1            => open,
      RXCOMMADET0               => RXCOMMADET0_OUT,
      RXCOMMADET1               => RXCOMMADET1_OUT,
      RXCOMMADETUSE0            => tied_to_vcc_i,
      RXCOMMADETUSE1            => tied_to_vcc_i,
      RXENMCOMMAALIGN0          => tied_to_ground_i,
      RXENMCOMMAALIGN1          => tied_to_ground_i,
      RXENPCOMMAALIGN0          => tied_to_ground_i,
      RXENPCOMMAALIGN1          => tied_to_ground_i,
      RXSLIDE0                  => RXSLIDE0_IN,
      RXSLIDE1                  => RXSLIDE1_IN,
      ----------------------- Receive Ports - PRBS Detection ---------------------
      PRBSCNTRESET0             => tied_to_ground_i,
      PRBSCNTRESET1             => tied_to_ground_i,
      RXENPRBSTST0              => tied_to_ground_vec_i(2 downto 0),
      RXENPRBSTST1              => tied_to_ground_vec_i(2 downto 0),
      RXPRBSERR0                => open,
      RXPRBSERR1                => open,
      ------------------- Receive Ports - RX Data Path interface -----------------
      RXDATA0                   => rxdata0_i,
      RXDATA1                   => rxdata1_i,
      RXDATAWIDTH0              => "00",
      RXDATAWIDTH1              => "00",
      RXRECCLK0                 => open,
      RXRECCLK1                 => open,
      RXRESET0                  => tied_to_ground_i,
      RXRESET1                  => tied_to_ground_i,
      RXUSRCLK0                 => RXUSRCLK0_IN,
      RXUSRCLK1                 => RXUSRCLK1_IN,
      RXUSRCLK20                => RXUSRCLK20_IN,
      RXUSRCLK21                => RXUSRCLK21_IN,
      ------- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
      GATERXELECIDLE0           => tied_to_ground_i,
      GATERXELECIDLE1           => tied_to_ground_i,
      IGNORESIGDET0             => tied_to_ground_i,
      IGNORESIGDET1             => tied_to_ground_i,
      RCALINEAST                => tied_to_ground_vec_i(4 downto 0),
      RCALINWEST                => tied_to_ground_vec_i(4 downto 0),
      RCALOUTEAST               => open,
      RCALOUTWEST               => open,
      RXCDRRESET0               => RXCDRRESET0_IN,
      RXCDRRESET1               => RXCDRRESET1_IN,
      RXELECIDLE0               => open,
      RXELECIDLE1               => open,
      RXEQMIX0                  => "00",
      RXEQMIX1                  => "00",
      RXN0                      => RXN0_IN,
      RXN1                      => RXN1_IN,
      RXP0                      => RXP0_IN,
      RXP1                      => RXP1_IN,
      ----------- Receive Ports - RX Elastic Buffer and Phase Alignment ----------
      RXBUFRESET0               => tied_to_ground_i,
      RXBUFRESET1               => tied_to_ground_i,
      RXBUFSTATUS0              => open,
      RXBUFSTATUS1              => open,
      RXENPMAPHASEALIGN0        => tied_to_ground_i,
      RXENPMAPHASEALIGN1        => tied_to_ground_i,
      RXPMASETPHASE0            => tied_to_ground_i,
      RXPMASETPHASE1            => tied_to_ground_i,
      RXSTATUS0                 => open,
      RXSTATUS1                 => open,
      --------------- Receive Ports - RX Loss-of-sync State Machine --------------
      RXLOSSOFSYNC0             => open,
      RXLOSSOFSYNC1             => open,
      -------------- Receive Ports - RX Pipe Control for PCI Express -------------
      PHYSTATUS0                => open,
      PHYSTATUS1                => open,
      RXVALID0                  => open,
      RXVALID1                  => open,
      -------------------- Receive Ports - RX Polarity Control -------------------
      RXPOLARITY0               => tied_to_ground_i,
      RXPOLARITY1               => tied_to_ground_i,
      ------------- Shared Ports - Dynamic Reconfiguration Port (DRP) ------------
      DADDR                     => tied_to_ground_vec_i(7 downto 0),
      DCLK                      => tied_to_ground_i,
      DEN                       => tied_to_ground_i,
      DI                        => tied_to_ground_vec_i(15 downto 0),
      DRDY                      => open,
      DRPDO                     => open,
      DWE                       => tied_to_ground_i,
      ---------------------------- TX/RX Datapath Ports --------------------------
      GTPCLKFBEAST              => GTPCLKFBEAST_OUT,
      GTPCLKFBSEL0EAST          => "10",
      GTPCLKFBSEL0WEST          => "00",
      GTPCLKFBSEL1EAST          => "11",
      GTPCLKFBSEL1WEST          => "01",
      GTPCLKFBWEST              => GTPCLKFBWEST_OUT,
      GTPCLKOUT0                => GTPCLKOUT0_OUT,
      GTPCLKOUT1                => GTPCLKOUT1_OUT,
      ------------------- Transmit Ports - 8b10b Encoder Control -----------------
      TXBYPASS8B10B0            => tied_to_ground_vec_i(3 downto 0),
      TXBYPASS8B10B1            => tied_to_ground_vec_i(3 downto 0),

      TXCHARDISPMODE0(0)           => TXCHARDISPMODE0_IN,
      TXCHARDISPMODE0(3 downto 1) => tied_to_ground_vec_i(2 downto 0),

      TXCHARDISPMODE1(0)           => TXCHARDISPMODE1_IN,
      TXCHARDISPMODE1(3 downto 1) => tied_to_ground_vec_i(2 downto 0),

      TXCHARDISPVAL0(0)           => TXCHARDISPVAL0_IN,
      TXCHARDISPVAL0(3 downto 1) => tied_to_ground_vec_i(2 downto 0),

      TXCHARDISPVAL1(0)           => TXCHARDISPVAL1_IN,
      TXCHARDISPVAL1(3 downto 1) => tied_to_ground_vec_i(2 downto 0),
      
      TXCHARISK0(3 downto 1)    => tied_to_ground_vec_i(2 downto 0),
      TXCHARISK0(0)             => TXCHARISK0_IN,
      TXCHARISK1(3 downto 1)    => tied_to_ground_vec_i(2 downto 0),
      TXCHARISK1(0)             => TXCHARISK1_IN,
      TXENC8B10BUSE0            => tied_to_vcc_i,
      TXENC8B10BUSE1            => tied_to_vcc_i,
      TXKERR0                   => open,
      TXKERR1                   => open,
      TXRUNDISP0                => TXRUNDISP0_OUT,
      TXRUNDISP1                => TXRUNDISP1_OUT,
      --------------- Transmit Ports - TX Buffer and Phase Alignment -------------
      TXBUFSTATUS0              => open,
      TXBUFSTATUS1              => open,
      TXENPMAPHASEALIGN0        => TXENPMAPHASEALIGN0_IN,
      TXENPMAPHASEALIGN1        => TXENPMAPHASEALIGN1_IN,
      TXPMASETPHASE0            => TXPMASETPHASE0_IN,
      TXPMASETPHASE1            => TXPMASETPHASE1_IN,
      ------------------ Transmit Ports - TX Data Path interface -----------------
      TXDATA0                   => txdata0_i,
      TXDATA1                   => txdata1_i,
      TXDATAWIDTH0              => "00",
      TXDATAWIDTH1              => "00",
      TXOUTCLK0                 => open,
      TXOUTCLK1                 => open,
      TXRESET0                  => tied_to_ground_i,
      TXRESET1                  => tied_to_ground_i,
      TXUSRCLK0                 => TXUSRCLK0_IN,
      TXUSRCLK1                 => TXUSRCLK1_IN,
      TXUSRCLK20                => TXUSRCLK20_IN,
      TXUSRCLK21                => TXUSRCLK21_IN,
      --------------- Transmit Ports - TX Driver and OOB signalling --------------
      TXBUFDIFFCTRL0            => "101",
      TXBUFDIFFCTRL1            => "101",
      TXDIFFCTRL0               => "0110",
      TXDIFFCTRL1               => "0110",
      TXINHIBIT0                => tied_to_ground_i,
      TXINHIBIT1                => tied_to_ground_i,
      TXN0                      => TXN0_OUT,
      TXN1                      => TXN1_OUT,
      TXP0                      => TXP0_OUT,
      TXP1                      => TXP1_OUT,
      TXPREEMPHASIS0            => "000",
      TXPREEMPHASIS1            => "000",
      --------------------- Transmit Ports - TX PRBS Generator -------------------
      TXENPRBSTST0              => TXENPRBSTST0_IN,
      TXENPRBSTST1              => TXENPRBSTST1_IN,
      TXPRBSFORCEERR0           => tied_to_ground_i,
      TXPRBSFORCEERR1           => tied_to_ground_i,
      -------------------- Transmit Ports - TX Polarity Control ------------------
      TXPOLARITY0               => tied_to_ground_i,
      TXPOLARITY1               => tied_to_ground_i,
      ----------------- Transmit Ports - TX Ports for PCI Express ----------------
      TXDETECTRX0               => tied_to_ground_i,
      TXDETECTRX1               => tied_to_ground_i,
      TXELECIDLE0               => tied_to_ground_i,
      TXELECIDLE1               => tied_to_ground_i,
      TXPDOWNASYNCH0            => tied_to_ground_i,
      TXPDOWNASYNCH1            => tied_to_ground_i,
      --------------------- Transmit Ports - TX Ports for SATA -------------------
      TXCOMSTART0               => tied_to_ground_i,
      TXCOMSTART1               => tied_to_ground_i,
      TXCOMTYPE0                => tied_to_ground_i,
      TXCOMTYPE1                => tied_to_ground_i

      );

end RTL;


