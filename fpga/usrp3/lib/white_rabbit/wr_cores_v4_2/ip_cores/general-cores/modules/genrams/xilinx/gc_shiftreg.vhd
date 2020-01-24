library ieee;

use ieee.STD_LOGIC_1164.all;
use ieee.NUMERIC_STD.all;

use work.genram_pkg.all;

entity gc_shiftreg is
  
  generic (
    g_size : integer);

  port (
    clk_i : in  std_logic;
    en_i  : in  std_logic;
    d_i   : in  std_logic;
    q_o   : out std_logic;
    a_i   : in  std_logic_vector(f_log2_size(g_size)-1 downto 0));

end gc_shiftreg;


architecture wrapper of gc_shiftreg is

  component SRLC32E
    port (
      Q   : out std_ulogic;
      A   : in  std_logic_vector (4 downto 0);
      CE  : in  std_ulogic;
      CLK : in  std_ulogic;
      D   : in  std_ulogic);
  end component;

  signal a  : std_logic_vector(4 downto 0);
  signal sr : std_logic_vector(g_size-1 downto 0);
  
begin  -- wrapper

  assert (g_size <= 32) report "gc_shiftreg[xilinx]: forced SRL32 implementation can be done only for 32-bit or smaller shift registers" severity warning;

  a <= std_logic_vector(resize(unsigned(a_i), 5));

  gen_srl32 : if(g_size <= 32) generate
    U_SRLC32 : SRLC32E
      port map (
        Q   => q_o,
        A   => a,
        CE  => en_i,
        CLK => clk_i,
        D   => d_i);
  end generate gen_srl32;

  gen_inferred : if(g_size > 32) generate

    p_srl : process(clk_i)
    begin
      if rising_edge(clk_i) then
        if en_i = '1' then
          sr <= sr(sr'left - 1 downto 0) & d_i;
        end if;
      end if;
    end process;

    q_o <= sr(TO_INTEGER(unsigned(a_i)));
  end generate gen_inferred;
  

end wrapper;
