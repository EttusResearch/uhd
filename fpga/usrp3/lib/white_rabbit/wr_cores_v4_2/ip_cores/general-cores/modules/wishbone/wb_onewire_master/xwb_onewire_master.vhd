--------------------------------------------------------------------------------
--  Modifications:
--      2016-08-24: by Jan Pospisil (j.pospisil@cern.ch)
--          * propagated CDR_N/O generics up the hierarchy; added assignments
--            to (new) unspecified WB signals
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.wishbone_pkg.all;

entity xwb_onewire_master is
  generic(
    g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity : t_wishbone_address_granularity := WORD;
    g_num_ports           : integer                        := 1;
    g_ow_btp_normal       : string                         := "5.0";
    g_ow_btp_overdrive    : string                         := "1.0";
    g_CDR_N               : integer                        := 4; -- normal    mode
    g_CDR_O               : integer                        := 0  -- overdrive mode
    );

  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    -- Wishbone
    slave_i : in  t_wishbone_slave_in;
    slave_o : out t_wishbone_slave_out;
    desc_o  : out t_wishbone_device_descriptor;

    owr_pwren_o : out std_logic_vector(g_num_ports -1 downto 0);
    owr_en_o    : out std_logic_vector(g_num_ports -1 downto 0);
    owr_i       : in  std_logic_vector(g_num_ports -1 downto 0)

    );

end xwb_onewire_master;

architecture rtl of xwb_onewire_master is

  component wb_onewire_master
    generic (
      g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity : t_wishbone_address_granularity := WORD;
      g_num_ports           : integer;
      g_ow_btp_normal       : string;
      g_ow_btp_overdrive    : string;
      g_CDR_N               : integer;
      g_CDR_O               : integer);
    port (
      clk_sys_i   : in  std_logic;
      rst_n_i     : in  std_logic;
      
      wb_cyc_i    : in  std_logic;
      wb_sel_i    : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_stb_i    : in  std_logic;
      wb_we_i     : in  std_logic;
      wb_adr_i    : in  std_logic_vector(2 downto 0);
      wb_dat_i    : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o    : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_ack_o    : out std_logic;
      wb_int_o    : out std_logic;
      wb_stall_o  : out std_logic;
      
      owr_pwren_o : out std_logic_vector(g_num_ports -1 downto 0);
      owr_en_o    : out std_logic_vector(g_num_ports -1 downto 0);
      owr_i       : in  std_logic_vector(g_num_ports -1 downto 0));
  end component;
  
begin  -- rtl

  U_Wrapped_1W : wb_onewire_master
    generic map (
      g_interface_mode      => g_interface_mode,
      g_address_granularity => g_address_granularity,
      g_num_ports           => g_num_ports,
      g_ow_btp_normal       => g_ow_btp_normal,
      g_ow_btp_overdrive    => g_ow_btp_overdrive,
      g_CDR_N               => g_CDR_N,
      g_CDR_O               => g_CDR_O)
    port map (
      clk_sys_i   => clk_sys_i,
      rst_n_i     => rst_n_i,
      wb_cyc_i    => slave_i.cyc,
      wb_sel_i    => slave_i.sel,
      wb_stb_i    => slave_i.stb,
      wb_we_i     => slave_i.we,
      wb_adr_i    => slave_i.adr(2 downto 0),
      wb_dat_i    => slave_i.dat,
      wb_dat_o    => slave_o.dat,
      wb_ack_o    => slave_o.ack,
      wb_int_o    => slave_o.int,
      wb_stall_o  => slave_o.stall,
      owr_pwren_o => owr_pwren_o,
      owr_en_o    => owr_en_o,
      owr_i       => owr_i);

  slave_o.err <= '0';
  slave_o.rty <= '0';

  desc_o <= (others => '0');
  
end rtl;
