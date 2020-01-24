-------------------------------------------------------------------------------
-- Title      : WR Streamers Private Packages
-- Project    : WR Streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : streamers_priv_pkg.vhd
-- Author     : Maciej Lipinski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL
-- Created    : 2017-04-20
-------------------------------------------------------------------------------
-- Description:
-- Private package of streamers: all the components/functions used only by
-- streamers, not useful by users/applications
-------------------------------------------------------------------------------
--
-- Copyright (c) 2017 CERN/BE-CO-HT
--
-- This source file is free software; you can redistribute it
-- and/or modify it under the terms of the GNU Lesser General
-- Public License as published by the Free Software Foundation;
-- either version 2.1 of the License, or (at your option) any
-- later version.
--
-- This source is distributed in the hope that it will be
-- useful, but WITHOUT ANY WARRANTY; without even the implied
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
-- PURPOSE.  See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General
-- Public License along with this source; if not, download it
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use IEEE.NUMERIC_STD.ALL;
use work.wishbone_pkg.all;  -- needed for t_wishbone_slave_in, etc
use work.streamers_pkg.all;
use work.wr_streamers_wbgen2_pkg.all;

package streamers_priv_pkg is

  component xtx_streamers_stats is
    generic (
      g_cnt_width            : integer := 32);
    port (
      clk_i                  : in std_logic;
      rst_n_i                : in std_logic;
      sent_frame_i           : in std_logic;
      reset_stats_i          : in std_logic;
      snapshot_ena_i         : in std_logic := '0';
      sent_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0));
  end component;

  component xrx_streamers_stats is
    generic (
      g_cnt_width            : integer := 32;
      g_acc_width            : integer := 64);
    port (
      clk_i                  : in std_logic;
      rst_n_i                : in std_logic;
      rcvd_frame_i           : in std_logic;
      lost_block_i           : in std_logic;
      lost_frame_i           : in std_logic;
      lost_frames_cnt_i      : in std_logic_vector(14 downto 0);
      rcvd_latency_i         : in  std_logic_vector(27 downto 0);
      rcvd_latency_valid_i   : in  std_logic;
      tm_time_valid_i        : in  std_logic;
      snapshot_ena_i         : in std_logic := '0';
      reset_stats_i          : in std_logic;
      rcvd_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
      lost_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
      lost_block_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
      latency_cnt_o          : out std_logic_vector(g_cnt_width-1 downto 0);
      latency_acc_overflow_o : out std_logic;
      latency_acc_o          : out std_logic_vector(g_acc_width-1  downto 0);
      latency_max_o          : out std_logic_vector(27  downto 0);
      latency_min_o          : out std_logic_vector(27  downto 0));
  end component;

  component  wr_streamers_wb is
    port (
      rst_n_i                                  : in     std_logic;
      clk_sys_i                                : in     std_logic;
      wb_adr_i                                 : in     std_logic_vector(5 downto 0);
      wb_dat_i                                 : in     std_logic_vector(31 downto 0);
      wb_dat_o                                 : out    std_logic_vector(31 downto 0);
      wb_cyc_i                                 : in     std_logic;
      wb_sel_i                                 : in     std_logic_vector(3 downto 0);
      wb_stb_i                                 : in     std_logic;
      wb_we_i                                  : in     std_logic;
      wb_ack_o                                 : out    std_logic;
      wb_stall_o                               : out    std_logic;
      regs_i                                   : in     t_wr_streamers_in_registers;
      regs_o                                   : out    t_wr_streamers_out_registers
    );
  end component;

  -- component from wr-core/modules/timing
  component pulse_stamper
    port (
      clk_ref_i       : in  std_logic;
      clk_sys_i       : in  std_logic;
      rst_n_i         : in  std_logic;
      pulse_a_i       : in  std_logic;
      tm_time_valid_i : in  std_logic;
      tm_tai_i        : in  std_logic_vector(39 downto 0);
      tm_cycles_i     : in  std_logic_vector(27 downto 0);
      tag_tai_o       : out std_logic_vector(39 downto 0);
      tag_cycles_o    : out std_logic_vector(27 downto 0);
      tag_valid_o     : out std_logic);
  end component;

  type t_pipe is record
    dvalid  : std_logic;
    dreq    : std_logic;
    sof     : std_logic;
    eof     : std_logic;
    error   : std_logic;
    data    : std_logic_vector(15 downto 0);
    addr    : std_logic_vector(1 downto 0);
    bytesel : std_logic;
  end record;

  component escape_detector
    generic (
      g_data_width  : integer;
      g_escape_code : std_logic_vector);
    port (
      clk_i             : in  std_logic;
      rst_n_i           : in  std_logic;
      d_i               : in  std_logic_vector(g_data_width-1 downto 0);
      d_detect_enable_i : in  std_logic;
      d_valid_i         : in  std_logic;
      d_req_o           : out std_logic;
      d_o               : out std_logic_vector(g_data_width-1 downto 0);
      d_escape_o        : out std_logic;
      d_valid_o         : out std_logic;
      d_req_i           : in  std_logic);
  end component;

  component dropping_buffer
    generic (
      g_size       : integer;
      g_data_width : integer);
    port (
      clk_i      : in  std_logic;
      rst_n_i    : in  std_logic;
      d_i        : in  std_logic_vector(g_data_width-1 downto 0);
      d_req_o    : out std_logic;
      d_drop_i   : in  std_logic;
      d_accept_i : in  std_logic;
      d_valid_i  : in  std_logic;
      d_o        : out std_logic_vector(g_data_width-1 downto 0);
      d_valid_o  : out std_logic;
      d_req_i    : in  std_logic);
  end component;

  component gc_escape_inserter
    generic (
      g_data_width  : integer;
      g_escape_code : std_logic_vector);
    port (
      clk_i             : in  std_logic;
      rst_n_i           : in  std_logic;
      d_i               : in  std_logic_vector(g_data_width-1 downto 0);
      d_insert_enable_i : in  std_logic;
      d_escape_i        : in  std_logic;
      d_valid_i         : in  std_logic;
      d_req_o           : out std_logic;
      d_o               : out std_logic_vector (g_data_width-1 downto 0);
      d_valid_o         : out std_logic;
      d_req_i           : in  std_logic);
  end component;
  -- functions
  function f_dbg_word_starting_at_bit(data_in, start_bit : std_logic_vector; g_data_width: integer) return std_logic_vector;

end streamers_priv_pkg;

package body streamers_priv_pkg is

  function f_dbg_word_starting_at_bit(data_in, start_bit : std_logic_vector; g_data_width: integer) return std_logic_vector is
    variable sb     : integer := 0;
    variable result : std_logic_vector(31 downto 0);
  begin
    sb     := to_integer(unsigned(start_bit));
    for i in 0 to 31 loop
      if (sb + i < g_data_width) then 
        result(i) := data_in(sb + i);
      else 
        result(i) := '0';
      end if;
    end loop;
    return result;
  end f_dbg_word_starting_at_bit;

end streamers_priv_pkg;