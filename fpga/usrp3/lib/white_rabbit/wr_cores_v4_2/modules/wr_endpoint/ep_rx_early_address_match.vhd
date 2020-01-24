-------------------------------------------------------------------------------
-- Title      : Rx Early Address Match
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rx_early_address_match.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 - 2015 CERN / BE-CO-HT
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

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.gencores_pkg.all;              -- for gc_crc_gen
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;

-- 1st stage in the RX pipeline: early address matching/header parsing
-- to filter out pause and HP frames in advance.

entity ep_rx_early_address_match is
  generic
    (
      g_early_hp_detection : boolean := false);
  port(
    clk_sys_i : in std_logic;
    clk_rx_i  : in std_logic;

    rst_n_sys_i : in std_logic;
    rst_n_rx_i  : in std_logic;

    snk_fab_i : in  t_ep_internal_fabric;
    src_fab_o : out t_ep_internal_fabric;

    match_done_o         : out std_logic;
    match_is_hp_o        : out std_logic;
    
    -- indicate that pause was detected
    match_is_pause_o     : out std_logic;
    
    -- tell quanta (for prio-based pause it is the greatest of all prios)
    match_pause_quanta_o : out std_logic_vector(15 downto 0);
    
    -- mask with priorities which shall be PAUSEd
    match_pause_prio_mask_o : out std_logic_vector(7 downto 0);
   
    -- once the PAUSE frame is decoded, it requests pausing from SWcore
    match_pause_p_o      : out std_logic; 
  
    regs_i : in t_ep_out_registers
    );

end ep_rx_early_address_match;

architecture behavioral of ep_rx_early_address_match is

  signal hdr_offset : std_logic_vector(16 downto 0);

  signal at_ethertype    : std_logic;
  signal at_vid          : std_logic;
  signal is_tagged       : std_logic;
  signal pause_match_int : std_logic_vector(7 downto 0);

  signal comb_pcp_matches_hp : std_logic;
  signal done_int            : std_logic;

  signal pause_prio_mask      : std_logic_vector(7 downto 0);
  signal match_pause_req     : std_logic;
  signal match_is_pause      : std_logic;
  signal is_perprio_pause    : std_logic;
  signal match_pause_quanta  : std_logic_vector(15 downto 0);

  function f_compare_slv (a : std_logic_vector; b : std_logic_vector) return std_logic is
  begin
    if(a = b) then
      return '1';
    else
      return '0';
    end if;
  end f_compare_slv;

 
  
