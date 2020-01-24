-------------------------------------------------------------------------------
-- Title      : Parametrizable synchronous FIFO (Generic version)
-- Project    : Generics RAMs and FIFOs collection
-------------------------------------------------------------------------------
-- File       : generic_sync_fifo_std.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2011-01-25
-- Last update: 2017-02-03
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Single-clock FIFO. 
-- - configurable data width and size
-- - configurable full/empty/almost full/almost empty/word count signals
-------------------------------------------------------------------------------
-- Copyright (c) 2011-2017 CERN
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2011-01-25  1.0      twlostow        Created
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.genram_pkg.all;

entity inferred_sync_fifo is

  generic (
    g_data_width : natural;
    g_size       : natural;
    g_show_ahead : boolean := false;

    -- Read-side flag selection
    g_with_empty        : boolean := true;   -- with empty flag
    g_with_full         : boolean := true;   -- with full flag
    g_with_almost_empty : boolean := false;
    g_with_almost_full  : boolean := false;
    g_with_count        : boolean := false;  -- with words counter

    g_almost_empty_threshold : integer := 0;  -- threshold for almost empty flag
    g_almost_full_threshold  : integer := 0;  -- threshold for almost full flag

    g_register_flag_outputs : boolean := true
    );

  port (
    rst_n_i : in std_logic := '1';

    clk_i : in std_logic;
    d_i   : in std_logic_vector(g_data_width-1 downto 0);
    we_i  : in std_logic;

    q_o  : out std_logic_vector(g_data_width-1 downto 0);
    rd_i : in  std_logic;

    empty_o        : out std_logic;
    full_o         : out std_logic;
    almost_empty_o : out std_logic;
    almost_full_o  : out std_logic;
    count_o        : out std_logic_vector(f_log2_size(g_size)-1 downto 0)
    );

end inferred_sync_fifo;

architecture syn of inferred_sync_fifo is

  constant c_pointer_width                         : integer := f_log2_size(g_size);
  signal   rd_ptr, wr_ptr, wr_ptr_d0, rd_ptr_muxed : unsigned(c_pointer_width-1 downto 0);
  signal   usedw                                   : unsigned(c_pointer_width downto 0);
  signal   full, empty                             : std_logic;
  signal   q_int                                   : std_logic_vector(g_data_width-1 downto 0);
  signal   we_int, rd_int                          : std_logic;
  signal   guard_bit                               : std_logic;

  signal q_comb : std_logic_vector(g_data_width-1 downto 0);
  
