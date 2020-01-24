------------------------------------------------------------------------------
-- Title      : Etherbone Wishbone Master FIFO
-- Project    : Etherbone Core
------------------------------------------------------------------------------
-- File       : eb_wbm_fifo.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2013-04-08
-- Last update: 2013-04-08
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Buffers Wishbone requests to resulting data
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

entity eb_wbm_fifo is
  generic(
    g_timeout_cycles : natural);
  port(
    clk_i       : in  std_logic;
    rstn_i      : in  std_logic;
    
    errreg_o    : out std_logic_vector(63 downto 0);
    wb_i        : in  t_wishbone_master_in;
    
    fsm_stb_i   : in  std_logic;
    fsm_busy_o  : out std_logic;
    fsm_full_o  : out std_logic;

    mux_pop_i   : in  std_logic;
    mux_dat_o   : out t_wishbone_data;
    mux_empty_o : out std_logic);
end eb_wbm_fifo;

architecture rtl of eb_wbm_fifo is
  
  constant c_size  : natural := c_queue_depth;
  constant c_sbits : natural := f_ceil_log2(c_size);
  constant c_tbits : natural := f_ceil_log2(g_timeout_cycles);
  
  signal s_wb_i_rdy    : std_logic;                     -- 1 when a strobe ackd
  signal r_inflight    : unsigned(c_sbits-1 downto 0);  -- # unacked strobes
  signal r_busy        : std_logic;                     -- 1 when r_inflight > 0
  signal r_queued      : unsigned(c_sbits-1 downto 0);  -- # unpoped strobes (>= r_inflight)
  signal r_full        : std_logic;                     -- 1 when r_queued > 0
  signal r_timeout     : unsigned(c_tbits-1 downto 0);  -- counts downto 0 between acks
  signal r_kill_ack    : std_logic;                     -- 1 when r_timeout expires
  signal r_errreg      : std_logic_vector(63 downto 0); -- shifts in ack when data pops
  signal s_ack         : std_logic;                     -- ack of ready data
  
begin

  s_wb_i_rdy <= wb_i.ack or wb_i.err or wb_i.rty or r_kill_ack;
  
  fsm_busy_o <= r_busy;
  busy : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_busy <= '0';
      r_inflight <= (others => '0');
    elsif rising_edge(clk_i) then
      if fsm_stb_i = '1' then
        if s_wb_i_rdy = '1' then         -- push+pop
          r_inflight <= r_inflight;
        else                             -- push
          r_busy <= '1';
          r_inflight <= r_inflight + 1;
        end if;
      else
        if s_wb_i_rdy = '1' then         -- pop
          if r_inflight = 1 then
            r_busy <= '0';
          end if;
          r_inflight <= r_inflight - 1;
        else                             -- noop
          r_inflight <= r_inflight;
        end if;
      end if;
    end if;
  end process;
  
  fsm_full_o <= r_full;
  full : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_full <= '0';
      r_queued <= (others => '0');
    elsif rising_edge(clk_i) then
      if fsm_stb_i = '1' then
        if mux_pop_i = '1' then          -- push+pop
          r_queued <= r_queued;
        else                             -- push
          if r_queued = c_size-1 then
            r_full <= '1';
          end if;
          r_queued <= r_queued + 1;
        end if;
      else
        if mux_pop_i = '1' then          -- pop
          r_full <= '0';
          r_queued <= r_queued - 1;
        else                             -- noop
          r_queued <= r_queued;
        end if;
      end if;
    end if;
  end process;
  
  kill_ack : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_kill_ack <= '0';
      r_timeout  <= to_unsigned(g_timeout_cycles, r_timeout'length);
    elsif rising_edge(clk_i) then
      if s_wb_i_rdy = '1' or r_busy = '0' then -- ackd?
        r_kill_ack <= '0';
        r_timeout  <= to_unsigned(g_timeout_cycles, r_timeout'length);
      else
        if r_timeout = 1 then
          r_kill_ack <= '1'; -- causes s_wb_i_rdy next cycle
        end if;
        r_timeout <= r_timeout - 1;
      end if;
    end if;
  end process;
  
  datfifo : eb_fifo
    generic map(
      g_width => c_wishbone_data_width+1,
      g_size  => c_size)
    port map(
      clk_i                           => clk_i,
      rstn_i                          => rstn_i,
      w_full_o                        => open,
      w_push_i                        => s_wb_i_rdy,
      w_dat_i(t_wishbone_data'range)  => wb_i.dat,
      w_dat_i(t_wishbone_data'length) => wb_i.ack,
      r_empty_o                       => mux_empty_o,
      r_pop_i                         => mux_pop_i,
      r_dat_o(t_wishbone_data'range)  => mux_dat_o,
      r_dat_o(t_wishbone_data'length) => s_ack);
  
  -- Shift the error register during data pop
  -- This ensures that it reflects state synchronous to the TX path
  errreg_o <= r_errreg;
  errreg : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_errreg <= (others => '0');
    elsif rising_edge(clk_i) then
      if mux_pop_i = '1' then
        r_errreg <= r_errreg(r_errreg'left-1 downto 0) & (not s_ack);
      end if;
    end if;
  end process;
  
end rtl;
