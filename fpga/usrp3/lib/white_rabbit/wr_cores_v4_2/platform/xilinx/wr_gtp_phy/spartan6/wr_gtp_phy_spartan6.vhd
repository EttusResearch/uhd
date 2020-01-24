-------------------------------------------------------------------------------
-- Title      : Deterministic Xilinx GTP wrapper - Spartan-6 top module
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : wr_gtp_phy_spartan6.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-11-18
-- Last update: 2017-05-24
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Dual channel wrapper for Xilinx Spartan-6 GTP adapted for
-- deterministic delays at 1.25 Gbps.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2010 CERN
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
-- 2010-11-18  0.4      twlostow  Initial release
-- 2011-02-07  0.5      twlostow  Verified on Spartan6 GTP (single channel only)
-- 2011-05-15  0.6      twlostow  Added reference clock output
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library unisim;
use unisim.vcomponents.all;

library work;
use work.gencores_pkg.all;
use work.disparity_gen_pkg.all;

entity wr_gtp_phy_spartan6 is

  generic (
    -- set to non-zero value to speed up the simulation by reducing some delays
    g_simulation      : integer := 1;
    g_force_disparity : integer := 0;
    g_enable_ch0      : integer := 1;
    g_enable_ch1      : integer := 1
    );

  port (
    -- Port 0

    -- dedicated GTP clock input
    gtp_clk_i : in std_logic;

    -- TX path, synchronous to ch0_ref_clk_i
    ch0_ref_clk_i : in std_logic;

    -- data input (8 bits, not 8b10b-encoded)
    ch0_tx_data_i : in std_logic_vector(7 downto 0);

    -- 1 when tx_data_i contains a control code, 0 when it's a data byte
    ch0_tx_k_i : in std_logic;

    -- disparity of the currently transmitted 8b10b code (1 = plus, 0 = minus).
    -- Necessary for the PCS to generate proper frame termination sequences.
    ch0_tx_disparity_o : out std_logic;

    -- Encoding error indication (1 = error, 0 = no error)
    ch0_tx_enc_err_o : out std_logic;

    -- RX path, synchronous to ch0_rx_rbclk_o.

    -- RX recovered clock
    ch0_rx_rbclk_o : out std_logic;

    -- 8b10b-decoded data output. The data output must be kept invalid before
    -- the transceiver is locked on the incoming signal to prevent the EP from
    -- detecting a false carrier.
    ch0_rx_data_o : out std_logic_vector(7 downto 0);

    -- 1 when the byte on rx_data_o is a control code
    ch0_rx_k_o : out std_logic;

    -- encoding error indication
    ch0_rx_enc_err_o : out std_logic;

    -- RX bitslide indication, indicating the delay of the RX path of the
    -- transceiver (in UIs). Must be valid when ch0_rx_data_o is valid.
    ch0_rx_bitslide_o : out std_logic_vector(3 downto 0);

    -- reset input, active hi
    ch0_rst_i : in std_logic;

    -- local loopback enable (Tx->Rx), active hi
    ch0_loopen_i : in std_logic;
    ch0_loopen_vec_i : in std_logic_vector(2 downto 0) := (others=>'0');

    -- PRBS select (see Xilinx UG386 Table 3-15; "000" = Standard operation, pattern generator off)
    ch0_tx_prbs_sel_i  : in std_logic_vector(2 downto 0) := (others=>'0');

    -- gtp0 ready: locked & aligned
    ch0_rdy_o : out std_logic;

-- Port 1
    ch1_ref_clk_i : in std_logic;

    ch1_tx_data_i      : in  std_logic_vector(7 downto 0) := "00000000";
    ch1_tx_k_i         : in  std_logic                    := '0';
    ch1_tx_disparity_o : out std_logic;
    ch1_tx_enc_err_o   : out std_logic;

    ch1_rx_data_o     : out std_logic_vector(7 downto 0);
    ch1_rx_rbclk_o    : out std_logic;
    ch1_rx_k_o        : out std_logic;
    ch1_rx_enc_err_o  : out std_logic;
    ch1_rx_bitslide_o : out std_logic_vector(3 downto 0);

    ch1_rst_i    : in std_logic := '0';
    ch1_loopen_i : in std_logic := '0';
    ch1_loopen_vec_i : in std_logic_vector(2 downto 0) := (others=>'0');
    ch1_tx_prbs_sel_i: in std_logic_vector(2 downto 0) := (others=>'0');
    ch1_rdy_o    : out std_logic;

-- Serial I/O

    pad_txn0_o : out std_logic;
    pad_txp0_o : out std_logic;

    pad_rxn0_i : in std_logic := '0';
    pad_rxp0_i : in std_logic := '0';

    pad_txn1_o : out std_logic;
    pad_txp1_o : out std_logic;

    pad_rxn1_i : in std_logic := '0';
    pad_rxp1_i : in std_logic := '0'
    );


