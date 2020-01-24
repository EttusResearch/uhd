library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;

package etherbone_pkg is
  constant c_etherbone_sdb : t_sdb_device := (
    abi_class     => x"0000", -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"4", --32-bit port granularity
    sdb_component => (
    addr_first    => x"0000000000000000",
    addr_last     => x"00000000000000ff",
    product => (
    vendor_id     => x"0000000000000651", -- GSI
    device_id     => x"68202b22",
    version       => x"00000001",
    date          => x"20130211",
    name          => "Etherbone-Config   ")));
  
  constant c_ebm_sdb : t_sdb_device := (
    abi_class     => x"0000", -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7", -- 8/16/32-bit port granularity
    sdb_component => (
    addr_first    => x"0000000000000000",
    addr_last     => x"0000000000ffffff",
    product => (
    vendor_id     => x"0000000000000651", -- GSI
    device_id     => x"00000815",
    version       => x"00000002",
    date          => x"20140615",
    name          => "Etherbone_Master   ")));
  
  function f_hi_adr_bits(ebm : t_sdb_device) return natural;
  
  
  component eb_raw_slave is
    generic(
      g_sdb_address    : std_logic_vector(63 downto 0);
      g_timeout_cycles : natural := 6250000; -- 100 ms at 62.5MHz
      g_bus_width      : natural);
    port(
      clk_i       : in  std_logic;
      nRst_i      : in  std_logic;
      snk_i       : in  t_wishbone_slave_in;
      snk_o       : out t_wishbone_slave_out;
      src_o       : out t_wishbone_master_out;
      src_i       : in  t_wishbone_master_in;
      cfg_slave_o : out t_wishbone_slave_out;
      cfg_slave_i : in  t_wishbone_slave_in;
      master_o    : out t_wishbone_master_out;
      master_i    : in  t_wishbone_master_in);
  end component;

  component eb_master_slave_wrapper is
  generic(
    g_with_master         : boolean := false;
    
    g_ebs_sdb_address     : std_logic_vector(63 downto 0);
    g_ebs_timeout_cycles  : natural := 6250000;
    g_ebs_mtu             : natural := 1500;
    
    g_ebm_adr_bits_hi     : natural := f_hi_adr_bits(c_ebm_sdb)
    
    );
  port(
    clk_i           : in  std_logic;
    nRst_i          : in  std_logic;
    
    --to wr core, ext wrf if 
    snk_i           : in  t_wrf_sink_in;
    snk_o           : out t_wrf_sink_out;
    src_o           : out t_wrf_source_out;
    src_i           : in  t_wrf_source_in;
  
    --ebs
    ebs_cfg_slave_o : out t_wishbone_slave_out;
    ebs_cfg_slave_i : in  t_wishbone_slave_in;
    ebs_wb_master_o : out t_wishbone_master_out;
    ebs_wb_master_i : in  t_wishbone_master_in;
    
    --ebm (optional)
    ebm_wb_slave_i  : in  t_wishbone_slave_in;
    ebm_wb_slave_o  : out t_wishbone_slave_out 
     
  );
  end component;
  
  component eb_ethernet_slave is
    generic(
      g_sdb_address    : std_logic_vector(63 downto 0);
      g_timeout_cycles : natural := 6250000; -- 100 ms at 62.5MHz
      g_mtu            : natural := 1500);
    port(
      clk_i       : in  std_logic;
      nRst_i      : in  std_logic;
      snk_i       : in  t_wrf_sink_in;
      snk_o       : out t_wrf_sink_out;
      src_o       : out t_wrf_source_out;
      src_i       : in  t_wrf_source_in;
      cfg_slave_o : out t_wishbone_slave_out;
      cfg_slave_i : in  t_wishbone_slave_in;
      master_o    : out t_wishbone_master_out;
      master_i    : in  t_wishbone_master_in);
  end component;

  -- Backwards compatability component; use eb_ethernet_slave
  component eb_slave_core is
    generic(
      g_sdb_address    : std_logic_vector(63 downto 0);
      g_timeout_cycles : natural := 6250000; -- 100 ms at 62.5MHz
      g_mtu            : natural := 1500);
    port(
      clk_i       : in  std_logic;
      nRst_i      : in  std_logic;
      snk_i       : in  t_wrf_sink_in;
      snk_o       : out t_wrf_sink_out;
      src_o       : out t_wrf_source_out;
      src_i       : in  t_wrf_source_in;
      cfg_slave_o : out t_wishbone_slave_out;
      cfg_slave_i : in  t_wishbone_slave_in;
      master_o    : out t_wishbone_master_out;
      master_i    : in  t_wishbone_master_in);
  end component;
  
  component eb_master_top is
generic(g_adr_bits_hi : natural := 8;
        g_mtu : natural := 32);
port(
  clk_i         : in  std_logic;
  rst_n_i       : in  std_logic;

  slave_i       : in  t_wishbone_slave_in;
  slave_o       : out t_wishbone_slave_out;
  
  framer_in   : out t_wishbone_slave_in;
  framer_out  : out t_wishbone_slave_out;
  
  src_i         : in  t_wrf_source_in;
  src_o         : out t_wrf_source_out
);
end component;



end etherbone_pkg;

package body etherbone_pkg is

   -- gets correct number of high address bits from ebm sdb
   function f_hi_adr_bits(ebm : t_sdb_device) return natural is
      variable ret : natural := 2;
      variable len : natural;
   begin
     len := f_hot_to_bin(ebm.sdb_component.addr_last);
     ret := c_wishbone_address_width - (len - 2);
     return ret;
   end f_hi_adr_bits;

end etherbone_pkg;
