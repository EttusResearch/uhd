-------------------------------------------------------------------------------
-- Title      : 1000BaseT/X MAC Endpoint - receive path PCS for 1000BaseX
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : ep_tx_pcs_tbi.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2009-06-16
-- Last update: 2017-02-20
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Module implements the transmit path for 802.3z 1000BaseX PCS.
-- This block interfaces the Ethernet framer to TX PMA (Physical Medium Attachment).
-- It performs preamble generation, insertion of idle patterns, all the low-level
-- signalling (including 8b10b coding). Strobing signal for taking TX timestamps
-- is also generated.
--
-- Module uses two separate clocks: 125 MHz tbi_tx_clk (or gtp_tx_clk)
-- (Transmit clock for PHY) which clocks 8b10b signalling layer, and 62.5 MHz
-- (clk_sys_i) which is used for data exchange with the rest of switch. Data
-- exchange between these clock domains  is done using an async FIFO.
--
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009-2017 CERN
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
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author   Description
-- 2009-06-16  0.1      twlostow Created (no error propagation supported yet)
-- 2010-04-06  0.2      twlostow Cleanup, new timestamping/LCR scheme
-- 2010-07-30  0.2      twlostow Fixed preamble length bug
-- 2010-11-18  0.4      twlostow Added support for Xilinx GTP transceivers.
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


library work;
use work.gencores_pkg.all;
use work.genram_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;


entity ep_tx_pcs_8bit is

  port (
-- reset (synchronous to refclk2, active LO)
    rst_n_i : in std_logic;

-- 62.5 MHz clock (refclk/2)
    clk_sys_i : in std_logic;

-- reset (phy_tx_clk_i sync)
    rst_txclk_n_i : in std_logic;

-------------------------------------------------------------------------------
-- TX Framer inteface
-------------------------------------------------------------------------------

-- TX Fabric input
    pcs_fab_i : in t_ep_internal_fabric;

-- HI pulse indicates an error during transmission of a frame (buffer underrun)
    pcs_error_o : out std_logic;

-- HI indicates that the PCS is busy (transmitting a frame or during autonegotiation)
    pcs_busy_o : out std_logic;

-- HI indicates that PCS FIFO is almost full.
    pcs_dreq_o : out std_logic;

-------------------------------------------------------------------------------
-- WB controller control signals
-------------------------------------------------------------------------------

    mdio_mcr_reset_i      : in std_logic;
-- Transmit Control Register, EN_PCS field
    mdio_mcr_pdown_i      : in std_logic;
-- Transmit Control Register, TX_CAL field
    mdio_wr_spec_tx_cal_i : in std_logic;

-- autonegotiation control
    an_tx_en_i  : in std_logic;
    an_tx_val_i : in std_logic_vector(15 downto 0);

-- Timestamp strobe
    timestamp_trigger_p_a_o : out std_logic;

-- RMON events
    rmon_tx_underrun : out std_logic;

-------------------------------------------------------------------------------
-- PHY Interface
-------------------------------------------------------------------------------

    phy_tx_clk_i       : in  std_logic;
    phy_tx_data_o      : out std_logic_vector(7 downto 0);
    phy_tx_k_o         : out std_logic;
    phy_tx_disparity_i : in  std_logic;
    phy_tx_enc_err_i   : in  std_logic
    );

end ep_tx_pcs_8bit;


architecture behavioral of ep_tx_pcs_8bit is

-- TX state machine definitions
  type t_tbif_tx_state is (TX_COMMA, TX_CAL, TX_CR1, TX_CR2, TX_CR3, TX_CR4, TX_SPD, TX_IDLE, TX_DATA, TX_PREAMBLE, TX_SFD, TX_EPD, TX_EXTEND, TX_GOTO_COMMA, TX_GEN_ERROR);

-- TX state machine signals

  signal tx_is_k            : std_logic;
  signal tx_catch_disparity : std_logic;
  signal tx_odata_reg       : std_logic_vector(7 downto 0);
  signal tx_state           : t_tbif_tx_state;
  signal tx_cntr            : unsigned(3 downto 0);
  signal tx_cr_alternate    : std_logic;

