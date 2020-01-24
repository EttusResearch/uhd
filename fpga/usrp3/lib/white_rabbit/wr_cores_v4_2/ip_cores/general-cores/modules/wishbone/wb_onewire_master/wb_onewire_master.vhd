------------------------------------------------------------------------------
-- Title      : Wishbone 1-Wire Master
-- Project    : General Cores Library (gencores)
------------------------------------------------------------------------------
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-05-18
-- Last update: 2012-02-23
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Wishbone Dallas/Maxim Semiconductor 1-Wire master.
-------------------------------------------------------------------------------
-- 
-- Based on sockit_owm project Copyright (c) 2010 Iztok Jeras.
-- http://opencores.org/project,sockit_owm
--
-- sockit_owm RTL is licensed under LGPL 3.

-- wb_onewire_master.vhd copyright (c) 2011 CERN
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2011-09-18  1.0      twlostow        Created
-- 2016-08-24  1.1      jpospisi        propagated CDR_N/O generics up the
--                                        hierarchy; added assignments to (new)
--                                        unspecified WB signals
-------------------------------------------------------------------------------

library ieee;
use ieee.STD_LOGIC_1164.all;

use work.gencores_pkg.all;
use work.wishbone_pkg.all;

entity wb_onewire_master is

  generic(
    g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity : t_wishbone_address_granularity := WORD;
    g_num_ports           : integer                        := 1;
    g_ow_btp_normal       : string                         := "5.0";
    g_ow_btp_overdrive    : string                         := "1.0";
    g_CDR_N               : integer                        := 4; -- normal    mode
    g_CDR_O               : integer                        := 0  -- overdrive mode
    );  

  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    wb_cyc_i   : in  std_logic;
    wb_sel_i   : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_adr_i   : in  std_logic_vector(2 downto 0);
    wb_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_ack_o   : out std_logic;
    wb_int_o   : out std_logic;
    wb_stall_o : out std_logic;

    owr_pwren_o : out std_logic_vector(g_num_ports -1 downto 0);
    owr_en_o    : out std_logic_vector(g_num_ports -1 downto 0);
    owr_i       : in  std_logic_vector(g_num_ports -1 downto 0)
    );

end wb_onewire_master;


architecture rtl of wb_onewire_master is

  component sockit_owm
    generic(
      BTP_N : string;
      BTP_O : string;
      OWN   : integer;
      CDR_N : integer;
      CDR_O : integer);

    port(
      clk     : in  std_logic;
      rst     : in  std_logic;
      bus_ren : in  std_logic;
      bus_wen : in  std_logic;
      bus_adr : in  std_logic_vector(0 downto 0);
      bus_wdt : in  std_logic_vector(31 downto 0);
      bus_rdt : out std_logic_vector(31 downto 0);
      bus_irq : out std_logic;
      owr_p   : out std_logic_vector(OWN-1 downto 0);
      owr_e   : out std_logic_vector(OWN-1 downto 0);
      owr_i   : in  std_logic_vector(OWN-1 downto 0)
      );
  end component;

  signal bus_wen : std_logic;
  signal bus_ren : std_logic;
  signal rst     : std_logic;

  signal slave_in  : t_wishbone_slave_in;
  signal slave_out : t_wishbone_slave_out;

  signal adp_out : t_wishbone_master_out;
  signal adp_in  : t_wishbone_master_in;

  signal rdat_int : std_logic_vector(31 downto 0);
  
begin  -- rtl

  slave_in.adr(2 downto 0)                          <= wb_adr_i;
  slave_in.adr(c_wishbone_address_width-1 downto 3) <= (others => '0');
  slave_in.cyc                                      <= wb_cyc_i;
  slave_in.stb                                      <= wb_stb_i;
  slave_in.sel                                      <= wb_sel_i;
  slave_in.dat                                      <= wb_dat_i;
  slave_in.we                                       <= wb_we_i;

  wb_int_o   <= slave_out.int;
  wb_dat_o   <= slave_out.dat;
  wb_stall_o <= slave_out.stall;
  wb_ack_o   <= slave_out.ack;


  U_Slave_adapter : wb_slave_adapter
    generic map (
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => true,
      g_slave_mode         => g_interface_mode,
      g_slave_granularity  => g_address_granularity)
    port map (
      clk_sys_i => clk_sys_i,
      rst_n_i   => rst_n_i,

      slave_i  => slave_in,
      slave_o  => slave_out,
      master_i => adp_in,
      master_o => adp_out);

  
  bus_wen <= adp_out.cyc and adp_out.stb and adp_out.we and not adp_in.ack;
  bus_ren <= adp_out.cyc and adp_out.stb and not (adp_out.we or adp_in.ack);

  process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        adp_in.ack <= '0';
      else
        adp_in.ack <= adp_out.stb and adp_out.cyc and not adp_in.ack;
        adp_in.dat <= rdat_int;
      end if;
    end if;
  end process;

  rst <= not rst_n_i;

  Wrapped_1wire : sockit_owm
    generic map (
      BTP_N => g_ow_btp_normal,
      BTP_O => g_ow_btp_overdrive,
      OWN   => g_num_ports,
      CDR_N => g_CDR_N,
      CDR_O => g_CDR_O)
    port map (
      clk     => clk_sys_i,
      rst     => rst,
      bus_ren => bus_ren,
      bus_wen => bus_wen,
      bus_adr => adp_out.adr(0 downto 0),
      bus_wdt => adp_out.dat,
      bus_rdt => rdat_int,
      bus_irq => adp_in.int,
      owr_p   => owr_pwren_o,
      owr_e   => owr_en_o,
      owr_i   => owr_i);

  adp_in.err <= '0';
  adp_in.rty <= '0';
  adp_in.stall <= '0';

end rtl;

