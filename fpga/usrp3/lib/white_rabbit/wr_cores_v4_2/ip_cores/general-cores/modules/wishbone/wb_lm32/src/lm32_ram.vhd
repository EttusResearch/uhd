-- Work-alike to lm32_ram.v, but using generic_simple_dpram

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.genram_pkg.all;

entity lm32_ram is
  generic(
    data_width    : natural := 1;
    address_width : natural := 1);
  port(
    read_clk      : in  std_logic;
    write_clk     : in  std_logic;
    reset         : in  std_logic;
    enable_read   : in  std_logic;
    read_address  : in  std_logic_vector(address_width-1 downto 0);
    enable_write  : in  std_logic;
    write_address : in  std_logic_vector(address_width-1 downto 0);
    write_data    : in  std_logic_vector(data_width   -1 downto 0);
    write_enable  : in  std_logic;
    read_data     : out std_logic_vector(data_width   -1 downto 0));
end lm32_ram;

architecture syn of lm32_ram is
  
  signal wea : std_logic;
  signal reb : std_logic;
  
  -- Emulate read-enable using another bypass
  signal old_data : std_logic_vector(data_width-1 downto 0);
  signal new_data : std_logic_vector(data_width-1 downto 0);
  signal data     : std_logic_vector(data_width-1 downto 0);

begin
  
  wea <= enable_write and write_enable;
  ram : generic_simple_dpram
    generic map(
      g_data_width               => data_width,
      g_size                     => 2**address_width,
      g_with_byte_enable         => false,
      g_addr_conflict_resolution => "write_first",
      g_dual_clock               => false) -- read_clk always = write_clk in LM32
    port map(
      clka_i => read_clk,
      wea_i  => wea,
      aa_i   => write_address,
      da_i   => write_data,
      clkb_i => write_clk,
      ab_i   => read_address,
      qb_o   => new_data);
  
  data <= old_data when reb='0' else new_data;
  read_data <= data;
  
  main : process(read_clk) is
  begin
    if rising_edge(read_clk) then
      old_data <= data;
      reb <= enable_read;
    end if;
  end process;
  
end syn;
