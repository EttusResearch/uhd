-- Work-alike to lm32_dp_ram.v, but using generic_simple_dpram

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.genram_pkg.all;

entity lm32_dp_ram is
  generic(
    addr_width : natural := 32;
    addr_depth : natural := 1024;
    data_width : natural := 8);
  port(
    clk_i   : in std_logic;
    rst_i   : in std_logic;
    we_i    : in std_logic;
    waddr_i : in  std_logic_vector(addr_width-1 downto 0);
    wdata_i : in  std_logic_vector(data_width-1 downto 0);
    raddr_i : in  std_logic_vector(addr_width-1 downto 0);
    rdata_o : out std_logic_vector(data_width-1 downto 0));
end lm32_dp_ram;

architecture syn of lm32_dp_ram is

  constant c_addr_width : natural := f_log2_size(addr_depth);

begin

  ram : generic_simple_dpram
    generic map(
      g_data_width               => data_width,
      g_size                     => addr_depth,
      g_with_byte_enable         => false,
      g_addr_conflict_resolution => "write_first",
      g_dual_clock               => false)
    port map(
      clka_i => clk_i,
      wea_i  => we_i,
      aa_i   => waddr_i(c_addr_width-1 downto 0),
      da_i   => wdata_i,
      clkb_i => clk_i,
      ab_i   => raddr_i(c_addr_width-1 downto 0),
      qb_o   => rdata_o);
  
end syn;
