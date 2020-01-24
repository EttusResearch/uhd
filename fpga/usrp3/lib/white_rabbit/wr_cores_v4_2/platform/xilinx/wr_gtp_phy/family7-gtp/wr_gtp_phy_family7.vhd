-------------------------------------------------------------------------------
-- Title      : Deterministic Xilinx GTP wrapper - artix-7 top module
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : wr_gtp_phy_family7.vhd
-- Author     : Peter Jansweijer, Rick Lohlefink, Tomasz Wlostowski
-- Company    : Nikhef, CERN BE-CO-HT
-- Created    : 2016-05-19
-- Last update: 2016-05-19
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Dual channel wrapper for Xilinx Artix-7 GTP adapted for
-- deterministic delays at 1.25 Gbps.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009-2011 CERN / BE-CO-HT
--
-- This source file is free software; you can redistribute it   
-- and/or modify it under the terms of the GNU Lesser General   
-- Public License as published by the Free Software Foundation; 
-- either version 2.1 of the License, or (at your option) any   
-- later version.                                               
--
-- This source is distributed in the hope that it will be       
-- useful, but WITHOUT ANY WARRANTY; without even the implied   
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
-- PURPOSE.  See the GNU Lesser General Public License for more 
-- details.                                                     
--
-- You should have received a copy of the GNU Lesser General    
-- Public License along with this source; if not, download it   
-- from http://www.gnu.org/licenses/lgpl-2.1.html
-- 
--
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author    Description
-- 2016-05-19  0.1      PeterJ    Initial release based on "wr_gtx_phy_kintex7.vhd"
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.gencores_pkg.all;

library unisim;
use unisim.vcomponents.all;

library work;

use work.disparity_gen_pkg.all;

entity wr_gtp_phy_family7 is
  generic (
    -- set to non-zero value to speed up the simulation by reducing some delays
    g_simulation     : integer := 0
  );
  port (
    -- Dedicated reference 125 MHz clock for the GTP transceiver
    clk_gtp_i        : in   std_logic;

    -- TX path, synchronous to tx_out_clk_o (62.5 MHz):
    tx_out_clk_o     : out  std_logic;
    tx_locked_o      : out  std_logic;

    -- data input (8 bits, not 8b10b-encoded)
    tx_data_i        : in   std_logic_vector(15 downto 0) := (others => '0');

    -- 1 when tx_data_i contains a control code, 0 when it's a data byte
    tx_k_i           : in   std_logic_vector(1 downto 0) := (others => '0');

    -- disparity of the currently transmitted 8b10b code (1 = plus, 0 = minus).
    -- Necessary for the PCS to generate proper frame termination sequences.
    -- Generated for the 2nd byte (LSB) of tx_data_i.
    tx_disparity_o   : out  std_logic;

    -- Encoding error indication (1 = error, 0 = no error)
    tx_enc_err_o     : out  std_logic;

    -- RX path, synchronous to ch0_rx_rbclk_o.

    -- RX recovered clock
    rx_rbclk_o       : out  std_logic;

    -- 8b10b-decoded data output. The data output must be kept invalid before
    -- the transceiver is locked on the incoming signal to prevent the EP from
    -- detecting a false carrier.
    rx_data_o        : out  std_logic_vector(15 downto 0);

    -- 1 when the byte on rx_data_o is a control code
    rx_k_o           : out  std_logic_vector(1 downto 0);

    -- encoding error indication
    rx_enc_err_o     : out  std_logic;

    -- RX bitslide indication, indicating the delay of the RX path of the
    -- transceiver (in UIs). Must be valid when ch0_rx_data_o is valid.
    rx_bitslide_o    : out  std_logic_vector(4 downto 0);

    -- reset input, active hi
    rst_i            : in   std_logic := '0';
    loopen_i         : in   std_logic_vector(2 downto 0) := (others => '0');
    tx_prbs_sel_i    : in   std_logic_vector(2 downto 0) := (others => '0');

    pad_rxn_i        : in   std_logic := '0';
    pad_rxp_i        : in   std_logic := '0';

    pad_txn_o        : out  std_logic;
    pad_txp_o        : out  std_logic;

    rdy_o            : out  std_logic
  );