begin  -- syn

  we_int <= we_i and not full;
  rd_int <= rd_i and not empty;
  
  U_FIFO_Ram : generic_dpram
    generic map (
      g_data_width               => g_data_width,
      g_size                     => g_size,
      g_with_byte_enable         => false,
      g_addr_conflict_resolution => "dont_care",
      g_dual_clock               => false)
    port map (
      rst_n_i => rst_n_i,
      clka_i  => clk_i,
      wea_i   => we_int,
      aa_i    => std_logic_vector(wr_ptr(c_pointer_width-1 downto 0)),
      da_i    => d_i,
      clkb_i  => '0',
      ab_i    => std_logic_vector(rd_ptr_muxed(c_pointer_width-1 downto 0)),
      qb_o    => q_comb);

  process(rd_ptr, rd_i, rd_int)
  begin
    if(rd_int = '1' and g_show_ahead) then
      rd_ptr_muxed <= rd_ptr + 1;
    elsif((rd_int = '1' and not g_show_ahead) or (g_show_ahead)) then
      rd_ptr_muxed <= rd_ptr;
    else
      rd_ptr_muxed <= rd_ptr - 1;
    end if;
  end process;

  q_o <= q_comb;

  p_pointers : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' then
        wr_ptr <= (others => '0');
        rd_ptr <= (others => '0');
      else
        if(we_int = '1') then
          wr_ptr <= wr_ptr + 1;
        end if;

        if(rd_int = '1') then
          rd_ptr <= rd_ptr + 1;
        end if;
      end if;
    end if;
  end process;

  gen_comb_flags_showahead : if(g_show_ahead = true) generate

    process(clk_i)
    begin
      if rising_edge(clk_i) then
        if ((rd_ptr + 1 = wr_ptr and rd_int = '1') or (rd_ptr = wr_ptr)) then
          empty <= '1';
        else
          empty <= '0';
        end if;
      end if;
    end process;
    full <= '1' when (wr_ptr + 1 = rd_ptr) else '0';

  end generate gen_comb_flags_showahead;

  gen_comb_flags : if(g_register_flag_outputs = false and g_show_ahead = false) generate
    empty <= '1' when (wr_ptr = rd_ptr and guard_bit = '0') else '0';
    full  <= '1' when (wr_ptr = rd_ptr and guard_bit = '1') else '0';

    p_guard_bit : process(clk_i)
    begin
      if rising_edge(clk_i) then
        if rst_n_i = '0' then
          guard_bit <= '0';
        elsif(wr_ptr + 1 = rd_ptr and we_int = '1') then
          guard_bit <= '1';
        elsif(rd_i = '1') then
          guard_bit <= '0';
        end if;
      end if;
    end process;
  end generate gen_comb_flags;

  gen_registered_flags : if(g_register_flag_outputs = true and g_show_ahead = false) generate
    p_reg_flags : process(clk_i)
    begin
      if rising_edge(clk_i) then
        
        if(rst_n_i = '0') then
          full  <= '0';
          empty <= '1';
        else
          if(usedw = 1 and rd_int = '1' and we_int = '0') then
            empty <= '1';
          elsif(we_int = '1' and rd_int = '0') then
            empty <= '0';
          end if;

          if(usedw = g_size-2 and we_int = '1' and rd_int = '0') then
            full <= '1';
          elsif(usedw = g_size-1 and rd_int = '1' and we_int = '0') then
            full <= '0';
          end if;
        end if;
        
      end if;
    end process;
  end generate gen_registered_flags;


  gen_with_word_counter : if(g_with_count or g_with_almost_empty or g_with_almost_full or g_register_flag_outputs) generate
    p_usedw_counter : process(clk_i)
    begin
      if rising_edge(clk_i) then
        if rst_n_i = '0' then
          usedw <= (others => '0');
        else
          if(we_int = '1' and rd_int = '0') then
            usedw <= usedw + 1;
          elsif(we_int = '0' and rd_int = '1') then
            usedw <= usedw - 1;
          end if;
        end if;
      end if;
    end process;

    count_o <= std_logic_vector(usedw(c_pointer_width-1 downto 0));

  end generate gen_with_word_counter;

  gen_with_almost_full : if(g_with_almost_full) generate
    process(clk_i)
    begin
      if rising_edge(clk_i) then
        if rst_n_i = '0' then
          almost_full_o <= '0';
        else
          if(usedw = g_almost_full_threshold-1) and (we_int = '1' and rd_int = '0') then
            almost_full_o <= '1';
          elsif (usedw = g_almost_full_threshold) and (rd_int = '1' and we_int = '0') then
            almost_full_o <= '0';
          end if;
        end if;
      end if;
    end process;
  end generate gen_with_almost_full;

  gen_without_almost_full : if(not g_with_almost_full) generate
    almost_full_o <= '0';
  end generate gen_without_almost_full;

  gen_with_almost_empty : if(g_with_almost_empty) generate
    process(clk_i)
    begin
      if rising_edge(clk_i) then
        if rst_n_i = '0' then
          almost_empty_o <= '1';
        else
          if(usedw = g_almost_empty_threshold+1) and (rd_int = '1' and we_int = '0') then
            almost_empty_o <= '1';
          elsif (usedw = g_almost_empty_threshold) and (we_int = '1' and rd_int = '0') then
            almost_empty_o <= '0';
          end if;
        end if;
      end if;
    end process;
  end generate gen_with_almost_empty;

  gen_without_almost_empty : if(not g_with_almost_empty) generate
    almost_empty_o <= '0';
  end generate gen_without_almost_empty;

  full_o  <= full;
  empty_o <= empty;

end syn;