begin  -- behavioral

  at_ethertype <= hdr_offset(5) and snk_fab_i.dvalid;
  at_vid       <= hdr_offset(7) and snk_fab_i.dvalid and is_tagged;

  src_fab_o <= snk_fab_i;

  p_hdr_offset_sreg : process(clk_rx_i)
  begin
    if rising_edge(clk_rx_i) then
      if (rst_n_rx_i = '0' or snk_fab_i.sof = '1') then
        hdr_offset(hdr_offset'left downto 1) <= (others => '0');
        hdr_offset(0)                        <= '1';
      elsif(snk_fab_i.dvalid = '1') then
        hdr_offset <= hdr_offset(hdr_offset'left-1 downto 0) & '0';
      end if;
    end if;
  end process;

  p_match_pause : process(clk_rx_i)
  begin
    if rising_edge(clk_rx_i) then
      if rst_n_rx_i = '0' or snk_fab_i.sof = '1' then
        pause_match_int      <= (others => '0');
        match_pause_quanta   <= (others => '0');
        match_is_pause       <= '0';
        pause_prio_mask      <= (others => '0');
        match_pause_req      <= '0';
        is_perprio_pause     <= '0';
      else
        if(snk_fab_i.dvalid = '1') then
          if(hdr_offset(0) = '1') then
            pause_match_int (0) <= f_compare_slv(snk_fab_i.data, x"0180");
          end if;
          if(hdr_offset(1) = '1') then
            pause_match_int (1) <= f_compare_slv(snk_fab_i.data, x"c200");
          end if;
          if(hdr_offset(2) = '1') then
            pause_match_int (2) <= f_compare_slv(snk_fab_i.data, x"0001");
          end if;
--           if(hdr_offset(3) = '1') then
--             pause_match_int (3) <= f_compare_slv(snk_fab_i.data, regs_i.mach_o);
--           end if;
--           if(hdr_offset(4) = '1') then
--             pause_match_int (4) <= f_compare_slv(snk_fab_i.data, regs_i.macl_o(31 downto 16));
--           end if;
--           if(hdr_offset(5) = '1') then
--             pause_match_int (5) <= f_compare_slv(snk_fab_i.data, regs_i.macl_o(15 downto 0));
--           end if;
          if(hdr_offset(6) = '1') then
            pause_match_int (3) <= f_compare_slv(snk_fab_i.data, x"8808");
          end if;
          if(hdr_offset(7) = '1') then
            pause_match_int (4) <= f_compare_slv(snk_fab_i.data, x"0001"); -- 802.3 PAUSE
            pause_match_int (5) <= f_compare_slv(snk_fab_i.data, x"0101"); -- 802.1Q PAUSE (per-prio)
          end if;
          if(hdr_offset(8) = '1') then
            if(f_compare_slv(pause_match_int, b"0001_1111") = '1') then  -- 802.3 PAUSE

              match_is_pause            <= '1'; -- to indicate that frame shall be dropped

              if(regs_i.fcr_rxpause_o = '1') then
                match_pause_req         <= '1';
                match_pause_quanta      <= snk_fab_i.data;  
                pause_prio_mask         <= (others => '1');
              end if;

            elsif(f_compare_slv(pause_match_int, b"0010_1111") = '1') then  -- 802.1Q PAUSE (per-prio)

              match_is_pause          <= '1'; -- to indicate that frame shall be dropped

              if(regs_i.fcr_rxpause_802_1q_o = '1') then
                pause_prio_mask       <=  snk_fab_i.data(7 downto 0);
                is_perprio_pause      <= '1';
              end if;
  
            end if;
          end if;
          if (is_perprio_pause ='1' and ((hdr_offset(16 downto 9) and pause_prio_mask) /= b"0000_0000")) then
            if(snk_fab_i.data > match_pause_quanta) then
              match_pause_quanta <= snk_fab_i.data;
            end if;  
          end if;
          if(hdr_offset(16) = '1' and is_perprio_pause = '1') then
            match_pause_req           <= '1';
          end if;
        end if;
      end if;
    end if;
  end process;

  match_is_pause_o        <= match_is_pause;
  match_pause_prio_mask_o <= pause_prio_mask;
  match_pause_quanta_o    <= match_pause_quanta;

  gen_with_early_hp_det: if(g_early_hp_detection) generate
  -- ML: the p_match_hp is not used, instead identification of HP is done in RTU 
  p_match_hp : process(clk_rx_i)
    variable index : integer;
  begin
    
    if rising_edge(clk_rx_i) then
      index := to_integer(unsigned(snk_fab_i.data(15 downto 13)));

      if rst_n_rx_i = '0' or snk_fab_i.sof = '1' then
        is_tagged     <= '0';
        match_is_hp_o <= '0';
      else
        if(at_ethertype = '1') then
          is_tagged <= f_compare_slv(snk_fab_i.data, x"8100");
        end if;

        if (at_vid = '1') then
          if(regs_i.rfcr_a_hp_o = '1' and regs_i.rfcr_hpap_o(index) = '1' and is_tagged = '1') then
            match_is_hp_o <= '1';
          else
            match_is_hp_o <= '0';
          end if;
        end if;
      end if;
    end if;
  end process;
  end generate gen_with_early_hp_det;
  
  gen_without_early_hp_det: if(not g_early_hp_detection) generate
    match_is_hp_o <='0';
  end generate gen_without_early_hp_det;

  p_gen_done : process(clk_rx_i)
  begin
    if rising_edge(clk_rx_i) then
      if rst_n_rx_i = '0' or snk_fab_i.sof = '1' then
        done_int <= '0';
      else
        if hdr_offset(8) = '1' or snk_fab_i.error = '1' then
          done_int <= '1';
        end if;
      end if;
    end if;
  end process;

  U_sync_done : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_sys_i,
      data_i   => done_int,
      ppulse_o => match_done_o);

  U_sync_pause : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_sys_i,
      data_i   => match_pause_req,
      ppulse_o => match_pause_p_o);

end behavioral;




