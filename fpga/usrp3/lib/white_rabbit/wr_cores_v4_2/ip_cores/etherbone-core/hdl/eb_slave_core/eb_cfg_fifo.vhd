------------------------------------------------------------------------------
-- Title      : Etherbone Config Master FIFO
-- Project    : Etherbone Core
------------------------------------------------------------------------------
-- File       : eb_cfg_fifo.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2013-04-08
-- Last update: 2013-04-08
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Buffers Config space requests
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

entity eb_cfg_fifo is
  generic(
    g_sdb_address : t_wishbone_address);
  port(
    clk_i       : in  std_logic;
    rstn_i      : in  std_logic;
    
    errreg_i    : in  std_logic_vector(63 downto 0);
    
    cfg_i       : in  t_wishbone_slave_in;
    cfg_o       : out t_wishbone_slave_out;
    
    fsm_stb_i   : in  std_logic;
    fsm_adr_i   : in  t_wishbone_address;
    fsm_full_o  : out std_logic;

    mux_pop_i   : in  std_logic;
    mux_dat_o   : out t_wishbone_data;
    mux_empty_o : out std_logic;
    
    my_mac_o    : out std_logic_vector(47 downto 0);
    my_ip_o     : out std_logic_vector(31 downto 0);
    my_port_o   : out std_logic_vector(15 downto 0));
end eb_cfg_fifo;

architecture rtl of eb_cfg_fifo is
  
  constant c_pad  : std_logic_vector(31 downto 16) := (others => '0');
  
  signal r_mac  : std_logic_vector(6*8-1 downto 0);
  signal r_ip   : std_logic_vector(4*8-1 downto 0);
  signal r_port : std_logic_vector(2*8-1 downto 0);
  
  signal s_fsm_adr     : std_logic_vector(2 downto 0);
  signal s_fifo_adr    : std_logic_vector(2 downto 0);
  signal s_fifo_empty  : std_logic;
  signal s_fifo_pop    : std_logic;
  signal r_cache_empty : std_logic;
  signal r_cache_adr   : std_logic_vector(2 downto 0);
  
  impure function update(x : std_logic_vector) return std_logic_vector is
    alias    y : std_logic_vector(x'length-1 downto 0) is x;
    variable o : std_logic_vector(x'length-1 downto 0);
  begin
    for i in (y'length/8)-1 downto 0 loop
      if cfg_i.sel(i) = '1' then
        o(i*8+7 downto i*8) := cfg_i.dat(i*8+7 downto i*8);
      else
        o(i*8+7 downto i*8) := y(i*8+7 downto i*8);
      end if;
    end loop;
    
    return o;
  end update;
  
begin

  cfg_o.int <= '0';
  cfg_o.err <= '0';
  cfg_o.rty <= '0';
  cfg_o.stall <= '0';

  cfg_wbs : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_mac  <= x"D15EA5EDBEEF";
      r_ip   <= x"C0A80064";
      r_port <= x"EBD0";
      
      cfg_o.ack <= '0';
      cfg_o.dat <= (others => '0');
    elsif rising_edge(clk_i) then
      if cfg_i.cyc = '1' and cfg_i.stb = '1' and cfg_i.we = '1' then
        case to_integer(unsigned(cfg_i.adr(4 downto 2))) is
          when 4 => r_mac(47 downto 32) <= update(r_mac(47 downto 32));
          when 5 => r_mac(31 downto  0) <= update(r_mac(31 downto  0));
          when 6 => r_ip   <= update(r_ip);
          when 7 => r_port <= update(r_port);
          when others => null;
        end case;
      end if;
      
      cfg_o.ack <= cfg_i.cyc and cfg_i.stb;
      
      case to_integer(unsigned(cfg_i.adr(4 downto 2))) is
        when 0 => cfg_o.dat <= errreg_i(63 downto 32);
        when 1 => cfg_o.dat <= errreg_i(31 downto  0);
        when 2 => cfg_o.dat <= (others => '0');
        when 3 => cfg_o.dat <= g_sdb_address;
        when 4 => cfg_o.dat <= c_pad & r_mac(47 downto 32);
        when 5 => cfg_o.dat <= r_mac(31 downto 0);
        when 6 => cfg_o.dat <= r_ip;
        when others => cfg_o.dat <= c_pad & r_port;
      end case;
      
    end if;
  end process;

  -- Discard writes.
  s_fsm_adr  <= fsm_adr_i(4 downto 2);
  
  fifo : eb_fifo
    generic map(
      g_width => 3,
      g_size  => c_queue_depth)
    port map(
      clk_i     => clk_i,
      rstn_i    => rstn_i,
      w_full_o  => fsm_full_o,
      w_push_i  => fsm_stb_i,
      w_dat_i   => s_fsm_adr,
      r_empty_o => s_fifo_empty,
      r_pop_i   => s_fifo_pop,
      r_dat_o   => s_fifo_adr);
      
  s_fifo_pop <= not s_fifo_empty and (r_cache_empty or mux_pop_i);
  
  cache : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_cache_empty <= '1';
      r_cache_adr   <= (others => '0');
    elsif rising_edge(clk_i) then
      if r_cache_empty = '1' or mux_pop_i = '1' then
        r_cache_empty <= s_fifo_empty;
        r_cache_adr   <= s_fifo_adr;
      end if;
    end if;
  end process;
  
  mux_empty_o <= r_cache_empty;
  
  with r_cache_adr select 
  mux_dat_o <= 
    errreg_i(63 downto 32)      when "000",
    errreg_i(31 downto  0)      when "001",
    x"00000000"                 when "010",
    g_sdb_address               when "011",
    c_pad & r_mac(47 downto 32) when "100",
            r_mac(31 downto  0) when "101",
    r_ip                        when "110",
    c_pad & r_port              when others;
    
  my_mac_o  <= r_mac;
  my_ip_o   <= r_ip;
  my_port_o <= r_port;

end rtl;