-- TX clock alignment FIFO signals
  signal fifo_packed_in, fifo_packed_out : std_logic_vector(17 downto 0);
  signal fifo_empty                      : std_logic;
  signal fifo_almost_empty               : std_logic;
  signal fifo_almost_full                : std_logic;
  signal fifo_enough_data                : std_logic;
  signal fifo_wr                         : std_logic;
  signal fifo_rd                         : std_logic;
  signal fifo_ready                      : std_logic;
  signal fifo_clear_n                    : std_logic;
  signal fifo_fab                        : t_ep_internal_fabric;

  signal tx_rdreq_toggle : std_logic;
  signal tx_odd_length   : std_logic;

  signal tx_busy  : std_logic;
  signal tx_error : std_logic;
  signal rst_n_tx : std_logic;

  signal mdio_mcr_reset_synced : std_logic;
  signal mdio_mcr_pdown_synced : std_logic;
  signal an_tx_en_synced       : std_logic;

begin

  U_sync_pcs_busy_o : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_i,
      data_i   => tx_busy,
      synced_o => pcs_busy_o);

  U_sync_pcs_error_o : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_i,
      data_i   => tx_error,
      ppulse_o => pcs_error_o);

  U_sync_an_tx_en : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => phy_tx_clk_i,
      rst_n_i  => '1',
      data_i   => an_tx_en_i,
      synced_o => an_tx_en_synced);

  U_sync_mcr_reset : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => phy_tx_clk_i,
      rst_n_i  => '1',
      data_i   => mdio_mcr_reset_i,
      synced_o => mdio_mcr_reset_synced,
      npulse_o => open,
      ppulse_o => open);

  U_sync_power_down : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => phy_tx_clk_i,
      rst_n_i  => '1',
      data_i   => mdio_mcr_pdown_i,
      synced_o => mdio_mcr_pdown_synced);

  phy_tx_data_o <= tx_odata_reg;
  phy_tx_k_o    <= tx_is_k;

  rst_n_tx <= rst_txclk_n_i and not mdio_mcr_reset_synced;

-------------------------------------------------------------------------------
-- Clock alignment FIFO
-------------------------------------------------------------------------------

  fifo_clear_n <= '0' when (rst_n_i = '0') or (mdio_mcr_pdown_synced = '1') else '1';

  f_pack_fifo_contents(pcs_fab_i, fifo_packed_in, fifo_wr, true);

  U_TX_FIFO : generic_async_fifo
    generic map (
      g_data_width             => 18,
      g_size                   => 64,
      g_with_rd_empty          => true,
      g_with_rd_almost_empty   => true,
      g_with_rd_count          => true,
      g_with_wr_almost_full    => true,
      g_almost_empty_threshold => 16,
      g_almost_full_threshold  => 56)  -- fixme: make this a generic (or WB register)
    port map (
      rst_n_i           => fifo_clear_n,
      clk_wr_i          => clk_sys_i,
      d_i               => fifo_packed_in,
      we_i              => fifo_wr,
      wr_empty_o        => open,
      wr_full_o         => open,
      wr_almost_empty_o => open,
      wr_almost_full_o  => fifo_almost_full,
      wr_count_o        => open,
      clk_rd_i          => phy_tx_clk_i,
      q_o               => fifo_packed_out,
      rd_i              => fifo_rd,
      rd_empty_o        => fifo_empty,
      rd_full_o         => open,
      rd_almost_empty_o => fifo_almost_empty,
      rd_almost_full_o  => open,
      rd_count_o        => open);

  fifo_enough_data <= not fifo_almost_empty;

  f_unpack_fifo_contents(fifo_packed_out, '1', fifo_fab, true);

  -----------------------------------------------------------------------------
  -- TX PCS state machine
  -----------------------------------------------------------------------------

  p_tx_fsm : process (phy_tx_clk_i)
  begin

    if rising_edge(phy_tx_clk_i) then