end wr_gtp_phy_spartan6;

architecture rtl of wr_gtp_phy_spartan6 is

  component WHITERABBITGTP_WRAPPER_TILE_SPARTAN6
    generic (
      TILE_SIM_GTPRESET_SPEEDUP : integer;
      TILE_CLK25_DIVIDER_0      : integer;
      TILE_CLK25_DIVIDER_1      : integer;
      TILE_PLL_DIVSEL_FB_0      : integer;
      TILE_PLL_DIVSEL_FB_1      : integer;
      TILE_PLL_DIVSEL_REF_0     : integer;
      TILE_PLL_DIVSEL_REF_1     : integer;
      TILE_PLL_SOURCE_0         : string;
      TILE_PLL_SOURCE_1         : string);
    port (
      LOOPBACK0_IN          : in  std_logic_vector(2 downto 0);
      LOOPBACK1_IN          : in  std_logic_vector(2 downto 0);
      REFCLKOUT0_OUT        : out std_logic;
      REFCLKOUT1_OUT        : out std_logic;
      CLK00_IN              : in  std_logic;
      CLK01_IN              : in  std_logic;
      CLK10_IN              : in  std_logic;
      CLK11_IN              : in  std_logic;
      GTPRESET0_IN          : in  std_logic;
      GTPRESET1_IN          : in  std_logic;
      PLLLKDET0_OUT         : out std_logic;
      PLLLKDET1_OUT         : out std_logic;
      RESETDONE0_OUT        : out std_logic;
      RESETDONE1_OUT        : out std_logic;
      RXCHARISK0_OUT        : out std_logic;
      RXCHARISK1_OUT        : out std_logic;
      RXDISPERR0_OUT        : out std_logic;
      RXDISPERR1_OUT        : out std_logic;
      RXNOTINTABLE0_OUT     : out std_logic;
      RXNOTINTABLE1_OUT     : out std_logic;
      RXBYTEISALIGNED0_OUT  : out std_logic;
      RXBYTEISALIGNED1_OUT  : out std_logic;
      RXCOMMADET0_OUT       : out std_logic;
      RXCOMMADET1_OUT       : out std_logic;
      RXSLIDE0_IN           : in  std_logic;
      RXSLIDE1_IN           : in  std_logic;
      RXDATA0_OUT           : out std_logic_vector(7 downto 0);
      RXDATA1_OUT           : out std_logic_vector(7 downto 0);
      RXUSRCLK0_IN          : in  std_logic;
      RXUSRCLK1_IN          : in  std_logic;
      RXUSRCLK20_IN         : in  std_logic;
      RXUSRCLK21_IN         : in  std_logic;
      RXCDRRESET0_IN        : in  std_logic;
      RXCDRRESET1_IN        : in  std_logic;
      RXN0_IN               : in  std_logic;
      RXN1_IN               : in  std_logic;
      RXP0_IN               : in  std_logic;
      RXP1_IN               : in  std_logic;
      GTPCLKFBEAST_OUT      : out std_logic_vector(1 downto 0);
      GTPCLKFBWEST_OUT      : out std_logic_vector(1 downto 0);
      GTPCLKOUT0_OUT        : out std_logic_vector(1 downto 0);
      GTPCLKOUT1_OUT        : out std_logic_vector(1 downto 0);
      TXCHARISK0_IN         : in  std_logic;
      TXCHARISK1_IN         : in  std_logic;
      TXCHARDISPMODE0_IN    : in  std_logic;
      TXCHARDISPMODE1_IN    : in  std_logic;
      TXCHARDISPVAL0_IN     : in  std_logic;
      TXCHARDISPVAL1_IN     : in  std_logic;
      TXRUNDISP0_OUT        : out std_logic_vector(3 downto 0);
      TXRUNDISP1_OUT        : out std_logic_vector(3 downto 0);
      TXENPMAPHASEALIGN0_IN : in  std_logic;
      TXENPMAPHASEALIGN1_IN : in  std_logic;
      TXPMASETPHASE0_IN     : in  std_logic;
      TXPMASETPHASE1_IN     : in  std_logic;
      TXDATA0_IN            : in  std_logic_vector(7 downto 0);
      TXDATA1_IN            : in  std_logic_vector(7 downto 0);
      TXUSRCLK0_IN          : in  std_logic;
      TXUSRCLK1_IN          : in  std_logic;
      TXUSRCLK20_IN         : in  std_logic;
      TXUSRCLK21_IN         : in  std_logic;
      TXN0_OUT              : out std_logic;
      TXN1_OUT              : out std_logic;
      TXP0_OUT              : out std_logic;
      TXP1_OUT              : out std_logic;
      TXENPRBSTST0_IN       : in  std_logic_vector(2 downto 0);
      TXENPRBSTST1_IN       : in  std_logic_vector(2 downto 0));
  end component;

  component gtp_phase_align
    generic(
      g_simulation : integer);
    port (
      gtp_rst_i                   : in  std_logic;
      gtp_tx_clk_i                : in  std_logic;
      gtp_tx_en_pma_phase_align_o : out std_logic;
      gtp_tx_pma_set_phase_o      : out std_logic;
      align_en_i                  : in  std_logic;
      align_done_o                : out std_logic);
  end component;

  component gtp_bitslide
    generic(
      g_simulation : integer;
      g_target     : string := "spartan6");
    port (
      gtp_rst_i                : in  std_logic;
      gtp_rx_clk_i             : in  std_logic;
      gtp_rx_comma_det_i       : in  std_logic;
      gtp_rx_byte_is_aligned_i : in  std_logic;
      serdes_ready_i           : in  std_logic;
      gtp_rx_slide_o           : out std_logic;
      gtp_rx_cdr_rst_o         : out std_logic;
      bitslide_o               : out std_logic_vector(4 downto 0);
      synced_o                 : out std_logic);
  end component;

  signal ch0_gtp_reset        : std_logic;
  signal ch0_gtp_loopback     : std_logic_vector(2 downto 0) := "000";
  signal ch0_gtp_reset_done   : std_logic;
  signal ch0_gtp_pll_lockdet  : std_logic;
  signal ch0_tx_pma_set_phase : std_logic                    := '0';

  signal ch0_tx_rundisp_vec : std_logic_vector(3 downto 0);

  signal ch0_tx_en_pma_phase_align : std_logic := '0';

  signal ch0_rx_data_int                : std_logic_vector(7 downto 0);
  signal ch0_rx_k_int                   : std_logic;
  signal ch0_rx_disperr, ch0_rx_invcode : std_logic;

  signal ch0_rx_byte_is_aligned : std_logic;
  signal ch0_rx_comma_det       : std_logic;
  signal ch0_rx_cdr_rst         : std_logic := '0';
  signal ch0_rx_rec_clk_pad     : std_logic;
  signal ch0_rx_rec_clk         : std_logic;
  signal ch0_rx_data            : std_logic_vector(7 downto 0);
  signal ch0_rx_k               : std_logic;
  signal ch0_rx_enc_err         : std_logic;
  signal ch0_rx_divclk          : std_logic;
  signal ch0_rx_slide           : std_logic := '0';

  signal ch0_gtp_locked : std_logic;
  signal ch0_align_done : std_logic;
  signal ch0_rx_synced  : std_logic;

  signal ch0_gtp_clkout_int                                : std_logic_vector(1 downto 0);
  signal ch0_rx_enable_output, ch0_rx_enable_output_synced : std_logic;


  signal ch1_gtp_reset        : std_logic;
  signal ch1_gtp_loopback     : std_logic_vector(2 downto 0) := "000";
  signal ch1_gtp_reset_done   : std_logic;
  signal ch1_gtp_pll_lockdet  : std_logic;
  signal ch1_tx_pma_set_phase : std_logic                    := '0';

  signal ch1_tx_rundisp_vec : std_logic_vector(3 downto 0);

  signal ch1_tx_en_pma_phase_align : std_logic := '0';

  signal ch1_rx_data_int                : std_logic_vector(7 downto 0);
  signal ch1_rx_k_int                   : std_logic;
  signal ch1_rx_disperr, ch1_rx_invcode : std_logic;

  signal ch1_rx_byte_is_aligned : std_logic;
  signal ch1_rx_comma_det       : std_logic;
  signal ch1_rx_cdr_rst         : std_logic := '0';
  signal ch1_rx_rec_clk_pad     : std_logic;
  signal ch1_rx_rec_clk         : std_logic;
  signal ch1_rx_data            : std_logic_vector(7 downto 0);
  signal ch1_rx_k               : std_logic;
  signal ch1_rx_enc_err         : std_logic;
  
  signal ch1_rx_divclk          : std_logic;
  signal ch1_rx_slide           : std_logic := '0';

  signal ch1_gtp_locked : std_logic;
  signal ch1_align_done : std_logic;
  signal ch1_rx_synced  : std_logic;

  signal ch1_gtp_clkout_int                                : std_logic_vector(1 downto 0);
  signal ch1_rx_enable_output, ch1_rx_enable_output_synced : std_logic;

  signal ch0_rst_synced    : std_logic;
  signal ch0_rst_d0        : std_logic;
  signal ch0_reset_counter : unsigned(9 downto 0);

  signal ch1_rst_synced    : std_logic;
  signal ch1_rst_d0        : std_logic;
  signal ch1_reset_counter : unsigned(9 downto 0);

  signal ch0_rx_bitslide_int : std_logic_vector(4 downto 0);
  signal ch1_rx_bitslide_int : std_logic_vector(4 downto 0);

  signal ch0_ref_clk_in : std_logic_vector(1 downto 0);
  signal ch1_ref_clk_in : std_logic_vector(1 downto 0);


  signal ch0_disparity_set : std_logic;
  signal ch1_disparity_set : std_logic;

  signal ch0_tx_chardispmode : std_logic;
  signal ch1_tx_chardispmode : std_logic;

  signal ch0_tx_chardispval : std_logic;
  signal ch1_tx_chardispval : std_logic;



  component enc_8b10b
    port (
      clk_i     : in  std_logic;
      rst_n_i   : in  std_logic;
      ctrl_i    : in  std_logic;
      in_8b_i   : in  std_logic_vector(7 downto 0);
      err_o     : out std_logic;
      dispar_o  : out std_logic;
      out_10b_o : out std_logic_vector(9 downto 0));
  end component;

  signal ch0_rst_n : std_logic;
  signal ch1_rst_n : std_logic;

  signal ch0_cur_disp  : t_8b10b_disparity;
  signal ch0_disp_pipe : std_logic_vector(1 downto 0);
  signal ch1_cur_disp  : t_8b10b_disparity;
  signal ch1_disp_pipe : std_logic_vector(1 downto 0);
  
