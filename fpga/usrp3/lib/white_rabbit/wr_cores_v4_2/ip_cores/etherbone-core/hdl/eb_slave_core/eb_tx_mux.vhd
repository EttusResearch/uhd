------------------------------------------------------------------------------
-- Title      : Etherbone TX MUX
-- Project    : Etherbone Core
------------------------------------------------------------------------------
-- File       : eb_tux_mux.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2013-04-08
-- Last update: 2013-04-08
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Combines output streams into a packet
-------------------------------------------------------------------------------
-- Copyright (c) 2013 GSI
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2013-04-08  1.0      terpstra        Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.eb_internals_pkg.all;

entity eb_tx_mux is
  port(
    clk_i       : in  std_logic;
    rstn_i      : in  std_logic;
    
    tag_pop_o   : out std_logic;
    tag_dat_i   : in  t_tag;
    tag_empty_i : in  std_logic;
    
    pass_pop_o   : out std_logic;
    pass_dat_i   : in  t_wishbone_data;
    pass_empty_i : in  std_logic;
    
    cfg_pop_o    : out std_logic;
    cfg_dat_i    : in  t_wishbone_data;
    cfg_empty_i  : in  std_logic;
    
    wbm_pop_o    : out std_logic;
    wbm_dat_i    : in  t_wishbone_data;
    wbm_empty_i  : in  std_logic;
    
    skip_stb_o   : out std_logic;
    skip_stall_i : in  std_logic;
    
    tx_cyc_o     : out std_logic;
    tx_stb_o     : out std_logic;
    tx_dat_o     : out t_wishbone_data;
    tx_stall_i   : in  std_logic);
end eb_tx_mux;

architecture rtl of eb_tx_mux is

  signal r_skip      : std_logic;
  signal r_tx_cyc    : std_logic;
  signal r_tx_stb    : std_logic;
  signal s_can_tx    : std_logic;
  signal s_dat_mux   : t_wishbone_data;
  signal s_tag_mux   : std_logic;
  signal s_pass_mux  : std_logic;
  signal s_cfg_mux   : std_logic;
  signal s_wbm_mux   : std_logic;
  signal s_tag_pop   : std_logic;
  signal r_tag_valid : std_logic;
  signal r_tag_value : t_tag;
  
begin

  -- We can write whenever TX is unstalled and/or not full
  s_can_tx <= not ((r_tx_cyc and r_tx_stb and tx_stall_i) or (r_skip and skip_stall_i));
  
  skip_stb_o <= r_skip;
  tx_cyc_o   <= r_tx_cyc;
  tx_stb_o   <= r_tx_stb;
  
  tx_out : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_skip   <= '0';
      r_tx_cyc <= '0';
      r_tx_stb <= '0';
      tx_dat_o <= (others => '0');
    elsif rising_edge(clk_i) then
      if s_can_tx = '1' then -- is prior operation complete?
        r_tx_stb <= not s_tag_mux and r_tag_valid;
        r_skip <= '0';
        tx_dat_o <= s_dat_mux;
        -- Control the TX cycle line
        if r_tag_valid = '1' then
          case r_tag_value is
            when c_tag_drop_tx => r_tx_cyc <= '0';
            when c_tag_pass_tx => r_tx_cyc <= '1';
            when c_tag_skip_tx => r_skip   <= '1';
            when others        => null;
          end case;
        end if;
      end if;
    end if;
  end process;
  
  with r_tag_value select
  s_dat_mux <= 
    pass_dat_i      when c_tag_pass_tx,
    pass_dat_i      when c_tag_pass_on,
    cfg_dat_i       when c_tag_cfg_req,
    pass_dat_i      when c_tag_cfg_ign,
    wbm_dat_i       when c_tag_wbm_req,
    pass_dat_i      when c_tag_wbm_ign,
    (others => '-') when others; -- c_tag_skip_tx, c_tag_drop_tx
  
  with r_tag_value select
  s_tag_mux <=
    pass_empty_i                  when c_tag_pass_tx,
    pass_empty_i                  when c_tag_pass_on,
    cfg_empty_i                   when c_tag_cfg_req,
    (pass_empty_i or cfg_empty_i) when c_tag_cfg_ign,
    wbm_empty_i                   when c_tag_wbm_req,
    (pass_empty_i or wbm_empty_i) when c_tag_wbm_ign,
    '0'                           when others; -- c_tag_skip_tx, c_tag_drop_tx
  
  with r_tag_value select
  s_pass_mux <=
    pass_empty_i                  when c_tag_pass_tx,
    pass_empty_i                  when c_tag_pass_on,
    (pass_empty_i or cfg_empty_i) when c_tag_cfg_ign,
    (pass_empty_i or wbm_empty_i) when c_tag_wbm_ign,
    '1'                           when others; -- c_tag_skip_tx, c_tag_drop_tx, c_tag_cfg_req, c_tag_wbm_req
  
  with r_tag_value select
  s_cfg_mux <=
    cfg_empty_i                   when c_tag_cfg_req,
    (pass_empty_i or cfg_empty_i) when c_tag_cfg_ign,
    '1'                           when others;
  
  with r_tag_value select
  s_wbm_mux <=
    wbm_empty_i                   when c_tag_wbm_req,
    (pass_empty_i or wbm_empty_i) when c_tag_wbm_ign,
    '1'                           when others;
  
  -- Pop the queue we fed into TX
  pass_pop_o <= s_can_tx and r_tag_valid and not s_pass_mux;
  cfg_pop_o  <= s_can_tx and r_tag_valid and not s_cfg_mux;
  wbm_pop_o  <= s_can_tx and r_tag_valid and not s_wbm_mux;
  s_tag_pop  <= s_can_tx and r_tag_valid and not s_tag_mux;
  
  -- Pop the tag FIFO if the register is empty/emptied
  tag_pop_o <= not tag_empty_i and (s_tag_pop or not r_tag_valid);
  tag_in : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_tag_valid <= '0';
      r_tag_value <= c_tag_drop_tx;
    elsif rising_edge(clk_i) then
      if s_tag_pop = '1' or r_tag_valid = '0' then
        r_tag_valid <= not tag_empty_i;
        r_tag_value <= tag_dat_i;
      end if;
    end if;
  end process;
  
end rtl;