-- The PCS is reset or disabled
      if(rst_n_tx = '0' or mdio_mcr_pdown_synced = '1') then
        tx_state                <= TX_COMMA;
        timestamp_trigger_p_a_o <= '0';
        fifo_rd                 <= '0';
        tx_error                <= '0';
        tx_odata_reg            <= (others => '0');
        tx_is_k                 <= '0';
        tx_cr_alternate         <= '0';
        tx_catch_disparity      <= '0';
        tx_cntr                 <= (others => '0');
        tx_odd_length           <= '0';
        tx_rdreq_toggle         <= '0';
        rmon_tx_underrun        <= '0';

      else

        case tx_state is

-------------------------------------------------------------------------------
-- State COMMA: sends K28.5 comma character (first byte of /I/ sequence)
-------------------------------------------------------------------------------
          when TX_COMMA =>
            tx_is_k      <= '1';
            tx_odata_reg <= c_K28_5;
            tx_state     <= TX_IDLE;
            fifo_rd      <= '0';
            fifo_ready   <= fifo_rd;

-------------------------------------------------------------------------------
-- State IDLE: sends the second code of the /I/ sequence with proper disparity\
-------------------------------------------------------------------------------
          when TX_IDLE =>

            -- clear the RMON/error pulse after 2 cycles (DATA->COMMA->IDLE) to
            -- make sure is't long enough to trigger the event counter
            rmon_tx_underrun <= '0';
            tx_error         <= '0';

-- endpoint wants to send Config_Reg
            if(an_tx_en_synced = '1') then
              tx_state        <= TX_CR1;
              tx_cr_alternate <= '0';
              fifo_rd         <= '0';

-- we've got a new frame in the FIFO
            elsif (fifo_fab.sof = '1' and fifo_ready = '1' and tx_cntr = "0000")then
              fifo_rd  <= '1';
              tx_state <= TX_SPD;
              tx_cntr  <= "0101";

-- host requested a calibration pattern
            elsif(mdio_wr_spec_tx_cal_i = '1') then
              tx_state        <= TX_CAL;
              fifo_rd         <= '0';
              tx_cr_alternate <= '0';
            else
-- continue sending idle sequences and checking if something has arrived in the
-- FIFO
              if(tx_cntr /= "0000") then
                fifo_rd <= '0';
              else
                fifo_rd <= (not fifo_empty) and fifo_enough_data;
              end if;
              tx_state <= TX_COMMA;
            end if;

            tx_is_k <= '0';

-- check the disparity of the previously emitted code and choose whether to send
-- /I1/ or /I2/
            if (phy_tx_disparity_i = '1' and tx_catch_disparity = '1') then
              tx_odata_reg <= c_d5_6;
            else
              tx_odata_reg <= c_d16_2;
            end if;

            tx_catch_disparity <= '0';

            if(tx_cntr /= "0000") then
              tx_cntr <= tx_cntr - 1;
            end if;
-------------------------------------------------------------------------------
-- State: CAL: transmit the calibration sequence
-------------------------------------------------------------------------------

          when TX_CAL =>
            tx_odata_reg <= c_k28_7;
            tx_is_k      <= '1';
            if(mdio_wr_spec_tx_cal_i = '0' and tx_cr_alternate = '1') then
              tx_state <= TX_COMMA;
            end if;

            tx_cr_alternate <= not tx_cr_alternate;

-------------------------------------------------------------------------------
-- States: CR1, CR2, CR3, CR4: send the /C/ Configuration code set
-------------------------------------------------------------------------------

          when TX_CR1 =>
            tx_is_k      <= '1';
            tx_odata_reg <= c_k28_5;
            tx_state     <= TX_CR2;

          when TX_CR2 =>
            tx_is_k <= '0';
            if (tx_cr_alternate = '1') then
              tx_odata_reg <= c_d21_5;
            else
              tx_odata_reg <= c_d2_2;
            end if;
            tx_cr_alternate <= not tx_cr_alternate;
            tx_state        <= TX_CR3;

          when TX_CR3 =>
            tx_odata_reg <= an_tx_val_i(7 downto 0);
            tx_state     <= TX_CR4;

          when TX_CR4 =>
            tx_odata_reg <= an_tx_val_i(15 downto 8);