begin  -- rtl
  -------------------------------------------------------------------------------
  -- Channel 0 logic
  -------------------------------------------------------------------------------

  gen_with_channel0 : if(g_enable_ch0 /= 0) generate
    ch0_rst_n          <= not ch0_gtp_reset;
    ch0_tx_disparity_o <= ch0_disp_pipe(0);

    ch0_gtp_reset      <= ch0_rst_synced or std_logic(not ch0_reset_counter(ch0_reset_counter'left));
    ch0_rx_rec_clk_pad <= ch0_gtp_clkout_int(1);
    ch0_ref_clk_in(0)  <= gtp_clk_i;
    ch0_ref_clk_in(1)  <= '0';
    -- Near-end PMA loopback or loopback selected with ch1_loopen_vec_i
    ch0_gtp_loopback <= "010" when(ch0_loopen_i = '1') else
                        ch0_loopen_vec_i;

    gen_disp_ch0 : process(ch0_ref_clk_i)
    begin
      if rising_edge(ch0_ref_clk_i) then
        if(ch0_tx_chardispmode = '1' or ch0_rst_n = '0') then
          if(g_force_disparity = 0) then
            ch0_cur_disp <= RD_MINUS;
          else
            ch0_cur_disp <= RD_PLUS;
          end if;
          ch0_disp_pipe <= (others => '0');
        else
          ch0_cur_disp     <= f_next_8b10b_disparity8(ch0_cur_disp, ch0_tx_k_i, ch0_tx_data_i);
          ch0_disp_pipe(0) <= to_std_logic(ch0_cur_disp);
          ch0_disp_pipe(1) <= ch0_disp_pipe(0);
        end if;
      end if;
    end process;


    p_gen_reset_ch0 : process(ch0_ref_clk_i)
    begin
      if rising_edge(ch0_ref_clk_i) then

        ch0_rst_d0     <= ch0_rst_i;
        ch0_rst_synced <= ch0_rst_d0;

        if(ch0_rst_synced = '1') then
          ch0_reset_counter <= (others => '0');
        else
          if(ch0_reset_counter(ch0_reset_counter'left) = '0') then
            ch0_reset_counter <= ch0_reset_counter + 1;
          end if;
        end if;
      end if;
    end process;


    U_Rbclk_buf_ch0 : BUFIO2
      port map (
        DIVCLK       => ch0_rx_divclk,
        IOCLK        => open,
        SERDESSTROBE => open,
        I            => ch0_rx_rec_clk_pad);

    U_Rbclk_bufg_ch0 : BUFG
      port map (
        I => ch0_rx_divclk,
        O => ch0_rx_rec_clk
        );

    ch0_gtp_locked   <= ch0_gtp_pll_lockdet and ch0_gtp_reset_done;
    ch0_tx_enc_err_o <= '0';

    U_align_ch0 : gtp_phase_align
      generic map (
        g_simulation => g_simulation) 
      port map (
        gtp_rst_i                   => ch0_gtp_reset,
        gtp_tx_clk_i                => ch0_ref_clk_i,
        gtp_tx_en_pma_phase_align_o => ch0_tx_en_pma_phase_align,
        gtp_tx_pma_set_phase_o      => ch0_tx_pma_set_phase,
        align_en_i                  => ch0_gtp_locked,
        align_done_o                => ch0_align_done);

    U_bitslide_ch0 : gtp_bitslide
      generic map (
        g_simulation => g_simulation)
      port map (
        gtp_rst_i                => ch0_gtp_reset,
        gtp_rx_clk_i             => ch0_rx_rec_clk,
        gtp_rx_comma_det_i       => ch0_rx_comma_det,
        gtp_rx_byte_is_aligned_i => ch0_rx_byte_is_aligned,
        serdes_ready_i           => ch0_gtp_locked,
        gtp_rx_slide_o           => ch0_rx_slide,
        gtp_rx_cdr_rst_o         => ch0_rx_cdr_rst,
        bitslide_o               => ch0_rx_bitslide_int,
        synced_o                 => ch0_rx_synced);

    ch0_rx_bitslide_o    <= ch0_rx_bitslide_int(3 downto 0);
    ch0_rx_enable_output <= ch0_rx_synced and ch0_align_done;

    U_sync_oen_ch0 : gc_sync_ffs
      generic map (
        g_sync_edge => "positive")
      port map (
        clk_i    => ch0_rx_rec_clk,
        rst_n_i  => '1',
        data_i   => ch0_rx_enable_output,
        synced_o => ch0_rx_enable_output_synced,
        npulse_o => open,
        ppulse_o => open);

    p_force_proper_disparity_ch0 : process(ch0_ref_clk_i, ch0_gtp_reset)
    begin
      if (ch0_gtp_reset = '1') then
        ch0_disparity_set   <= '0';
        ch0_tx_chardispval  <= '0';
        ch0_tx_chardispmode <= '0';
      elsif rising_edge(ch0_ref_clk_i) then
        if(ch0_disparity_set = '0' and ch0_tx_k_i = '1' and ch0_tx_data_i = x"bc" and ch0_align_done = '1') then
          ch0_disparity_set <= '1';
          if(g_force_disparity = 0) then
            ch0_tx_chardispval <= '0';
          else
            ch0_tx_chardispval <= '1';
          end if;
          ch0_tx_chardispmode <= '1';
        else
          ch0_tx_chardispmode <= '0';
          ch0_tx_chardispval  <= '0';
        end if;
      end if;
    end process;

    p_gen_output_ch0 : process(ch0_rx_rec_clk, ch0_gtp_reset)
    begin
      if(ch0_gtp_reset = '1') then
        ch0_rx_data      <= (others => '0');
        ch0_rx_k         <= '0';
        ch0_rx_enc_err   <= '0';
        
      elsif rising_edge(ch0_rx_rec_clk) then
        if(ch0_rx_enable_output_synced = '0') then
-- make sure the output data is invalid when the link is down and that it will
-- trigger the sync loss detection
          ch0_rx_data      <= (others => '0');
          ch0_rx_k         <= '1';
          ch0_rx_enc_err   <= '1';
        else
          ch0_rx_data      <= ch0_rx_data_int;
          ch0_rx_k         <= ch0_rx_k_int;
          ch0_rx_enc_err   <= ch0_rx_disperr or ch0_rx_invcode;
        end if;
      end if;
    end process;


-- drive the recovered clock output
    ch0_rx_rbclk_o <= ch0_rx_rec_clk;
    -- drive ch0 ready indicator as well
    ch0_rdy_o <= ch0_rx_enable_output_synced;
    -- Note that the above clock assignment takes one delta delay in a simulator.
    -- In order to keep clock and data signals aligned, re-assign rx_data, rx_k
    -- and rx_enc_err (also adding one delta delay). This is purely necessary for
    -- proper simulation only.
    ch0_rx_data_o    <= ch0_rx_data;
    ch0_rx_k_o       <= ch0_rx_k;
    ch0_rx_enc_err_o <= ch0_rx_enc_err;
  end generate gen_with_channel0;

  -------------------------------------------------------------------------------
  -- Channel 1 logic
  -------------------------------------------------------------------------------

  gen_with_channel1 : if(g_enable_ch1 /= 0) generate

    ch1_rst_n          <= not ch1_gtp_reset;
    ch1_tx_disparity_o <= ch1_disp_pipe(1);

    ch1_gtp_reset      <= ch1_rst_synced or std_logic(not ch1_reset_counter(ch1_reset_counter'left));
    ch1_rx_rec_clk_pad <= ch1_gtp_clkout_int(1);
    ch1_ref_clk_in(0)  <= gtp_clk_i;
    ch1_ref_clk_in(1)  <= '0';
    -- Near-end PMA loopback or loopback selected with ch1_loopen_vec_i
    ch1_gtp_loopback <= "010" when(ch1_loopen_i = '1') else
                        ch1_loopen_vec_i;

    gen_disp_ch1 : process(ch1_ref_clk_i)
    begin
      if rising_edge(ch1_ref_clk_i) then
        if(ch1_tx_chardispmode = '1' or ch1_rst_n = '0') then
          if(g_force_disparity = 0) then
            ch1_cur_disp <= RD_MINUS;
          else
            ch1_cur_disp <= RD_PLUS;
          end if;
          ch1_disp_pipe <= (others => '0');
        else
          ch1_cur_disp     <= f_next_8b10b_disparity8(ch1_cur_disp, ch1_tx_k_i, ch1_tx_data_i);
          ch1_disp_pipe(0) <= to_std_logic(ch1_cur_disp);
          ch1_disp_pipe(1) <= ch1_disp_pipe(0);
        end if;
      end if;
    end process;

    p_gen_reset_ch1 : process(ch1_ref_clk_i)
    begin
      if rising_edge(ch1_ref_clk_i) then

        ch1_rst_d0     <= ch1_rst_i;
        ch1_rst_synced <= ch1_rst_d0;

        if(ch1_rst_synced = '1') then
          ch1_reset_counter <= (others => '0');
        else
          if(ch1_reset_counter(ch1_reset_counter'left) = '0') then
            ch1_reset_counter <= ch1_reset_counter + 1;
          end if;
        end if;
      end if;
    end process;

    U_Rbclk_buf_ch1 : BUFIO2
      port map (
        DIVCLK       => ch1_rx_divclk,
        IOCLK        => open,
        SERDESSTROBE => open,
        I            => ch1_rx_rec_clk_pad);

    U_Rbclk_bufg_ch1 : BUFG
      port map (
        I => ch1_rx_divclk,
        O => ch1_rx_rec_clk
        );

    ch1_gtp_locked   <= ch1_gtp_pll_lockdet and ch1_gtp_reset_done;
    ch1_tx_enc_err_o <= '0';

    U_align_ch1 : gtp_phase_align
      generic map (
        g_simulation => g_simulation) 
      port map (
        gtp_rst_i                   => ch1_gtp_reset,
        gtp_tx_clk_i                => ch1_ref_clk_i,
        gtp_tx_en_pma_phase_align_o => ch1_tx_en_pma_phase_align,
        gtp_tx_pma_set_phase_o      => ch1_tx_pma_set_phase,
        align_en_i                  => ch1_gtp_locked,
        align_done_o                => ch1_align_done);

    U_bitslide_ch1 : gtp_bitslide
      generic map (
        g_simulation => g_simulation)
      port map (
        gtp_rst_i                => ch1_gtp_reset,
        gtp_rx_clk_i             => ch1_rx_rec_clk,
        gtp_rx_comma_det_i       => ch1_rx_comma_det,
        gtp_rx_byte_is_aligned_i => ch1_rx_byte_is_aligned,
        serdes_ready_i           => ch1_gtp_locked,
        gtp_rx_slide_o           => ch1_rx_slide,
        gtp_rx_cdr_rst_o         => ch1_rx_cdr_rst,
        bitslide_o               => ch1_rx_bitslide_int,
        synced_o                 => ch1_rx_synced);

    ch1_rx_bitslide_o    <= ch1_rx_bitslide_int(3 downto 0);
    ch1_rx_enable_output <= ch1_rx_synced and ch1_align_done;

    U_sync_oen_ch1 : gc_sync_ffs
      generic map (
        g_sync_edge => "positive")
      port map (
        clk_i    => ch1_rx_rec_clk,
        rst_n_i  => '1',
        data_i   => ch1_rx_enable_output,
        synced_o => ch1_rx_enable_output_synced,
        npulse_o => open,
        ppulse_o => open);

    p_force_proper_disparity_ch1 : process(ch1_ref_clk_i, ch1_gtp_reset)
    begin
      if (ch1_gtp_reset = '1') then
        ch1_disparity_set   <= '0';
        ch1_tx_chardispval  <= '0';
        ch1_tx_chardispmode <= '0';
        
      elsif rising_edge(ch1_ref_clk_i) then
        if(ch1_disparity_set = '0' and ch1_tx_k_i = '1' and ch1_tx_data_i = x"bc" and ch1_align_done = '1') then
          ch1_disparity_set <= '1';
          if(g_force_disparity = 0) then
            ch1_tx_chardispval <= '0';
          else
            ch1_tx_chardispval <= '1';
          end if;
          ch1_tx_chardispmode <= '1';
        else
          ch1_tx_chardispmode <= '0';
          ch1_tx_chardispval  <= '0';
        end if;
      end if;
    end process;

    p_gen_output_ch1 : process(ch1_rx_rec_clk, ch1_rst_i)
    begin
      if(ch1_rst_i = '1') then
        ch1_rx_data    <= (others => '0');
        ch1_rx_k       <= '0';
        ch1_rx_enc_err <= '0';
        
      elsif rising_edge(ch1_rx_rec_clk) then
        if(ch1_rx_enable_output_synced = '0') then
-- make sure the output data is invalid when the link is down and that it will
-- trigger the sync loss detection
          ch1_rx_data    <= (others => '0');
          ch1_rx_k       <= '1';
          ch1_rx_enc_err <= '1';
        else
          ch1_rx_data    <= ch1_rx_data_int;
          ch1_rx_k       <= ch1_rx_k_int;
          ch1_rx_enc_err <= ch1_rx_disperr or ch1_rx_invcode;
        end if;
      end if;
    end process;

    ch1_rx_rbclk_o <= ch1_rx_rec_clk;
    ch1_rdy_o <= ch1_rx_enable_output_synced;
    -- Note that the above clock assignment takes one delta delay in a simulator.
    -- In order to keep clock and data signals aligned, re-assign rx_data, rx_k
    -- and rx_enc_err (also adding one delta delay). This is purely necessary for
    -- proper simulation only.
    ch1_rx_data_o    <= ch1_rx_data;
    ch1_rx_k_o       <= ch1_rx_k;
    ch1_rx_enc_err_o <= ch1_rx_enc_err;
  end generate gen_with_channel1;

  U_GTP_TILE_INST : WHITERABBITGTP_WRAPPER_TILE_SPARTAN6
    generic map
    (
      TILE_SIM_GTPRESET_SPEEDUP => g_simulation,  -- Set to 1 to speed up sim reset
      TILE_CLK25_DIVIDER_0      => 5,
      TILE_CLK25_DIVIDER_1      => 5,
      TILE_PLL_DIVSEL_FB_0      => 2,
      TILE_PLL_DIVSEL_FB_1      => 2,
      TILE_PLL_DIVSEL_REF_0     => 1,
      TILE_PLL_DIVSEL_REF_1     => 1,

      -- 
      TILE_PLL_SOURCE_0 => "PLL0",
      TILE_PLL_SOURCE_1 => "PLL1"
      )
    port map
    (
      ------------------------ Loopback and Powerdown Ports ----------------------
      LOOPBACK0_IN => ch0_gtp_loopback,
      LOOPBACK1_IN => ch1_gtp_loopback,
      --------------------------------- PLL Ports --------------------------------

      REFCLKOUT0_OUT => open,
      REFCLKOUT1_OUT => open,
      CLK00_IN       => ch0_ref_clk_in(0),
      CLK01_IN       => ch1_ref_clk_in(0),
      CLK10_IN       => ch0_ref_clk_in(1),
      CLK11_IN       => ch1_ref_clk_in(1),
      GTPRESET0_IN   => ch0_gtp_reset,
      GTPRESET1_IN   => ch1_gtp_reset,
      PLLLKDET0_OUT  => ch0_gtp_pll_lockdet,
      PLLLKDET1_OUT  => ch1_gtp_pll_lockdet,
      RESETDONE0_OUT => ch0_gtp_reset_done,
      RESETDONE1_OUT => ch1_gtp_reset_done,

      ----------------------- Receive Ports - 8b10b Decoder ----------------------
      RXCHARISK0_OUT    => ch0_rx_k_int,
      RXCHARISK1_OUT    => ch1_rx_k_int,
      RXDISPERR0_OUT    => ch0_rx_disperr,
      RXDISPERR1_OUT    => ch1_rx_disperr,
      RXNOTINTABLE0_OUT => ch0_rx_invcode,
      RXNOTINTABLE1_OUT => ch1_rx_invcode,

      --------------- Receive Ports - Comma Detection and Alignment --------------
      RXBYTEISALIGNED0_OUT => ch0_rx_byte_is_aligned,
      RXBYTEISALIGNED1_OUT => ch1_rx_byte_is_aligned,
      RXCOMMADET0_OUT      => ch0_rx_comma_det,
      RXCOMMADET1_OUT      => ch1_rx_comma_det,
      RXSLIDE0_IN          => ch0_rx_slide,
      RXSLIDE1_IN          => ch1_rx_slide,

      ------------------- Receive Ports - RX Data Path interface -----------------
      RXDATA0_OUT   => ch0_rx_data_int,
      RXDATA1_OUT   => ch1_rx_data_int,
      RXUSRCLK0_IN  => ch0_rx_rec_clk,
      RXUSRCLK1_IN  => ch1_rx_rec_clk,
      RXUSRCLK20_IN => ch0_rx_rec_clk,
      RXUSRCLK21_IN => ch1_rx_rec_clk,

      ------- Receive Ports - RX Driver,OOB signalling,Coupling and Eq.,CDR ------
      RXCDRRESET0_IN => ch0_rx_cdr_rst,
      RXCDRRESET1_IN => ch1_rx_cdr_rst,
      RXN0_IN        => pad_rxn0_i,
      RXN1_IN        => pad_rxn1_i,
      RXP0_IN        => pad_rxp0_i,
      RXP1_IN        => pad_rxp1_i,

      ---------------------------- TX/RX Datapath Ports --------------------------
      GTPCLKFBEAST_OUT => open,
      GTPCLKFBWEST_OUT => open,
      GTPCLKOUT0_OUT   => ch0_gtp_clkout_int,
      GTPCLKOUT1_OUT   => ch1_gtp_clkout_int,
      ------------------- Transmit Ports - 8b10b Encoder Control -----------------
      TXCHARISK0_IN    => ch0_tx_k_i,
      TXCHARISK1_IN    => ch1_tx_k_i,
      TXRUNDISP0_OUT   => ch0_tx_rundisp_vec,
      TXRUNDISP1_OUT   => ch1_tx_rundisp_vec,

      TXCHARDISPMODE0_IN => ch0_tx_chardispmode,
      TXCHARDISPMODE1_IN => ch1_tx_chardispmode,
      TXCHARDISPVAL0_IN  => ch0_tx_chardispval,
      TXCHARDISPVAL1_IN  => ch1_tx_chardispval,

      --------------- Transmit Ports - TX Buffer and Phase Alignment -------------
      TXENPMAPHASEALIGN0_IN => ch0_tx_en_pma_phase_align,
      TXENPMAPHASEALIGN1_IN => ch1_tx_en_pma_phase_align,
      TXPMASETPHASE0_IN     => ch0_tx_pma_set_phase,
      TXPMASETPHASE1_IN     => ch1_tx_pma_set_phase,
      ------------------ Transmit Ports - TX Data Path interface -----------------
      TXDATA0_IN            => ch0_tx_data_i,
      TXDATA1_IN            => ch1_tx_data_i,
      TXUSRCLK0_IN          => ch0_ref_clk_i,
      TXUSRCLK1_IN          => ch1_ref_clk_i,
      TXUSRCLK20_IN         => ch0_ref_clk_i,
      TXUSRCLK21_IN         => ch1_ref_clk_i,
      --------------- Transmit Ports - TX Driver and OOB signalling --------------
      TXN0_OUT              => pad_txn0_o,
      TXN1_OUT              => pad_txn1_o,
      TXP0_OUT              => pad_txp0_o,
      TXP1_OUT              => pad_txp1_o,
      --------------- Transmit Ports - TX PRBS Generator -------------------------
      TXENPRBSTST0_IN       => ch0_tx_prbs_sel_i,
      TXENPRBSTST1_IN       => ch1_tx_prbs_sel_i

      );


end rtl;