end entity wr_gtp_phy_family7;

--------------------------------------------------------------------------------
-- Object        : Architecture work.wr_gtp_phy_family7.structure
-- Last modified : Mon Nov 23 12:54:18 2015.
--------------------------------------------------------------------------------

architecture structure of wr_gtp_phy_family7 is
  constant REQ_DELAY           : integer := 500;                      -- unit = ns
  constant CLK_PER             : integer := 8;                        -- unit = ns
  constant INITIAL_WAIT_CYCLES : integer := REQ_DELAY / CLK_PER;      -- Required 500 ns divided by RefClk period
  constant TOTAL_DELAY         : integer := INITIAL_WAIT_CYCLES + 10; -- Add 10 clock cycles as delay to be sure
  
  constant c_rxcdrlock_max     : integer := 3;
  constant c_reset_cnt_max     : integer := 64;                       -- Reset pulse width 64 * 8 = 512 ns
  
  type state_type is (init, count, count_done, wait_reset);
  signal state : state_type;
  
  signal rst_synced         : std_logic;
  signal rst_int            : std_logic;
  signal rx_rec_clk         : std_logic;
  signal rx_rec_clk_bufin   : std_logic;
  
  attribute buffer_type     : string;
  attribute buffer_type of rx_rec_clk_bufin : signal is "bufg";
  
  signal tx_out_clk         :    std_logic;
  signal tx_out_clk_bufin   :    std_logic;
  
  --attribute buffer_type of tx_out_clk_bufin : signal is "bufg";
  
  signal rx_lost_lock       : std_logic;
  signal ready_for_reset    : std_logic := '0';
  signal serdes_ready       : std_logic := '0';
  signal rx_slide           : std_logic := '0';
  signal rx_rst_done        : std_logic;
  signal tx_rst_done        : std_logic;
  signal rx_comma_det       : std_logic;
  signal rx_byte_is_aligned : std_logic;
  signal forced_rx_reset    : std_logic;
  signal rx_synced          : std_logic;
  signal rst_done           : std_logic;
  signal rst_done_n         : std_logic;
  signal pll_locked_i       : std_logic;
  signal pll_locked_n_i     : std_logic;
  signal rx_reset           : std_logic;
  signal rx_k_int           : std_logic_vector(1 downto 0);
  signal rx_data_int        : std_logic_vector(15 downto 0);
  signal rx_disp_err        : std_logic_vector(1 downto 0);
  signal rx_code_err        : std_logic_vector(1 downto 0);
  signal cur_disp           : t_8b10b_disparity;
  signal tx_is_k_swapped    : std_logic_vector(1 downto 0);
  signal tx_data_swapped    : std_logic_vector(15 downto 0);

  component whiterabbit_gtpe2_channel_wrapper is
  generic
  (
      -- Simulation attributes
      EXAMPLE_SIMULATION             : integer  := 0;      -- Set to 1 for simulation
      WRAPPER_SIM_GTRESET_SPEEDUP    : string := "FALSE" -- Set to "true" to speed up sim reset
  );
  port
  (
      --_________________________________________________________________________
      --____________________________CHANNEL PORTS________________________________
      GT0_DRP_BUSY_OUT         : out std_logic; 
      ---------------------------- Channel - DRP Ports  --------------------------
      GT0_DRPADDR_IN           : in  std_logic_vector(8 downto 0);
      GT0_DRPCLK_IN            : in  std_logic;
      GT0_DRPDI_IN             : in  std_logic_vector(15 downto 0);
      GT0_DRPDO_OUT            : out std_logic_vector(15 downto 0);
      GT0_DRPEN_IN             : in  std_logic;
      GT0_DRPRDY_OUT           : out std_logic;
      GT0_DRPWE_IN             : in  std_logic;
      ------------------------------- Loopback Ports -----------------------------
      GT0_LOOPBACK_IN          : in  std_logic_vector(2 downto 0);
      --------------------- RX Initialization and Reset Ports --------------------
      GT0_RXUSERRDY_IN         : in  std_logic;
      -------------------------- RX Margin Analysis Ports ------------------------
      GT0_EYESCANDATAERROR_OUT : out std_logic;
      ------------------ Receive Ports - FPGA RX Interface Ports -----------------
      GT0_RXDATA_OUT           : out  std_logic_vector(15 downto 0);
      GT0_RXUSRCLK_IN          : in  std_logic;
      GT0_RXUSRCLK2_IN         : in  std_logic;
      ------------------ Receive Ports - RX 8B/10B Decoder Ports -----------------
      GT0_RXCHARISCOMMA_OUT    : out  std_logic_vector(1 downto 0);
      GT0_RXCHARISK_OUT        : out  std_logic_vector(1 downto 0);
      GT0_RXDISPERR_OUT        : out  std_logic_vector(1 downto 0);
      GT0_RXNOTINTABLE_OUT     : out  std_logic_vector(1 downto 0);
      ------------------------ Receive Ports - RX AFE Ports ----------------------
      GT0_GTPRXN_IN            : in  std_logic;
      GT0_GTPRXP_IN            : in  std_logic;
      -------------- Receive Ports - RX Byte and Word Alignment Ports ------------
      GT0_RXBYTEISALIGNED_OUT  : out  std_logic;
      GT0_RXCOMMADET_OUT       : out  std_logic;
      GT0_RXSLIDE_IN           : in  std_logic;
      --------------------- Receive Ports - RX Equilizer Ports -------------------
      GT0_RXLPMHFHOLD_IN       : in  std_logic;
      GT0_RXLPMLFHOLD_IN       : in  std_logic;
      --------------- Receive Ports - RX Fabric Output Control Ports -------------
      GT0_RXOUTCLK_OUT         : out std_logic;
      ------------- Receive Ports - RX Initialization and Reset Ports ------------
      GT0_GTRXRESET_IN         : in  std_logic;
      -------------- Receive Ports -RX Initialization and Reset Ports ------------
      GT0_RXRESETDONE_OUT      : out  std_logic;
      --------------------- TX Initialization and Reset Ports --------------------
      GT0_GTTXRESET_IN         : in  std_logic;
      GT0_TXUSERRDY_IN         : in  std_logic;
      ------------------ Transmit Ports - FPGA TX Interface Ports ----------------
      GT0_TXDATA_IN            : in  std_logic_vector(15 downto 0);
      GT0_TXUSRCLK_IN          : in  std_logic;
      GT0_TXUSRCLK2_IN         : in  std_logic;
      ------------------ Transmit Ports - TX 8B/10B Encoder Ports ----------------
      GT0_TXCHARISK_IN         : in  std_logic_vector(1 downto 0);
      --------------- Transmit Ports - TX Configurable Driver Ports --------------
      GT0_GTPTXN_OUT           : out std_logic;
      GT0_GTPTXP_OUT           : out std_logic;
      ----------- Transmit Ports - TX Fabric Clock Output Control Ports ----------
      GT0_TXOUTCLK_OUT         : out std_logic;
      GT0_TXOUTCLKFABRIC_OUT   : out std_logic;
      GT0_TXOUTCLKPCS_OUT      : out std_logic;
      ------------- Transmit Ports - TX Initialization and Reset Ports -----------
      GT0_TXRESETDONE_OUT      : out std_logic;
      ------------------ Transmit Ports - pattern Generator Ports ----------------
      GT0_TXPRBSSEL_IN         : in  std_logic_vector(2 downto 0);
  
  
      --____________________________COMMON PORTS________________________________
      ----------------- Common Block - GTPE2_COMMON Clocking Ports ---------------
      GT0_GTREFCLK0_IN         : in  std_logic;
      -------------------------- Common Block - PLL Ports ------------------------
      GT0_PLL1LOCK_OUT         : out std_logic;
      GT0_PLL1LOCKDETCLK_IN    : in  std_logic;
      GT0_PLL1REFCLKLOST_OUT   : out std_logic;
      GT0_PLL1RESET_IN         : in  std_logic
  
  
  );
  end component whiterabbit_gtpe2_channel_wrapper;
  
  component gtp_bitslide is
  generic (
    g_simulation             :    integer;
    g_target                 :    string := "artix7"
  );
  port (
    gtp_rst_i                : in  std_logic;
    gtp_rx_clk_i             : in  std_logic;
    gtp_rx_comma_det_i       : in  std_logic;
    gtp_rx_byte_is_aligned_i : in  std_logic;
    serdes_ready_i           : in  std_logic;
    gtp_rx_slide_o           : out std_logic;
    gtp_rx_cdr_rst_o         : out std_logic;
    bitslide_o               : out std_logic_vector(4 downto 0);
    synced_o                 : out std_logic
  );
  end component;

  component BUFG
    port (
      I : in  std_ulogic;
      O : out  std_ulogic);
  end component BUFG;

  function f_to_bool(x : integer) return string is
    begin
      if(x /= 0) then
        return "TRUE";
      else
        return "FALSE";
      end if;
    end f_to_bool;

