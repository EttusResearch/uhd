--
-- Copyright 2015 National Instruments
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity addsub_vhdl is
  generic (
    width_g : natural := 16);
  port (
    clk_i : in std_ulogic;
    rst_i : in std_ulogic;

    i0_tdata  : in  std_ulogic_vector(width_g * 2 - 1 downto 0);
    i0_tlast  : in  std_ulogic;
    i0_tvalid : in  std_ulogic;
    i0_tready : out std_ulogic;

    i1_tdata  : in  std_ulogic_vector(width_g * 2 - 1 downto 0);
    i1_tlast  : in  std_ulogic;
    i1_tvalid : in  std_ulogic;
    i1_tready : out std_ulogic;

    sum_tdata  : out std_ulogic_vector(width_g * 2 - 1 downto 0);
    sum_tlast  : out std_ulogic;
    sum_tvalid : out std_ulogic;
    sum_tready : in  std_ulogic;

    diff_tdata  : out std_ulogic_vector(width_g * 2 - 1 downto 0);
    diff_tlast  : out std_ulogic;
    diff_tvalid : out std_ulogic;
    diff_tready : in  std_ulogic);
end entity addsub_vhdl;

architecture rtl of addsub_vhdl is

  component split_stream_fifo is
    generic (
      WIDTH       : natural := 16;
      ACTIVE_MASK : std_ulogic_vector(3 downto 0);
      FIFO_SIZE    : natural := 6);
    port (
      clk       : in std_ulogic;
      reset     : in std_ulogic;
      clear     : in std_ulogic;
      i_tdata   : in std_ulogic_vector(WIDTH - 1 downto 0);
      i_tlast   : in std_ulogic;
      i_tvalid  : in std_ulogic;
      i_tready  : out std_ulogic;
      o0_tdata  : out std_ulogic_vector(WIDTH - 1 downto 0);
      o0_tlast  : out std_ulogic;
      o0_tvalid : out std_ulogic;
      o0_tready : in  std_ulogic;
      o1_tdata  : out std_ulogic_vector(WIDTH - 1 downto 0);
      o1_tlast  : out std_ulogic;
      o1_tvalid : out std_ulogic;
      o1_tready : in  std_ulogic;
      o2_tdata  : out std_ulogic_vector(WIDTH - 1 downto 0);
      o2_tlast  : out std_ulogic;
      o2_tvalid : out std_ulogic;
      o2_tready : in  std_ulogic;
      o3_tdata  : out std_ulogic_vector(WIDTH - 1 downto 0);
      o3_tlast  : out std_ulogic;
      o3_tvalid : out std_ulogic;
      o3_tready : in  std_ulogic);
  end component split_stream_fifo;

  signal sum_a : unsigned(width_g - 1 downto 0);
  signal sum_b : unsigned(width_g - 1 downto 0);

  signal diff_a : unsigned(width_g - 1 downto 0);
  signal diff_b : unsigned(width_g - 1 downto 0);

  signal int_tdata  : std_ulogic_vector(width_g * 4 - 1 downto 0);
  signal int_tlast  : std_ulogic;
  signal int_tvalid : std_ulogic;
  signal int_tready : std_ulogic;

  signal sum  : std_ulogic_vector(width_g * 4 - 1 downto 0);
  signal diff : std_ulogic_vector(width_g * 4 - 1 downto 0);

begin

  i0_tready <= int_tvalid and int_tready;
  i1_tready <= int_tvalid and int_tready;

  sum_a <= unsigned(i0_tdata(width_g * 2 - 1 downto width_g)) +
           unsigned(i1_tdata(width_g * 2 - 1 downto width_g));

  sum_b <= unsigned(i0_tdata(width_g - 1 downto 0)) +
           unsigned(i1_tdata(width_g - 1 downto 0));

  diff_a <= unsigned(i0_tdata(width_g * 2 - 1 downto width_g)) -
            unsigned(i1_tdata(width_g * 2 - 1 downto width_g));

  diff_b <= unsigned(i0_tdata(width_g - 1 downto 0)) -
            unsigned(i1_tdata(width_g - 1 downto 0));

  int_tdata <= std_ulogic_vector(sum_a) &
               std_ulogic_vector(sum_b) &
               std_ulogic_vector(diff_a) &
               std_ulogic_vector(diff_b);

  int_tlast  <= i0_tlast; -- Follow first input
  int_tvalid <= i0_tvalid and i1_tvalid;

  splitter : split_stream_fifo
    generic map (
      WIDTH       => 4 * width_g,
      ACTIVE_MASK => "0011",
      FIFO_SIZE    => 6)
    port map (
      clk       => clk_i,
      reset     => rst_i,
      clear     => '0',
      i_tdata   => int_tdata,
      i_tlast   => int_tlast,
      i_tvalid  => int_tvalid,
      i_tready  => int_tready,
      o0_tdata  => sum,
      o0_tlast  => sum_tlast,
      o0_tvalid => sum_tvalid,
      o0_tready => sum_tready,
      o1_tdata  => diff,
      o1_tlast  => diff_tlast,
      o1_tvalid => diff_tvalid,
      o1_tready => diff_tready,
      o2_tdata  => open,
      o2_tlast  => open,
      o2_tvalid => open,
      o2_tready => '1',
      o3_tdata  => open,
      o3_tlast  => open,
      o3_tvalid => open,
      o3_tready => '1');

  sum_tdata  <= sum(sum'high downto width_g * 2);
  diff_tdata <= diff(width_g * 2 - 1 downto diff'low);

end architecture rtl;