-- check if the autonegotiation control still wants the Config_Reg to be sent
            if(an_tx_en_synced = '1') then
              tx_state <= TX_CR1;
            else
              tx_state <= TX_COMMA;
            end if;

-------------------------------------------------------------------------------
-- State SPD: sends a start-of-packet delimeter
-------------------------------------------------------------------------------
          when TX_SPD =>
            fifo_rd      <= '0';
            tx_is_k      <= '1';
            tx_odata_reg <= c_k27_7;
            tx_state     <= TX_PREAMBLE;

-------------------------------------------------------------------------------
-- State PREAMBLE: produces an Ethernet preamble
-------------------------------------------------------------------------------
          when TX_PREAMBLE =>
            tx_is_k      <= '0';
            tx_odata_reg <= c_preamble_char;

            if (tx_cntr = "0000") then
              tx_state        <= TX_SFD;
              tx_rdreq_toggle <= '1';
            end if;

            tx_cntr <= tx_cntr - 1;


-------------------------------------------------------------------------------
-- State SFD: outputs the start-of-frame delimeter (last byte of the preamble)
-------------------------------------------------------------------------------
          when TX_SFD =>

            tx_odata_reg            <= c_preamble_sfd;
            tx_rdreq_toggle         <= '1';
            tx_state                <= TX_DATA;
            timestamp_trigger_p_a_o <= '1';

          when TX_DATA =>


            -- toggle the TX FIFO request line, so we read a 16-bit word
            -- every 2 phy_tx_clk_i periods

            fifo_rd <= tx_rdreq_toggle and not (fifo_empty or fifo_fab.eof);

            if((fifo_empty = '1' or fifo_fab.error = '1') and fifo_fab.eof = '0') then  --
              -- FIFO underrun?
              tx_odata_reg     <= c_k30_7;  -- emit error propagation code
              tx_is_k          <= '1';
              tx_state         <= TX_GEN_ERROR;
              tx_error         <= not fifo_fab.error;
              rmon_tx_underrun <= '1';
            else

              if tx_rdreq_toggle = '1' then  -- send 16-bit word MSB or LSB
                tx_odata_reg <= fifo_fab.data(15 downto 8);
              else
                tx_odata_reg <= fifo_fab.data(7 downto 0);
              end if;

              tx_rdreq_toggle <= not tx_rdreq_toggle;

              -- handle the end of frame both for even- and odd-length frames
              tx_odd_length <= fifo_fab.bytesel;

              if (fifo_fab.eof = '1' and (tx_rdreq_toggle = '0' or (tx_rdreq_toggle = '1' and fifo_fab.bytesel = '1'))) then
                tx_state <= TX_EPD;
                fifo_rd  <= '0';
              end if;
            end if;

-------------------------------------------------------------------------------
-- State EPD: send End-of-frame delimeter
-------------------------------------------------------------------------------
          when TX_EPD =>
            timestamp_trigger_p_a_o <= '0';

            tx_is_k      <= '1';
            tx_odata_reg <= c_k29_7;
            tx_state     <= TX_EXTEND;

--------------------------------------------------------------------------------
-- State EXTEND: send the carrier extension
-------------------------------------------------------------------------------
          when TX_EXTEND =>
            tx_odata_reg <= c_k23_7;
            if(tx_odd_length = '0')then
              tx_state           <= TX_COMMA;
              tx_catch_disparity <= '1';
            else
              tx_odd_length <= '0';
            end if;

            tx_cntr <= "1000";

-------------------------------------------------------------------------------
-- State GEN_ERROR: entered when an error occured. Just terminates the frame.
-------------------------------------------------------------------------------
          when TX_GEN_ERROR =>
            tx_state <= TX_EPD;

          when others => null;
        end case;
      end if;
    end if;
  end process;

  process(phy_tx_clk_i)
  begin
    if rising_edge(phy_tx_clk_i) then
      if fifo_empty = '0' or (tx_state /= TX_IDLE and tx_state /= TX_COMMA) then
        tx_busy <= '1';
      else
        tx_busy <= '0';
      end if;
    end if;
  end process;

  pcs_dreq_o <= not fifo_almost_full;

end behavioral;