begin

  U_EdgeDet_rst_i : gc_sync_ffs port map (
    clk_i    => clk_gtp_i,
    rst_n_i    => '1',
    data_i    => rst_i,
    ppulse_o  => rst_synced);
  
  p_reset_pulse : process(clk_gtp_i, rst_synced)
    variable reset_cnt      : integer range 0 to c_reset_cnt_max;
  begin
    if(rst_synced = '1') then
      reset_cnt := 0;
      rst_int <= '1';
    elsif rising_edge(clk_gtp_i) then
      if reset_cnt /= c_reset_cnt_max then
        reset_cnt := reset_cnt + 1;
        rst_int <= '1';
      else
        rst_int <= '0';
      end if;
    end if;
  end process;  

  -- ug482 "GTP Transceiver TX/RX Reset in Response to Completion of Configuration"
  --   1. Wait a minimum of 500 ns after configuration is complete
  process(clk_gtp_i, rst_int) is
    variable reset_counter : integer range 0 to TOTAL_DELAY := 0;
  begin
    if rst_int = '1' then
      state <= init;
    elsif rising_edge(clk_gtp_i) then
      case state is
        when init =>
          reset_counter := 0;
          state <= count;
        when count =>
          if reset_counter = TOTAL_DELAY then
            reset_counter := 0;
            state <= count_done;
          else
            reset_counter := reset_counter + 1;
            state <= count;
          end if;
        when count_done =>
          state <= wait_reset;
        when wait_reset =>
          state <= wait_reset;
      end case;
    end if;
  end process;
  
  ready_for_reset <= '1' when state = count_done else '0';
  
  -- 7-Series GTP RXCDRLOCK is reserved (ug482 Table 4.11) and can not be used for detection of proper RX lock.
  -- Instead use rx_code_err (i.e. RXNOTINTABLE) to check integrity of the received characters.
  process(rx_rec_clk, rst_int) is
  begin
    if rst_int = '1' then
      rx_lost_lock <= '1';
    elsif rising_edge(rx_rec_clk) then
      if rx_synced = '1' then
        if rx_code_err > "00" then
          rx_lost_lock <= '1';
        else
          rx_lost_lock <= '0';
        end if;
      else
        rx_lost_lock <= '0';
      end if;
    end if;
  end process;

  tx_enc_err_o <= '0';
  
  U_BUF_TxOutClk: BUFG
    port map(
      I => tx_out_clk_bufin,
      O => tx_out_clk);

  tx_out_clk_o <= tx_out_clk;
  tx_locked_o <= pll_locked_i;
    
  U_BUF_RxRecClk: BUFG
    port map(
      I => rx_rec_clk_bufin,
      O => rx_rec_clk);
      
  rx_rbclk_o <= rx_rec_clk;

  tx_is_k_swapped <= tx_k_i(0) & tx_k_i(1);
  tx_data_swapped <= tx_data_i(7 downto 0) & tx_data_i(15 downto 8);
  
  U_GTP_INST : whiterabbit_gtpe2_channel_wrapper
  generic map
  (
    -- Simulation attributes
    EXAMPLE_SIMULATION           => g_simulation,
    WRAPPER_SIM_GTRESET_SPEEDUP  => f_to_bool(g_simulation)
  )
  port map
  (
    --_________________________________________________________________________
    --_________________________________________________________________________
    --____________________________CHANNEL PORTS________________________________
    GT0_DRP_BUSY_OUT         =>  open,
    ---------------------------- Channel - DRP Ports  --------------------------
    GT0_DRPADDR_IN           =>  (others => '0'),
    GT0_DRPCLK_IN            =>  clk_gtp_i,
    GT0_DRPDI_IN             =>  (others => '0'),
    GT0_DRPDO_OUT            =>  open,
    GT0_DRPEN_IN             =>  '0',
    GT0_DRPRDY_OUT           =>  open,
    GT0_DRPWE_IN             =>  '0',
    ------------------------------- Loopback Ports -----------------------------
    GT0_LOOPBACK_IN          =>  loopen_i,
    --------------------- RX Initialization and Reset Ports --------------------
    GT0_RXUSERRDY_IN         =>  pll_locked_i,
    -------------------------- RX Margin Analysis Ports ------------------------
    GT0_EYESCANDATAERROR_OUT =>  open,
    ------------------ Receive Ports - FPGA RX Interface Ports -----------------
    GT0_RXDATA_OUT           =>  rx_data_int,
    GT0_RXUSRCLK_IN          =>  rx_rec_clk,
    GT0_RXUSRCLK2_IN         =>  rx_rec_clk,
    ------------------ Receive Ports - RX 8B/10B Decoder Ports -----------------
    GT0_RXCHARISCOMMA_OUT    =>  open,
    GT0_RXCHARISK_OUT        =>  rx_k_int,
    GT0_RXDISPERR_OUT        =>  rx_disp_err,
    GT0_RXNOTINTABLE_OUT     =>  rx_code_err,
    ------------------------ Receive Ports - RX AFE Ports ----------------------
    GT0_GTPRXN_IN            =>  pad_rxn_i,
    GT0_GTPRXP_IN            =>  pad_rxp_i,
    -------------- Receive Ports - RX Byte and Word Alignment Ports ------------
    GT0_RXBYTEISALIGNED_OUT  =>  rx_byte_is_aligned,
    GT0_RXCOMMADET_OUT       =>  rx_comma_det,
    GT0_RXSLIDE_IN           =>  rx_slide,
    --------------------- Receive Ports - RX Equilizer Ports -------------------
    GT0_RXLPMHFHOLD_IN       =>  '0',
    GT0_RXLPMLFHOLD_IN       =>  '0',
    --------------- Receive Ports - RX Fabric Output Control Ports -------------
    GT0_RXOUTCLK_OUT         =>  rx_rec_clk_bufin,
    ------------- Receive Ports - RX Initialization and Reset Ports ------------
    GT0_GTRXRESET_IN         =>  rx_reset,
    -------------- Receive Ports -RX Initialization and Reset Ports ------------
    GT0_RXRESETDONE_OUT      =>  rx_rst_done,
    --------------------- TX Initialization and Reset Ports --------------------
    GT0_GTTXRESET_IN         =>  pll_locked_n_i,
    GT0_TXUSERRDY_IN         =>  pll_locked_i,
    ------------------ Transmit Ports - FPGA TX Interface Ports ----------------
    GT0_TXDATA_IN            =>  tx_data_swapped,
    GT0_TXUSRCLK_IN          =>  tx_out_clk,
    GT0_TXUSRCLK2_IN         =>  tx_out_clk,
    ------------------ Transmit Ports - TX 8B/10B Encoder Ports ----------------
    GT0_TXCHARISK_IN         =>  tx_is_k_swapped,
    --------------- Transmit Ports - TX Configurable Driver Ports --------------
    GT0_GTPTXN_OUT           =>  pad_txn_o,
    GT0_GTPTXP_OUT           =>  pad_txp_o,
    ----------- Transmit Ports - TX Fabric Clock Output Control Ports ----------
    GT0_TXOUTCLK_OUT         =>  tx_out_clk_bufin,
    GT0_TXOUTCLKFABRIC_OUT   =>  open,
    GT0_TXOUTCLKPCS_OUT      =>  open,
    ------------- Transmit Ports - TX Initialization and Reset Ports -----------
    GT0_TXRESETDONE_OUT      =>  tx_rst_done,
        ------------------ Transmit Ports - pattern Generator Ports ----------------
    GT0_TXPRBSSEL_IN         =>  tx_prbs_sel_i,

    --____________________________COMMON PORTS________________________________
    ----------------- Common Block - GTPE2_COMMON Clocking Ports ---------------
    GT0_GTREFCLK0_IN         =>  clk_gtp_i,
    -------------------------- Common Block - PLL Ports ------------------------
    GT0_PLL1LOCK_OUT         =>  pll_locked_i,
    GT0_PLL1LOCKDETCLK_IN    =>  '0',
    GT0_PLL1REFCLKLOST_OUT   =>  open,
    GT0_PLL1RESET_IN         =>  ready_for_reset
  );
  
  U_Bitslide : gtp_bitslide
  generic map (
    g_simulation             =>  g_simulation,
    g_target                 =>  ("artix7")
  )
  port map (
    gtp_rst_i                =>  rst_done_n,
    gtp_rx_clk_i             =>  rx_rec_clk,
    gtp_rx_comma_det_i       =>  rx_comma_det,
    gtp_rx_byte_is_aligned_i =>  rx_byte_is_aligned,
    serdes_ready_i           =>  serdes_ready,
    gtp_rx_slide_o           =>  rx_slide,
    gtp_rx_cdr_rst_o         =>  forced_rx_reset,
    bitslide_o               =>  rx_bitslide_o,
    synced_o                 =>  rx_synced
  );

  pll_locked_n_i <= not pll_locked_i;
  serdes_ready <= not rx_lost_lock and pll_locked_i and tx_rst_done and rx_rst_done;
  rst_done <= tx_rst_done and rx_rst_done;
  rst_done_n <= not rst_done;
  rx_reset <= pll_locked_n_i or forced_rx_reset;
  rdy_o <= serdes_ready;

  p_gen_rx_outputs : process(rx_rec_clk, rst_done_n)
  begin
    if(rst_done_n = '1') then
      rx_data_o    <= (others => '0');
      rx_k_o       <= (others => '0');
      rx_enc_err_o <= '0';
    elsif rising_edge(rx_rec_clk) then
      if(serdes_ready = '1' and rx_synced = '1') then
      rx_data_o    <= rx_data_int(7 downto 0) & rx_data_int(15 downto 8);
      rx_k_o       <= rx_k_int(0) & rx_k_int(1);
      rx_enc_err_o <= rx_disp_err(0) or rx_disp_err(1) or rx_code_err(0) or rx_code_err(1);
      else
      rx_data_o    <= (others => '1');
      rx_k_o       <= (others => '1');
      rx_enc_err_o <= '1';
      end if;
    end if;
  end process;

  p_gen_tx_disparity : process(tx_out_clk, rst_done_n)
  begin
    if rising_edge(tx_out_clk) then
      if rst_done_n = '1' then
      cur_disp <= RD_MINUS;
      else
      cur_disp <= f_next_8b10b_disparity16(cur_disp, tx_k_i, tx_data_i);
      end if;
    end if;
  end process;

  tx_disparity_o <= to_std_logic(cur_disp);
end architecture structure ; -- of wr_gtp_phy_family7

